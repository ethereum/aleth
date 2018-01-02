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

#include <test/tools/libtesteth/TestHelper.h>
#include <test/tools/libtesteth/ImportTest.h>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <test/tools/libtesteth/BlockChainHelper.h>
#include <test/tools/libtesteth/Options.h>
#include <test/tools/libtestutils/Common.h>
#include <test/tools/libtestutils/TestLastBlockHashes.h>
#include <test/tools/jsontests/BlockChainTests.h>

#include <boost/filesystem/path.hpp>

using namespace dev;
using namespace dev::test;
using namespace std;
namespace fs = boost::filesystem;

namespace
{
vector<h256> lastHashes(u256 _currentBlockNumber)
{
	vector<h256> ret;
	for (u256 i = 1; i <= 256 && i <= _currentBlockNumber; ++i)
		ret.push_back(sha3(toString(_currentBlockNumber - i)));
	return ret;
}
}

ImportTest::ImportTest(json_spirit::mObject const& _input, json_spirit::mObject& _output):
	m_statePre(0, OverlayDB(), eth::BaseState::Empty),
	m_statePost(0, OverlayDB(), eth::BaseState::Empty),
	m_testInputObject(_input),
	m_testOutputObject(_output)
{
	importEnv(_input.at("env").get_obj());
	importTransaction(_input.at("transaction").get_obj());
	importState(_input.at("pre").get_obj(), m_statePre);
}

