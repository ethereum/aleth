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

#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcore/CommonIO.h>
#include <libethereum/BlockChain.h>
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

void doStateTests(json_spirit::mValue& _v, bool _fillin)
{
	if (string(boost::unit_test::framework::current_test_case().p_name) != "stRandom")
		TestOutputHelper::initTest(_v);

	for (auto& i: _v.get_obj())
	{
		string testname = i.first;
		json_spirit::mObject& o = i.second.get_obj();

		if (!TestOutputHelper::passTest(o, testname))
			continue;

		BOOST_REQUIRE_MESSAGE(o.count("env") > 0, testname + "env not set!");
		BOOST_REQUIRE_MESSAGE(o.count("pre") > 0, testname + "pre not set!");
		BOOST_REQUIRE_MESSAGE(o.count("transaction") > 0, testname + "transaction not set!");

		ImportTest importer(o, _fillin);
		const State importedStatePost = importer.m_statePost;
		bytes output;

		Listener::ExecTimeGuard guard{i.first};

		if (importer.m_envInfo.number() >= dev::test::c_testHomesteadBlock)
			output = importer.executeTest(eth::Network::HomesteadTest);
		else
			output = importer.executeTest(eth::Network::FrontierTest);

		if (_fillin)
		{
#if ETH_FATDB
			if (importer.exportTest(output))
				cerr << testname << endl;
#else
			BOOST_THROW_EXCEPTION(Exception() << errinfo_comment(testname + "You can not fill tests when FATDB is switched off"));
#endif
		}
		else
		{
			BOOST_REQUIRE(o.count("post") > 0);
			BOOST_REQUIRE(o.count("out") > 0);

			// check output
			checkOutput(output, o);

			// check logs
			checkLog(importer.m_logs, importer.m_logsExpected);

			// check addresses
#if ETH_FATDB
			ImportTest::compareStates(importer.m_statePost, importedStatePost);
#endif
			BOOST_CHECK_MESSAGE(importer.m_statePost.rootHash() == h256(o["postStateRoot"].get_str()), testname + "wrong post state root");
		}
	}
}
} }// Namespace Close

BOOST_AUTO_TEST_SUITE(StateTestsEIP)

