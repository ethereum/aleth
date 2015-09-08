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
#include <libethcore/EthashAux.h>
#include <libethereum/Client.h>
#include <libevm/ExtVMFace.h>
#include <liblll/Compiler.h>
#include <libevm/VMFactory.h>
#include "Stats.h"

using namespace std;
using namespace dev::eth;

namespace dev
{
namespace eth
{

void mine(Client& c, int numBlocks)
{
	auto startBlock = c.blockChain().details().number;

	c.startMining();
	while(c.blockChain().details().number < startBlock + numBlocks)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	c.stopMining();
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

void mine(Block& s, BlockChain const& _bc)
{
	std::unique_ptr<SealEngineFace> sealer(Ethash::createSealEngine());
	s.commitToSeal(_bc);
	Notified<bytes> sealed;
	sealer->onSealGenerated([&](bytes const& sealedHeader){ sealed = sealedHeader; });
	sealer->generateSeal(s.info());
	sealed.waitNot({});
	sealer.reset();
	s.sealBlock(sealed);
}

void mine(Ethash::BlockHeader& _bi)
{
	std::unique_ptr<SealEngineFace> sealer(Ethash::createSealEngine());
	Notified<bytes> sealed;
	sealer->onSealGenerated([&](bytes const& sealedHeader){ sealed = sealedHeader; });
	sealer->generateSeal(_bi);
	sealed.waitNot({});
	sealer.reset();
	_bi = Ethash::BlockHeader(sealed, CheckNothing, h256{}, HeaderData);
}

}

namespace test
{

struct ValueTooLarge: virtual Exception {};
struct MissingFields : virtual Exception {};

bigint const c_max256plus1 = bigint(1) << 256;

ImportTest::ImportTest(json_spirit::mObject& _o, bool isFiller, testType testTemplate):
	m_statePre(OverlayDB(), eth::BaseState::Empty),
	m_statePost(OverlayDB(), eth::BaseState::Empty),
	m_testObject(_o)
{
	if (testTemplate == testType::StateTests)
	{
		importEnv(_o["env"].get_obj());
		importTransaction(_o["transaction"].get_obj());
		importState(_o["pre"].get_obj(), m_statePre);
		if (!isFiller)
		{
			if (_o.count("post"))
				importState(_o["post"].get_obj(), m_statePost);
			else
				importState(_o["postState"].get_obj(), m_statePost);
			m_logsExpected = importLog(_o["logs"].get_array());
		}
	}
}

//executes an imported transacton on preState
bytes ImportTest::executeTest()
{
	ExecutionResult res;
	eth::State tmpState = m_statePre;
	try
	{
		std::pair<ExecutionResult, TransactionReceipt>  execOut = m_statePre.execute(m_envInfo, m_transaction);
		res = execOut.first;
		m_logs = execOut.second.log();
	}
	catch (Exception const& _e)
	{
		cnote << "Exception: " << diagnostic_information(_e);
	}
	catch (std::exception const& _e)
	{
		cnote << "state execution exception: " << _e.what();
	}

	m_statePre.commit();
	m_statePost = m_statePre;
	m_statePre = tmpState;

	return res.output;
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
	m_envInfo.setGasLimit(toInt(_o["currentGasLimit"]));
	m_envInfo.setDifficulty(toInt(_o["currentDifficulty"]));
	m_envInfo.setNumber(toInt(_o["currentNumber"]));
	m_envInfo.setTimestamp(toInt(_o["currentTimestamp"]));
	m_envInfo.setBeneficiary(Address(_o["currentCoinbase"].get_str()));
	m_envInfo.setLastHashes( lastHashes( m_envInfo.number() ) );
}

// import state from not fully declared json_spirit::mObject, writing to _stateOptionsMap which fields were defined in json

void ImportTest::importState(json_spirit::mObject const& _o, State& _state, AccountMaskMap& o_mask)
{		
	std::string jsondata = json_spirit::write_string((json_spirit::mValue)_o, false);
	_state.populateFrom(jsonToAccountMap(jsondata, &o_mask));
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
	importTransaction(o_tr, m_transaction);
}

int ImportTest::compareStates(State const& _stateExpect, State const& _statePost, AccountMaskMap const _expectedStateOptions, WhenError _throw)
{
	#define CHECK(a,b)						\
		{									\
			if (_throw == WhenError::Throw) \
			{								\
				BOOST_CHECK_MESSAGE(a, b);	\
				if (!a)						\
					return 1;				\
			}								\
			else							\
				BOOST_WARN_MESSAGE(a,b);	\
		}

	for (auto const& a: _stateExpect.addresses())
	{
		CHECK(_statePost.addressInUse(a.first), TestOutputHelper::testName() +  "Compare States: " << a.first << " missing expected address!");
		if (_statePost.addressInUse(a.first))
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

			if (addressOptions.hasBalance())
				CHECK((_stateExpect.balance(a.first) == _statePost.balance(a.first)),
				TestOutputHelper::testName() + "Check State: " << a.first <<  ": incorrect balance " << _statePost.balance(a.first) << ", expected " << _stateExpect.balance(a.first));

			if (addressOptions.hasNonce())
				CHECK((_stateExpect.transactionsFrom(a.first) == _statePost.transactionsFrom(a.first)),
				TestOutputHelper::testName() + "Check State: " << a.first <<  ": incorrect nonce " << _statePost.transactionsFrom(a.first) << ", expected " << _stateExpect.transactionsFrom(a.first));

			if (addressOptions.hasStorage())
			{
				unordered_map<u256, u256> stateStorage = _statePost.storage(a.first);
				for (auto const& s: _stateExpect.storage(a.first))
					CHECK((stateStorage[s.first] == s.second),
					TestOutputHelper::testName() + "Check State: " << a.first <<  ": incorrect storage [" << s.first << "] = " << toHex(stateStorage[s.first]) << ", expected [" << s.first << "] = " << toHex(s.second));

				//Check for unexpected storage values
				stateStorage = _stateExpect.storage(a.first);
				for (auto const& s: _statePost.storage(a.first))
					CHECK((stateStorage[s.first] == s.second),
					TestOutputHelper::testName() + "Check State: " << a.first <<  ": incorrect storage [" << s.first << "] = " << toHex(s.second) << ", expected [" << s.first << "] = " << toHex(stateStorage[s.first]));
			}

			if (addressOptions.hasCode())
				CHECK((_stateExpect.code(a.first) == _statePost.code(a.first)),
				TestOutputHelper::testName() + "Check State: " << a.first <<  ": incorrect code '" << toHex(_statePost.code(a.first)) << "', expected '" << toHex(_stateExpect.code(a.first)) << "'");
		}
	}
	return 0;
}

int ImportTest::exportTest(bytes const& _output)
{
	int err = 0;
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
		State expectState(OverlayDB(), eth::BaseState::Empty);
		importState(m_testObject["expect"].get_obj(), expectState, stateMap);
		compareStates(expectState, m_statePost, stateMap, Options::get().checkState ? WhenError::Throw : WhenError::DontThrow);
		m_testObject.erase(m_testObject.find("expect"));
	}

