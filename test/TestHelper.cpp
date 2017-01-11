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
/** @file TestHelper.cpp
 * @author Marko Simovic <markobarko@gmail.com>
 * @date 2014
 */

#include "TestHelper.h"

#include <thread>
#include <chrono>
#include <fstream>
#if !defined(_WIN32)
#include <stdio.h>
#endif
#include <boost/algorithm/string/trim.hpp>
#include <libethashseal/EthashAux.h>
#include <libethashseal/Ethash.h>
#include <libethashseal/GenesisInfo.h>
#include <libethcore/BasicAuthority.h>
#include <libethereum/Client.h>
#include <libevm/ExtVMFace.h>
#include <libevm/VMFactory.h>
#include "Stats.h"

using namespace std;
using namespace dev::eth;

namespace dev
{
namespace eth
{

void printHelp()
{
	cout << "Usage: " << std::endl;
	cout << std::left;
	cout << std::endl << "Setting test suite" << std::endl;
	cout << setw(30) <<	"-t <TestSuite>" << setw(25) << "Execute test operations" << std::endl;
	cout << setw(30) << "-t <TestSuite>/<TestCase>" << std::endl;

	cout << std::endl << "Debugging" << std::endl;
	cout << setw(30) << "--singletest <TestName>" << setw(25) << "Run on a single test" << std::endl;
	cout << setw(30) << "--singletest <TestFile> <TestName>" << std::endl;
	cout << setw(30) << "--verbosity <level>" << setw(25) << "Set logs verbosity. 0 - silent, 1 - only errors, 2 - informative, >2 - detailed" << std::endl;
	cout << setw(30) << "--vm <interpreter|jit|smart>" << setw(25) << "Set VM type for VMTests suite" << std::endl;
	cout << setw(30) << "--vmtrace" << setw(25) << "Enable VM trace for the test. (Require build with VMTRACE=1)" << std::endl;
	cout << setw(30) << "--stats <OutFile>" << setw(25) << "Output debug stats to the file" << std::endl;
	cout << setw(30) << "--filltest <FileData>" << setw(25) << "Try fill tests from the given json stream" << std::endl;
	cout << setw(30) << "--checktest <FileData>" << setw(25) << "Try run tests from the given json stream" << std::endl;

	cout << std::endl << "Additional Tests" << std::endl;
	cout << setw(30) << "--performance" << setw(25) << "Enable perfomance tests" << std::endl;
	cout << setw(30) << "--quadratic" << setw(25) << "Enable quadratic complexity tests" << std::endl;
	cout << setw(30) << "--memory" << setw(25) << "Enable memory consuming tests" << std::endl;
	cout << setw(30) << "--inputLimits" << setw(25) << "Enable inputLimits tests" << std::endl;
	cout << setw(30) << "--bigdata" << setw(25) << "Enable bigdata tests" << std::endl;
	cout << setw(30) << "--wallet" << setw(25) << "Enable wallet tests" << std::endl;
	cout << setw(30) << "--all" << setw(25) << "Enable all tests" << std::endl;

	cout << std::endl << "Test Creation" << std::endl;
	cout << setw(30) << "--filltests" << setw(25) << "Run test fillers" << std::endl;
	cout << setw(30) << "--checkstate" << setw(25) << "Enable expect section state checks" << std::endl;
	cout << setw(30) << "--createRandomTest" << setw(25) << "Create random test and output it to the console" << std::endl;
	//cout << setw(30) << "--fulloutput" << setw(25) << "Disable address compression in the output field" << std::endl;

	cout << setw(30) << "--help" << setw(25) << "Display list of command arguments" << std::endl;
}

string netIdToString(eth::Network _netId)
{
	switch(_netId)
	{
		case eth::Network::FrontierTest: return "Frontier";
		case eth::Network::HomesteadTest: return "Homestead";
		case eth::Network::EIP150Test: return "EIP150";
		case eth::Network::EIP158Test: return "EIP158";
		default: return "other";
	}
	return "unknown";
}

void mine(Client& c, int numBlocks)
{
	auto startBlock = c.blockChain().details().number;

	c.startSealing();
	while(c.blockChain().details().number < startBlock + numBlocks)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	c.stopSealing();
}

void connectClients(Client& c1, Client& c2)
{
	(void)c1;
	(void)c2;
	// TODO: Move to WebThree. eth::Client no longer handles networking.
#if 0
	short c1Port = 20000;
	short c2Port = 21000;
	c1.startNetwork(c1Port);
	c2.startNetwork(c2Port);
	c2.connect("127.0.0.1", c1Port);
#endif
}

void mine(Block& s, BlockChain const& _bc, SealEngineFace* _sealer)
{
	s.commitToSeal(_bc, s.info().extraData());
	Notified<bytes> sealed;
	_sealer->onSealGenerated([&](bytes const& sealedHeader){ sealed = sealedHeader; });
	_sealer->generateSeal(s.info());
	sealed.waitNot({});
	_sealer->onSealGenerated([](bytes const&){});
	s.sealBlock(sealed);
}

void mine(BlockHeader& _bi, SealEngineFace* _sealer, bool _verify)
{
	Notified<bytes> sealed;
	_sealer->onSealGenerated([&](bytes const& sealedHeader){ sealed = sealedHeader; });
	_sealer->generateSeal(_bi);
	sealed.waitNot({});
	_sealer->onSealGenerated([](bytes const&){});
	_bi = BlockHeader(sealed, HeaderData);
//	cdebug << "Block mined" << Ethash::boundary(_bi).hex() << Ethash::nonce(_bi) << _bi.hash(WithoutSeal).hex();
	if (_verify) //sometimes it is needed to mine incorrect blockheaders for testing
		_sealer->verify(JustSeal, _bi);
}

}

namespace test
{

struct ValueTooLarge: virtual Exception {};
struct MissingFields : virtual Exception {};

bigint const c_max256plus1 = bigint(1) << 256;

ImportTest::ImportTest(json_spirit::mObject& _o, bool isFiller, testType testTemplate):
	m_statePre(0, OverlayDB(), eth::BaseState::Empty),
	m_statePost(0, OverlayDB(), eth::BaseState::Empty),
	m_testObject(_o),
	m_testType(testTemplate)
{
	if (testTemplate == testType::StateTests || m_testType == testType::GeneralStateTest)
	{
		importEnv(_o["env"].get_obj());
		importTransaction(_o["transaction"].get_obj());
		importState(_o["pre"].get_obj(), m_statePre);
		if (!isFiller && m_testType == testType::StateTests)
		{
			if (_o.count("post"))
				importState(_o["post"].get_obj(), m_statePost);
			else
				importState(_o["postState"].get_obj(), m_statePost);
			m_logsExpected = importLog(_o["logs"].get_array());
		}
	}
}

bytes ImportTest::executeTest()
{
	if (m_testType == testType::StateTests)
	{
		eth::Network network = eth::Network::MainNetwork;
		std::pair<eth::State, ImportTest::execOutput> out = executeTransaction(network, m_envInfo, m_statePre, m_transaction);
		m_statePost = out.first;
		m_logs = out.second.second.log();
		return out.second.first.output;
	}
	else if (m_testType == testType::GeneralStateTest)
	{
		vector<eth::Network> networks;
		if (!Options::get().singleTestNet.empty())
		{
			if (netIdToString(eth::Network::FrontierTest) == Options::get().singleTestNet)
				networks.push_back(eth::Network::FrontierTest);
			else if (netIdToString(eth::Network::HomesteadTest) == Options::get().singleTestNet)
				networks.push_back(eth::Network::HomesteadTest);
			else if (netIdToString(eth::Network::EIP150Test) == Options::get().singleTestNet)
				networks.push_back(eth::Network::EIP150Test);
			else if (netIdToString(eth::Network::EIP158Test) == Options::get().singleTestNet)
				networks.push_back(eth::Network::EIP158Test);
		}
		else
		{
			networks.push_back(eth::Network::FrontierTest);
			networks.push_back(eth::Network::HomesteadTest);
			networks.push_back(eth::Network::EIP150Test);
			networks.push_back(eth::Network::EIP158Test);
		}

		vector<transactionToExecute> transactionResults;
		for (size_t j = 0; j < networks.size(); j++)
		{
			for (size_t i = 0; i < m_transactions.size(); i++)
			{
				eth::Network network = networks[j];
				std::pair<eth::State, ImportTest::execOutput> out = executeTransaction(network, m_envInfo, m_statePre, m_transactions[i].transaction);
				m_transactions[i].postState = out.first;
				m_transactions[i].netId = network;
				transactionResults.push_back(m_transactions[i]);
			}
		}
		m_transactions.clear();
		m_transactions = transactionResults;
		return bytes();
	}

	BOOST_ERROR("Error when executing test ImportTest::executeTest()");
	return bytes();
}

std::pair<eth::State, ImportTest::execOutput> ImportTest::executeTransaction(eth::Network const _sealEngineNetwork, eth::EnvInfo const& _env, eth::State const& _preState, eth::Transaction const& _tr)
{
	eth::State initialState = _preState;
	try
	{
		unique_ptr<SealEngineFace> se(ChainParams(genesisInfo(_sealEngineNetwork)).createSealEngine());
		bool removeEmptyAccounts = m_envInfo.number() >= se->chainParams().u256Param("EIP158ForkBlock");
		ImportTest::execOutput execOut = initialState.execute(_env, *se.get(), _tr);
		initialState.commit(removeEmptyAccounts ? State::CommitBehaviour::RemoveEmptyAccounts : State::CommitBehaviour::KeepEmptyAccounts);
		return std::pair<eth::State, ImportTest::execOutput>(initialState, execOut);
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
	ExecutionResult emptyRes;
	LogEntries emptyLogs;
	ImportTest::execOutput execOut = make_pair(emptyRes, TransactionReceipt(h256(), u256(), emptyLogs));
	return std::pair<eth::State, ImportTest::execOutput>(initialState, execOut);
}

json_spirit::mObject& ImportTest::makeAllFieldsHex(json_spirit::mObject& _o)
{
	static const set<string> hashes {"bloom" , "coinbase", "hash", "mixHash", "parentHash", "receiptTrie",
									 "stateRoot", "transactionsTrie", "uncleHash", "currentCoinbase",
									 "previousHash", "to", "address", "caller", "origin", "secretKey", "data"};

	for (auto& i: _o)
	{
		std::string key = i.first;
		if (hashes.count(key))
			continue;

		std::string str;
		json_spirit::mValue value = i.second;

		if (value.type() == json_spirit::int_type)
			str = toString(value.get_int());
		else if (value.type() == json_spirit::str_type)
			str = value.get_str();
		else continue;

		_o[key] = (str.substr(0, 2) == "0x") ? str : toCompactHex(toInt(str), HexPrefix::Add, 1);
	}
	return _o;
}

void ImportTest::importEnv(json_spirit::mObject& _o)
{
	BOOST_REQUIRE(_o.count("currentGasLimit") > 0);
	BOOST_REQUIRE(_o.count("currentDifficulty") > 0);
	BOOST_REQUIRE(_o.count("currentNumber") > 0);
	BOOST_REQUIRE(_o.count("currentTimestamp") > 0);
	BOOST_REQUIRE(_o.count("currentCoinbase") > 0);
	auto gasLimit = toInt(_o["currentGasLimit"]);
	BOOST_REQUIRE(gasLimit <= std::numeric_limits<int64_t>::max());
	m_envInfo.setGasLimit(gasLimit.convert_to<int64_t>());
	m_envInfo.setDifficulty(toInt(_o["currentDifficulty"]));
	m_envInfo.setNumber(toInt(_o["currentNumber"]));
	m_envInfo.setTimestamp(toInt(_o["currentTimestamp"]));
	m_envInfo.setAuthor(Address(_o["currentCoinbase"].get_str()));
	m_envInfo.setLastHashes( lastHashes( m_envInfo.number() ) );
}

// import state from not fully declared json_spirit::mObject, writing to _stateOptionsMap which fields were defined in json
void replaceLLLinState(json_spirit::mObject& _o)
{
	for (auto& account: _o.count("alloc") ? _o["alloc"].get_obj() : _o.count("accounts") ? _o["accounts"].get_obj() : _o)
	{
		auto obj = account.second.get_obj();
		if (obj.count("code") && obj["code"].type() == json_spirit::str_type)
		{
			string code = obj["code"].get_str();
			obj["code"] = compileLLL(code);
		}
		account.second = obj;
	}
}

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
	for (auto const& i: mask)
		//check that every parameter was declared in state object
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
	if (m_testType == testType::StateTests)				//Import a single transaction
		importTransaction(o_tr, m_transaction);
	else if (m_testType == testType::GeneralStateTest)	//Parse extended transaction
	{
		BOOST_REQUIRE(o_tr.count("gasLimit") > 0);
		size_t dataVectorSize = o_tr.at("data").get_array().size();
		size_t gasVectorSize = o_tr.at("gasLimit").get_array().size();
		size_t valueVectorSize = o_tr.at("value").get_array().size();

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
				BOOST_ERROR(TestOutputHelper::testName() + "expectedStateOptions map does not match expectedState in checkExpectedState!");
				break;
			}
		}

