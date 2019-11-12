// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "WebThree.h"

#include <libethashseal/Ethash.h>
#include <libethereum/ClientTest.h>
#include <libethereum/EthereumCapability.h>

#include <aleth/buildinfo.h>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace dev;
using namespace dev::p2p;
using namespace dev::eth;
using namespace dev::shh;

static_assert(BOOST_VERSION >= 106400, "Wrong boost headers version");

WebThreeDirect::WebThreeDirect(std::string const& _clientVersion,
    boost::filesystem::path const& _dbPath, boost::filesystem::path const& _snapshotPath,
    eth::ChainParams const& _params, WithExisting _we, NetworkConfig const& _n,
    bytesConstRef _network, bool _testing)
  : m_clientVersion(_clientVersion), m_net(_clientVersion, _n, _network)
{
    if (_testing)
        m_ethereum.reset(new eth::ClientTest(
            _params, (int)_params.networkID, m_net, shared_ptr<GasPricer>(), _dbPath, _we));
    else
        m_ethereum.reset(new eth::Client(_params, (int)_params.networkID, m_net,
            shared_ptr<GasPricer>(), _dbPath, _snapshotPath, _we));

    m_ethereum->startWorking();
    const auto* buildinfo = aleth_get_buildinfo();
    m_ethereum->setExtraData(rlpList(0, string{buildinfo->project_version}.substr(0, 5) + "++" +
                                            string{buildinfo->git_commit_hash}.substr(0, 4) +
                                            string{buildinfo->build_type}.substr(0, 1) +
                                            string{buildinfo->system_name}.substr(0, 5) +
                                            string{buildinfo->compiler_id}.substr(0, 3)));
}

WebThreeDirect::~WebThreeDirect()
{
    // Utterly horrible right now - WebThree owns everything (good), but:
    // m_net (Host) owns the eth::EthereumHost via a shared_ptr.
    // The eth::EthereumHost depends on eth::Client (it maintains a reference to the BlockChain field of Client).
    // eth::Client (owned by us via a unique_ptr) uses eth::EthereumHost (via a weak_ptr).
    // Really need to work out a clean way of organising ownership and guaranteeing startup/shutdown is perfect.

    // Have to call stop here to get the Host to kill its io_context otherwise we end up with
    // left-over reads, still referencing Sessions getting deleted *after* m_ethereum is reset,
    // causing bad things to happen, since the guarantee is that m_ethereum is only reset *after*
    // all sessions have ended (sessions are allowed to use bits of data owned by m_ethereum).
    m_net.stop();
}

std::string WebThreeDirect::composeClientVersion(std::string const& _client)
{
    const auto* buildinfo = aleth_get_buildinfo();
    return _client + "/v" + buildinfo->project_version + "/" + buildinfo->system_name + "/" +
           buildinfo->compiler_id + buildinfo->compiler_version + "/" + buildinfo->build_type;
}

std::vector<PeerSessionInfo> WebThreeDirect::peers()
{
    return m_net.peerSessionInfos();
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

void WebThreeDirect::addNode(p2p::NodeID const& _node, bi::tcp::endpoint const& _host)
{
    m_net.addNode(_node, NodeIPEndpoint(_host.address(), _host.port(), _host.port()));
}

void WebThreeDirect::requirePeer(p2p::NodeID const& _node, bi::tcp::endpoint const& _host)
{
    m_net.requirePeer(_node, NodeIPEndpoint(_host.address(), _host.port(), _host.port()));
}

void WebThreeDirect::addPeer(NodeSpec const& _s, PeerType _t)
{
    m_net.addPeer(_s, _t);
}

