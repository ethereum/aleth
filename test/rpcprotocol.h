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

#include <mutex>
#include <future>
#include <libwebthree/Common.h>
#include <libwebthree/NetProtocol.h>
#include <libethereum/Interface.h>
#include <libp2p/Common.h>

namespace dev
{

// inherit ethereum interface?
class EthereumRPCService;
class EthereumRPCServer: public NetProtocol
{
public:
	static NetMsgServiceType serviceId() { return EthereumService; }
	EthereumRPCServer(NetConnection* _conn, EthereumRPCService *_service);

	NetMsgSequence sendRLPStream(RLPStream &s);
	void receiveMessage(NetMsg _msg);
	
	std::string test() { return "a"; }
protected:
	EthereumRPCService* m_service;
};

	
// client will probably need to use service as well, as endpoint registration will still be used to install multiple handlers for a single connection (ultimately this is so that 3 different protocols waiting on 8 different requets will all be deferred/blocked until reconnection completes)
class EthereumRPCClient: public NetProtocol, public eth::Interface
{
public:
	static NetMsgServiceType serviceId() { return EthereumService; }
	EthereumRPCClient(NetConnection* _conn, void *): NetProtocol(_conn) {}
	
	void receiveMessage(NetMsg _msg);
	
	NetMsgSequence sendRLPStream(RLPStream &s);
	bytes performRequest(NetMsgType _type) { RLPStream s; s.appendList(0); return performRequest(_type, s); }
	bytes performRequest(NetMsgType _type, RLPStream& _s);
	
protected:
	typedef std::promise<std::shared_ptr<NetMsg> > promiseResponse;
	typedef std::future<std::shared_ptr<NetMsg> > futureResponse;
	std::map<NetMsgSequence,promiseResponse*> m_promises;
	std::mutex x_promises;										///< Mutex concurrent access to m_promises.
	
public:
	void transact(Secret _secret, u256 _value, Address _dest, bytes const& _data = bytes(), u256 _gas = 10000, u256 _gasPrice = 10 * eth::szabo);
	Address transact(Secret _secret, u256 _endowment, bytes const& _init, u256 _gas = 10000, u256 _gasPrice = 10 * eth::szabo);
	void inject(bytesConstRef _rlp);
	void flushTransactions();
	bytes call(Secret _secret, u256 _value, Address _dest, bytes const& _data = bytes(), u256 _gas = 10000, u256 _gasPrice = 10 * eth::szabo);
	
	u256 balanceAt(Address _a) const { return balanceAt(_a, m_default); }
	u256 countAt(Address _a) const { return countAt(_a, m_default); }
	u256 stateAt(Address _a, u256 _l) const { return stateAt(_a, _l, m_default); }
	bytes codeAt(Address _a) const { return codeAt(_a, m_default); }
	std::map<u256, u256> storageAt(Address _a) const { return storageAt(_a, m_default); }
	
	u256 balanceAt(Address _a, int _block) const;
	u256 countAt(Address _a, int _block) const;
	u256 stateAt(Address _a, u256 _l, int _block) const;
	bytes codeAt(Address _a, int _block) const;
	std::map<u256, u256> storageAt(Address _a, int _block) const;
	
	eth::PastMessages messages(unsigned _watchId) const {};
	eth::PastMessages messages(eth::MessageFilter const& _filter) const {};
	
	/// Install, uninstall and query watches.
	unsigned installWatch(eth::MessageFilter const& _filter) {};
	unsigned installWatch(h256 _filterId) {};
	void uninstallWatch(unsigned _watchId) {};
	bool peekWatch(unsigned _watchId) const {};
	bool checkWatch(unsigned _watchId) {};
	
	// TODO: Block query API.
	
	// [EXTRA API]:
	
	/// @returns The height of the chain.
	unsigned number() const {};
	
	/// Get a map containing each of the pending transactions.
	/// @TODO: Remove in favour of transactions().
	eth::Transactions pending() const {};
	
	/// Differences between transactions.
	eth::StateDiff diff(unsigned _txi, h256 _block) const {};
	eth::StateDiff diff(unsigned _txi, int _block) const {};
	
	/// Get a list of all active addresses.
	Addresses addresses() const { return addresses(m_default); }
	Addresses addresses(int _block) const {};
	
	/// Get the remaining gas limit in this block.
	u256 gasLimitRemaining() const {};
	
	// [MINING API]:
	
	/// Set the coinbase address.
	void setAddress(Address _us) {};
	/// Get the coinbase address.
	Address address() const {};
	
	/// Stops mining and sets the number of mining threads (0 for automatic).
	void setMiningThreads(unsigned _threads = 0) {};
	/// Get the effective number of mining threads.
	unsigned miningThreads() const {};
	
	/// Start mining.
	/// NOT thread-safe - call it & stopMining only from a single thread
	void startMining() {};
	/// Stop mining.
	/// NOT thread-safe
	void stopMining() {};
	/// Are we mining now?
	bool isMining() {};
	
	/// Check the progress of the mining.
	eth::MineProgress miningProgress() const {};
	
	
	
};

}