	// export post state
	m_testObject["post"] = fillJsonWithState(m_statePost);
	m_testObject["postStateRoot"] = toHex(m_statePost.rootHash().asBytes());

	// export pre state
	m_testObject["pre"] = fillJsonWithState(m_statePre);
	m_testObject["env"] = makeAllFieldsHex(m_testObject["env"].get_obj());
	m_testObject["transaction"] = makeAllFieldsHex(m_testObject["transaction"].get_obj());
	return err;
}

json_spirit::mObject fillJsonWithTransaction(Transaction _txn)
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

json_spirit::mObject fillJsonWithState(State _state)
{
	json_spirit::mObject oState;
	for (auto const& a: _state.addresses())
	{
		json_spirit::mObject o;
		o["balance"] = toCompactHex(_state.balance(a.first), HexPrefix::Add, 1);
		o["nonce"] = toCompactHex(_state.transactionsFrom(a.first), HexPrefix::Add, 1);
		{
			json_spirit::mObject store;
			for (auto const& s: _state.storage(a.first))
				store[toCompactHex(s.first, HexPrefix::Add, 1)] = toCompactHex(s.second, HexPrefix::Add, 1);
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

bytes importCode(json_spirit::mObject& _o)
{
	bytes code;
	if (_o["code"].type() == json_spirit::str_type)
		if (_o["code"].get_str().find("0x") != 0)
			code = compileLLL(_o["code"].get_str(), false);
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

void executeTests(const string& _name, const string& _testPathAppendix, const boost::filesystem::path _pathToFiller, std::function<void(json_spirit::mValue&, bool)> doTests)
{
	string testPath = getTestPath();
	testPath += _testPathAppendix;

	if (Options::get().stats)
		Listener::registerListener(Stats::get());

	if (Options::get().fillTests)
	{
		try
		{
			cnote << "Populating tests...";
			json_spirit::mValue v;
			boost::filesystem::path p(__FILE__);
			string s = asString(dev::contents(_pathToFiller.string() + "/" + _name + "Filler.json"));
			BOOST_REQUIRE_MESSAGE(s.length() > 0, "Contents of " + _pathToFiller.string() + "/" + _name + "Filler.json is empty.");
			json_spirit::read_string(s, v);
			doTests(v, true);
			writeFile(testPath + "/" + _name + ".json", asBytes(json_spirit::write_string(v, true)));
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
		cnote << "TEST " << _name << ":";
		json_spirit::mValue v;
		string s = asString(dev::contents(testPath + "/" + _name + ".json"));
		BOOST_REQUIRE_MESSAGE(s.length() > 0, "Contents of " + testPath + "/" + _name + ".json is empty. Have you cloned the 'tests' repo branch develop and set ETHEREUM_TEST_PATH to its path?");
		json_spirit::read_string(s, v);
		Listener::notifySuiteStarted(_name);
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

Options::Options()
{
	auto argc = boost::unit_test::framework::master_test_suite().argc;
	auto argv = boost::unit_test::framework::master_test_suite().argv;

	for (auto i = 0; i < argc; ++i)
	{
		auto arg = std::string{argv[i]};
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
			vmtrace = true;
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
		else if (arg == "--nonetwork")
			nonetwork = true;
		else if (arg == "--network")
			nonetwork = false;
		else if (arg == "--nodag")
			nodag = true;
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
			if (i + 1 < argc) // two params
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
		}
		else if (arg == "--createRandomTest")
			createRandomTest = true;
	}

	//Default option
	if (logVerbosity == Verbosity::NiceReport)
		g_logVerbosity = -1;	//disable cnote but leave cerr and cout
}

Options const& Options::get()
{
	static Options instance;
	return instance;
}

LastHashes lastHashes(u256 _currentBlockNumber)
{
	LastHashes ret;
	for (u256 i = 1; i <= 256 && i <= _currentBlockNumber; ++i)
		ret.push_back(sha3(toString(_currentBlockNumber - i)));
	return ret;
}

dev::eth::Ethash::BlockHeader constructHeader(
	h256 const& _parentHash,
	h256 const& _sha3Uncles,
	Address const& _coinbaseAddress,
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
	rlpStream.appendList(Ethash::BlockHeader::Fields);

	rlpStream << _parentHash << _sha3Uncles << _coinbaseAddress << _stateRoot << _transactionsRoot << _receiptsRoot << _logBloom
		<< _difficulty << _number << _gasLimit << _gasUsed << _timestamp << _extraData << h256{} << Nonce{};

	return Ethash::BlockHeader(rlpStream.out(), CheckNothing, h256{}, HeaderData);
}

void updateEthashSeal(dev::eth::Ethash::BlockHeader& _header, h256 const& _mixHash, dev::eth::Nonce const& _nonce)
{
	RLPStream source;
	_header.streamRLP(source);
	RLP sourceRlp(source.out());
	RLPStream header;
	header.appendList(Ethash::BlockHeader::Fields);
	for (size_t i = 0; i < BlockInfo::BasicFields; i++)
		header << sourceRlp[i];

	header << _mixHash << _nonce;
	_header = Ethash::BlockHeader(header.out(), CheckNothing, h256{}, HeaderData);
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

void Listener::notifyTestFinished()
{
	if (g_listener)
		g_listener->testFinished();
}

size_t TestOutputHelper::m_currTest = 0;
size_t TestOutputHelper::m_maxTests = 0;
string TestOutputHelper::m_currentTestName = "n/a";

using namespace boost;
void TestOutputHelper::initTest(json_spirit::mValue& _v)
{
	std::string testCaseName = boost::unit_test::framework::current_test_case().p_name;
	std::cout << "Test Case \"" + testCaseName + "\": " << std::endl;
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

	if (test::Options::get().singleTest && test::Options::get().singleTestName != _testName)
	{
		_o.clear();
		return false;
	}

	cnote << _testName;
	_testName = "(" + _testName + ") ";
	m_currentTestName = _testName;
	return true;
}

} } // namespaces