bytes ImportTest::executeTest()
{
	assert(m_envInfo);

	vector<eth::Network> networks;
	if (!Options::get().singleTestNet.empty())
		networks.push_back(stringToNetId(Options::get().singleTestNet));
	else
		networks = test::getNetworks();

	vector<transactionToExecute> transactionResults;
	for (auto const& net : networks)
	{
		if (isDisabledNetwork(net))
			continue;

		for (auto& tr : m_transactions)
		{
			Options const& opt = Options::get();
			if(opt.trDataIndex != -1 && opt.trDataIndex != tr.dataInd)
				continue;
			if(opt.trGasIndex != -1 && opt.trGasIndex != tr.gasInd)
				continue;
			if(opt.trValueIndex != -1 && opt.trValueIndex != tr.valInd)
				continue;

			std::tie(tr.postState, tr.output, tr.changeLog) =
				executeTransaction(net, *m_envInfo, m_statePre, tr.transaction);
			tr.netId = net;
			transactionResults.push_back(tr);
		}
	}

	//Generate blockchain test filler
	if (Options::get().fillchain)
	{
		string testnameOrig = TestOutputHelper::get().testName();
		for (auto& tr : m_transactions)
		{
			json_spirit::mObject json;
			json_spirit::mObject testObj;

			//generate the test name with transaction detail
			string postfix = "_d" + toString(tr.dataInd);
			postfix += "g" + toString(tr.gasInd);
			postfix += "v" + toString(tr.valInd);
			string testname = testnameOrig + postfix;

			//basic genesis
			json_spirit::mObject genesisObj = TestBlockChain::defaultGenesisBlockJson();
			genesisObj["coinbase"] = toString(m_envInfo->author());
			genesisObj["gasLimit"] = toCompactHexPrefixed(m_envInfo->gasLimit());
			genesisObj["timestamp"] = toCompactHexPrefixed(m_envInfo->timestamp() - 50);
			testObj["genesisBlockHeader"] = genesisObj;
			testObj["pre"] = fillJsonWithState(m_statePre);

			//generate expect sections for this transaction
			if (m_testInputObject.count("expect"))
			{
				State s = State (0, OverlayDB(), eth::BaseState::Empty);
				AccountMaskMap m = std::unordered_map<Address, AccountMask>();
				StateAndMap smap {s, m};
				vector<size_t> stateIndexesToPrint; //not used
				json_spirit::mArray expetSectionArray;

				for (auto const& net : networks)
				{
					tr.netId = net;

					// Calculate the block reward
					ChainParams const chainParams{genesisInfo(net)};
					EVMSchedule const schedule = chainParams.scheduleForBlockNumber(1);
					u256 const blockReward = chainParams.blockReward(schedule);

					TrExpectSection search {tr, smap};
					for (auto const& exp: m_testInputObject.at("expect").get_array())
					{
						TrExpectSection* search2 = &search;
						checkGeneralTestSectionSearch(exp.get_obj(), stateIndexesToPrint, "", search2);
						if (search.second.first.addresses().size() != 0) //if match in the expect sections for this tr found
						{
							//replace expected mining reward (in state tests it is 0)
							json_spirit::mObject obj = fillJsonWithState(search2->second.first, search2->second.second);
							for (auto& adr: obj)
							{
								if (adr.first == toHexPrefixed(m_envInfo->author()))
								{
									if (adr.second.get_obj().count("balance"))
									{
										u256 expectCoinbaseBalance = toInt(adr.second.get_obj()["balance"]);
										expectCoinbaseBalance += blockReward;
										adr.second.get_obj()["balance"] = toCompactHexPrefixed(expectCoinbaseBalance);
									}
								}
							}

							json_spirit::mObject expetSectionObj;
							expetSectionObj["network"] = test::netIdToString(net);
							expetSectionObj["result"] = obj;
							expetSectionArray.push_back(expetSectionObj);
							break;
						}
					}//for exp
				}// for net

				testObj["expect"] = expetSectionArray;
			}//expect

			//rewrite header section for a block by the statetest parameters
			json_spirit::mObject rewriteHeader;
			rewriteHeader["gasLimit"] = toCompactHexPrefixed(m_envInfo->gasLimit());
			rewriteHeader["difficulty"] = toCompactHexPrefixed(m_envInfo->difficulty());
			rewriteHeader["timestamp"] = toCompactHexPrefixed(m_envInfo->timestamp());
			rewriteHeader["updatePoW"] = "1";

			json_spirit::mArray blocksArr;
			json_spirit::mArray transcArr;
			transcArr.push_back(fillJsonWithTransaction(tr.transaction));
			json_spirit::mObject blocksObj;
			blocksObj["blockHeaderPremine"] = rewriteHeader;
			blocksObj["transactions"] = transcArr;
			blocksObj["uncleHeaders"] = json_spirit::mArray();
			blocksArr.push_back(blocksObj);
			testObj["blocks"] = blocksArr;
			json[testname] = testObj;

			// Write a filler file to the filler folder
			BCGeneralStateTestsSuite genSuite;
			fs::path const testFillerFile = genSuite.getFullPathFiller(TestOutputHelper::get().caseName()) / fs::path(testname + "Filler.json");
			writeFile(testFillerFile, asBytes(json_spirit::write_string((mValue)json, true)));

			// Execute test filling for this file
			genSuite.executeTest(TestOutputHelper::get().caseName(), testFillerFile);

		} //transactions
	}//fillchain

	m_transactions.clear();
	m_transactions = transactionResults;
	return bytes();
}

void ImportTest::checkBalance(eth::State const& _pre, eth::State const& _post, bigint _miningReward)
{
	bigint preBalance = 0;
	bigint postBalance = 0;
	for (auto const& addr : _pre.addresses())
		preBalance += addr.second;
	for (auto const& addr : _post.addresses())
		postBalance += addr.second;

	//account could destroy ether if it suicides to itself
	BOOST_REQUIRE_MESSAGE(preBalance + _miningReward >= postBalance, "Error when comparing states: preBalance + miningReward < postBalance (" + toString(preBalance) + " < " + toString(postBalance) + ") " + TestOutputHelper::get().testName());
}

