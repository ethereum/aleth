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
/** @file InterfaceStub.h
 * @author Marek Kotewicz <marek@ethdev.com>
 * @date 2015
 */

#pragma once

#include "Interface.h"
#include "LogFilter.h"

namespace dev {

namespace eth {

	
struct InstalledFilter
{
	InstalledFilter(LogFilter const& _f): filter(_f) {}

	LogFilter filter;
	unsigned refCount = 1;
	LocalisedLogEntries changes;
};
	
static const h256 PendingChangedFilter = u256(0);
static const h256 ChainChangedFilter = u256(1);
	
static const LogEntry SpecialLogEntry = LogEntry(Address(), h256s(), bytes());
static const LocalisedLogEntry InitialChange(SpecialLogEntry, 0);
	
struct ClientWatch
{
	ClientWatch(): lastPoll(std::chrono::system_clock::now()) {}
	explicit ClientWatch(h256 _id, Reaping _r): id(_id), lastPoll(_r == Reaping::Automatic ? std::chrono::system_clock::now() : std::chrono::system_clock::time_point::max()) {}

	h256 id;
	LocalisedLogEntries changes = LocalisedLogEntries{ InitialChange };
	mutable std::chrono::system_clock::time_point lastPoll = std::chrono::system_clock::now();
};
	
	
struct WatchChannel: public LogChannel { static const char* name() { return "(o)"; } static const int verbosity = 7; };
#define cwatch dev::LogOutputStream<dev::eth::WatchChannel, true>()
struct WorkInChannel: public LogChannel { static const char* name() { return ">W>"; } static const int verbosity = 16; };
struct WorkOutChannel: public LogChannel { static const char* name() { return "<W<"; } static const int verbosity = 16; };
struct WorkChannel: public LogChannel { static const char* name() { return "-W-"; } static const int verbosity = 16; };
#define cwork dev::LogOutputStream<dev::eth::WorkChannel, true>()
#define cworkin dev::LogOutputStream<dev::eth::WorkInChannel, true>()
#define cworkout dev::LogOutputStream<dev::eth::WorkOutChannel, true>()

class InterfaceStub: public dev::eth::Interface
{
public:
	InterfaceStub() {}
	virtual ~InterfaceStub() {}
	
	/// Submits the given message-call transaction.
	virtual void transact(Secret _secret, u256 _value, Address _dest, bytes const& _data = bytes(), u256 _gas = 10000, u256 _gasPrice = 10 * szabo) override;

	/// Submits a new contract-creation transaction.
	/// @returns the new contract's address (assuming it all goes through).
	virtual Address transact(Secret _secret, u256 _endowment, bytes const& _init, u256 _gas = 10000, u256 _gasPrice = 10 * szabo) override;
	
	/// Blocks until all pending transactions have been processed.
	virtual void flushTransactions() override;
	
	/// Makes the given call. Nothing is recorded into the state.
	virtual bytes call(Secret _secret, u256 _value, Address _dest, bytes const& _data = bytes(), u256 _gas = 10000, u256 _gasPrice = 10 * szabo, int _blockNumber = 0) override;
	
	using Interface::balanceAt;
	using Interface::countAt;
	using Interface::stateAt;
	using Interface::codeAt;
	using Interface::storageAt;

	u256 balanceAt(Address _a, int _block) const override;
	u256 countAt(Address _a, int _block) const override;
	u256 stateAt(Address _a, u256 _l, int _block) const override;
	bytes codeAt(Address _a, int _block) const override;
	std::map<u256, u256> storageAt(Address _a, int _block) const override;
	
	virtual LocalisedLogEntries logs(unsigned _watchId) const override;
	virtual LocalisedLogEntries logs(LogFilter const& _filter) const override;

	/// Install, uninstall and query watches.
	virtual unsigned installWatch(LogFilter const& _filter, Reaping _r = Reaping::Automatic) override;
	virtual unsigned installWatch(h256 _filterId, Reaping _r = Reaping::Automatic) override;
	virtual bool uninstallWatch(unsigned _watchId) override;
	virtual LocalisedLogEntries peekWatch(unsigned _watchId) const override;
	virtual LocalisedLogEntries checkWatch(unsigned _watchId) override;

	h256 hashFromNumber(unsigned _number) const override;
	eth::BlockInfo blockInfo(h256 _hash) const override;
	eth::BlockDetails blockDetails(h256 _hash) const override;
	eth::Transaction transaction(h256 _transactionHash) const override;
	eth::Transaction transaction(h256 _blockHash, unsigned _i) const override;
	eth::Transactions transactions(h256 _blockHash) const override;
	eth::TransactionHashes transactionHashes(h256 _blockHash) const override;
	eth::BlockInfo uncle(h256 _blockHash, unsigned _i) const override;
	unsigned transactionCount(h256 _blockHash) const override;
	unsigned uncleCount(h256 _blockHash) const override;
	unsigned number() const override;
	eth::Transactions pending() const override;

	using Interface::addresses;
	Addresses addresses(int _block) const override;
	u256 gasLimitRemaining() const override;

	/// Set coinbase address.
	void setAddress(Address _us) override;
	/// Get the coinbase address.`
	Address address() const override;

virtual BlockChain const& bc() const = 0;	
virtual State asOf(int _h) const = 0;
virtual State preMine() const = 0;
virtual State postMine() const = 0;

protected:
	
	TransactionQueue m_tq;					///< Maintains a list of incoming transactions not yet in a block on the blockchain.
	
	// filters
	mutable Mutex m_filterLock;
	std::map<h256, InstalledFilter> m_filters;
	std::map<unsigned, ClientWatch> m_watches;

private:

};

}}
