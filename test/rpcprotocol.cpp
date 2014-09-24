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
/** @file rpcprotocol.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 */

#include <memory>
#include <libwebthree/Common.h>
#include <libwebthree/NetConnection.h>
#include "rpcprotocol.h"
#include "rpcservice.h"

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::p2p;

EthereumRPCServer::EthereumRPCServer(NetConnection* _conn, NetServiceFace* _service): NetProtocol(_conn), m_service(dynamic_cast<EthereumRPCService*>(_service))
{
	
}

void EthereumRPCServer::receiveMessage(NetMsg const& _msg)
{
	clog(RPCNote) << "[" << this->serviceId() << "] receiveMessage";
	
	RLP req(_msg.rlp());
	RLPStream resp;
	NetMsgType result;
	switch (_msg.type())
	{
		case 255:
		{
			resp.appendList(1);
			resp << m_service->test();
			
			result = 1;
			break;
		}
			
		case RequestSubmitTransaction:
		{
			Secret s(req[0].toHash<Secret>());
			u256 v(req[1].toInt<u256>());
			Address d(req[2].toHash<Address>());
			bytes const& data(req[3].toBytes());
			u256 g(req[4].toInt<u256>());
			u256 gp(req[5].toInt<u256>());
			m_service->ethereum()->transact(s, v, d, data, g, gp);
			
			result = 1;
			break;
		}
			
		case RequestCreateContract:
		{
			Secret s(req[0].toHash<Secret>());
			u256 e(req[1].toInt<u256>());
			bytes const& data(req[2].toBytes());
			u256 g(req[3].toInt<u256>());
			u256 gp(req[4].toInt<u256>());
			Address a(m_service->ethereum()->transact(s, e, data, g, gp));
			
			result = 1;
			resp.appendList(1);
			resp << a;
			break;
		}

		case RequestRLPInject:
		{
			m_service->ethereum()->inject(req[0].toBytesConstRef());
			
			result = 1;
			break;
		}

		case RequestFlushTransactions:
		{
			m_service->ethereum()->flushTransactions();
			
			result = 1;
			break;
		}

		case RequestCallTransaction:
		{
			Secret s(req[0].toHash<Secret>());
			u256 v(req[1].toInt<u256>());
			Address d(req[2].toHash<Address>());
			bytes const& data(req[3].toBytes());
			u256 g(req[4].toInt<u256>());
			u256 gp(req[5].toInt<u256>());
			bytes b(m_service->ethereum()->call(s, v, d, data, g, gp));

			result = 1;
			resp.appendList(1);
			resp << b;
			break;
		}

		case RequestBalanceAt:
		{
			u256 b(m_service->ethereum()->balanceAt(Address(req[0].toHash<Address>()), req[1].toInt()));
			
			clog(RPCNote) << "got balance: " << b;
			
			result = 1;
			resp.appendList(1);
			resp << b;
			break;
		}
			
		case RequestCountAt:
		{
			int block = req[1].toInt();
			u256 b(m_service->ethereum()->countAt(Address(req[0].toHash<Address>()), block));
			
			result = 1;
			resp.appendList(1);
			resp << b;
			break;
		}
			
		case RequestStateAt:
		{
			u256 b(m_service->ethereum()->stateAt(Address(req[0].toHash<Address>()), req[1].toInt<u256>(), req[2].toInt()));
			
			result = 1;
			resp.appendList(1);
			resp << b;
			break;
		}
			
		case RequestCodeAt:
		{
			bytes b(m_service->ethereum()->codeAt(Address(req[0].toHash<Address>()), req[1].toInt()));
			
			result = 1;
			resp.appendList(1);
			resp << b;
			break;
		}
			
		case RequestStorageAt:
		{
			std::map<u256, u256> store(m_service->ethereum()->storageAt(Address(req[0].toHash<Address>()), req[1].toInt()));

			result = 1;
			resp.appendList(1);
			resp.appendList(store.size());
			for (auto s: store)
				resp << s;
			
			break;
		}

		case RequestMessages:
		default:
			result = 2;
	}
	
	NetMsg response(serviceId(), _msg.sequence(), result, RLP(resp.out()));
	connection()->send(response);
}

