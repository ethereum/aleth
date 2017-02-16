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

		ImportTest importer(o, _fillin, testType::StateTests);
		const State importedStatePost = importer.m_statePost;
		bytes output;

		Listener::ExecTimeGuard guard{i.first};
		output = importer.executeTest();

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
			ImportTest::compareStates(importedStatePost, importer.m_statePost);
#endif
			BOOST_CHECK_MESSAGE(importer.m_statePost.rootHash() == h256(o["postStateRoot"].get_str()), testname + "wrong post state root");
		}
	}
}
} }// Namespace Close

//BOOST_AUTO_TEST_SUITE(StateTestsEIP158)
//BOOST_AUTO_TEST_CASE(stZeroCallsRevertTestEIP158)
//{
//	dev::test::executeTests("stZeroCallsRevertTest", "/StateTests/EIP158", "/StateTestsFiller/EIP158", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stCodeSizeLimitEIP158)
//{
//	dev::test::executeTests("stCodeSizeLimit", "/StateTests/EIP158", "/StateTestsFiller/EIP158", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stCreateTestEIP158)
//{
//	dev::test::executeTests("stCreateTest", "/StateTests/EIP158", "/StateTestsFiller/EIP158", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stEIP158SpecificTest)
//{
//	dev::test::executeTests("stEIP158SpecificTest", "/StateTests/EIP158", "/StateTestsFiller/EIP158", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stNonZeroCallsTest)
//{
//	dev::test::executeTests("stNonZeroCallsTest", "/StateTests/EIP158", "/StateTestsFiller/EIP158", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stZeroCallsTest)
//{
//	dev::test::executeTests("stZeroCallsTest", "/StateTests/EIP158", "/StateTestsFiller/EIP158", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stChangedTestsEIP150)
//{
//	dev::test::executeTests("stChangedTests", "/StateTests/EIP158/EIP150", "/StateTestsFiller/EIP158/EIP150", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stEIPsingleCodeGasPricesEIP150)
//{
//	dev::test::executeTests("stEIPsingleCodeGasPrices", "/StateTests/EIP158/EIP150", "/StateTestsFiller/EIP158/EIP150", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stEIPSpecificTestEIP150)
//{
//	dev::test::executeTests("stEIPSpecificTest", "/StateTests/EIP158/EIP150", "/StateTestsFiller/EIP158/EIP150", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stMemExpandingEIPCallsEIP150)
//{
//	dev::test::executeTests("stMemExpandingEIPCalls", "/StateTests/EIP158/EIP150", "/StateTestsFiller/EIP158/EIP150", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stBoundsTestEIP)
//{
//	dev::test::executeTests("stBoundsTest", "/StateTests/EIP158/Homestead", "/StateTestsFiller/EIP158/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stHomeSteadSpecificEIP)
//{
//	dev::test::executeTests("stHomeSteadSpecific", "/StateTests/EIP158/Homestead", "/StateTestsFiller/EIP158/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stCallDelegateCodesCallCodeEIP)
//{
//	dev::test::executeTests("stCallDelegateCodesCallCode", "/StateTests/EIP158/Homestead", "/StateTestsFiller/EIP158/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stCallDelegateCodesEIP)
//{
//	dev::test::executeTests("stCallDelegateCodes", "/StateTests/EIP158/Homestead", "/StateTestsFiller/EIP158/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stDelegatecallTestEIP)
//{
//	dev::test::executeTests("stDelegatecallTest", "/StateTests/EIP158/Homestead", "/StateTestsFiller/EIP158/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stCallCodesEIP)
//{
//	dev::test::executeTests("stCallCodes", "/StateTests/EIP158/Homestead", "/StateTestsFiller/EIP158/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stSystemOperationsTestEIP)
//{
//	dev::test::executeTests("stSystemOperationsTest", "/StateTests/EIP158/Homestead", "/StateTestsFiller/EIP158/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stCallCreateCallCodeTestEIP)
//{
//	dev::test::executeTests("stCallCreateCallCodeTest", "/StateTests/EIP158/Homestead", "/StateTestsFiller/EIP158/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stPreCompiledContractsEIP)
//{
//	dev::test::executeTests("stPreCompiledContracts", "/StateTests/EIP158/Homestead", "/StateTestsFiller/EIP158/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stLogTestsEIP)
//{
//	dev::test::executeTests("stLogTests", "/StateTests/EIP158/Homestead", "/StateTestsFiller/EIP158/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stRecursiveCreateEIP)
//{
//	dev::test::executeTests("stRecursiveCreate", "/StateTests/EIP158/Homestead", "/StateTestsFiller/EIP158/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stInitCodeTestEIP)
//{
//	dev::test::executeTests("stInitCodeTest", "/StateTests/EIP158/Homestead", "/StateTestsFiller/EIP158/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stTransactionTestEIP)
//{
//	dev::test::executeTests("stTransactionTest", "/StateTests/EIP158/Homestead", "/StateTestsFiller/EIP158/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stSpecialTestEIP)
//{
//	dev::test::executeTests("stSpecialTest", "/StateTests/EIP158/Homestead", "/StateTestsFiller/EIP158/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stRefundTestEIP)
//{
//	dev::test::executeTests("stRefundTest", "/StateTests/EIP158/Homestead", "/StateTestsFiller/EIP158/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stQuadraticComplexityTestEIP)
//{
//	if (test::Options::get().quadratic)
//		dev::test::executeTests("stQuadraticComplexityTest", "/StateTests/EIP158/Homestead", "/StateTestsFiller/EIP158/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stMemoryStressTestEIP)
//{
//	if (test::Options::get().memory)
//		dev::test::executeTests("stMemoryStressTest", "/StateTests/EIP158/Homestead", "/StateTestsFiller/EIP158/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stMemoryTestEIP)
//{
//	dev::test::executeTests("stMemoryTest", "/StateTests/EIP158/Homestead", "/StateTestsFiller/EIP158/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stWalletTestEIP)
//{
//	dev::test::executeTests("stWalletTest", "/StateTests/EIP158/Homestead", "/StateTestsFiller/EIP158/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_SUITE_END()