		if (addressOptions.shouldExist())
		{
			CHECK(_statePost.addressInUse(a.first), TestOutputHelper::testName() +  "Compare States: " << a.first << " missing expected address!");
		}
		else
		{
			CHECK(!_statePost.addressInUse(a.first), TestOutputHelper::testName() +  "Compare States: " << a.first << " address not expected to exist!");
		}

		if (_statePost.addressInUse(a.first))
		{

			if (addressOptions.hasBalance())
				CHECK((_stateExpect.balance(a.first) == _statePost.balance(a.first)),
				TestOutputHelper::testName() + "Check State: " << a.first <<  ": incorrect balance " << _statePost.balance(a.first) << ", expected " << _stateExpect.balance(a.first));

			if (addressOptions.hasNonce())
				CHECK((_stateExpect.getNonce(a.first) == _statePost.getNonce(a.first)),
				TestOutputHelper::testName() + "Check State: " << a.first <<  ": incorrect nonce " << _statePost.getNonce(a.first) << ", expected " << _stateExpect.getNonce(a.first));

			if (addressOptions.hasStorage())
			{
				map<h256, pair<u256, u256>> stateStorage = _statePost.storage(a.first);
				for (auto const& s: _stateExpect.storage(a.first))
					CHECK((stateStorage[s.first] == s.second),
					TestOutputHelper::testName() + "Check State: " << a.first << ": incorrect storage [" << s.second.first << "] = " << toHex(stateStorage[s.first].second) << ", expected [" << s.second.first << "] = " << toHex(s.second.second));

				//Check for unexpected storage values
				map<h256, pair<u256, u256>> expectedStorage = _stateExpect.storage(a.first);
				for (auto const& s: _statePost.storage(a.first))
					CHECK((expectedStorage[s.first] == s.second),
					TestOutputHelper::testName() + "Check State: " << a.first <<  ": incorrect storage [" << s.second.first << "] = " << toHex(s.second.second) << ", expected [" << s.second.first << "] = " << toHex(expectedStorage[s.first].second));
			}

			if (addressOptions.hasCode())
				CHECK((_stateExpect.code(a.first) == _statePost.code(a.first)),
				TestOutputHelper::testName() + "Check State: " << a.first <<  ": incorrect code '" << toHex(_statePost.code(a.first)) << "', expected '" << toHex(_stateExpect.code(a.first)) << "'");
		}
	}

