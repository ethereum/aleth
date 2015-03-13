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

#include "Interface.h"

namespace dev {

namespace eth {

class InterfaceStub: public dev::eth::Interface
{
public:
	InterfaceStub() {}
	virtual ~InterfaceStub() {}
	
	void transact(Secret _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice) override {}
	Address transact(Secret _secret, u256 _endowment, bytes const& _init, u256 _gas, u256 _gasPrice) override {}
	void inject(bytesConstRef _rlp) override {}
	void flushTransactions() override {}
	bytes call(Secret _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice, int _blockNumber = 0) override {}

	using Interface::balanceAt;
	u256 balanceAt(Address _a, int _block) const override;
	u256 countAt(Address _a, int _block) const override {}
	u256 stateAt(Address _a, u256 _l, int _block) const override {}
	bytes codeAt(Address _a, int _block) const override {}
std::map<u256, u256> storageAt(Address _a, int _block) const override {}
eth::LocalisedLogEntries logs(unsigned _watchId) const override {}
eth::LocalisedLogEntries logs(eth::LogFilter const& _filter) const override {}
unsigned installWatch(eth::LogFilter const& _filter) override {}
unsigned installWatch(h256 _filterId) override {}
void uninstallWatch(unsigned _watchId) override {}
eth::LocalisedLogEntries peekWatch(unsigned _watchId) const override {}
eth::LocalisedLogEntries checkWatch(unsigned _watchId) override {}
h256 hashFromNumber(unsigned _number) const override;
eth::BlockInfo blockInfo(h256 _hash) const override {}
eth::BlockDetails blockDetails(h256 _hash) const override {}
eth::Transaction transaction(h256 _transactionHash) const override {}
eth::Transaction transaction(h256 _blockHash, unsigned _i) const override {}
eth::BlockInfo uncle(h256 _blockHash, unsigned _i) const override {}
unsigned transactionCount(h256 _blockHash) const override;
unsigned uncleCount(h256 _blockHash) const override;
unsigned number() const override;
eth::Transactions pending() const override {}
eth::StateDiff diff(unsigned _txi, h256 _block) const override {}
eth::StateDiff diff(unsigned _txi, int _block) const override {}
Addresses addresses(int _block) const override {}
u256 gasLimitRemaining() const override {}
void setAddress(Address _us) override {}
Address address() const override {}
void setMiningThreads(unsigned _threads) override {}
unsigned miningThreads() const override {}
void startMining() override {}
void stopMining() override {}
bool isMining() override {}
eth::MineProgress miningProgress() const override {}
std::pair<h256, u256> getWork() override { return std::pair<h256, u256>(); }
bool submitWork(eth::ProofOfWork::Proof const&) override { return false; }
eth::Transactions transactions(h256 _blockHash) const override {}
eth::TransactionHashes transactionHashes(h256 _blockHash) const override {}

virtual BlockChain const& bc() const = 0;
virtual State asOf(int _h) const = 0;
virtual State preMine() const = 0;
virtual State postMine() const = 0;

private:
//BlockChain const& m_bc;
State m_state;
};

}}
