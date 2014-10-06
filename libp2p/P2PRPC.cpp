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
/** @file P2PRPC.cpp
 * @autohr Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "P2PRPC.h"

using namespace std;
using namespace dev;
using namespace dev::p2p;

void P2PRPCServer::receiveMessage(NetMsg const& _msg)
{
	clog(RPCNote) << "[" << this->serviceId() << "] receiveMessage";
	
	RLP req(_msg.rlp());
	RLPStream resp;
	NetMsgType result;
	switch (_msg.type())
	{
		case Peers:
		{
			auto peers = m_service->net()->peers();
			resp.appendList(peers.size());
			for (auto p: peers)
			{
				resp.appendList(7);
				resp << p.clientVersion;
				resp << p.host;
				resp << (unsigned)p.port;
				resp << 1; // (unsigned)std::chrono::duration_cast<std::chrono::milliseconds>(p.lastPing).count();
				resp << p.caps;		// std::set<std::string> caps;
				resp << p.socket;
				resp.appendList(p.notes.size());
				for (auto n: p.notes)
					resp << n;
			}
			
			result = 1;
			break;
		}
			
		case PeerCount:
		{
			auto ret = (int)m_service->net()->peerCount();
			resp.appendList(1) << ret;
			result = 1;
			break;
		}

		case ConnectToPeer:
		{
			m_service->net()->connect(req[0].toString(), (short)req[1].toInt<unsigned>());
			result = 1;
			break;
		}
			
		case HaveNetwork:
		{
			auto ret = (bool)(m_service->net()->peerCount() != 0);
			resp.appendList(1) << ret;
			result = 1;
			break;
		}
			
		case SavePeers:
		{
			auto ret = m_service->net()->savePeers();
			resp.appendList(1) << ret;
			result = 1;
			break;
		}
			
		case RestorePeers:
		{
			m_service->net()->restorePeers(req[0].data());
			result = 1;
			break;
		}

		default:
			result = 2;
	}
	
	if (!resp.out().size())
		resp.appendList(0);

	assert(resp.out().size());
	NetMsg response(serviceId(), _msg.sequence(), result, RLP(resp.out()));
	connection()->send(response);
}


std::vector<PeerInfo> P2PRPCClient::peers()
{
	std::vector<PeerInfo> peers;
	bytes b = performRequest(Peers);
	RLP r(b);
	for (auto p: r)
	{
		PeerInfo pi;
		pi.clientVersion = p[0].toString();
		pi.host = p[1].toString();
		pi.port = (short)p[2].toInt<unsigned>();
		pi.lastPing = std::chrono::milliseconds(0 /*r[3].toInt<unsigned>()*/); // std::chrono::steady_clock::duration lastPing;
		for (auto c: p[4])
			pi.caps.insert(c.toString());
		pi.socket = p[5].toInt<unsigned>();
		for (auto n: p[6])
			pi.notes[n[0].toString()] = n[1].toString();
		peers.push_back(pi);
	}

	return std::move(peers);
}

size_t P2PRPCClient::peerCount() const
{
	bytes b = const_cast<P2PRPCClient*>(this)->performRequest(PeerCount);
	return RLP(b)[0].toInt<int>();
}

void P2PRPCClient::connect(std::string const& _seedHost, unsigned short _port)
{
	RLPStream s(2);
	s << _seedHost << _port;
	performRequest(ConnectToPeer, s);
}

bool P2PRPCClient::haveNetwork()
{
	bytes b = performRequest(HaveNetwork);
	return RLP(b)[0].toInt<bool>();
}

bytes P2PRPCClient::savePeers()
{
	bytes b = performRequest(SavePeers);
	return std::move(RLP(b)[0].data().toBytes());
}

void P2PRPCClient::restorePeers(bytesConstRef _saved)
{
	RLPStream s(1);
	s << _saved;
	performRequest(RestorePeers, s);
}