	return wasError;
}

void parseJsonStrValueIntoVector(json_spirit::mValue const& _json, vector<string>& _out)
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

template<class T>
bool inArray(vector<T> const& _array, const T _val)
{
	for (size_t i = 0; i  < _array.size(); i++)
		if (_array[i] == _val)
			return true;
	return false;
}

void ImportTest::checkGeneralTestSection(json_spirit::mObject const& _expects, vector<size_t>& _errorTransactions, string const& _network) const
{
	vector<int> d;
	vector<int> g;
	vector<int> v;
	vector<string> network;
	if (_network.empty())
		parseJsonStrValueIntoVector(_expects.at("network"), network);
	else
		network.push_back(_network);

	BOOST_CHECK_MESSAGE(network.size() > 0, TestOutputHelper::testName() + "Network array not set!");
	vector<string> allowednetworks = {netIdToString(eth::Network::FrontierTest), netIdToString(eth::Network::HomesteadTest),
									netIdToString(eth::Network::EIP150Test), netIdToString(eth::Network::EIP158Test), "ALL"};
	for(size_t i=0; i<network.size(); i++)
		BOOST_CHECK_MESSAGE(inArray(allowednetworks, network.at(i)), TestOutputHelper::testName() + "Specified Network not found: " + network.at(i));

	if (_expects.count("indexes"))
	{
		json_spirit::mObject const& indexes = _expects.at("indexes").get_obj();
		parseJsonIntValueIntoVector(indexes.at("data"), d);
		parseJsonIntValueIntoVector(indexes.at("gas"), g);
		parseJsonIntValueIntoVector(indexes.at("value"), v);
		BOOST_CHECK_MESSAGE(d.size() > 0 && g.size() > 0 && v.size() > 0, TestOutputHelper::testName() + "Indexes arrays not set!");
	}
	else
		BOOST_ERROR(TestOutputHelper::testName() + "indexes section not set!");

	bool foundResults = false;
	for(size_t i = 0; i < m_transactions.size(); i++)
	{
		transactionToExecute t = m_transactions[i];
		if (inArray(network, netIdToString(t.netId)) || network[0] == "ALL")
		if ((inArray(d, t.dataInd) || d[0] == -1) && (inArray(g, t.gasInd) || g[0] == -1) && (inArray(v, t.valInd) || v[0] == -1))
		{
			string trInfo = netIdToString(t.netId) + " data: " + toString(t.dataInd) + " gas: " + toString(t.gasInd) + " val: " + toString(t.valInd);
			if (_expects.count("result"))
			{
				State postState = t.postState;
				eth::AccountMaskMap stateMap;
				State expectState(0, OverlayDB(), eth::BaseState::Empty);
				importState(_expects.at("result").get_obj(), expectState, stateMap);
				int errcode = compareStates(expectState, postState, stateMap, Options::get().checkState ? WhenError::Throw : WhenError::DontThrow);
				if (errcode > 0)
				{
					cerr << trInfo << std::endl;
					_errorTransactions.push_back(i);
				}
			}
			else if (_expects.count("hash"))
				BOOST_CHECK_MESSAGE(_expects.at("hash").get_str() == toHex(t.postState.rootHash().asBytes()), TestOutputHelper::testName() + "Expected another postState hash! " + trInfo);
			else
				BOOST_ERROR(TestOutputHelper::testName() + "Expect section or postState missing some fields!");

			foundResults = true;

			//if a single transaction check then stop once found
			if (network[0] != "ALL" && d[0] != -1 && g[0] != -1 && v[0] != -1)
			if (d.size() == 1 && g.size() == 1 && v.size() == 1)
				break;
		}
	}
	BOOST_CHECK_MESSAGE(foundResults, TestOutputHelper::testName() + "Expect results was not found in test execution!");
}

