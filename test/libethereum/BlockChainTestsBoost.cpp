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

#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <test/TestHelper.h>
#include <test/BlockChainHelper.h>

class frontierFixture { public:	frontierFixture() { dev::test::TestBlockChain::s_sealEngineNetwork = eth::Network::FrontierTest; } };
class homesteadFixture { public:	homesteadFixture() { dev::test::TestBlockChain::s_sealEngineNetwork = eth::Network::HomesteadTest; } };
class transitionFixture { public: 	transitionFixture() { dev::test::TestBlockChain::s_sealEngineNetwork = eth::Network::Test; } };
class eip150Fixture { public:	eip150Fixture() { dev::test::TestBlockChain::s_sealEngineNetwork = eth::Network::EIP150Test; } };


//BlockChainTestsTransition
BOOST_FIXTURE_TEST_SUITE(BlockChainTestsTransition, transitionFixture)
BOOST_AUTO_TEST_CASE(bcSimpleTransition) { dev::test::executeTests("bcSimpleTransitionTest", "/BlockchainTests/TestNetwork",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/TestNetwork", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcTheDaoTest) { dev::test::executeTests("bcTheDaoTest", "/BlockchainTests/TestNetwork",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/TestNetwork", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcEIP150Test) { dev::test::executeTests("bcEIP150Test", "/BlockchainTests/TestNetwork",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/TestNetwork", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_SUITE_END()


//BlockChainTestsEIP150
BOOST_FIXTURE_TEST_SUITE(BlockChainTestsEIP150, eip150Fixture)
BOOST_AUTO_TEST_CASE(bcForkStressTestEIP150) {	dev::test::executeTests("bcForkStressTest", "/BlockchainTests/EIP150",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/EIP150", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcSuicideIssueEIP150)
 {
	if (dev::test::Options::get().memory)
		dev::test::executeTests("bcSuicideIssue", "/BlockchainTests/EIP150",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/EIP150", dev::test::doBlockchainTests);
 }
BOOST_AUTO_TEST_CASE(bcTotalDifficultyTestEIP150) {	dev::test::executeTests("bcTotalDifficultyTest", "/BlockchainTests/EIP150",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/EIP150", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcMultiChainTestEIP150) {	dev::test::executeTests("bcMultiChainTest", "/BlockchainTests/EIP150",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/EIP150", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcRPC_API_TestEIP150) {	dev::test::executeTests("bcRPC_API_Test", "/BlockchainTests/EIP150",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/EIP150", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcValidBlockTestEIP150) {	dev::test::executeTests("bcValidBlockTest", "/BlockchainTests/EIP150",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/EIP150", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcInvalidHeaderTestEIP150) {	dev::test::executeTests("bcInvalidHeaderTest", "/BlockchainTests/EIP150",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/EIP150", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcUncleHeaderValiditiyEIP150) {	dev::test::executeTests("bcUncleHeaderValiditiy", "/BlockchainTests/EIP150",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/EIP150", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcGasPricerTestEIP150) {	dev::test::executeTests("bcGasPricerTest", "/BlockchainTests/EIP150",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/EIP150", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcUncleTestEIP150) {	dev::test::executeTests("bcUncleTest", "/BlockchainTests/EIP150",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/EIP150", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcBlockGasLimitTestEIP150) {	dev::test::executeTests("bcBlockGasLimitTest", "/BlockchainTests/EIP150",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/EIP150", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcStateTestEIP150) {	dev::test::executeTests("bcStateTest", "/BlockchainTests/EIP150",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/EIP150", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcWalletTestEIP150)
{
	if (test::Options::get().wallet)
		dev::test::executeTests("bcWalletTest", "/BlockchainTests/EIP150",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/EIP150", dev::test::doBlockchainTests);
}
BOOST_AUTO_TEST_SUITE_END()


//BlockChainTestsHomestead
BOOST_FIXTURE_TEST_SUITE(BlockChainTestsHomestead, homesteadFixture)
BOOST_AUTO_TEST_CASE(bcExploitTestHomestead) {	dev::test::executeTests("bcExploitTest", "/BlockchainTests/Homestead",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/Homestead", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcSuicideIssueHomestead)
{
	if (dev::test::Options::get().memory)
		dev::test::executeTests("bcSuicideIssue", "/BlockchainTests/Homestead",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/Homestead", dev::test::doBlockchainTests);
}
BOOST_AUTO_TEST_CASE(bcShanghaiLoveHomestead) {	dev::test::executeTests("bcShanghaiLove", "/BlockchainTests/Homestead",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/Homestead", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcForkStressTestHomestead) {	dev::test::executeTests("bcForkStressTest", "/BlockchainTests/Homestead",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/Homestead", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcTotalDifficultyTestHomestead) {	dev::test::executeTests("bcTotalDifficultyTest", "/BlockchainTests/Homestead",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/Homestead", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcMultiChainTestHomestead) {	dev::test::executeTests("bcMultiChainTest", "/BlockchainTests/Homestead",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/Homestead", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcInvalidRLPTestHomestead)
{
	std::string fillersPath = dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/Homestead";
	if (!dev::test::Options::get().fillTests)
		dev::test::executeTests("bcInvalidRLPTest", "/BlockchainTests/Homestead",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/Homestead", dev::test::doBlockchainTests);
	else
	{
		dev::test::TestOutputHelper::initTest();
		dev::test::copyFile(fillersPath + "/bcInvalidRLPTest.json", dev::test::getTestPath() + "/BlockchainTests/Homestead/bcInvalidRLPTest.json");
	}
}
BOOST_AUTO_TEST_CASE(bcRPC_API_TestHomestead) {	dev::test::executeTests("bcRPC_API_Test", "/BlockchainTests/Homestead",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/Homestead", dev::test::doBlockchainTests);}
BOOST_AUTO_TEST_CASE(bcValidBlockTestHomestead) {	dev::test::executeTests("bcValidBlockTest", "/BlockchainTests/Homestead",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/Homestead", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcInvalidHeaderTestHomestead) {	dev::test::executeTests("bcInvalidHeaderTest", "/BlockchainTests/Homestead",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/Homestead", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcUncleHeaderValiditiyHomestead) {	dev::test::executeTests("bcUncleHeaderValiditiy", "/BlockchainTests/Homestead",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/Homestead", dev::test::doBlockchainTests);}
BOOST_AUTO_TEST_CASE(bcGasPricerTestHomestead) {	dev::test::executeTests("bcGasPricerTest", "/BlockchainTests/Homestead",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/Homestead", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcUncleTestHomestead) {	dev::test::executeTests("bcUncleTest", "/BlockchainTests/Homestead",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/Homestead", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcBlockGasLimitTestHomestead) {	dev::test::executeTests("bcBlockGasLimitTest", "/BlockchainTests/Homestead",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/Homestead", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcStateTestHomestead) {	dev::test::executeTests("bcStateTest", "/BlockchainTests/Homestead",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/Homestead", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcWalletTestHomestead)
{
	if (test::Options::get().wallet)
		dev::test::executeTests("bcWalletTest", "/BlockchainTests/Homestead",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller/Homestead", dev::test::doBlockchainTests);
}
BOOST_AUTO_TEST_SUITE_END()


//BlockChainTests
BOOST_FIXTURE_TEST_SUITE(BlockChainTests, frontierFixture)
BOOST_AUTO_TEST_CASE(bcForkBlockTest)
{
	std::string fillersPath = dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller";
	if (!dev::test::Options::get().fillTests)
		dev::test::executeTests("bcForkBlockTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests);
	else
	{
		dev::test::TestOutputHelper::initTest();
		dev::test::copyFile(fillersPath + "/bcForkBlockTest.json", dev::test::getTestPath() + "/BlockchainTests/bcForkBlockTest.json");
	}
}
BOOST_AUTO_TEST_CASE(bcForkUncleTest)
{
	std::string fillersPath = dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller";
	if (!dev::test::Options::get().fillTests)
		dev::test::executeTests("bcForkUncle", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests);
	else
	{
		dev::test::TestOutputHelper::initTest();
		dev::test::copyFile(fillersPath + "/bcForkUncle.json", dev::test::getTestPath() + "/BlockchainTests/bcForkUncle.json");
	}
}
BOOST_AUTO_TEST_CASE(bcForkStressTest) {	dev::test::executeTests("bcForkStressTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcMultiChainTest) {	dev::test::executeTests("bcMultiChainTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcTotalDifficultyTest) {	dev::test::executeTests("bcTotalDifficultyTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcInvalidRLPTest)
{
	std::string fillersPath = dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller";
	if (!dev::test::Options::get().fillTests)
		dev::test::executeTests("bcInvalidRLPTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests);
	else
	{
		dev::test::TestOutputHelper::initTest();
		dev::test::copyFile(fillersPath + "/bcInvalidRLPTest.json", dev::test::getTestPath() + "/BlockchainTests/bcInvalidRLPTest.json");
	}
}
BOOST_AUTO_TEST_CASE(bcRPC_API_Test) {	dev::test::executeTests("bcRPC_API_Test", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcValidBlockTest) {	dev::test::executeTests("bcValidBlockTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcInvalidHeaderTest) {	dev::test::executeTests("bcInvalidHeaderTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcUncleTest) {	dev::test::executeTests("bcUncleTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcUncleHeaderValiditiy) {	dev::test::executeTests("bcUncleHeaderValiditiy", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcGasPricerTest) {	dev::test::executeTests("bcGasPricerTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcBlockGasLimitTest) {	dev::test::executeTests("bcBlockGasLimitTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(bcWalletTest)
{
	if (test::Options::get().wallet)
		dev::test::executeTests("bcWalletTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests);
}
BOOST_AUTO_TEST_CASE(bcStateTest) {	dev::test::executeTests("bcStateTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests); }
BOOST_AUTO_TEST_CASE(userDefinedFile)
{
	dev::test::TestBlockChain::s_sealEngineNetwork = dev::test::Options::get().sealEngineNetwork;
	dev::test::userDefinedTest(dev::test::doBlockchainTests);
}
BOOST_AUTO_TEST_SUITE_END()
