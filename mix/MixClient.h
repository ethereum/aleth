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
/** @file MixClient.h
 * @author Yann yann@ethdev.com
 * @author Arkadiy Paronyan arkadiy@ethdev.com
 * @date 2015
 * Ethereum IDE client.
 */

#pragma once

#include <vector>
#include <string>
#include <libethereum/InterfaceStub.h>
#include <libethereum/Client.h>
#include "MachineStates.h"

namespace dev
{
namespace mix
{

class MixBlockChain: public dev::eth::BlockChain
{
public:
	MixBlockChain(std::string const& _path, h256 _stateRoot): BlockChain(createGenesisBlock(_stateRoot), _path, true) {}
		
	static bytes createGenesisBlock(h256 _stateRoot);
};

class MixClient: public dev::eth::InterfaceStub
{
public:
	MixClient(std::string const& _dbPath);
	virtual ~MixClient();
	/// Reset state to the empty state with given balance.
	void resetState(std::map<Secret, u256> _accounts);
	void mine();
	ExecutionResult const& lastExecution() const;
	ExecutionResults const& executions() const;

	//dev::eth::Interface
	void transact(Secret _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice) override;
	Address transact(Secret _secret, u256 _endowment, bytes const& _init, u256 _gas, u256 _gasPrice) override;
	bytes call(Secret _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice, int _blockNumber) override;
	bool uninstallWatch(unsigned _watchId) override;
	eth::LocalisedLogEntries peekWatch(unsigned _watchId) const override;
	eth::LocalisedLogEntries checkWatch(unsigned _watchId) override;
	eth::StateDiff diff(unsigned _txi, h256 _block) const override;
	eth::StateDiff diff(unsigned _txi, int _block) const override;
	u256 gasLimitRemaining() const override;
	void setAddress(Address _us) override;
	Address address() const override;
	void setMiningThreads(unsigned _threads) override;
	unsigned miningThreads() const override;
	void startMining() override;
	void stopMining() override;
	bool isMining() override;
	eth::MineProgress miningProgress() const override;
	std::pair<h256, u256> getWork() override { return std::pair<h256, u256>(); }
	bool submitWork(eth::ProofOfWork::Proof const&) override { return false; }
	
	/// @returns the last mined block information
	eth::BlockInfo blockInfo() const;
	std::vector<KeyPair> userAccounts() { return m_userAccounts; }

	virtual dev::eth::State asOf(int _block) const override;
	virtual dev::eth::BlockChain& bc() { return *m_bc; }
	virtual dev::eth::BlockChain const& bc() const override { return *m_bc; }
	virtual dev::eth::State preMine() const override { return m_startState; }
	virtual dev::eth::State postMine() const override { return m_state; }
	
private:
	void executeTransaction(dev::eth::Transaction const& _t, eth::State& _state, bool _call);
	void noteChanged(h256Set const& _filters);

	std::vector<KeyPair> m_userAccounts;
	eth::State m_state;
	eth::State m_startState;
	OverlayDB m_stateDB;
	std::auto_ptr<MixBlockChain> m_bc;
	mutable boost::shared_mutex x_state;
	ExecutionResults m_executions;
	std::string m_dbPath;
	unsigned m_minigThreads;
};

}

}