int ImportTest::exportTest(bytes const& _output)
{
	int err = 0;
	if (m_testType == testType::GeneralStateTest)
	{
		vector<size_t> stateIndexesToPrint;
		if (m_testObject.count("expect") > 0)
		{
			for (auto const& exp: m_testObject["expect"].get_array())
				checkGeneralTestSection(exp.get_obj(), stateIndexesToPrint);
			m_testObject.erase(m_testObject.find("expect"));
		}

		size_t k = 0;
		std::map<string, json_spirit::mArray> postState;
		for(size_t i = 0; i < m_transactions.size(); i++)
		{
			json_spirit::mObject obj;
			json_spirit::mObject obj2;
			obj["data"] = m_transactions[i].dataInd;
			obj["gas"] = m_transactions[i].gasInd;
			obj["value"] = m_transactions[i].valInd;
			obj2["indexes"] = obj;
			obj2["hash"] = toHex(m_transactions[i].postState.rootHash().asBytes());
			if (stateIndexesToPrint.size())
			if (i == stateIndexesToPrint[k] && Options::get().checkState)
			{
				obj2["postState"] = fillJsonWithState(m_transactions[i].postState);
				k++;
			}
			postState[netIdToString(m_transactions[i].netId)].push_back(obj2);
		}

		json_spirit::mObject obj;
		for(std::map<string, json_spirit::mArray>::iterator it = postState.begin(); it != postState.end(); ++it)
			obj[it->first] = it->second;

		m_testObject["post"] = obj;
	}
	else
	{

		// export output
		m_testObject["out"] = (_output.size() > 4096 && !Options::get().fulloutput) ? "#" + toString(_output.size()) : toHex(_output, 2, HexPrefix::Add);

		// compare expected output with post output
		if (m_testObject.count("expectOut") > 0)
		{
			std::string warning = "Check State: Error! Unexpected output: " + m_testObject["out"].get_str() + " Expected: " + m_testObject["expectOut"].get_str();
			if (Options::get().checkState)
			{
				bool statement = (m_testObject["out"].get_str() == m_testObject["expectOut"].get_str());
				BOOST_CHECK_MESSAGE(statement, warning);
				if (!statement)
					err = 1;
			}
			else
				BOOST_WARN_MESSAGE(m_testObject["out"].get_str() == m_testObject["expectOut"].get_str(), warning);

			m_testObject.erase(m_testObject.find("expectOut"));
		}

		// export logs
		m_testObject["logs"] = exportLog(m_logs);

		// compare expected state with post state
		if (m_testObject.count("expect") > 0)
		{
			eth::AccountMaskMap stateMap;
			State expectState(0, OverlayDB(), eth::BaseState::Empty);
			importState(m_testObject["expect"].get_obj(), expectState, stateMap);
			compareStates(expectState, m_statePost, stateMap, Options::get().checkState ? WhenError::Throw : WhenError::DontThrow);
			m_testObject.erase(m_testObject.find("expect"));
		}

		// export post state
		m_testObject["post"] = fillJsonWithState(m_statePost);
		m_testObject["postStateRoot"] = toHex(m_statePost.rootHash().asBytes());
	}

	// export pre state
	m_testObject["pre"] = fillJsonWithState(m_statePre);
	m_testObject["env"] = makeAllFieldsHex(m_testObject["env"].get_obj());
	m_testObject["transaction"] = makeAllFieldsHex(m_testObject["transaction"].get_obj());
	return err;
}

