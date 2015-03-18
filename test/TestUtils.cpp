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
#include <boost/filesystem.hpp>
#include <libtestutils/BlockChainLoader.h>
#include <libtestutils/ShortLivingDirectory.h>
#include <libtestutils/Common.h>
#include "TestUtils.h"

// used methods from TestHelper:
// getTestPath
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

}
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

bool LoadTestFileFixture::m_loaded = false;
Json::Value LoadTestFileFixture::m_json;

LoadTestFileFixture::LoadTestFileFixture()
{
	if (!m_loaded)
	{
		m_json = loadJsonFromFile(toTestFilePath(getCommandLineArgument("--eth_testfile")));
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
		BlockChainLoader bcl(m_json[name]);
		callback(m_json[name], bcl.bc(), bcl.state());
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
