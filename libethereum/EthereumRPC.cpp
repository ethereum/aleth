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
/** @file EthereumRPC.cpp
 * @autohr Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "EthereumRPC.h"

using namespace std;
using namespace dev;
using namespace dev::eth;

void EthereumRPCServer::receiveMessage(NetMsg const& _msg)
{
	clog(RPCNote) << "[" << this->serviceId() << "] receiveMessage";
	
	RLP req(_msg.rlp());
	RLPStream resp;
	NetMsgType result;
	switch (_msg.type())
	{
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

		case RequestMessagesWithWatchId:
		{
			unsigned watchId = req[0].toInt<unsigned>();
			eth::PastMessages msgs = m_service->ethereum()->messages(watchId);
			resp.appendList(msgs.size());
			for (auto msg: msgs)
				msg.streamOut(resp);
			
			result = 1;
			break;
		}
			
		case RequestMessagesWithFilter:
		{
			MessageFilter filter(req[0].toBytesConstRef());
			eth::PastMessages msgs = m_service->ethereum()->messages(filter);
			resp.appendList(msgs.size());
			for (auto msg: msgs)
				msg.streamOut(resp);
			
			result = 1;
			break;
		}
			
		case InstallWatchWithFilter:
		{
			auto ret = m_service->ethereum()->installWatch(MessageFilter(req[0].toBytesConstRef()));
			resp.appendList(1) << ret;
			
			result = 1;
			break;
		}
			
		case InstallWatchWithFilterId:
		{
			auto ret = m_service->ethereum()->installWatch(req[0].toHash<h256>());
			resp.appendList(1) << ret;
			
			result = 1;
			break;
		}
			
		case UninstallWatch:
		{
			m_service->ethereum()->uninstallWatch(req[0].toInt<unsigned>());
			
			result = 1;
			break;
		}

		case PeekWatch:
		{
			auto ret = m_service->ethereum()->peekWatch(req[0].toInt<unsigned>());
			resp.appendList(1) << ret;
			
			result = 1;
			break;
		}
			
		case CheckWatch:
		{
			auto ret = m_service->ethereum()->checkWatch(req[0].toInt<unsigned>());
			resp.appendList(1) << ret;
			
			result = 1;
			break;
		}

		case Number:
		{
			auto ret = m_service->ethereum()->number();
			resp.appendList(1) << ret;
			
			result = 1;
			break;
		}

		case PendingTransactions:
		{
			auto txs = m_service->ethereum()->pending();
			resp.appendList(txs.size());
			for (auto tx: txs)
				tx.fillStream(resp);

			result = 1;
			break;
		}

		case Diff:
		{
			auto sd = m_service->ethereum()->diff(req[0].toInt<unsigned>(),req[1].toHash<h256>());
			resp.appendList(sd.accounts.size());
			for (auto addrDiff: sd.accounts)
			{
				resp.appendList(2) << addrDiff.first;
				resp.appendList(5);
				resp << make_pair(addrDiff.second.exist.from(), addrDiff.second.exist.to());
				resp << make_pair(addrDiff.second.balance.from(), addrDiff.second.balance.to());
				resp << make_pair(addrDiff.second.nonce.from(), addrDiff.second.nonce.to());
				resp.appendList(addrDiff.second.storage.size());
				for (auto st: addrDiff.second.storage)
					resp.appendList(2) << st.first << make_pair(st.second.from(), st.second.to());
				resp << make_pair(addrDiff.second.code.from(), addrDiff.second.code.to());
			}
			
			result = 1;
			break;
		}
			
		case GetAddresses:
		{
			auto ret = m_service->ethereum()->addresses(req[0].toInt<int>());
			resp << ret;
			
			result = 1;
			break;
		}

		case GasLimitRemaining:
		{
			auto ret = m_service->ethereum()->gasLimitRemaining();
			resp << ret;
			
			result = 1;
			break;
		}

		case SetCoinbase:
		{
			// Should we maintain separate State for each client?
			
			result = 2; // support TBD
			break;
		}
			
		case GetCoinbase:
		{
			auto ret = m_service->ethereum()->address();
			resp << ret;
			
			result = 1;
			break;
		}

		// SetMining: What to do here for apps? (also see SetCoinbase)
		// start/stop/isMining/miningProgress/config
		// eth::MineProgress miningProgress() const

		default:
			result = 2;
	}
	
	NetMsg response(serviceId(), _msg.sequence(), result, RLP(resp.out()));
	connection()->send(response);
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

eth::PastMessages EthereumRPCClient::messages(unsigned _watchId) const
{
	RLPStream s(1);
	s << _watchId;
	bytes r = const_cast<EthereumRPCClient*>(this)->performRequest(RequestMessagesWithWatchId, s);
	
	PastMessages p;
	for (auto m: RLP(r))
		p.push_back(PastMessage(m.toBytesConstRef()));
	
	return std::move(p);
}

eth::PastMessages EthereumRPCClient::messages(eth::MessageFilter const& _filter) const
{
	RLPStream s(1);
	_filter.fillStream(s);
	bytes r = const_cast<EthereumRPCClient*>(this)->performRequest(RequestMessagesWithFilter, s);
	
	PastMessages p;
	for (auto m: RLP(r))
		p.push_back(PastMessage(m.toBytesConstRef()));
	
	return std::move(p);
}

unsigned EthereumRPCClient::installWatch(eth::MessageFilter const& _filter)
{
	RLPStream s;
	_filter.fillStream(s);
	bytes r = performRequest(InstallWatchWithFilter, s);
	return RLP(r)[0].toInt<unsigned>();
}

unsigned EthereumRPCClient::installWatch(h256 _filterId)
{
	RLPStream s(1);
	s << _filterId;
	bytes r = performRequest(InstallWatchWithFilterId, s);
	return RLP(r)[0].toInt<unsigned>();
}

void EthereumRPCClient::uninstallWatch(unsigned _watchId)
{
	RLPStream s(1);
	s << _watchId;
	performRequest(UninstallWatch, s);
}

bool EthereumRPCClient::peekWatch(unsigned _watchId) const
{
	RLPStream s(1);
	s << _watchId;
	bytes r = const_cast<EthereumRPCClient*>(this)->performRequest(PeekWatch, s);
	return RLP(r)[0].toInt<bool>();
}

bool EthereumRPCClient::checkWatch(unsigned _watchId)
{
	RLPStream s(1);
	s << _watchId;
	bytes r = performRequest(CheckWatch, s);
	return RLP(r)[0].toInt<bool>();
}

unsigned EthereumRPCClient::number() const
{
	bytes r = const_cast<EthereumRPCClient*>(this)->performRequest(Number);
	return RLP(r)[0].toInt<unsigned>();
}

eth::Transactions EthereumRPCClient::pending() const
{
	bytes r = const_cast<EthereumRPCClient*>(this)->performRequest(PendingTransactions);
	
	eth::Transactions txs;
	for (auto tx: RLP(r))
		txs.push_back(Transaction(tx.toBytesConstRef()));

	return txs;
}

eth::StateDiff EthereumRPCClient::diff(unsigned _txi, h256 _block) const
{
	RLPStream s(2);
	s << _txi << _block;
	bytes r = const_cast<EthereumRPCClient*>(this)->performRequest(Diff, s);
	
	eth::StateDiff sd;
	for (auto i: RLP(r))
	{
		// 0 is address
		Address addr = i[0].toHash<Address>();
		RLP drlp = i[1];
		AccountDiff d;
		d.exist = eth::Diff<bool>(drlp[0][0].toInt<bool>(), drlp[0][1].toInt<bool>());
		d.balance = eth::Diff<u256>(drlp[1][0].toInt<u256>(), drlp[1][1].toInt<u256>());
		d.nonce = eth::Diff<u256>(drlp[2][0].toInt<u256>(), drlp[2][1].toInt<u256>());
		for (auto stg: drlp[3])
			d.storage[stg[0].toInt<u256>()] = eth::Diff<u256>(stg[1][0].toInt<u256>(),stg[1][0].toInt<u256>());
		d.code = eth::Diff<bytes>(drlp[4][0].toBytes(), drlp[2][1].toBytes());
		sd.accounts[addr] = d;
	}

	return std::move(sd);
}

eth::StateDiff EthereumRPCClient::diff(unsigned _txi, int _block) const
{
	return diff(_txi,static_cast<h256>(_block));
}

Addresses EthereumRPCClient::addresses(int _block) const
{
	RLPStream s(1);
	s << _block;
	bytes r = const_cast<EthereumRPCClient*>(this)->performRequest(GetAddresses, s);
	
	Addresses addrs;
	for (auto a: RLP(r))
		addrs.push_back(a.toHash<Address>());
	
	return std::move(addrs);
}

u256 EthereumRPCClient::gasLimitRemaining() const
{
	bytes r = const_cast<EthereumRPCClient*>(this)->performRequest(GasLimitRemaining);
	return RLP(r)[0].toInt<u256>();
}

void EthereumRPCClient::setAddress(Address _us)
{
	RLPStream s(1);
	s << _us;
	performRequest(SetCoinbase, s);
}

Address EthereumRPCClient::address() const
{
	bytes r = const_cast<EthereumRPCClient*>(this)->performRequest(GetCoinbase);
	return RLP(r)[0].toHash<Address>();
}