std::tuple<eth::State, ImportTest::ExecOutput, eth::ChangeLog> ImportTest::executeTransaction(eth::Network const _sealEngineNetwork, eth::EnvInfo const& _env, eth::State const& _preState, eth::Transaction const& _tr)
{
	assert(m_envInfo);

	State initialState = _preState;
	ExecOutput out(std::make_pair(eth::ExecutionResult(), eth::TransactionReceipt(h256(), u256(), eth::LogEntries())));
	try
	{
		unique_ptr<SealEngineFace> se(ChainParams(genesisInfo(_sealEngineNetwork)).createSealEngine());
		if (Options::get().jsontrace)
		{
			StandardTrace st;
			st.setShowMnemonics();
			st.setOptions(Options::get().jsontraceOptions);
			out = initialState.execute(_env, *se.get(), _tr, Permanence::Committed, st.onOp());
			cout << st.json();
			cout << "{\"stateRoot\": \"" << initialState.rootHash().hex() << "\"}";
		}
		else
			out = initialState.execute(_env, *se.get(), _tr, Permanence::Uncommitted);

		// the changeLog might be broken under --jsontrace, because it uses intialState.execute with Permanence::Committed rather than Permanence::Uncommitted
		eth::ChangeLog changeLog = initialState.changeLog();
		ImportTest::checkBalance(_preState, initialState);

		//Finalize the state manually (clear logs)
		bool removeEmptyAccounts = m_envInfo->number() >= se->chainParams().EIP158ForkBlock;
		initialState.commit(removeEmptyAccounts ? State::CommitBehaviour::RemoveEmptyAccounts : State::CommitBehaviour::KeepEmptyAccounts);
		return std::make_tuple(initialState, out, changeLog);
	}
	catch (Exception const& _e)
	{
		cnote << "Exception: " << diagnostic_information(_e);
	}
	catch (std::exception const& _e)
	{
		cnote << "state execution exception: " << _e.what();
	}

	initialState.commit(State::CommitBehaviour::KeepEmptyAccounts);
	return std::make_tuple(initialState, out, initialState.changeLog());
}

json_spirit::mObject ImportTest::makeAllFieldsHex(json_spirit::mObject const& _input, bool _isHeader)
{
	static const set<string> hashes {"bloom" , "coinbase", "hash", "mixHash", "parentHash", "receiptTrie",
									 "stateRoot", "transactionsTrie", "uncleHash", "currentCoinbase",
									 "previousHash", "to", "address", "caller", "origin", "secretKey", "data", "extraData"};

	json_spirit::mObject output = _input;

	for (auto const& i: output)
	{
		bool isHash = false;
        bool isData = false;
        std::string key = i.first;

        if (key == "extraData")
            continue;

        if (key == "data")
            isData = true;

        if (hashes.count(key))
			isHash = true;

		if (_isHeader && key == "nonce")
			isHash = true;

		std::string str;
		json_spirit::mValue value = i.second;

		if (value.type() == json_spirit::int_type)
			str = toString(value.get_int());
		else if (value.type() == json_spirit::str_type)
            str = isData ? replaceLLL(value.get_str()) : value.get_str();
        else if (value.type() == json_spirit::array_type)
		{
			json_spirit::mArray arr;
			for (auto const& j: value.get_array())
			{
                str = isData ? replaceLLL(j.get_str()) : j.get_str();
                arr.push_back((str.substr(0, 2) == "0x") ? str : toCompactHexPrefixed(toInt(str), 1));
			}
			output[key] = arr;
			continue;
		}
		else continue;

		if (isHash)
			output[key] = (str.substr(0, 2) == "0x" || str.empty()) ? str : "0x" + str;
		else
			output[key] = (str.substr(0, 2) == "0x") ? str : toCompactHexPrefixed(toInt(str), 1);
	}
	return output;
}

