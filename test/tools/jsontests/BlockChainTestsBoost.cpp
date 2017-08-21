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
/** @file BlockChainTestsBoost.cpp
 * @author Christoph Jentzsch <cj@ethdev.com>, Dimitry Khokhlov <dimitry@ethdev.com>
 * @date 2016
 * BlockChain boost test cases.
 */

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <test/tools/libtesteth/TestHelper.h>
#include <test/tools/libtesteth/BlockChainHelper.h>

class bcTestFixture {
	public:
	bcTestFixture()
	{
		string casename = boost::unit_test::framework::current_test_case().p_name;
		if (casename == "bcForgedTest")
		{
			std::string fillersPath =  dev::test::getTestPath() + "/src/BlockchainTestsFiller/bcForgedTest";
			std::vector<boost::filesystem::path> files = test::getJsonFiles(fillersPath);

			for (auto const& file : files)
			{
				if (!dev::test::Options::get().filltests)
				{
					clog << "\\/ " << file.filename().string() << "\n";
					dev::test::executeTests(file.filename().string(), "/BlockchainTests/bcForgedTest", "/BlockchainTestsFiller/bcForgedTest", dev::test::doBlockchainTests);
				}
				else
				{
					dev::test::TestOutputHelper::initTest();
					string copyto = dev::test::getTestPath() + "/BlockchainTests/bcForgedTest/" + file.filename().string();
					clog << "Copying " + fillersPath + "/" + file.filename().string();
					clog << " TO " << copyto;
					dev::test::copyFile(fillersPath + "/" + file.filename().string(), dev::test::getTestPath() + "/BlockchainTests/bcForgedTest/" + file.filename().string());
					BOOST_REQUIRE_MESSAGE(boost::filesystem::exists(copyto), "Error when copying the test file!");
					dev::test::TestOutputHelper::finishTest();
				}
			}
			return;
		}

		//skip wallet test as it takes too much time (250 blocks) run it with --all flag
		if (casename == "bcWalletTest" && !test::Options::get().wallet)
		{
			clog << "skipping bcWalletTest. Use --all to run it.\n";
			return;
		}

		fillAllFilesInFolder(casename);
	}

	void fillAllFilesInFolder(string const& _folder)
	{
		std::string fillersPath = test::getTestPath() + "/src/BlockchainTestsFiller/" + _folder;

		string filter;
		if (test::Options::get().filltests)
			filter = test::Options::get().singleTestName.empty() ? string() : test::Options::get().singleTestName + "Filler";

		std::vector<boost::filesystem::path> files = test::getJsonFiles(fillersPath, filter);
		int testcount = files.size() * test::getNetworks().size(); //1 test case translated to each fork.

		//include fillers into test count
		if (test::Options::get().filltests)
			testcount += testcount / test::getNetworks().size();

		test::TestOutputHelper::initTest(testcount);
		for (auto const& file: files)
		{
			test::TestOutputHelper::setCurrentTestFileName(file.filename().string());
			test::executeTests(file.filename().string(), "/BlockchainTests/" + _folder, "/BlockchainTestsFiller/" + _folder, dev::test::doBlockchainTestNoLog);
		}
		test::TestOutputHelper::finishTest();
	}
};

class bcTransitionFixture {
	public:
	bcTransitionFixture()
	{
		string casename = boost::unit_test::framework::current_test_case().p_name;
		fillAllFilesInFolder("TransitionTests/", casename);
	}

	void fillAllFilesInFolder(string const& _subfolder, string const& _folder)
	{
		std::string fillersPath = test::getTestPath() + "/src/BlockchainTestsFiller/" + _subfolder + _folder;

		string filter;
		if (test::Options::get().filltests)
			filter = test::Options::get().singleTestName.empty() ? string() : test::Options::get().singleTestName + "Filler";

		std::vector<boost::filesystem::path> files = test::getJsonFiles(fillersPath, filter);
		int testcount = files.size();

		//include fillers into test count
		if (test::Options::get().filltests)
			testcount *= 2;

		test::TestOutputHelper::initTest(testcount);
		for (auto const& file: files)
		{
			test::TestOutputHelper::setCurrentTestFileName(file.filename().string());
			test::executeTests(file.filename().string(), "/BlockchainTests/" + _subfolder + _folder, "/BlockchainTestsFiller/" + _subfolder +_folder, dev::test::doTransitionTest);
		}
		test::TestOutputHelper::finishTest();
	}
};

class bcGeneralTestsFixture
{
	public:
	bcGeneralTestsFixture()
	{
		//general tests are filled from state tests
		//skip this test suite if not run with --all flag (cases are already tested in state tests)
		if (test::Options::get().filltests || !test::Options::get().performance)
			return;

		string casename = boost::unit_test::framework::current_test_case().p_name;
		runAllFilesInFolder("GeneralStateTests/" + casename);
	}

	void runAllFilesInFolder(string const& _folder)
	{
		std::vector<boost::filesystem::path> files = test::getJsonFiles(test::getTestPath() + "/BlockchainTests/" +_folder);
		int testcount = files.size() * test::getNetworks().size();  //each file contains a test per network fork

		test::TestOutputHelper::initTest(testcount);
		for (auto const& file: files)
		{
			test::TestOutputHelper::setCurrentTestFileName(file.filename().string());
			test::executeTests(file.filename().string(), "/BlockchainTests/" + _folder, "/BlockchainTests/" +_folder, dev::test::doBlockchainTestNoLog);
		}
		test::TestOutputHelper::finishTest();
	}
};