//BOOST_AUTO_TEST_SUITE(StateTestsEIP)
//BOOST_AUTO_TEST_CASE(stChangedOnEIPTest)
//{
//	dev::test::executeTests("stChangedTests", "/StateTests/EIP150", "/StateTestsFiller/EIP150", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stEIPSpecificTest)
//{
//	dev::test::executeTests("stEIPSpecificTest", "/StateTests/EIP150", "/StateTestsFiller/EIP150", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stEIPsingleCodeGasPrices)
//{
//	dev::test::executeTests("stEIPsingleCodeGasPrices", "/StateTests/EIP150", "/StateTestsFiller/EIP150", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stMemExpandingEIPCalls)
//{
//	dev::test::executeTests("stMemExpandingEIPCalls", "/StateTests/EIP150", "/StateTestsFiller/EIP150", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stBoundsTestEIP)
//{
//	dev::test::executeTests("stBoundsTest", "/StateTests/EIP150/Homestead", "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stHomeSteadSpecificEIP)
//{
//	dev::test::executeTests("stHomeSteadSpecific", "/StateTests/EIP150/Homestead", "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stCallDelegateCodesCallCodeEIP)
//{
//	dev::test::executeTests("stCallDelegateCodesCallCode", "/StateTests/EIP150/Homestead", "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stCallDelegateCodesEIP)
//{
//	dev::test::executeTests("stCallDelegateCodes", "/StateTests/EIP150/Homestead", "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stDelegatecallTestEIP)
//{
//	dev::test::executeTests("stDelegatecallTest", "/StateTests/EIP150/Homestead", "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stCallCodesEIP)
//{
//	dev::test::executeTests("stCallCodes", "/StateTests/EIP150/Homestead", "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stSystemOperationsTestEIP)
//{
//	dev::test::executeTests("stSystemOperationsTest", "/StateTests/EIP150/Homestead", "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stCallCreateCallCodeTestEIP)
//{
//	dev::test::executeTests("stCallCreateCallCodeTest", "/StateTests/EIP150/Homestead", "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stPreCompiledContractsEIP)
//{
//	dev::test::executeTests("stPreCompiledContracts", "/StateTests/EIP150/Homestead", "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stLogTestsEIP)
//{
//	dev::test::executeTests("stLogTests", "/StateTests/EIP150/Homestead", "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stRecursiveCreateEIP)
//{
//	dev::test::executeTests("stRecursiveCreate", "/StateTests/EIP150/Homestead", "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stInitCodeTestEIP)
//{
//	dev::test::executeTests("stInitCodeTest", "/StateTests/EIP150/Homestead", "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stTransactionTestEIP)
//{
//	dev::test::executeTests("stTransactionTest", "/StateTests/EIP150/Homestead", "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stSpecialTestEIP)
//{
//	dev::test::executeTests("stSpecialTest", "/StateTests/EIP150/Homestead", "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stRefundTestEIP)
//{
//	dev::test::executeTests("stRefundTest", "/StateTests/EIP150/Homestead", "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stQuadraticComplexityTestEIP)
//{
//	if (test::Options::get().quadratic)
//		dev::test::executeTests("stQuadraticComplexityTest", "/StateTests/EIP150/Homestead", "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stMemoryStressTestEIP)
//{
//	if (test::Options::get().memory)
//		dev::test::executeTests("stMemoryStressTest", "/StateTests/EIP150/Homestead", "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stMemoryTestEIP)
//{
//	dev::test::executeTests("stMemoryTest", "/StateTests/EIP150/Homestead", "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stWalletTestEIP)
//{
//	dev::test::executeTests("stWalletTest", "/StateTests/EIP150/Homestead", "/StateTestsFiller/EIP150/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_SUITE_END()