void ImportTest::importEnv(json_spirit::mObject const& _o)
{
	BOOST_REQUIRE(_o.count("currentGasLimit") > 0);
	BOOST_REQUIRE(_o.count("currentDifficulty") > 0);
	BOOST_REQUIRE(_o.count("currentNumber") > 0);
	BOOST_REQUIRE(_o.count("currentTimestamp") > 0);
	BOOST_REQUIRE(_o.count("currentCoinbase") > 0);
	auto gasLimit = toInt(_o.at("currentGasLimit"));
	BOOST_REQUIRE(gasLimit <= std::numeric_limits<int64_t>::max());
	BlockHeader header;
	header.setGasLimit(gasLimit.convert_to<int64_t>());
	header.setDifficulty(toInt(_o.at("currentDifficulty")));
	header.setNumber(toInt(_o.at("currentNumber")));
	header.setTimestamp(toInt(_o.at("currentTimestamp")));
	header.setAuthor(Address(_o.at("currentCoinbase").get_str()));

	m_lastBlockHashes.reset(new TestLastBlockHashes(lastHashes(header.number())));
	m_envInfo.reset(new EnvInfo(header, *m_lastBlockHashes, 0));
}

// import state from not fully declared json_spirit::mObject, writing to _stateOptionsMap which fields were defined in json
void ImportTest::importState(json_spirit::mObject const& _o, State& _state, AccountMaskMap& o_mask)
{
	//Compile LLL code of the test Fillers using external call to lllc
	json_spirit::mObject o = _o;
	replaceLLLinState(o);
	std::string jsondata = json_spirit::write_string((json_spirit::mValue)o, false);
	_state.populateFrom(jsonToAccountMap(jsondata, 0, &o_mask));
}

void ImportTest::importState(json_spirit::mObject const& _o, State& _state)
{
	AccountMaskMap mask;
	importState(_o, _state, mask);
	//check that every parameter was declared in state object
	for (auto const& i: mask)
		if (!i.second.allSet())
			BOOST_THROW_EXCEPTION(MissingFields() << errinfo_comment("Import State: Missing state fields!"));
}

void ImportTest::importTransaction (json_spirit::mObject const& _o, eth::Transaction& o_tr)
{
	if (_o.count("secretKey") > 0)
	{
		BOOST_REQUIRE(_o.count("nonce") > 0);
		BOOST_REQUIRE(_o.count("gasPrice") > 0);
		BOOST_REQUIRE(_o.count("gasLimit") > 0);
		BOOST_REQUIRE(_o.count("to") > 0);
		BOOST_REQUIRE(_o.count("value") > 0);
		BOOST_REQUIRE(_o.count("data") > 0);

		if (bigint(_o.at("nonce").get_str()) >= c_max256plus1)
			BOOST_THROW_EXCEPTION(ValueTooLarge() << errinfo_comment("Transaction 'nonce' is equal or greater than 2**256") );
		if (bigint(_o.at("gasPrice").get_str()) >= c_max256plus1)
			BOOST_THROW_EXCEPTION(ValueTooLarge() << errinfo_comment("Transaction 'gasPrice' is equal or greater than 2**256") );
		if (bigint(_o.at("gasLimit").get_str()) >= c_max256plus1)
			BOOST_THROW_EXCEPTION(ValueTooLarge() << errinfo_comment("Transaction 'gasLimit' is equal or greater than 2**256") );
		if (bigint(_o.at("value").get_str()) >= c_max256plus1)
			BOOST_THROW_EXCEPTION(ValueTooLarge() << errinfo_comment("Transaction 'value' is equal or greater than 2**256") );

		o_tr = _o.at("to").get_str().empty() ?
			Transaction(toInt(_o.at("value")), toInt(_o.at("gasPrice")), toInt(_o.at("gasLimit")), importData(_o), toInt(_o.at("nonce")), Secret(_o.at("secretKey").get_str())) :
			Transaction(toInt(_o.at("value")), toInt(_o.at("gasPrice")), toInt(_o.at("gasLimit")), Address(_o.at("to").get_str()), importData(_o), toInt(_o.at("nonce")), Secret(_o.at("secretKey").get_str()));
	}
	else
	{
		BOOST_REQUIRE(_o.count("nonce"));
		BOOST_REQUIRE(_o.count("gasPrice"));
		BOOST_REQUIRE(_o.count("gasLimit"));
		BOOST_REQUIRE(_o.count("to"));
		BOOST_REQUIRE(_o.count("value"));
		BOOST_REQUIRE(_o.count("data"));
		BOOST_REQUIRE(_o.count("v"));
		BOOST_REQUIRE(_o.count("r"));
		BOOST_REQUIRE(_o.count("s"));

		RLPStream transactionRLPStream = createRLPStreamFromTransactionFields(_o);
		RLP transactionRLP(transactionRLPStream.out());
		try
		{
			o_tr = Transaction(transactionRLP.data(), CheckTransaction::Everything);
		}
		catch (InvalidSignature)
		{
			// create unsigned transaction
			o_tr = _o.at("to").get_str().empty() ?
				Transaction(toInt(_o.at("value")), toInt(_o.at("gasPrice")), toInt(_o.at("gasLimit")), importData(_o), toInt(_o.at("nonce"))) :
				Transaction(toInt(_o.at("value")), toInt(_o.at("gasPrice")), toInt(_o.at("gasLimit")), Address(_o.at("to").get_str()), importData(_o), toInt(_o.at("nonce")));
		}
		catch (Exception& _e)
		{
			cnote << "invalid transaction" << boost::diagnostic_information(_e);
		}
	}
}

