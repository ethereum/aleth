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
/** @file rpcprotocol.h
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 */

#pragma once

#include <libdevnet/NetProtocol.h>
#include "rpcservice.h"

namespace dev
{
	
class TestServiceProtocol: public NetServiceProtocol<TestService>
{
public:
	static NetMsgServiceType serviceId() { return 255; }
	TestServiceProtocol(NetConnection* _conn, NetServiceFace* _service): NetServiceProtocol(_conn, _service) {}
	
	std::string protocolString() { return "protocolString"; }
	
	void receiveMessage(NetMsg const& _msg)
	{
		switch (_msg.type()) {
			case 1:
				this->getService()->m_interface->stringTest();
				break;
				
			default:
				break;
		}
	}
};

class TestRPCProtocol: public NetRPCClientProtocol<TestServiceProtocol>
{
public:
	static NetMsgServiceType serviceId() { return 255; }
	TestRPCProtocol(NetConnection* _conn): NetRPCClientProtocol(_conn){}

	std::string protocolString() { return "protocolString"; }

	std::string getRemoteServiceString()
	{
		bytes response(performRequest((NetMsgType)1));
		return std::string((const char*)response.data());
	}
	
	std::string getRemoteInterfaceString()
	{
		bytes response(performRequest((NetMsgType)2));
		return std::string((const char*)response.data());
	}
};

}


// WebThree API:
//	std::vector<p2p::PeerInfo> peers();
//	size_t peerCount() const;
//	void connect(std::string const& _host, unsigned short _port);
//std::vector<PeerInfo> RPCProtocol::peers()
//{
//	// RequestPeers
//	std::vector<PeerInfo> peers;
//	RLP response(RLP(performRequest(RequestPeers)));
//	for (auto r: response[1])
//	{
//		unsigned short p = r[2].toInt();
//		std::chrono::duration<int> t(r[3].toInt());
//		peers.push_back(PeerInfo({r[0].toString(),r[1].toString(),p,t}));
//	}
//	lock_guard<mutex> l(x_peers);
//	m_peers = peers;
//	return peers;
//}
//
//size_t RPCProtocol::peerCount() const
//{
//	lock_guard<mutex> l(x_peers);
//	return m_peers.size();
//}
//
//void RPCProtocol::connect(std::string const& _host, unsigned short _port)
//{
//	RLPStream s(2);
//	s.append(_host);
//	s.append(_port);
//
//	performRequest(ConnectToPeer, s);
//}