BOOST_AUTO_TEST_CASE(stChangedOnEIPTest)
{
	dev::test::executeTests("stChangedTests", "/StateTests/EIP150",dev::test::getFolder(__FILE__) + "/StateTestsFiller/EIP150", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stEIPSpecificTest)
{
	dev::test::executeTests("stEIPSpecificTest", "/StateTests/EIP150",dev::test::getFolder(__FILE__) + "/StateTestsFiller/EIP150", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stEIPsingleCodeGasPrices)
{
	dev::test::executeTests("stEIPsingleCodeGasPrices", "/StateTests/EIP150",dev::test::getFolder(__FILE__) + "/StateTestsFiller/EIP150", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stMemExpandingEIPCalls)
{
	dev::test::executeTests("stMemExpandingEIPCalls", "/StateTests/EIP150",dev::test::getFolder(__FILE__) + "/StateTestsFiller/EIP150", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stBoundsTestEIP)
{
	dev::test::executeTests("stBoundsTest", "/StateTests/EIP150/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stHomeSteadSpecificEIP)
{
	dev::test::executeTests("stHomeSteadSpecific", "/StateTests/EIP150/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stCallDelegateCodesCallCodeEIP)
{
	dev::test::executeTests("stCallDelegateCodesCallCode", "/StateTests/EIP150/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stCallDelegateCodesEIP)
{
	dev::test::executeTests("stCallDelegateCodes", "/StateTests/EIP150/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stDelegatecallTestEIP)
{
	dev::test::executeTests("stDelegatecallTest", "/StateTests/EIP150/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stCallCodesEIP)
{
	dev::test::executeTests("stCallCodes", "/StateTests/EIP150/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stSystemOperationsTestEIP)
{
	dev::test::executeTests("stSystemOperationsTest", "/StateTests/EIP150/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stCallCreateCallCodeTestEIP)
{
	dev::test::executeTests("stCallCreateCallCodeTest", "/StateTests/EIP150/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stPreCompiledContractsEIP)
{
	dev::test::executeTests("stPreCompiledContracts", "/StateTests/EIP150/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stLogTestsEIP)
{
	dev::test::executeTests("stLogTests", "/StateTests/EIP150/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stRecursiveCreateEIP)
{
	dev::test::executeTests("stRecursiveCreate", "/StateTests/EIP150/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stInitCodeTestEIP)
{
	dev::test::executeTests("stInitCodeTest", "/StateTests/EIP150/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stTransactionTestEIP)
{
	dev::test::executeTests("stTransactionTest", "/StateTests/EIP150/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stSpecialTestEIP)
{
	dev::test::executeTests("stSpecialTest", "/StateTests/EIP150/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stRefundTestEIP)
{
	dev::test::executeTests("stRefundTest", "/StateTests/EIP150/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stQuadraticComplexityTestEIP)
{
	if (test::Options::get().quadratic)
		dev::test::executeTests("stQuadraticComplexityTest", "/StateTests/EIP150/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stMemoryStressTestEIP)
{
	if (test::Options::get().memory)
		dev::test::executeTests("stMemoryStressTest", "/StateTests/EIP150/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stMemoryTestEIP)
{
	dev::test::executeTests("stMemoryTest", "/StateTests/EIP150/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stWalletTestEIP)
{
	dev::test::executeTests("stWalletTest", "/StateTests/EIP150/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(StateTestsHomestead)
BOOST_AUTO_TEST_CASE(stBoundsTestHomestead)
{
	dev::test::executeTests("stBoundsTest", "/StateTests/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stHomeSteadSpecific)
{
	dev::test::executeTests("stHomeSteadSpecific", "/StateTests/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stCallDelegateCodesCallCodeHomestead)
{
	dev::test::executeTests("stCallDelegateCodesCallCode", "/StateTests/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stCallDelegateCodesHomestead)
{
	dev::test::executeTests("stCallDelegateCodes", "/StateTests/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stDelegatecallTestHomestead)
{
	dev::test::executeTests("stDelegatecallTest", "/StateTests/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stCallCodesHomestead)
{
	dev::test::executeTests("stCallCodes", "/StateTests/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stSystemOperationsTestHomestead)
{
	dev::test::executeTests("stSystemOperationsTest", "/StateTests/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stCallCreateCallCodeTestHomestead)
{
	dev::test::executeTests("stCallCreateCallCodeTest", "/StateTests/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stPreCompiledContractsHomestead)
{
	dev::test::executeTests("stPreCompiledContracts", "/StateTests/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stLogTestsHomestead)
{
	dev::test::executeTests("stLogTests", "/StateTests/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stRecursiveCreateHomestead)
{
	dev::test::executeTests("stRecursiveCreate", "/StateTests/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stInitCodeTestHomestead)
{
	dev::test::executeTests("stInitCodeTest", "/StateTests/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stTransactionTestHomestead)
{
	dev::test::executeTests("stTransactionTest", "/StateTests/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stSpecialTestHomestead)
{
	dev::test::executeTests("stSpecialTest", "/StateTests/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stRefundTestHomestead)
{
	dev::test::executeTests("stRefundTest", "/StateTests/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stQuadraticComplexityTestHomestead)
{
	if (test::Options::get().quadratic)
		dev::test::executeTests("stQuadraticComplexityTest", "/StateTests/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stMemoryStressTestHomestead)
{
	if (test::Options::get().memory)
		dev::test::executeTests("stMemoryStressTest", "/StateTests/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stMemoryTestHomestead)
{
	dev::test::executeTests("stMemoryTest", "/StateTests/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_CASE(stWalletTestHomestead)
{
	dev::test::executeTests("stWalletTest", "/StateTests/Homestead",dev::test::getFolder(__FILE__) + "/StateTestsFiller/Homestead", dev::test::doStateTests);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StateTests)
BOOST_AUTO_TEST_CASE(stCallCodes)
{
	dev::test::executeTests("stCallCodes", "/StateTests",dev::test::getFolder(__FILE__) + "/StateTestsFiller", dev::test::doStateTests);
}

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

BOOST_AUTO_TEST_CASE(stTransitionTest)
{
	dev::test::executeTests("stTransitionTest", "/StateTests",dev::test::getFolder(__FILE__) + "/StateTestsFiller", dev::test::doStateTests);
}

//todo: Force stRandom to be tested on Homestead Seal Engine ?
BOOST_AUTO_TEST_CASE(stRandom)
{
	test::Options::get(); // parse command line options, e.g. to enable JIT

	if (dev::test::Options::get().fillTests)
	{
		test::TestOutputHelper::initTest();
		std::string fillersPath = dev::test::getFolder(__FILE__) + "/StateTestsFiller/RandomTests";

		vector<boost::filesystem::path> testFiles;
		boost::filesystem::directory_iterator iterator(fillersPath);
		for(; iterator != boost::filesystem::directory_iterator(); ++iterator)
			if (boost::filesystem::is_regular_file(iterator->path()) && iterator->path().extension() == ".json")
			{
				if (test::Options::get().singleTest)
				{
					string fileboost = iterator->path().filename().string();
					string filesingletest = test::Options::get().singleTestName; //parse singletest as a filename of random test
					if (fileboost == filesingletest || fileboost.substr(0, fileboost.length()-5) == filesingletest)
						testFiles.push_back(iterator->path());
				}
				else
					testFiles.push_back(iterator->path());
			}

		test::TestOutputHelper::setMaxTests(testFiles.size() * 2);
		for (auto& path: testFiles)
		{
			std::string filename = path.filename().string();
			test::TestOutputHelper::setCurrentTestFileName(filename);
			filename = filename.substr(0, filename.length() - 5); //without .json
			dev::test::executeTests(filename, "/StateTests/RandomTests",dev::test::getFolder(__FILE__) + "/StateTestsFiller/RandomTests", dev::test::doStateTests, false);
		}
		return; //executeTests has already run test after filling
	}

	string testPath = dev::test::getTestPath();
	testPath += "/StateTests/RandomTests";

	vector<boost::filesystem::path> testFiles;
	boost::filesystem::directory_iterator iterator(testPath);
	for(; iterator != boost::filesystem::directory_iterator(); ++iterator)
		if (boost::filesystem::is_regular_file(iterator->path()) && iterator->path().extension() == ".json")
		{
			if (test::Options::get().singleTest)
			{
				string fileboost = iterator->path().filename().string();
				string filesingletest = test::Options::get().singleTestName; //parse singletest as a filename of random test
				if (fileboost == filesingletest || fileboost.substr(0, fileboost.length()-5) == filesingletest)
					testFiles.push_back(iterator->path());
			}
			else
				testFiles.push_back(iterator->path());
		}

	test::TestOutputHelper::initTest();
	test::TestOutputHelper::setMaxTests(testFiles.size());

	for (auto& path: testFiles)
	{
		try
		{
			cnote << "Testing ..." << path.filename();
			test::TestOutputHelper::setCurrentTestFileName(path.filename().string());
			json_spirit::mValue v;
			string s = asString(dev::contents(path.string()));
			BOOST_REQUIRE_MESSAGE(s.length() > 0, "Content of " + path.string() + " is empty. Have you cloned the 'tests' repo branch develop and set ETHEREUM_TEST_PATH to its path?");
			json_spirit::read_string(s, v);
			test::Listener::notifySuiteStarted(path.filename().string());
			dev::test::doStateTests(v, false);
		}
		catch (Exception const& _e)
		{
			BOOST_ERROR(path.filename().string() + "Failed test with Exception: " << diagnostic_information(_e));
		}
		catch (std::exception const& _e)
		{
			BOOST_ERROR(path.filename().string() + "Failed test with Exception: " << _e.what());
		}
	}
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

BOOST_AUTO_TEST_CASE(userDefinedFileState)
{
	dev::test::userDefinedTest(dev::test::doStateTests);
}

BOOST_AUTO_TEST_SUITE_END()