BOOST_FIXTURE_TEST_SUITE(BlockchainTests, bcTestFixture)

BOOST_AUTO_TEST_CASE(bcStateTests){}
BOOST_AUTO_TEST_CASE(bcBlockGasLimitTest){}
BOOST_AUTO_TEST_CASE(bcGasPricerTest){}
BOOST_AUTO_TEST_CASE(bcInvalidHeaderTest){}
BOOST_AUTO_TEST_CASE(bcUncleHeaderValiditiy){}
BOOST_AUTO_TEST_CASE(bcUncleTest){}
BOOST_AUTO_TEST_CASE(bcValidBlockTest){}
BOOST_AUTO_TEST_CASE(bcWalletTest){}
BOOST_AUTO_TEST_CASE(bcTotalDifficultyTest){}
BOOST_AUTO_TEST_CASE(bcMultiChainTest){}
BOOST_AUTO_TEST_CASE(bcForkStressTest){}
BOOST_AUTO_TEST_CASE(bcForgedTest){}
BOOST_AUTO_TEST_CASE(bcRandomBlockhashTest){}
BOOST_AUTO_TEST_CASE(bcExploitTest){}

BOOST_AUTO_TEST_SUITE_END()

//Transition from fork to fork tests
BOOST_FIXTURE_TEST_SUITE(TransitionTests, bcTransitionFixture)

BOOST_AUTO_TEST_CASE(bcFrontierToHomestead){}
BOOST_AUTO_TEST_CASE(bcHomesteadToDao){}
BOOST_AUTO_TEST_CASE(bcHomesteadToEIP150){}
BOOST_AUTO_TEST_CASE(bcEIP158ToByzantium){}

BOOST_AUTO_TEST_SUITE_END()

//General tests in form of blockchain tests
BOOST_FIXTURE_TEST_SUITE(BCGeneralStateTests, bcGeneralTestsFixture)

//Frontier Tests
BOOST_AUTO_TEST_CASE(stCallCodes){}
BOOST_AUTO_TEST_CASE(stCallCreateCallCodeTest){}
BOOST_AUTO_TEST_CASE(stExample){}
BOOST_AUTO_TEST_CASE(stInitCodeTest){}
BOOST_AUTO_TEST_CASE(stLogTests){}
BOOST_AUTO_TEST_CASE(stMemoryTest){}
BOOST_AUTO_TEST_CASE(stPreCompiledContracts){}
BOOST_AUTO_TEST_CASE(stRandom){}
BOOST_AUTO_TEST_CASE(stRecursiveCreate){}
BOOST_AUTO_TEST_CASE(stRefundTest){}
BOOST_AUTO_TEST_CASE(stSolidityTest){}
BOOST_AUTO_TEST_CASE(stSpecialTest){}
BOOST_AUTO_TEST_CASE(stSystemOperationsTest){}
BOOST_AUTO_TEST_CASE(stTransactionTest){}
BOOST_AUTO_TEST_CASE(stTransitionTest){}
BOOST_AUTO_TEST_CASE(stWalletTest){}

//Homestead Tests
BOOST_AUTO_TEST_CASE(stCallDelegateCodesCallCodeHomestead){}
BOOST_AUTO_TEST_CASE(stCallDelegateCodesHomestead){}
BOOST_AUTO_TEST_CASE(stHomesteadSpecific){}
BOOST_AUTO_TEST_CASE(stDelegatecallTestHomestead){}

//EIP150 Tests
BOOST_AUTO_TEST_CASE(stChangedEIP150){}
BOOST_AUTO_TEST_CASE(stEIP150singleCodeGasPrices){}
BOOST_AUTO_TEST_CASE(stMemExpandingEIP150Calls){}
BOOST_AUTO_TEST_CASE(stEIP150Specific){}

//EIP158 Tests
BOOST_AUTO_TEST_CASE(stEIP158Specific){}
BOOST_AUTO_TEST_CASE(stNonZeroCallsTest){}
BOOST_AUTO_TEST_CASE(stZeroCallsTest){}
BOOST_AUTO_TEST_CASE(stZeroCallsRevert){}
BOOST_AUTO_TEST_CASE(stCodeSizeLimit){}
BOOST_AUTO_TEST_CASE(stCreateTest){}
BOOST_AUTO_TEST_CASE(stRevertTest){}

//Metropolis Tests
BOOST_AUTO_TEST_CASE(stStackTests){}
BOOST_AUTO_TEST_CASE(stStaticCall){}
BOOST_AUTO_TEST_CASE(stReturnDataTest){}
BOOST_AUTO_TEST_CASE(stZeroKnowledge){}

//Stress Tests
BOOST_AUTO_TEST_CASE(stAttackTest){}
BOOST_AUTO_TEST_CASE(stMemoryStressTest){}
BOOST_AUTO_TEST_CASE(stQuadraticComplexityTest){}

BOOST_AUTO_TEST_SUITE_END()
