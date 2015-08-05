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
/** @file state.cpp
 * @author Christoph Jentzsch <cj@ethdev.com>
 * @date 2014
 * State test functions.
 */

#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>

#include "../JsonSpiritHeaders.h"
#include <libdevcore/CommonIO.h>
#include <libethereum/CanonBlockChain.h>
#include <libethereum/State.h>
#include <libethereum/ExtVM.h>
#include <libethereum/Defaults.h>
#include <libevm/VM.h>
#include <test/TestHelper.h>

using namespace std;
using namespace json_spirit;
using namespace dev;
using namespace dev::eth;

namespace dev {  namespace test {

void doStateTests(json_spirit::mValue& v, bool _fillin)
{
	for (auto& i: v.get_obj())
	{
		mObject& o = i.second.get_obj();
		if (test::Options::get().singleTest && test::Options::get().singleTestName != i.first)
		{
			o.clear();
			continue;
		}

		std::cout << "  " << i.first << std::endl;
		TBOOST_REQUIRE((o.count("env") > 0));
		TBOOST_REQUIRE((o.count("pre") > 0));
		TBOOST_REQUIRE((o.count("transaction") > 0));

		ImportTest importer(o, _fillin);
		const State importedStatePost = importer.m_statePost;
		bytes output;

		try
		{
			Listener::ExecTimeGuard guard{i.first};
			output = importer.executeTest();
		}
		catch (Exception const& _e)
		{
			cnote << "Exception: " << diagnostic_information(_e);
			//theState.commit();
		}
		catch (std::exception const& _e)
		{
			cnote << "state execution exception: " << _e.what();
		}

		if (_fillin)
		{
#if ETH_FATDB
			importer.exportTest(output);
#else
			BOOST_THROW_EXCEPTION(Exception() << errinfo_comment("You can not fill tests when FATDB is switched off"));
#endif
		}
		else
		{
			TBOOST_REQUIRE((o.count("post") > 0));
			TBOOST_REQUIRE((o.count("out") > 0));

			// check output
			checkOutput(output, o);

			// check logs
			checkLog(importer.m_logs, importer.m_logsExpected);

			// check addresses
#if ETH_FATDB
			ImportTest::compareStates(importer.m_statePost, importedStatePost);
#endif
			TBOOST_CHECK_MESSAGE((importer.m_statePost.rootHash() == h256(o["postStateRoot"].get_str())), "wrong post state root");
		}
	}
}
} }// Namespace Close

BOOST_AUTO_TEST_SUITE(StateTests)