void ImportTest::importTransaction(json_spirit::mObject const& o_tr)
{
	//Parse extended transaction
	BOOST_REQUIRE(o_tr.count("gasLimit") > 0);
	size_t dataVectorSize = o_tr.at("data").get_array().size();
	size_t gasVectorSize = o_tr.at("gasLimit").get_array().size();
	size_t valueVectorSize = o_tr.at("value").get_array().size();

	BOOST_REQUIRE_MESSAGE(dataVectorSize > 0, "Transaction should has at least 1 data item!");
	BOOST_REQUIRE_MESSAGE(gasVectorSize > 0, "Transaction should has at least 1 gas item!");
	BOOST_REQUIRE_MESSAGE(valueVectorSize > 0, "Transaction should has at least 1 value item!");

	for (size_t d = 0; d < dataVectorSize; d++)
		for (size_t g = 0; g < gasVectorSize; g++)
			for (size_t v = 0; v < valueVectorSize; v++)
			{
				json_spirit::mValue gas = o_tr.at("gasLimit").get_array().at(g);
				json_spirit::mValue value = o_tr.at("value").get_array().at(v);
				json_spirit::mValue data = o_tr.at("data").get_array().at(d);

				json_spirit::mObject o_tr_tmp = o_tr;
				o_tr_tmp["data"] = data;
				o_tr_tmp["gasLimit"] = gas;
				o_tr_tmp["value"] = value;

				importTransaction(o_tr_tmp, m_transaction);

				transactionToExecute execData(d, g, v, m_transaction);
				m_transactions.push_back(execData);
			}
}

