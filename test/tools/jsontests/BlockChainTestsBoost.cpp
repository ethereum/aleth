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
					clog << "\\/ " << file.filename().string() << std::endl;
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
		fillAllFilesInFolder(casename);
	}

	void fillAllFilesInFolder(string const& _folder)
	{
		std::string fillersPath = test::getTestPath() + "/src/BlockchainTestsFiller/" + _folder;

		string filter = test::Options::get().singleTestName.empty() ? string() : test::Options::get().singleTestName + "Filler";
		std::vector<boost::filesystem::path> files = test::getJsonFiles(fillersPath, filter);
		int testcount = files.size() * test::getNetworks().size(); //1 test case translated to each fork.

		//include fillers into test count
		if (test::Options::get().filltests)
			testcount += testcount / test::getNetworks().size();

		test::TestOutputHelper::initTest(testcount);
		for (auto const& file: files)
		{
			test::TestOutputHelper::setCurrentTestFileName(file.filename().string());
			test::executeTests(file.filename().string(), "/BlockchainTests/"+_folder, "/BlockchainTestsFiller/"+_folder, dev::test::doBlockchainTestNoLog);
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