BOOST_AUTO_TEST_CASE(stExample)
{
	dev::test::executeTests("stExample", "/StateTests",dev::test::getFolder(__FILE__) + "/StateTestsFiller", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stSystemOperationsTest)
{
	dev::test::executeTests("stSystemOperationsTest", "/StateTests",dev::test::getFolder(__FILE__) + "/StateTestsFiller", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stCallCreateCallCodeTest)
{
	dev::test::executeTests("stCallCreateCallCodeTest", "/StateTests",dev::test::getFolder(__FILE__) + "/StateTestsFiller", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stPreCompiledContracts)
{
	dev::test::executeTests("stPreCompiledContracts", "/StateTests",dev::test::getFolder(__FILE__) + "/StateTestsFiller", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stLogTests)
{
	dev::test::executeTests("stLogTests", "/StateTests",dev::test::getFolder(__FILE__) + "/StateTestsFiller", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stRecursiveCreate)
{
	dev::test::executeTests("stRecursiveCreate", "/StateTests",dev::test::getFolder(__FILE__) + "/StateTestsFiller", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stInitCodeTest)
{
	dev::test::executeTests("stInitCodeTest", "/StateTests",dev::test::getFolder(__FILE__) + "/StateTestsFiller", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stTransactionTest)
{
	dev::test::executeTests("stTransactionTest", "/StateTests",dev::test::getFolder(__FILE__) + "/StateTestsFiller", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stSpecialTest)
{
	dev::test::executeTests("stSpecialTest", "/StateTests",dev::test::getFolder(__FILE__) + "/StateTestsFiller", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stRefundTest)
{
	dev::test::executeTests("stRefundTest", "/StateTests",dev::test::getFolder(__FILE__) + "/StateTestsFiller", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stBlockHashTest)
{
	dev::test::executeTests("stBlockHashTest", "/StateTests",dev::test::getFolder(__FILE__) + "/StateTestsFiller", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stQuadraticComplexityTest)
{
	if (test::Options::get().quadratic)
		dev::test::executeTests("stQuadraticComplexityTest", "/StateTests",dev::test::getFolder(__FILE__) + "/StateTestsFiller", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stMemoryStressTest)
{
	if (test::Options::get().memory)
		dev::test::executeTests("stMemoryStressTest", "/StateTests",dev::test::getFolder(__FILE__) + "/StateTestsFiller", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stSolidityTest)
{
	dev::test::executeTests("stSolidityTest", "/StateTests",dev::test::getFolder(__FILE__) + "/StateTestsFiller", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stMemoryTest)
{
	dev::test::executeTests("stMemoryTest", "/StateTests",dev::test::getFolder(__FILE__) + "/StateTestsFiller", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stWalletTest)
{
	dev::test::executeTests("stWalletTest", "/StateTests",dev::test::getFolder(__FILE__) + "/StateTestsFiller", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stCreateTest)
{
	for (int i = 1; i < boost::unit_test::framework::master_test_suite().argc; ++i)
	{
		string arg = boost::unit_test::framework::master_test_suite().argv[i];
		if (arg == "--createtest")
		{
			if (boost::unit_test::framework::master_test_suite().argc <= i + 2)
			{
				cnote << "usage: ./testeth --createtest <PathToConstructor> <PathToDestiny>\n";
				return;
			}
			try
			{
				cnote << "Populating tests...";
				json_spirit::mValue v;
				string s = asString(dev::contents(boost::unit_test::framework::master_test_suite().argv[i + 1]));
				BOOST_REQUIRE_MESSAGE(s.length() > 0, "Content of " + (string)boost::unit_test::framework::master_test_suite().argv[i + 1] + " is empty.");
				json_spirit::read_string(s, v);
				dev::test::doStateTests(v, true);
				writeFile(boost::unit_test::framework::master_test_suite().argv[i + 2], asBytes(json_spirit::write_string(v, true)));
			}
			catch (Exception const& _e)
			{
				BOOST_ERROR("Failed state test with Exception: " << diagnostic_information(_e));
			}
			catch (std::exception const& _e)
			{
				BOOST_ERROR("Failed state test with Exception: " << _e.what());
			}
		}
	}
}

BOOST_AUTO_TEST_CASE(stRandom)
{
	test::Options::get(); // parse command line options, e.g. to enable JIT

	string testPath = dev::test::getTestPath();
	testPath += "/StateTests/RandomTests";

	vector<boost::filesystem::path> testFiles;
	boost::filesystem::directory_iterator iterator(testPath);
	for(; iterator != boost::filesystem::directory_iterator(); ++iterator)
		if (boost::filesystem::is_regular_file(iterator->path()) && iterator->path().extension() == ".json")
			testFiles.push_back(iterator->path());

	for (auto& path: testFiles)
	{
		try
		{
			cnote << "Testing ..." << path.filename();
			json_spirit::mValue v;
			string s = asString(dev::contents(path.string()));
			BOOST_REQUIRE_MESSAGE(s.length() > 0, "Content of " + path.string() + " is empty. Have you cloned the 'tests' repo branch develop and set ETHEREUM_TEST_PATH to its path?");
			json_spirit::read_string(s, v);
			test::Listener::notifySuiteStarted(path.filename().string());
			dev::test::doStateTests(v, false);
		}
		catch (Exception const& _e)
		{
			BOOST_ERROR("Failed test with Exception: " << diagnostic_information(_e));
		}
		catch (std::exception const& _e)
		{
			BOOST_ERROR("Failed test with Exception: " << _e.what());
		}
	}
}

BOOST_AUTO_TEST_CASE(userDefinedFileState)
{
	dev::test::userDefinedTest(dev::test::doStateTests);
}

BOOST_AUTO_TEST_SUITE_END()
