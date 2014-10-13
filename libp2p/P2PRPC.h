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
/** @file P2PRPC.h
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <mutex>
#include <libdevnet/NetProtocol.h>
#include <libp2p/Host.h>

namespace dev
{
namespace p2p
{
	
enum P2PRequestMsgType : NetMsgType
{
	Peers = 0x01,
	PeerCount,
	ConnectToPeer,
	HaveNetwork,
	SaveNodes,
	RestoreNodes
};

class P2PRPCServer;
	
/**
 * @brief Provides network RPC interface as a service endpoint. When P2PRPC
 * is added to an endpoint it will automatically assign callback methods for the 
 * P2PRPC protocol, which, will interpret and handle RPC requests. As it's a
 * NetService, P2PRPC can alternatively be used by using registerConnection
 * for each connection which needs to respond to P2PRPC messages.
 */
class P2PRPC: public NetService<P2PRPCServer>
{
public:
	P2PRPC(p2p::Host* _host): m_net(_host) { }
	
	p2p::Host* net() { return m_net; }
	
protected:
	p2p::Host* m_net;
};
	
	
class P2PRPC;

/**
 * @brief Provides protocol implementation for handling server-side of P2P RPC
 * connections. Each instance handles a single connection and is additionally 
 * passed a pointer to P2PRPC service.
 * All incoming (call) requests receive a response which have the same 
 * sequence id of the request.
 */
class P2PRPCServer: public NetServiceProtocol<P2PRPC>
{
public:
	static NetMsgServiceType serviceId() { return P2PService; }
	P2PRPCServer(NetConnection* _conn, NetServiceFace* _service): NetServiceProtocol(_conn, _service) {};
	void receiveMessage(NetMsg const& _msg);
};

/**
 * @brief Provides protocol implementation and state for handling client-side of P2P RPC connections.
 */
class P2PRPCClient: public NetRPCClientProtocol<P2PRPCClient>
{
public:
	static NetMsgServiceType serviceId() { return P2PService; }
	P2PRPCClient(NetConnection* _conn): NetRPCClientProtocol(_conn) {}

	std::vector<p2p::PeerInfo> peers();
	size_t peerCount() const;
	void connect(std::string const& _seedHost, unsigned short _port);
	bool haveNetwork();
	bytes saveNodes();
	void restoreNodes(bytesConstRef _saved);
};

}
}