EthereumRPCClient::EthereumRPCClient(NetConnection* _conn, void *): NetProtocol(_conn)
{
	_conn->setDataMessageHandler(serviceId(), [=](NetMsg const& _msg)
	{
		receiveMessage(_msg);
	});
}

void EthereumRPCClient::receiveMessage(NetMsg const& _msg)
{
	// client should look for Success,Exception, and promised responses
	clog(RPCNote) << "[" << this->serviceId() << "] receiveMessage";
	
	// !! check promises and set value
	
	RLP res(_msg.rlp());
	switch (_msg.type())
	{
		case 0:
			// false
			break;
			
		case 1:
			// success/true (second item is result, unless bool/void)
			if (auto p = m_promises[_msg.sequence()])
				p->set_value(make_shared<NetMsg>(_msg));
			break;
			
		case 2:
			// exception (second item is result)
			break;
	}
}

bytes EthereumRPCClient::performRequest(NetMsgType _type, RLPStream& _s)
{
	promiseResponse p;
	futureResponse f = p.get_future();

	NetMsg msg(serviceId(), nextDataSequence(), _type, RLP(_s.out()));
	{
		lock_guard<mutex> l(x_promises);
		m_promises.insert(make_pair(msg.sequence(),&p));
	}
	connection()->send(msg);

	// What happens if we're disconnected???
	auto s = f.wait_until(std::chrono::steady_clock::now() + std::chrono::seconds(20 + (rand() % 15)));
	{
		lock_guard<mutex> l(x_promises);
		m_promises.erase(msg.sequence());
	}
	if (s != future_status::ready)
		throw WebThreeRequestTimeout();
	return std::move(f.get()->rlp());
}

void EthereumRPCClient::transact(Secret _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice)
{
	RLPStream s(6);
	s << _secret << _value << _dest << _data << _gas << _gasPrice;
	performRequest(RequestSubmitTransaction, s);
}

Address EthereumRPCClient::transact(Secret _secret, u256 _endowment, bytes const& _init, u256 _gas, u256 _gasPrice)
{
	RLPStream s(5);
	s << _secret << _endowment << _init << _gas << _gasPrice;
	
	return Address(RLP(performRequest(RequestCreateContract, s)).toBytes());
}

void EthereumRPCClient::inject(bytesConstRef _rlp)
{
	RLPStream s(1);
	s.append(_rlp);
	performRequest(RequestRLPInject, s);
}

void EthereumRPCClient::flushTransactions()
{
	performRequest(RequestFlushTransactions);
}

bytes EthereumRPCClient::call(Secret _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice)
{
	RLPStream s(6);
	s << _secret << _value << _dest << _data << _gas << _gasPrice;
	return RLP(performRequest(RequestCallTransaction, s)).toBytes();
}


u256 EthereumRPCClient::balanceAt(Address _a, int _block) const
{
	RLPStream s(2);
	s << _a << _block;
	bytes r = const_cast<EthereumRPCClient*>(this)->performRequest(RequestBalanceAt, s);
	return u256(RLP(r)[0].toInt<u256>());
}

u256 EthereumRPCClient::countAt(Address _a, int _block) const
{
	RLPStream s(2);
	s << _a << _block;
	RLP r(const_cast<EthereumRPCClient*>(this)->performRequest(RequestCountAt, s));
	
	return r.toInt<u256>();
}

u256 EthereumRPCClient::stateAt(Address _a, u256 _l, int _block) const
{
	RLPStream s(3);
	s << _a << _l << _block;
	RLP r(const_cast<EthereumRPCClient*>(this)->performRequest(RequestStateAt, s));
	
	return r.toInt<u256>();
}

bytes EthereumRPCClient::codeAt(Address _a, int _block) const
{
	RLPStream s(2);
	s << _a << _block;
	RLP r(const_cast<EthereumRPCClient*>(this)->performRequest(RequestCodeAt, s));
	
	return r.toBytes();
}

std::map<u256, u256> EthereumRPCClient::storageAt(Address _a, int _block) const
{
	std::map<u256, u256> store;
	
	RLPStream s(2);
	s << _a << _block;
	bytes r = const_cast<EthereumRPCClient*>(this)->performRequest(RequestStorageAt, s);
	for (auto s: RLP(r))
		store.insert(make_pair( s[0].toInt<u256>(), s[1].toInt<u256>() ));
	
	return store;
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



