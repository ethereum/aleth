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
 * Helper functions to work with json::spirit and test files
 */

#include <test/libtesteth/TestHelper.h>
#include <test/libtesteth/TestOutputHelper.h>
#include <test/libtesteth/Options.h>
#if !defined(_WIN32)
#include <stdio.h>
#endif
#include <boost/algorithm/string/trim.hpp>
#include <libethereum/Client.h>
#include <test/libtesteth/Stats.h>

using namespace std;
using namespace dev::eth;

namespace dev
{
namespace eth
{

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


string netIdToString(eth::Network _netId)
{
	switch(_netId)
	{
		case eth::Network::FrontierTest: return "Frontier";
		case eth::Network::HomesteadTest: return "Homestead";
		case eth::Network::EIP150Test: return "EIP150";
		case eth::Network::EIP158Test: return "EIP158";
		case eth::Network::MetropolisTest: return "Metropolis";
		default: return "other";
	}
	return "unknown";
}

eth::Network stringToNetId(string const& _netname)
{
	if (netIdToString(eth::Network::FrontierTest) == _netname)
		return eth::Network::FrontierTest;
	if (netIdToString(eth::Network::HomesteadTest) == _netname)
		return eth::Network::HomesteadTest;
	if (netIdToString(eth::Network::EIP150Test) == _netname)
		return eth::Network::EIP150Test;
	if (netIdToString(eth::Network::EIP158Test) == _netname)
		return eth::Network::EIP158Test;
	if (netIdToString(eth::Network::MetropolisTest) == _netname)
		return eth::Network::MetropolisTest;
	BOOST_ERROR(TestOutputHelper::testName() + " network not found: " + _netname);
	return eth::Network::FrontierTest;
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
	AccountMaskMap emptyMap;
	return fillJsonWithState(_state, emptyMap);
}

json_spirit::mObject fillJsonWithState(State const& _state, eth::AccountMaskMap const& _map)
{
	bool mapEmpty = (_map.size() == 0);
	json_spirit::mObject oState;
	for (auto const& a: _state.addresses())
	{
		if (_map.size() && _map.find(a.first) == _map.end())
			continue;

		json_spirit::mObject o;
		if (mapEmpty || _map.at(a.first).hasBalance())
			o["balance"] = toCompactHex(_state.balance(a.first), HexPrefix::Add, 1);
		if (mapEmpty || _map.at(a.first).hasNonce())
			o["nonce"] = toCompactHex(_state.getNonce(a.first), HexPrefix::Add, 1);
		{
			if (mapEmpty || _map.at(a.first).hasStorage())
			{
				json_spirit::mObject store;
				for (auto const& s: _state.storage(a.first))
					store[toCompactHex(s.second.first, HexPrefix::Add, 1)] = toCompactHex(s.second.second, HexPrefix::Add, 1);
				o["storage"] = store;
			}
		}

		if (mapEmpty || _map.at(a.first).hasCode())
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

void checkOutput(bytesConstRef _output, json_spirit::mObject& _o)
{
	int j = 0;
	auto expectedOutput = _o["out"].get_str();

	if (expectedOutput.find("#") == 0)
		BOOST_CHECK(_output.size() == toInt(expectedOutput.substr(1)));
	else if (_o["out"].type() == json_spirit::array_type)
		for (auto const& d: _o["out"].get_array())
		{
			BOOST_CHECK_MESSAGE(_output[j] == toInt(d), "Output byte [" << j << "] different!");
			++j;
		}
	else if (expectedOutput.find("0x") == 0)
		BOOST_CHECK(_output.contentsEqual(fromHex(expectedOutput.substr(2))));
	else
		BOOST_CHECK(_output.contentsEqual(fromHex(expectedOutput)));
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
		doTests(v_singleTest, test::Options::get().filltests);
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

	if (Options::get().filltests)
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



} } // namespaces
