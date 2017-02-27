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
/** @file
 * Helper class for managing data when running state tests
 */

#pragma once
#include <test/libtestutils/Common.h>
#include <libethashseal/GenesisInfo.h>
#include <test/libtesteth/JsonSpiritHeaders.h>
#include <libethereum/State.h>

namespace dev
{
namespace test
{

enum class testType
{
	StateTests,
	BlockChainTests,
	GeneralStateTest,
	Other
};

class ImportTest
{
public:
	ImportTest(json_spirit::mObject& _o, bool isFiller, testType testTemplate = testType::StateTests);

	// imports
	void importEnv(json_spirit::mObject& _o);
	static void importState(json_spirit::mObject const& _o, eth::State& _state);
	static void importState(json_spirit::mObject const& _o, eth::State& _state, eth::AccountMaskMap& o_mask);
	static void importTransaction (json_spirit::mObject const& _o, eth::Transaction& o_tr);
	void importTransaction(json_spirit::mObject const& _o);
	static json_spirit::mObject& makeAllFieldsHex(json_spirit::mObject& _o);

	bytes executeTest();
	int exportTest(bytes const& _output);
	static int compareStates(eth::State const& _stateExpect, eth::State const& _statePost, eth::AccountMaskMap const _expectedStateOptions = eth::AccountMaskMap(), WhenError _throw = WhenError::Throw);
	void checkGeneralTestSection(json_spirit::mObject const& _expects, std::vector<size_t>& _errorTransactions, std::string const& _network="") const;

	eth::State m_statePre;
	eth::State m_statePost;
	eth::LogEntries m_logs;
	eth::LogEntries m_logsExpected;

private:
	typedef std::pair<eth::ExecutionResult, eth::TransactionReceipt> execOutput;
	std::pair<eth::State, execOutput> executeTransaction(eth::Network const _sealEngineNetwork, eth::EnvInfo const& _env, eth::State const& _preState, eth::Transaction const& _tr);

	eth::EnvInfo m_envInfo;
	eth::Transaction m_transaction;

	//General State Tests
	struct transactionToExecute
	{
		transactionToExecute(int d, int g, int v, eth::Transaction const& t):
			dataInd(d), gasInd(g), valInd(v), transaction(t), postState(0), netId(eth::Network::MainNetwork) {}
		int dataInd;
		int gasInd;
		int valInd;
		eth::Transaction transaction;
		eth::State postState;
		eth::Network netId;
	};
	std::vector<transactionToExecute> m_transactions;
	using StateAndMap = std::pair<eth::State, eth::AccountMaskMap>;
	using TrExpectSection = std::pair<transactionToExecute, StateAndMap>;
	void checkGeneralTestSectionSearch(json_spirit::mObject const& _expects, std::vector<size_t>& _errorTransactions, std::string const& _network = "", TrExpectSection* _search = NULL) const;

	json_spirit::mObject& m_testObject;
	testType m_testType;
};

} //namespace test
} //namespace dev