json_spirit::mObject fillJsonWithTransaction(Transaction const& _txn)
{
	json_spirit::mObject txObject;
	txObject["nonce"] = toCompactHex(_txn.nonce(), HexPrefix::Add, 1);
	txObject["data"] = toHex(_txn.data(), 2, HexPrefix::Add);
	txObject["gasLimit"] = toCompactHex(_txn.gas(), HexPrefix::Add, 1);
	txObject["gasPrice"] = toCompactHex(_txn.gasPrice(), HexPrefix::Add, 1);
	txObject["r"] = toCompactHex(_txn.signature().r, HexPrefix::Add, 1);
	txObject["s"] = toCompactHex(_txn.signature().s, HexPrefix::Add, 1);
	txObject["v"] = toCompactHex(_txn.signature().v + 27, HexPrefix::Add, 1);
	txObject["to"] = _txn.isCreation() ? "" : toString(_txn.receiveAddress());
	txObject["value"] = toCompactHex(_txn.value(), HexPrefix::Add, 1);
	return txObject;
}

json_spirit::mObject fillJsonWithState(State const& _state)
{
	json_spirit::mObject oState;
	for (auto const& a: _state.addresses())
	{
		json_spirit::mObject o;
		o["balance"] = toCompactHex(_state.balance(a.first), HexPrefix::Add, 1);
		o["nonce"] = toCompactHex(_state.getNonce(a.first), HexPrefix::Add, 1);
		{
			json_spirit::mObject store;
			for (auto const& s: _state.storage(a.first))
				store[toCompactHex(s.second.first, HexPrefix::Add, 1)] = toCompactHex(s.second.second, HexPrefix::Add, 1);
			o["storage"] = store;
		}
		o["code"] = toHex(_state.code(a.first), 2, HexPrefix::Add);
		oState[toString(a.first)] = o;
	}
	return oState;
}

json_spirit::mArray exportLog(eth::LogEntries _logs)
{
	json_spirit::mArray ret;
	if (_logs.size() == 0) return ret;
	for (LogEntry const& l: _logs)
	{
		json_spirit::mObject o;
		o["address"] = toString(l.address);
		json_spirit::mArray topics;
		for (auto const& t: l.topics)
			topics.push_back(toString(t));
		o["topics"] = topics;
		o["data"] = toHex(l.data, 2, HexPrefix::Add);
		o["bloom"] = toString(l.bloom());
		ret.push_back(o);
	}
	return ret;
}

u256 toInt(json_spirit::mValue const& _v)
{
	switch (_v.type())
	{
	case json_spirit::str_type: return u256(_v.get_str());
	case json_spirit::int_type: return (u256)_v.get_uint64();
	case json_spirit::bool_type: return (u256)(uint64_t)_v.get_bool();
	case json_spirit::real_type: return (u256)(uint64_t)_v.get_real();
	default: cwarn << "Bad type for scalar: " << _v.type();
	}
	return 0;
}

byte toByte(json_spirit::mValue const& _v)
{
	switch (_v.type())
	{
	case json_spirit::str_type: return (byte)stoi(_v.get_str());
	case json_spirit::int_type: return (byte)_v.get_uint64();
	case json_spirit::bool_type: return (byte)_v.get_bool();
	case json_spirit::real_type: return (byte)_v.get_real();
	default: cwarn << "Bad type for scalar: " << _v.type();
	}
	return 0;
}

bytes importByteArray(std::string const& _str)
{
	return fromHex(_str.substr(0, 2) == "0x" ? _str.substr(2) : _str, WhenError::Throw);
}

bytes importData(json_spirit::mObject const& _o)
{
	bytes data;
	if (_o.at("data").type() == json_spirit::str_type)
		data = importByteArray(_o.at("data").get_str());
	else
		for (auto const& j: _o.at("data").get_array())
			data.push_back(toByte(j));
	return data;
}

string compileLLL(string const& _code)
{
	if (_code == "")
		return "0x";
	if (_code.substr(0,2) == "0x" && _code.size() >= 2)
		return _code;

#if defined(_WIN32)
	BOOST_ERROR("LLL compilation only supported on posix systems.");
	return "";
#else
	char input[1024];
	boost::filesystem::path path(boost::filesystem::temp_directory_path() / boost::filesystem::unique_path());
	string cmd = string("lllc ") + path.string();
	writeFile(path.string(), _code);

	FILE *fp = popen(cmd.c_str(), "r");
	if (fp == NULL)
		BOOST_ERROR("Failed to run lllc");
	if (fgets(input, sizeof(input) - 1, fp) == NULL)
		BOOST_ERROR("Reading empty file for lllc");
	pclose(fp);

	boost::filesystem::remove(path);
	string result(input);
	result = "0x" + boost::trim_copy(result);
	return result;
#endif
}

bytes importCode(json_spirit::mObject& _o)
{
	bytes code;
	if (_o["code"].type() == json_spirit::str_type)
		if (_o["code"].get_str().find("0x") != 0)
			code = fromHex(compileLLL(_o["code"].get_str()));
		else
			code = fromHex(_o["code"].get_str().substr(2));
	else if (_o["code"].type() == json_spirit::array_type)
	{
		code.clear();
		for (auto const& j: _o["code"].get_array())
			code.push_back(toByte(j));
	}
	return code;
}