//BOOST_AUTO_TEST_SUITE(StateTestsHomestead)
//BOOST_AUTO_TEST_CASE(stZeroCallsRevertTest)
//{
//	dev::test::executeTests("stZeroCallsRevertTest", "/StateTests/Homestead", "/StateTestsFiller/Homestead", dev::test::doStateTests);
//}
//BOOST_AUTO_TEST_CASE(stBoundsTestHomestead)
//{
//	dev::test::executeTests("stBoundsTest", "/StateTests/Homestead", "/StateTestsFiller/Homestead", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stHomeSteadSpecific)
//{
//	dev::test::executeTests("stHomeSteadSpecific", "/StateTests/Homestead", "/StateTestsFiller/Homestead", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stCallDelegateCodesCallCodeHomestead)
//{
//	dev::test::executeTests("stCallDelegateCodesCallCode", "/StateTests/Homestead", "/StateTestsFiller/Homestead", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stCallDelegateCodesHomestead)
//{
//	dev::test::executeTests("stCallDelegateCodes", "/StateTests/Homestead", "/StateTestsFiller/Homestead", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stDelegatecallTestHomestead)
//{
//	dev::test::executeTests("stDelegatecallTest", "/StateTests/Homestead", "/StateTestsFiller/Homestead", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stCallCodesHomestead)
//{
//	dev::test::executeTests("stCallCodes", "/StateTests/Homestead", "/StateTestsFiller/Homestead", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stSystemOperationsTestHomestead)
//{
//	dev::test::executeTests("stSystemOperationsTest", "/StateTests/Homestead", "/StateTestsFiller/Homestead", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stCallCreateCallCodeTestHomestead)
//{
//	dev::test::executeTests("stCallCreateCallCodeTest", "/StateTests/Homestead", "/StateTestsFiller/Homestead", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stPreCompiledContractsHomestead)
//{
//	dev::test::executeTests("stPreCompiledContracts", "/StateTests/Homestead", "/StateTestsFiller/Homestead", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stLogTestsHomestead)
//{
//	dev::test::executeTests("stLogTests", "/StateTests/Homestead", "/StateTestsFiller/Homestead", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stRecursiveCreateHomestead)
//{
//	dev::test::executeTests("stRecursiveCreate", "/StateTests/Homestead", "/StateTestsFiller/Homestead", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stInitCodeTestHomestead)
//{
//	dev::test::executeTests("stInitCodeTest", "/StateTests/Homestead", "/StateTestsFiller/Homestead", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stTransactionTestHomestead)
//{
//	dev::test::executeTests("stTransactionTest", "/StateTests/Homestead", "/StateTestsFiller/Homestead", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stSpecialTestHomestead)
//{
//	dev::test::executeTests("stSpecialTest", "/StateTests/Homestead", "/StateTestsFiller/Homestead", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stRefundTestHomestead)
//{
//	dev::test::executeTests("stRefundTest", "/StateTests/Homestead", "/StateTestsFiller/Homestead", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stQuadraticComplexityTestHomestead)
//{
//	if (test::Options::get().quadratic)
//		dev::test::executeTests("stQuadraticComplexityTest", "/StateTests/Homestead", "/StateTestsFiller/Homestead", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stMemoryStressTestHomestead)
//{
//	if (test::Options::get().memory)
//		dev::test::executeTests("stMemoryStressTest", "/StateTests/Homestead", "/StateTestsFiller/Homestead", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stMemoryTestHomestead)
//{
//	dev::test::executeTests("stMemoryTest", "/StateTests/Homestead", "/StateTestsFiller/Homestead", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stWalletTestHomestead)
//{
//	dev::test::executeTests("stWalletTest", "/StateTests/Homestead", "/StateTestsFiller/Homestead", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_SUITE_END()


