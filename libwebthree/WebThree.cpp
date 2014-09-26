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

#include <chrono>
#include <thread>
#include <boost/filesystem.hpp>
#include <libdevcore/Log.h>
#include <libp2p/Host.h>
#include <libethereum/Defaults.h>
#include <libethereum/EthereumHost.h>
#include <libwhisper/WhisperPeer.h>
#include <libethereum/EthereumRPC.h>
using namespace std;
using namespace dev;
using namespace dev::p2p;
using namespace dev::eth;
using namespace dev::shh;

WebThreeDirect::WebThreeDirect(std::string const& _clientVersion, std::string const& _dbPath, bool _forceClean, std::set<std::string> const& _interfaces, NetworkPreferences const& _n):
	m_clientVersion(_clientVersion),
	m_net(_clientVersion, _n),
	m_rpcEndpoint(new NetEndpoint(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 30310)))
{
	if (_dbPath.size())
		Defaults::setDBPath(_dbPath);

	bool startRpc = false;
	if (_interfaces.count("eth"))
	{
		m_ethereum.reset(new eth::Client(&m_net, _dbPath, _forceClean));
		m_ethereumRpcService.reset(new EthereumRPC(m_ethereum.get()));
		m_rpcEndpoint->registerService(m_ethereumRpcService.get());
		startRpc = true;
	}

//	if (_interfaces.count("shh"))
//	{
//		m_whisper = new eth::Whisper(m_net.get());
////		m_whisperRpcService.reset(new WhisperRPC(m_whisper.get()));
////		m_rpcEndpoint.registerService(m_whisperRpcService.get());
////		startRpc = true;
//	}
	
	if (startRpc)
		m_rpcEndpoint->start();
}

WebThreeDirect::~WebThreeDirect()
{
}

std::vector<PeerInfo> WebThreeDirect::peers()
{
	return m_net.peers();
}

size_t WebThreeDirect::peerCount() const
{
	return m_net.peerCount();
}

void WebThreeDirect::setIdealPeerCount(size_t _n)
{
	return m_net.setIdealPeerCount(_n);
}

bytes WebThreeDirect::savePeers()
{
	return m_net.savePeers();
}

void WebThreeDirect::restorePeers(bytesConstRef _saved)
{
	return m_net.restorePeers(_saved);
}

void WebThreeDirect::connect(std::string const& _seedHost, unsigned short _port)
{
	m_net.connect(_seedHost, _port);
}

WebThree::WebThree():
	Worker("webthree-client"),
	m_io(),
	m_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 30310),
	m_connection(new NetConnection(m_io, m_endpoint))
{
	startWorking();
	
	m_ethereum = new EthereumRPCClient(m_connection.get());
	// m_whisper = new WhisperRPCClient(m_connection.get());
	m_connection->start();
}

WebThree::~WebThree()
{
	stopWorking();
	
	if (m_ethereum)
		delete m_ethereum;
}

void WebThree::doWork()
{
	if (m_io.stopped())
		m_io.reset();
	m_io.poll();
}