int ImportTest::compareStates(State const& _stateExpect, State const& _statePost, AccountMaskMap const _expectedStateOptions, WhenError _throw)
{
	bool wasError = false;
	#define CHECK(a,b)						\
		{									\
			if (_throw == WhenError::Throw) \
			{								\
				BOOST_CHECK_MESSAGE(a, b);	\
				if (!a)						\
					return 1;				\
			}								\
			else							\
			{								\
				BOOST_WARN_MESSAGE(a,b);	\
				if (!a)						\
					wasError = true;		\
			}								\
		}

	for (auto const& a: _stateExpect.addresses())
	{
		AccountMask addressOptions(true);
		if(_expectedStateOptions.size())
		{
			try
			{
				addressOptions = _expectedStateOptions.at(a.first);
			}
			catch(std::out_of_range const&)
			{
				BOOST_ERROR(TestOutputHelper::get().testName() + " expectedStateOptions map does not match expectedState in checkExpectedState!");
				break;
			}
		}

		if (addressOptions.shouldExist())
		{
			CHECK(_statePost.addressInUse(a.first), TestOutputHelper::get().testName() +  " Compare States: " << a.first << " missing expected address!");
		}
		else
		{
			CHECK(!_statePost.addressInUse(a.first), TestOutputHelper::get().testName() +  " Compare States: " << a.first << " address not expected to exist!");
		}

		if (_statePost.addressInUse(a.first))
		{

			if (addressOptions.hasBalance())
				CHECK((_stateExpect.balance(a.first) == _statePost.balance(a.first)),
				TestOutputHelper::get().testName() + " Check State: " << a.first <<  ": incorrect balance " << _statePost.balance(a.first) << ", expected " << _stateExpect.balance(a.first));

			if (addressOptions.hasNonce())
				CHECK((_stateExpect.getNonce(a.first) == _statePost.getNonce(a.first)),
				TestOutputHelper::get().testName() + " Check State: " << a.first <<  ": incorrect nonce " << _statePost.getNonce(a.first) << ", expected " << _stateExpect.getNonce(a.first));

			if (addressOptions.hasStorage())
			{
				map<h256, pair<u256, u256>> stateStorage = _statePost.storage(a.first);
				for (auto const& s: _stateExpect.storage(a.first))
					CHECK((stateStorage[s.first] == s.second),
					TestOutputHelper::get().testName() + " Check State: " << a.first << ": incorrect storage [" << toCompactHexPrefixed(s.second.first) << "] = " << toCompactHexPrefixed(stateStorage[s.first].second) << ", expected [" << toCompactHexPrefixed(s.second.first) << "] = " << toCompactHexPrefixed(s.second.second));

				//Check for unexpected storage values
				map<h256, pair<u256, u256>> expectedStorage = _stateExpect.storage(a.first);
				for (auto const& s: _statePost.storage(a.first))
					CHECK((expectedStorage[s.first] == s.second),
					TestOutputHelper::get().testName() + " Check State: " << a.first << ": incorrect storage [" << toCompactHexPrefixed(s.second.first) << "] = " << toCompactHexPrefixed(s.second.second) << ", expected [" << toCompactHexPrefixed(s.second.first) << "] = " << toCompactHexPrefixed(expectedStorage[s.first].second));
			}

			if (addressOptions.hasCode())
				CHECK((_stateExpect.code(a.first) == _statePost.code(a.first)),
				TestOutputHelper::get().testName() + " Check State: " << a.first <<  ": incorrect code '" << toHexPrefixed(_statePost.code(a.first)) << "', expected '" << toHexPrefixed(_stateExpect.code(a.first)) << "'");
		}
	}

	return wasError;
}

void ImportTest::parseJsonStrValueIntoVector(json_spirit::mValue const& _json, vector<string>& _out)
{
	if (_json.type() == json_spirit::array_type)
	{
		for (auto const& val: _json.get_array())
			_out.push_back(val.get_str());
	}
	else
		_out.push_back(_json.get_str());
}

void parseJsonIntValueIntoVector(json_spirit::mValue const& _json, vector<int>& _out)
{
	if (_json.type() == json_spirit::array_type)
	{
		for (auto const& val: _json.get_array())
			_out.push_back(val.get_int());
	}
	else
		_out.push_back(_json.get_int());
}

void ImportTest::checkAllowedNetwork(std::vector<std::string> const& _networks)
{
	vector<eth::Network> const& allnetworks = test::getNetworks();
	vector<string> allowedNetowks;
	allowedNetowks.push_back("ALL");
	for (auto const& net : allnetworks)
		allowedNetowks.push_back(test::netIdToString(net));

	for (auto const& net: _networks)
	{
		if (!inArray(allowedNetowks, net))
		{
			//Can't use boost at this point
			std::cerr << TestOutputHelper::get().testName() + " Specified Network not found: " << net << "\n";
			exit(1);
		}
	}
}

bool ImportTest::checkGeneralTestSection(json_spirit::mObject const& _expects, vector<size_t>& _errorTransactions, string const& _network) const
{
	return checkGeneralTestSectionSearch(_expects, _errorTransactions, _network, NULL);
}