LogEntries importLog(json_spirit::mArray& _a)
{
	LogEntries logEntries;
	for (auto const& l: _a)
	{
		json_spirit::mObject o = l.get_obj();
		BOOST_REQUIRE(o.count("address") > 0);
		BOOST_REQUIRE(o.count("topics") > 0);
		BOOST_REQUIRE(o.count("data") > 0);
		BOOST_REQUIRE(o.count("bloom") > 0);
		LogEntry log;
		log.address = Address(o["address"].get_str());
		for (auto const& t: o["topics"].get_array())
			log.topics.push_back(h256(t.get_str()));
		log.data = importData(o);
		logEntries.push_back(log);
	}
	return logEntries;
}

void checkOutput(bytes const& _output, json_spirit::mObject& _o)
{
	int j = 0;

	if (_o["out"].get_str().find("#") == 0)
		BOOST_CHECK((u256)_output.size() == toInt(_o["out"].get_str().substr(1)));
	else if (_o["out"].type() == json_spirit::array_type)
		for (auto const& d: _o["out"].get_array())
		{
			BOOST_CHECK_MESSAGE(_output[j] == toInt(d), "Output byte [" << j << "] different!");
			++j;
		}
	else if (_o["out"].get_str().find("0x") == 0)
		BOOST_CHECK(_output == fromHex(_o["out"].get_str().substr(2)));
	else
		BOOST_CHECK(_output == fromHex(_o["out"].get_str()));
}

void checkStorage(map<u256, u256> _expectedStore, map<u256, u256> _resultStore, Address _expectedAddr)
{
	_expectedAddr = _expectedAddr; //unsed parametr when macro
	for (auto&& expectedStorePair : _expectedStore)
	{
		auto& expectedStoreKey = expectedStorePair.first;
		auto resultStoreIt = _resultStore.find(expectedStoreKey);
		if (resultStoreIt == _resultStore.end())
			BOOST_ERROR(_expectedAddr << ": missing store key " << expectedStoreKey);
		else
		{
			auto& expectedStoreValue = expectedStorePair.second;
			auto& resultStoreValue = resultStoreIt->second;
			BOOST_CHECK_MESSAGE(expectedStoreValue == resultStoreValue, _expectedAddr << ": store[" << expectedStoreKey << "] = " << resultStoreValue << ", expected " << expectedStoreValue);
		}
	}
	BOOST_CHECK_EQUAL(_resultStore.size(), _expectedStore.size());
	for (auto&& resultStorePair: _resultStore)
	{
		if (!_expectedStore.count(resultStorePair.first))
			BOOST_ERROR(_expectedAddr << ": unexpected store key " << resultStorePair.first);
	}
}

void checkLog(LogEntries _resultLogs, LogEntries _expectedLogs)
{
	BOOST_REQUIRE_EQUAL(_resultLogs.size(), _expectedLogs.size());

	for (size_t i = 0; i < _resultLogs.size(); ++i)
	{
		BOOST_CHECK_EQUAL(_resultLogs[i].address, _expectedLogs[i].address);
		BOOST_CHECK_EQUAL(_resultLogs[i].topics, _expectedLogs[i].topics);
		BOOST_CHECK(_resultLogs[i].data == _expectedLogs[i].data);
	}
}

void checkCallCreates(eth::Transactions _resultCallCreates, eth::Transactions _expectedCallCreates)
{
	BOOST_REQUIRE_EQUAL(_resultCallCreates.size(), _expectedCallCreates.size());

	for (size_t i = 0; i < _resultCallCreates.size(); ++i)
	{
		BOOST_CHECK(_resultCallCreates[i].data() == _expectedCallCreates[i].data());
		BOOST_CHECK(_resultCallCreates[i].receiveAddress() == _expectedCallCreates[i].receiveAddress());
		BOOST_CHECK(_resultCallCreates[i].gas() == _expectedCallCreates[i].gas());
		BOOST_CHECK(_resultCallCreates[i].value() == _expectedCallCreates[i].value());
	}
}

void userDefinedTest(std::function<void(json_spirit::mValue&, bool)> doTests)
{
	if (!Options::get().singleTest)
		return;

	if (Options::get().singleTestFile.empty() || Options::get().singleTestName.empty())
	{
		cnote << "Missing user test specification\nUsage: testeth --singletest <filename> <testname>\n";
		return;
	}

	auto& filename = Options::get().singleTestFile;
	auto& testname = Options::get().singleTestName;

	if (g_logVerbosity != -1)
		VerbosityHolder sentinel(12);

	try
	{
		cnote << "Testing user defined test: " << filename;
		json_spirit::mValue v;
		string s = contentsString(filename);
		BOOST_REQUIRE_MESSAGE(s.length() > 0, "Contents of " + filename + " is empty. ");
		json_spirit::read_string(s, v);
		json_spirit::mObject oSingleTest;

		json_spirit::mObject::const_iterator pos = v.get_obj().find(testname);
		if (pos == v.get_obj().end())
		{
			cnote << "Could not find test: " << testname << " in " << filename << "\n";
			return;
		}
		else
			oSingleTest[pos->first] = pos->second;

		json_spirit::mValue v_singleTest(oSingleTest);
		doTests(v_singleTest, test::Options::get().fillTests);
	}
	catch (Exception const& _e)
	{
		BOOST_ERROR("Failed Test with Exception: " << diagnostic_information(_e));
	}
	catch (std::exception const& _e)
	{
		BOOST_ERROR("Failed Test with Exception: " << _e.what());
	}
}