//BOOST_AUTO_TEST_SUITE(StateTests)
//BOOST_AUTO_TEST_CASE(stCallCodes)
//{
//	dev::test::executeTests("stCallCodes", "/StateTests", "/StateTestsFiller", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stExample)
//{
//	dev::test::executeTests("stExample", "/StateTests", "/StateTestsFiller", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stSystemOperationsTest)
//{
//	dev::test::executeTests("stSystemOperationsTest", "/StateTests", "/StateTestsFiller", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stCallCreateCallCodeTest)
//{
//	dev::test::executeTests("stCallCreateCallCodeTest", "/StateTests", "/StateTestsFiller", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stPreCompiledContracts)
//{
//	dev::test::executeTests("stPreCompiledContracts", "/StateTests", "/StateTestsFiller", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stLogTests)
//{
//	dev::test::executeTests("stLogTests", "/StateTests", "/StateTestsFiller", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stRecursiveCreate)
//{
//	dev::test::executeTests("stRecursiveCreate", "/StateTests", "/StateTestsFiller", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stInitCodeTest)
//{
//	dev::test::executeTests("stInitCodeTest", "/StateTests", "/StateTestsFiller", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stTransactionTest)
//{
//	dev::test::executeTests("stTransactionTest", "/StateTests", "/StateTestsFiller", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stSpecialTest)
//{
//	dev::test::executeTests("stSpecialTest", "/StateTests", "/StateTestsFiller", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stRefundTest)
//{
//	dev::test::executeTests("stRefundTest", "/StateTests", "/StateTestsFiller", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stBlockHashTest)
//{
//	dev::test::executeTests("stBlockHashTest", "/StateTests", "/StateTestsFiller", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stQuadraticComplexityTest)
//{
//	if (test::Options::get().quadratic)
//		dev::test::executeTests("stQuadraticComplexityTest", "/StateTests", "/StateTestsFiller", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stMemoryStressTest)
//{
//	if (test::Options::get().memory)
//		dev::test::executeTests("stMemoryStressTest", "/StateTests", "/StateTestsFiller", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stSolidityTest)
//{
//	dev::test::executeTests("stSolidityTest", "/StateTests", "/StateTestsFiller", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stMemoryTest)
//{
//	dev::test::executeTests("stMemoryTest", "/StateTests", "/StateTestsFiller", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stWalletTest)
//{
//	dev::test::executeTests("stWalletTest", "/StateTests", "/StateTestsFiller", dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_CASE(stTransitionTest)
//{
//	dev::test::executeTests("stTransitionTest", "/StateTests", "/StateTestsFiller", dev::test::doStateTests);
//}