bool ImportTest::checkGeneralTestSectionSearch(json_spirit::mObject const& _expects, vector<size_t>& _errorTransactions, string const& _network, TrExpectSection* _search) const
{
	vector<int> d;
	vector<int> g;
	vector<int> v;
	vector<string> network;
	if (_network.empty())
		parseJsonStrValueIntoVector(_expects.at("network"), network);
	else
		network.push_back(_network);

	BOOST_CHECK_MESSAGE(network.size() > 0, TestOutputHelper::get().testName() + " Network array not set!");
	checkAllowedNetwork(network);

	if (!Options::get().singleTestNet.empty())
	{
		//skip this check if we execute transactions only on another specified network
		if (!inArray(network, Options::get().singleTestNet) && !inArray(network, string{"ALL"}))
			return false;
	}

	if (_expects.count("indexes"))
	{
		json_spirit::mObject const& indexes = _expects.at("indexes").get_obj();
		parseJsonIntValueIntoVector(indexes.at("data"), d);
		parseJsonIntValueIntoVector(indexes.at("gas"), g);
		parseJsonIntValueIntoVector(indexes.at("value"), v);
		BOOST_CHECK_MESSAGE(d.size() > 0 && g.size() > 0 && v.size() > 0, TestOutputHelper::get().testName() + " Indexes arrays not set!");

		//Skip this check if does not fit to options request
		Options const& opt = Options::get();
		if (!inArray(d, opt.trDataIndex) && !inArray(d, -1) && opt.trDataIndex != -1)
			return false;
		if (!inArray(g, opt.trGasIndex) && !inArray(g, -1) && opt.trGasIndex != -1)
			return false;
		if (!inArray(v, opt.trValueIndex) && !inArray(v, -1) && opt.trValueIndex != -1)
			return false;
	}
	else
		BOOST_ERROR(TestOutputHelper::get().testName() + " indexes section not set!");

	bool foundResults = false;
	std::vector<transactionToExecute> lookTransactions;
	if (_search)
		lookTransactions.push_back(_search->first);
	else
		lookTransactions = m_transactions;
	for(size_t i = 0; i < lookTransactions.size(); i++)
	{
		transactionToExecute const& tr = lookTransactions[i];
		if (inArray(network, netIdToString(tr.netId)) || network[0] == "ALL")
		if ((inArray(d, tr.dataInd) || d[0] == -1) && (inArray(g, tr.gasInd) || g[0] == -1) && (inArray(v, tr.valInd) || v[0] == -1))
		{
			string trInfo = netIdToString(tr.netId) + " data: " + toString(tr.dataInd) + " gas: " + toString(tr.gasInd) + " val: " + toString(tr.valInd);
			if (_expects.count("result"))
			{
				Options const& opt = Options::get();
				//filter transactions if a specific index set in options
				if ((opt.trDataIndex != -1 && opt.trDataIndex != tr.dataInd) ||
					(opt.trGasIndex != -1 && opt.trGasIndex != tr.gasInd) ||
					(opt.trValueIndex != -1 && opt.trValueIndex != tr.valInd))
					continue;

				State postState = tr.postState;
				eth::AccountMaskMap stateMap;
				State expectState(0, OverlayDB(), eth::BaseState::Empty);
				importState(_expects.at("result").get_obj(), expectState, stateMap);
				if (_search)
				{
					_search->second.first = expectState;
					_search->second.second = stateMap;
					return true;
				}
				int errcode = compareStates(expectState, postState, stateMap, WhenError::Throw);
				if (errcode > 0)
				{
					cerr << trInfo << "\n";
					_errorTransactions.push_back(i);
				}
			}
			else if (_expects.count("hash"))
			{
				//checking filled state test against client
				BOOST_CHECK_MESSAGE(_expects.at("hash").get_str() == toHexPrefixed(tr.postState.rootHash().asBytes()),
									TestOutputHelper::get().testName() + " on " + test::netIdToString(tr.netId) + ": Expected another postState hash! expected: " + _expects.at("hash").get_str() + " actual: " + toHexPrefixed(tr.postState.rootHash().asBytes()) + " in " + trInfo);
				if (_expects.count("logs"))
					BOOST_CHECK_MESSAGE(_expects.at("logs").get_str() == exportLog(tr.output.second.log()),
									TestOutputHelper::get().testName() + " on " + test::netIdToString(tr.netId) + " Transaction log mismatch! expected: " + _expects.at("logs").get_str() + " actual: " + exportLog(tr.output.second.log()) + " in " + trInfo);
				else
					BOOST_ERROR(TestOutputHelper::get().testName() + "PostState missing logs field!");
			}
			else
				BOOST_ERROR(TestOutputHelper::get().testName() + " Expect section or postState missing some fields!");

			foundResults = true;

			//if a single transaction check then stop once found
			if (network[0] != "ALL" && d[0] != -1 && g[0] != -1 && v[0] != -1)
			if (network.size() == 1 && d.size() == 1 && g.size() == 1 && v.size() == 1)
				break;
		}
	}
	if (!_search) //if search for a single transaction in one of the expect sections then don't need this output.
		BOOST_CHECK_MESSAGE(foundResults, TestOutputHelper::get().testName() + " Expect results was not found in test execution!");
	return foundResults;
}

