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
/** @file TestUtils.cpp
 * @author Marek Kotewicz <marek@ethdev.com>
 * @date 2015
 */

#include <ctime>
#include <random>
#include <thread>
#include <boost/test/unit_test.hpp>
#include <libdevcore/CommonIO.h>
#include <libdevcrypto/FileSystem.h>
#include "TestUtils.h"

// used methods from TestHelper:
// getTestPath
// importByteArray
#include "TestHelper.h"

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

namespace dev
{
namespace test
{

bool getCommandLineOption(std::string const& _name);
std::string getCommandLineArgument(std::string const& _name, bool _require = false);
std::string loadFile(std::string const& _path);
Json::Value loadTestFile(std::string const& _filename);
dev::eth::BlockInfo toBlockInfo(Json::Value const& _json);
bytes toBlockChain(Json::Value const& _json);
int randomNumber();
std::string getRandomPathForTest(std::string const& _test);

}
}

int dev::test::randomNumber()
{
	static std::mt19937 randomGenerator(time(0));
	randomGenerator.seed(std::random_device()());
	return std::uniform_int_distribution<int>(1)(randomGenerator);
}

std::string dev::test::getRandomPathForTest(std::string const& _test)
{
	std::stringstream stream;
	stream << getDataDir() << "/EthereumTests/" << _test << randomNumber();
	return stream.str();
}

bool dev::test::getCommandLineOption(string const& _name)
{
	auto argc = boost::unit_test::framework::master_test_suite().argc;
	auto argv = boost::unit_test::framework::master_test_suite().argv;
	bool result = false;
	for (auto i = 0; !result && i < argc; ++i)
		result = _name == argv[i];
	return result;
}

std::string dev::test::getCommandLineArgument(string const& _name, bool _require)
{
	auto argc = boost::unit_test::framework::master_test_suite().argc;
	auto argv = boost::unit_test::framework::master_test_suite().argv;
	for (auto i = 1; i < argc; ++i)
	{
		string str = argv[i];
		if (_name == str.substr(0, _name.size()))
			return str.substr(str.find("=") + 1);
	}
	if (_require)
		BOOST_ERROR("Failed getting command line argument: " << _name << " from: " << argv);
	return "";
}

std::string dev::test::loadFile(std::string const& _path)
{
	string s = asString(dev::contents(_path));
	BOOST_REQUIRE_MESSAGE(s.length() > 0, "Contents of " + _path + " is empty. Have you cloned the 'tests' repo branch develop and set ETHEREUM_TEST_PATH to its path?");
	return s;
}

Json::Value dev::test::loadTestFile(std::string const& _filename)
{
	Json::Reader reader;
	Json::Value result;
	string path = getTestPath() + "/" + _filename + ".json";
	reader.parse(loadFile(path), result);
	clog << "FIXTURE: loaded test from file: " << path << endl;
	return result;
}

dev::eth::BlockInfo dev::test::toBlockInfo(Json::Value const& _json)
{
	RLPStream rlpStream;
	auto size = _json.getMemberNames().size();
	rlpStream.appendList(_json["hash"].empty() ? size : (size - 1));
	rlpStream << toBytes(_json["parentHash"].asString());
	rlpStream << toBytes(_json["uncleHash"].asString());
	rlpStream << toBytes(_json["coinbase"].asString());
	rlpStream << toBytes(_json["stateRoot"].asString());
	rlpStream << toBytes(_json["transactionsTrie"].asString());
	rlpStream << toBytes(_json["receiptTrie"].asString());
	rlpStream << toBytes(_json["bloom"].asString());
	rlpStream << bigint(_json["difficulty"].asString());
	rlpStream << bigint(_json["number"].asString());
	rlpStream << bigint(_json["gasLimit"].asString());
	rlpStream << bigint(_json["gasUsed"].asString());
	rlpStream << bigint(_json["timestamp"].asString());
	rlpStream << fromHex(_json["extraData"].asString());
	rlpStream << toBytes(_json["mixHash"].asString());
	rlpStream << toBytes(_json["nonce"].asString());
	
	BlockInfo result;
	RLP rlp(rlpStream.out());
	result.populateFromHeader(rlp, IgnoreNonce);
	return result;
}

bytes dev::test::toBytes(std::string const& _str)
{
	return importByteArray(_str);
}