void executeTests(const string& _name, const string& _testPathAppendix, const string& _fillerPathAppendix, std::function<void(json_spirit::mValue&, bool)> doTests, bool _addFillerSuffix)
{
	string testPath = getTestPath() + _testPathAppendix;
	string testFillerPath = getTestPath() + "/src/" + _fillerPathAppendix;

	if (Options::get().stats)
		Listener::registerListener(Stats::get());

	string name = _name;
	if (_name.rfind("Filler.json") != std::string::npos)
		name = _name.substr(0, _name.rfind("Filler.json"));

	if (Options::get().fillTests)
	{
		try
		{
			cnote << "Populating tests...";
			json_spirit::mValue v;
			boost::filesystem::path p(__FILE__);

			string nameEnding = _addFillerSuffix ? "Filler.json" : ".json";
			string testfilename = testFillerPath + "/" + name + nameEnding;
			string 	s = asString(dev::contents(testfilename));
			BOOST_REQUIRE_MESSAGE(s.length() > 0, "Contents of " + testfilename + " is empty.");

			json_spirit::read_string(s, v);
			doTests(v, true);
			writeFile(testPath + "/" + name + ".json", asBytes(json_spirit::write_string(v, true)));
		}
		catch (Exception const& _e)
		{
			BOOST_ERROR(TestOutputHelper::testName() + "Failed filling test with Exception: " << diagnostic_information(_e));
		}
		catch (std::exception const& _e)
		{
			BOOST_ERROR(TestOutputHelper::testName() + "Failed filling test with Exception: " << _e.what());
		}
	}
	try
	{
		cnote << "TEST " << name << ":";
		json_spirit::mValue v;
		string s = asString(dev::contents(testPath + "/" + name + ".json"));
		BOOST_REQUIRE_MESSAGE(s.length() > 0, "Contents of " + testPath + "/" + name + ".json is empty. Have you cloned the 'tests' repo branch develop and set ETHEREUM_TEST_PATH to its path?");
		json_spirit::read_string(s, v);
		Listener::notifySuiteStarted(name);
		doTests(v, false);
	}
	catch (Exception const& _e)
	{
		BOOST_ERROR(TestOutputHelper::testName() + "Failed test with Exception: " << diagnostic_information(_e));
	}
	catch (std::exception const& _e)
	{
		BOOST_ERROR(TestOutputHelper::testName() + "Failed test with Exception: " << _e.what());
	}
}

void copyFile(std::string const& _source, std::string const& _destination)
{
	std::ifstream src(_source, std::ios::binary);
	std::ofstream dst(_destination, std::ios::binary);
	dst << src.rdbuf();
}

RLPStream createRLPStreamFromTransactionFields(json_spirit::mObject const& _tObj)
{
	//Construct Rlp of the given transaction
	RLPStream rlpStream;
	rlpStream.appendList(_tObj.size());

	if (_tObj.count("nonce"))
		rlpStream << bigint(_tObj.at("nonce").get_str());

	if (_tObj.count("gasPrice"))
		rlpStream << bigint(_tObj.at("gasPrice").get_str());

	if (_tObj.count("gasLimit"))
		rlpStream << bigint(_tObj.at("gasLimit").get_str());

	if (_tObj.count("to"))
	{
		if (_tObj.at("to").get_str().empty())
			rlpStream << "";
		else
			rlpStream << importByteArray(_tObj.at("to").get_str());
	}

	if (_tObj.count("value"))
		rlpStream << bigint(_tObj.at("value").get_str());

	if (_tObj.count("data"))
		rlpStream << importData(_tObj);

	if (_tObj.count("v"))
		rlpStream << bigint(_tObj.at("v").get_str());

	if (_tObj.count("r"))
		rlpStream << bigint(_tObj.at("r").get_str());

	if (_tObj.count("s"))
		rlpStream <<  bigint(_tObj.at("s").get_str());

	if (_tObj.count("extrafield"))
		rlpStream << bigint(_tObj.at("extrafield").get_str());

	return rlpStream;
}

Options::Options(int argc, char** argv)
{
	for (auto i = 0; i < argc; ++i)
	{
		auto arg = std::string{argv[i]};
		if (arg == "--help")
		{
			printHelp();
			exit(0);
		}
		else
		if (arg == "--vm" && i + 1 < argc)
		{
			string vmKind = argv[++i];
			if (vmKind == "interpreter")
				VMFactory::setKind(VMKind::Interpreter);
			else if (vmKind == "jit")
				VMFactory::setKind(VMKind::JIT);
			else if (vmKind == "smart")
				VMFactory::setKind(VMKind::Smart);
			else
				cerr << "Unknown VM kind: " << vmKind << endl;
		}
		else if (arg == "--jit") // TODO: Remove deprecated option "--jit"
			VMFactory::setKind(VMKind::JIT);
		else if (arg == "--vmtrace")
		{
			vmtrace = true;
			g_logVerbosity = 13;
		}
		else if (arg == "--filltests")
			fillTests = true;
		else if (arg == "--stats" && i + 1 < argc)
		{
			stats = true;
			statsOutFile = argv[i + 1];
		}
		else if (arg == "--performance")
			performance = true;
		else if (arg == "--quadratic")
			quadratic = true;
		else if (arg == "--memory")
			memory = true;
		else if (arg == "--inputlimits")
			inputLimits = true;
		else if (arg == "--bigdata")
			bigData = true;
		else if (arg == "--checkstate")
			checkState = true;
		else if (arg == "--wallet")
			wallet = true;
		else if (arg == "--all")
		{
			performance = true;
			quadratic = true;
			memory = true;
			inputLimits = true;
			bigData = true;
			wallet = true;
		}
		else if (arg == "--singletest" && i + 1 < argc)
		{
			singleTest = true;
			auto name1 = std::string{argv[i + 1]};
			if (i + 2 < argc) // two params
			{
				auto name2 = std::string{argv[i + 2]};
				if (name2[0] == '-') // not param, another option
					singleTestName = std::move(name1);
				else
				{
					singleTestFile = std::move(name1);
					singleTestName = std::move(name2);
				}
			}
			else
				singleTestName = std::move(name1);
		}
		else if (arg == "--singlenet" && i + 1 < argc)
			singleTestNet = std::string{argv[i + 1]};
		else if (arg == "--fulloutput")
			fulloutput = true;
		else if (arg == "--verbosity" && i + 1 < argc)
		{
			static std::ostringstream strCout; //static string to redirect logs to
			std::string indentLevel = std::string{argv[i + 1]};
			if (indentLevel == "0")
			{
				logVerbosity = Verbosity::None;
				std::cout.rdbuf(strCout.rdbuf());
				std::cerr.rdbuf(strCout.rdbuf());
			}
			else if (indentLevel == "1")
				logVerbosity = Verbosity::NiceReport;
			else
				logVerbosity = Verbosity::Full;

			int indentLevelInt = atoi(argv[i + 1]);
			if (indentLevelInt > g_logVerbosity)
				g_logVerbosity = indentLevelInt;
		}
		else if (arg == "--createRandomTest")
			createRandomTest = true;
		else if (arg == "-t" && i + 1 < argc)
			rCurrentTestSuite = std::string{argv[i + 1]};
		else if (arg == "--checktest" || arg == "--filltest")
		{
			//read all line to the end
			for (int j = i+1; j < argc; ++j)
				rCheckTest += argv[j];
			break;
		}
		else if (arg == "--nonetwork")
			nonetwork = true;
	}

	//Default option
	if (logVerbosity == Verbosity::NiceReport)
		g_logVerbosity = -1;	//disable cnote but leave cerr and cout
}