////todo: Force stRandom to be tested on Homestead Seal Engine ?
//BOOST_AUTO_TEST_CASE(stRandom)
//{
//	test::Options::get(); // parse command line options, e.g. to enable JIT
//	string testPath = dev::test::getTestPath() + "/StateTests/RandomTests";
//	string fillersPath = dev::test::getTestPath() + "/src/StateTestsFiller/RandomTests";

//	if (dev::test::Options::get().filltests)
//	{
//		test::TestOutputHelper::initTest();
//		vector<boost::filesystem::path> testFiles;
//		boost::filesystem::directory_iterator iterator(fillersPath);
//		for(; iterator != boost::filesystem::directory_iterator(); ++iterator)
//			if (boost::filesystem::is_regular_file(iterator->path()) && iterator->path().extension() == ".json")
//			{
//				if (test::Options::get().singleTest)
//				{
//					string fileboost = iterator->path().filename().string();
//					string filesingletest = test::Options::get().singleTestName; //parse singletest as a filename of random test
//					if (fileboost == filesingletest || fileboost.substr(0, fileboost.length()-5) == filesingletest)
//						testFiles.push_back(iterator->path());
//				}
//				else
//					testFiles.push_back(iterator->path());
//			}

//		test::TestOutputHelper::setMaxTests(testFiles.size() * 2);
//		for (auto& path: testFiles)
//		{
//			std::string filename = path.filename().string();
//			test::TestOutputHelper::setCurrentTestFileName(filename);
//			filename = filename.substr(0, filename.length() - 5); //without .json
//			dev::test::executeTests(filename, "/StateTests/RandomTests", "/StateTestsFiller/RandomTests", dev::test::doStateTests, false);
//		}
//		return; //executeTests has already run test after filling
//	}

//	vector<boost::filesystem::path> testFiles;
//	boost::filesystem::directory_iterator iterator(testPath);
//	for(; iterator != boost::filesystem::directory_iterator(); ++iterator)
//		if (boost::filesystem::is_regular_file(iterator->path()) && iterator->path().extension() == ".json")
//		{
//			if (test::Options::get().singleTest)
//			{
//				string fileboost = iterator->path().filename().string();
//				string filesingletest = test::Options::get().singleTestName; //parse singletest as a filename of random test
//				if (fileboost == filesingletest || fileboost.substr(0, fileboost.length()-5) == filesingletest)
//					testFiles.push_back(iterator->path());
//			}
//			else
//				testFiles.push_back(iterator->path());
//		}

//	test::TestOutputHelper::initTest();
//	test::TestOutputHelper::setMaxTests(testFiles.size());

//	for (auto& path: testFiles)
//	{
//		try
//		{
//			cnote << "Testing ..." << path.filename();
//			test::TestOutputHelper::setCurrentTestFileName(path.filename().string());
//			json_spirit::mValue v;
//			string s = asString(dev::contents(path.string()));
//			BOOST_REQUIRE_MESSAGE(s.length() > 0, "Content of " + path.string() + " is empty. Have you cloned the 'tests' repo branch develop and set ETHEREUM_TEST_PATH to its path?");
//			json_spirit::read_string(s, v);
//			test::Listener::notifySuiteStarted(path.filename().string());
//			dev::test::doStateTests(v, false);
//		}
//		catch (Exception const& _e)
//		{
//			BOOST_ERROR(path.filename().string() + "Failed test with Exception: " << diagnostic_information(_e));
//		}
//		catch (std::exception const& _e)
//		{
//			BOOST_ERROR(path.filename().string() + "Failed test with Exception: " << _e.what());
//		}
//	}
//}

//BOOST_AUTO_TEST_CASE(userDefinedFileState)
//{
//	dev::test::userDefinedTest(dev::test::doStateTests);
//}

//BOOST_AUTO_TEST_SUITE_END()