void ImportTest::traceStateDiff()
{
	string network = "ALL";
	Options const& opt = Options::get();
	if (!opt.singleTestNet.empty())
		network = opt.singleTestNet;

	int d = opt.trDataIndex;
	int g = opt.trGasIndex;
	int v = opt.trValueIndex;

	for(auto const& tr : m_transactions)
	{
		if (network == netIdToString(tr.netId) || network == "ALL")
		if ((d == tr.dataInd || d == -1) && (g == tr.gasInd || g == -1) && (v == tr.valInd || v == -1))
		{
			std::ostringstream log;
			log << "trNetID: " << netIdToString(tr.netId) << "\n";
			log << "trDataInd: " << tr.dataInd << " tdGasInd: " << tr.gasInd << " trValInd: " << tr.valInd << "\n";
			dev::LogOutputStream<eth::StateTrace, false>() << log.str();
			fillJsonWithStateChange(m_statePre, tr.postState, tr.changeLog); //output std log
		}
	}
}

int ImportTest::exportTest()
{
	int err = 0;
	vector<size_t> stateIndexesToPrint;
	if (m_testInputObject.count("expect") > 0)
	{
		for (auto const& exp: m_testInputObject.at("expect").get_array())
			checkGeneralTestSection(exp.get_obj(), stateIndexesToPrint);
	}

	std::map<string, json_spirit::mArray> postState;
	for(size_t i = 0; i < m_transactions.size(); i++)
	{
		transactionToExecute const& tr = m_transactions[i];
		json_spirit::mObject obj;
		json_spirit::mObject obj2;
		obj["data"] = tr.dataInd;
		obj["gas"] = tr.gasInd;
		obj["value"] = tr.valInd;
		obj2["indexes"] = obj;
		obj2["hash"] = toHexPrefixed(tr.postState.rootHash().asBytes());
		obj2["logs"] = exportLog(tr.output.second.log());

		//Print the post state if transaction has failed on expect section
		auto it = std::find(std::begin(stateIndexesToPrint), std::end(stateIndexesToPrint), i);
		if (it != std::end(stateIndexesToPrint))
			obj2["postState"] = fillJsonWithState(tr.postState);

		if (Options::get().statediff)
			obj2["stateDiff"] = fillJsonWithStateChange(m_statePre, tr.postState, tr.changeLog);

		postState[netIdToString(tr.netId)].push_back(obj2);
	}

	json_spirit::mObject obj;
	for(std::map<string, json_spirit::mArray>::iterator it = postState.begin(); it != postState.end(); ++it)
		obj[it->first] = it->second;

	m_testOutputObject["post"] = obj;
	m_testOutputObject["pre"] = fillJsonWithState(m_statePre);
	m_testOutputObject["env"] = makeAllFieldsHex(m_testInputObject.at("env").get_obj());
	m_testOutputObject["transaction"] = makeAllFieldsHex(m_testInputObject.at("transaction").get_obj());
	return err;
}