dev::eth::State TestUtils::toState(Json::Value const& _json)
{
	State state(Address(), OverlayDB(), BaseState::Empty);
	for (string const& name: _json.getMemberNames())
	{
		Json::Value o = _json[name];

		Address address = Address(name);
		bytes code = fromHex(o["code"].asString().substr(2));

		if (code.size())
		{
			state.m_cache[address] = Account(toInt(o["balance"].asString()), Account::ContractConception);
			state.m_cache[address].setCode(code);
		}
		else
			state.m_cache[address] = Account(toInt(o["balance"].asString()), Account::NormalCreation);

		for (string const& j: o["storage"].getMemberNames())
			state.setStorage(address, toInt(j), toInt(o["storage"][j].asString()));

		for (auto i = 0; i < toInt(o["nonce"].asString()); ++i)
			state.noteSending(address);
		
		state.ensureCached(address, false, false);
	}

	return state;
}

bytes dev::test::toBlockChain(Json::Value const& _json)
{
	BlockInfo bi = toBlockInfo(_json["genesisBlockHeader"]);
	RLPStream rlpStream;
	bi.streamRLP(rlpStream, WithNonce);
	
	RLPStream fullStream(3);
	fullStream.appendRaw(rlpStream.out());
	fullStream.appendRaw(RLPEmptyList);
	fullStream.appendRaw(RLPEmptyList);
	bi.verifyInternals(&fullStream.out());
	
	return fullStream.out();
}

bool LoadTestFileFixture::m_loaded = false;
Json::Value LoadTestFileFixture::m_json;

LoadTestFileFixture::LoadTestFileFixture()
{
	if (!m_loaded)
	{
		m_json = loadTestFile(getCommandLineArgument("--eth_testfile"));
		m_loaded = true;
	}
}

void ParallelFixture::enumerateThreads(std::function<void()> callback)
{
	size_t threadsCount = std::stoul(getCommandLineArgument("--eth_threads"), nullptr, 10);
	
	vector<thread> workers;
	for (size_t i = 0; i < threadsCount; i++)
		workers.emplace_back(callback);
	
	for_each(workers.begin(), workers.end(), [](thread &t)
	{
		t.join();
	});
}

void BlockChainFixture::enumerateBlockchains(std::function<void(Json::Value const&, dev::eth::BlockChain&, State state)> callback)
{
	for (string const& name: m_json.getMemberNames())
	{
		Json::Value o = m_json[name];

		State state = TestUtils::toState(o["pre"]);
		state.commit();

		string path = getRandomPathForTest("InterfaceStub");
		ShortLivingDirectory directory(path);
		BlockChain bc(toBlockChain(o), path, true);
		clog << "FIXTURE: initalized blockchain at path: " << path << endl;

		for (auto const& block: o["blocks"])
		{
			bytes rlp = toBytes(block["rlp"].asString());
			bc.import(rlp, state.db());
			state.sync(bc);
		}

		callback(o, bc, state);
	}
}

class FixedInterface: public dev::eth::InterfaceStub
{
public:
	FixedInterface(BlockChain const& _bc, State _state) :  m_bc(_bc), m_state(_state) {}
	virtual ~FixedInterface() {}
	
	// stub
	virtual void flushTransactions() override {}
	virtual BlockChain const& bc() const override { return m_bc; }
	virtual State asOf(int _h) const override { (void)_h; return m_state; }
	virtual State asOf(h256 _h) const override { (void)_h; return m_state; }
	virtual State preMine() const override { return m_state; }
	virtual State postMine() const override { return m_state; }
	virtual void prepareForTransaction() override {}
	
private:
	BlockChain const& m_bc;
	State m_state;
};

void InterfaceStubFixture::enumerateInterfaces(std::function<void(Json::Value const&, dev::eth::InterfaceStub&)> callback)
{
	enumerateBlockchains([&callback](Json::Value const& _json, BlockChain& _bc, State _state) -> void
	{
		FixedInterface client(_bc, _state);
		callback(_json, client);
	});
}

void ParallelInterfaceStubFixture::enumerateInterfaces(std::function<void(Json::Value const&, dev::eth::InterfaceStub&)> callback)
{
	InterfaceStubFixture::enumerateInterfaces([this, &callback](Json::Value const& _json, dev::eth::InterfaceStub& _client) -> void
	{
		// json is being copied here
		enumerateThreads([callback, _json, &_client]() -> void
		{
			callback(_json, _client);
		});
	});
}