Options const& Options::get(int argc, char** argv)
{
	static Options instance(argc, argv);
	return instance;
}

LastHashes lastHashes(u256 _currentBlockNumber)
{
	LastHashes ret;
	for (u256 i = 1; i <= 256 && i <= _currentBlockNumber; ++i)
		ret.push_back(sha3(toString(_currentBlockNumber - i)));
	return ret;
}

dev::eth::BlockHeader constructHeader(
	h256 const& _parentHash,
	h256 const& _sha3Uncles,
	Address const& _author,
	h256 const& _stateRoot,
	h256 const& _transactionsRoot,
	h256 const& _receiptsRoot,
	dev::eth::LogBloom const& _logBloom,
	u256 const& _difficulty,
	u256 const& _number,
	u256 const& _gasLimit,
	u256 const& _gasUsed,
	u256 const& _timestamp,
	bytes const& _extraData)
{
	RLPStream rlpStream;
	rlpStream.appendList(15);

	rlpStream << _parentHash << _sha3Uncles << _author << _stateRoot << _transactionsRoot << _receiptsRoot << _logBloom
		<< _difficulty << _number << _gasLimit << _gasUsed << _timestamp << _extraData << h256{} << Nonce{};

	return BlockHeader(rlpStream.out(), HeaderData);
}

void updateEthashSeal(dev::eth::BlockHeader& _header, h256 const& _mixHash, h64 const& _nonce)
{
	Ethash::setNonce(_header, _nonce);
	Ethash::setMixHash(_header, _mixHash);
}

namespace
{
	Listener* g_listener;
}

void Listener::registerListener(Listener& _listener)
{
	g_listener = &_listener;
}

void Listener::notifySuiteStarted(std::string const& _name)
{
	if (g_listener)
		g_listener->suiteStarted(_name);
}

void Listener::notifyTestStarted(std::string const& _name)
{
	if (g_listener)
		g_listener->testStarted(_name);
}

void Listener::notifyTestFinished(int64_t _gasUsed)
{
	if (g_listener)
		g_listener->testFinished(_gasUsed);
}

size_t TestOutputHelper::m_currTest = 0;
size_t TestOutputHelper::m_maxTests = 0;
string TestOutputHelper::m_currentTestName = "n/a";
string TestOutputHelper::m_currentTestCaseName = "n/a";
string TestOutputHelper::m_currentTestFileName = "n/a";

using namespace boost;
void TestOutputHelper::initTest(int _maxTests)
{
	Ethash::init();
	BasicAuthority::init();
	NoProof::init();
	m_currentTestCaseName = boost::unit_test::framework::current_test_case().p_name;
	std::cout << "Test Case \"" + m_currentTestCaseName + "\": " << std::endl;
	m_maxTests = _maxTests;
	m_currTest = 0;
}

void TestOutputHelper::initTest(json_spirit::mValue& _v)
{
	Ethash::init();
	BasicAuthority::init();
	NoProof::init();
	m_currentTestCaseName = boost::unit_test::framework::current_test_case().p_name;
	std::cout << "Test Case \"" + m_currentTestCaseName + "\": " << std::endl;
	m_maxTests = _v.get_obj().size();
	m_currTest = 0;
}

bool TestOutputHelper::passTest(json_spirit::mObject& _o, std::string& _testName)
{
	m_currTest++;
	int m_testsPerProgs = std::max(1, (int)(m_maxTests / 4));
	if (m_currTest % m_testsPerProgs == 0 || m_currTest ==  m_maxTests)
	{
		int percent = int(m_currTest*100/m_maxTests);
		std::cout << percent << "%";
		if (percent != 100)
			std::cout << "...";
		std::cout << std::endl;
	}

	if (m_currentTestCaseName != "stRandom")
	if (test::Options::get().singleTest && test::Options::get().singleTestName != _testName)
	{
		_o.clear();
		return false;
	}

	cnote << _testName;
	//_testName = (m_currentTestFileName == "n/a") ? "(" + _testName + ") " : "(" + m_currentTestFileName + "/" +  _testName + ") ";
	m_currentTestName = _testName + " ";
	return true;
}

} } // namespaces
