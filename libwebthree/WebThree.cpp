/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file WebThree.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "WebThree.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <libethereum/Defaults.h>
#include <libethereum/EthereumHost.h>
#include <libethereum/ClientTest.h>
#include <libethashseal/EthashClient.h>
#include "BuildInfo.h"
#include <libethashseal/Ethash.h>
using namespace std;
using namespace dev;
using namespace dev::p2p;
using namespace dev::eth;
using namespace dev::shh;

static_assert(BOOST_VERSION >= 106400, "Wrong boost headers version");

WebThreeDirect::WebThreeDirect(
	std::string const& _clientVersion,
	boost::filesystem::path const& _dbPath,
	eth::ChainParams const& _params,
	WithExisting _we,
	std::set<std::string> const& _interfaces,
	NetworkPreferences const& _n,
	bytesConstRef _network,
	bool _testing
):
	m_clientVersion(_clientVersion),
	m_net(_clientVersion, _n, _network)
{
	if (_dbPath.size())
		Defaults::setDBPath(_dbPath);
	if (_interfaces.count("eth"))
	{
		Ethash::init();
		NoProof::init();
		if (_params.sealEngineName == "Ethash")
			m_ethereum.reset(new eth::EthashClient(_params, (int)_params.networkID, &m_net, shared_ptr<GasPricer>(), _dbPath, _we));
		else if (_params.sealEngineName == "NoProof" && _testing)
			m_ethereum.reset(new eth::ClientTest(_params, (int)_params.networkID, &m_net, shared_ptr<GasPricer>(), _dbPath, _we));
		else
			m_ethereum.reset(new eth::Client(_params, (int)_params.networkID, &m_net, shared_ptr<GasPricer>(), _dbPath, _we));
		m_ethereum->startWorking();
		string bp = DEV_QUOTED(ETH_BUILD_PLATFORM);
		vector<string> bps;
		boost::split(bps, bp, boost::is_any_of("/"));
		bps[0] = bps[0].substr(0, 5);
		bps[1] = bps[1].substr(0, 3);
		bps.back() = bps.back().substr(0, 3);
		m_ethereum->setExtraData(rlpList(0, string(dev::Version) + "++" + string(DEV_QUOTED(ETH_COMMIT_HASH)).substr(0, 4) + (ETH_CLEAN_REPO ? "-" : "*") + string(DEV_QUOTED(ETH_BUILD_TYPE)).substr(0, 1) + boost::join(bps, "/")));
	}
}

WebThreeDirect::~WebThreeDirect()
{
	// Utterly horrible right now - WebThree owns everything (good), but:
	// m_net (Host) owns the eth::EthereumHost via a shared_ptr.
	// The eth::EthereumHost depends on eth::Client (it maintains a reference to the BlockChain field of Client).
	// eth::Client (owned by us via a unique_ptr) uses eth::EthereumHost (via a weak_ptr).
	// Really need to work out a clean way of organising ownership and guaranteeing startup/shutdown is perfect.

	// Have to call stop here to get the Host to kill its io_service otherwise we end up with left-over reads,
	// still referencing Sessions getting deleted *after* m_ethereum is reset, causing bad things to happen, since
	// the guarantee is that m_ethereum is only reset *after* all sessions have ended (sessions are allowed to
	// use bits of data owned by m_ethereum).
	m_net.stop();
	m_ethereum.reset();
}

std::string WebThreeDirect::composeClientVersion(std::string const& _client)
{
	return _client + "/" + \
		"v" + dev::Version + "/" + \
		DEV_QUOTED(ETH_BUILD_OS) + "/" + \
		DEV_QUOTED(ETH_BUILD_COMPILER) + "/" + \
		DEV_QUOTED(ETH_BUILD_JIT_MODE) + "/" + \
		DEV_QUOTED(ETH_BUILD_TYPE) + "/" + \
		string(DEV_QUOTED(ETH_COMMIT_HASH)).substr(0, 8) + \
		(ETH_CLEAN_REPO ? "" : "*") + "/";
}

p2p::NetworkPreferences const& WebThreeDirect::networkPreferences() const
{
	return m_net.networkPreferences();
}

void WebThreeDirect::setNetworkPreferences(p2p::NetworkPreferences const& _n, bool _dropPeers)
{
	auto had = isNetworkStarted();
	if (had)
		stopNetwork();
	m_net.setNetworkPreferences(_n, _dropPeers);
	if (had)
		startNetwork();
}

std::vector<PeerSessionInfo> WebThreeDirect::peers()
{
	return m_net.peerSessionInfo();
}

size_t WebThreeDirect::peerCount() const
{
	return m_net.peerCount();
}

void WebThreeDirect::setIdealPeerCount(size_t _n)
{
	return m_net.setIdealPeerCount(_n);
}

void WebThreeDirect::setPeerStretch(size_t _n)
{
	return m_net.setPeerStretch(_n);
}

bytes WebThreeDirect::saveNetwork()
{
	return m_net.saveNetwork();
}

void WebThreeDirect::addNode(NodeID const& _node, bi::tcp::endpoint const& _host)
{
	m_net.addNode(_node, NodeIPEndpoint(_host.address(), _host.port(), _host.port()));
}

void WebThreeDirect::requirePeer(NodeID const& _node, bi::tcp::endpoint const& _host)
{
	m_net.requirePeer(_node, NodeIPEndpoint(_host.address(), _host.port(), _host.port()));
}

void WebThreeDirect::addPeer(NodeSpec const& _s, PeerType _t)
{
	m_net.addPeer(_s, _t);
}

