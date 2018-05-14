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
#include <test/tools/libtestutils/Common.h>
#include <libethashseal/GenesisInfo.h>
#include <test/tools/libtesteth/JsonSpiritHeaders.h>
#include <libethereum/State.h>

namespace dev
{
namespace test
{

class ImportTest
{
public:
	ImportTest(json_spirit::mObject const& _input, json_spirit::mObject& _output);

	// imports
	void importEnv(json_spirit::mObject const& _o);
	static void importState(json_spirit::mObject const& _o, eth::State& _state);
	static void importState(json_spirit::mObject const& _o, eth::State& _state, eth::AccountMaskMap& o_mask);
	static void importTransaction (json_spirit::mObject const& _o, eth::Transaction& o_tr);
	void importTransaction(json_spirit::mObject const& _o);
	static json_spirit::mObject makeAllFieldsHex(json_spirit::mObject const& _o, bool _isHeader = false);
    static void parseJsonStrValueIntoSet(
        json_spirit::mValue const& _json, std::set<std::string>& _out);

    enum testType
    {
        StateTest,
        BlockchainTest
    };
    static std::set<eth::Network> getAllNetworksFromExpectSections(
        json_spirit::mArray const& _expects, testType _testType);


    //check functions
	//check that networks in the vector are allowed
    static void checkAllowedNetwork(std::string const& _network);
    static void checkAllowedNetwork(std::set<std::string> const& _networks);
    static void checkBalance(eth::State const& _pre, eth::State const& _post, bigint _miningReward = 0);

    bytes executeTest(bool _isFilling);
    int exportTest();
	static int compareStates(eth::State const& _stateExpect, eth::State const& _statePost, eth::AccountMaskMap const _expectedStateOptions = eth::AccountMaskMap(), WhenError _throw = WhenError::Throw);
	bool checkGeneralTestSection(json_spirit::mObject const& _expects, std::vector<size_t>& _errorTransactions, std::string const& _network="") const;
	void traceStateDiff();

	eth::State m_statePre;
	eth::State m_statePost;

private:
	using ExecOutput = std::pair<eth::ExecutionResult, eth::TransactionReceipt>;
	std::tuple<eth::State, ExecOutput, eth::ChangeLog> executeTransaction(eth::Network const _sealEngineNetwork, eth::EnvInfo const& _env, eth::State const& _preState, eth::Transaction const& _tr);

	std::unique_ptr<eth::LastBlockHashesFace const> m_lastBlockHashes;
	std::unique_ptr<eth::EnvInfo> m_envInfo;
	eth::Transaction m_transaction;

	//General State Tests
	struct transactionToExecute
	{
		transactionToExecute(int d, int g, int v, eth::Transaction const& t):
			dataInd(d), gasInd(g), valInd(v), transaction(t), postState(0), netId(eth::Network::MainNetwork),
			output(std::make_pair(eth::ExecutionResult(), eth::TransactionReceipt(h256(), u256(), eth::LogEntries()))) {}
		int dataInd;
		int gasInd;
		int valInd;
		eth::Transaction transaction;
		eth::State postState;
		eth::ChangeLog changeLog;
		eth::Network netId;
		ExecOutput output;
	};
	std::vector<transactionToExecute> m_transactions;
	using StateAndMap = std::pair<eth::State, eth::AccountMaskMap>;
	using TrExpectSection = std::pair<transactionToExecute, StateAndMap>;
	bool checkGeneralTestSectionSearch(json_spirit::mObject const& _expects, std::vector<size_t>& _errorTransactions, std::string const& _network = "", TrExpectSection* _search = NULL) const;

    /// Create blockchain test fillers for specified _networks and test information (env, pre, txs)
    /// of Importtest then fill blockchain fillers into tests.
    void makeBlockchainTestFromStateTest(std::set<eth::Network> const& _networks) const;

    json_spirit::mObject const& m_testInputObject;
	json_spirit::mObject& m_testOutputObject;

    Logger m_logger{createLogger(VerbosityInfo, "state")};
};

template<class T>
bool inArray(std::vector<T> const& _array, const T& _val)
{
	for (auto const& obj: _array)
		if (obj == _val)
			return true;
	return false;
}

} //namespace test
} //namespace dev
