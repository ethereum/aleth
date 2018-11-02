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
/** @file gasPricer.cpp
 * @author Christoph Jentzsch <cj@ethdev.com>
 * @date 2015
 * Gas pricer tests
 */

#include <libethashseal/Ethash.h>
#include <libethereum/BlockChain.h>
#include <libethereum/ChainParams.h>
#include <libethereum/GasPricer.h>
#include <libethereum/BasicGasPricer.h>
#include <test/tools/libtesteth/TestHelper.h>
#include <test/tools/libtestutils/BlockChainLoader.h>
#include <boost/filesystem/path.hpp>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;
namespace fs = boost::filesystem;

namespace dev {  namespace test {

void executeGasPricerTest(string const& name, double _etherPrice, double _blockFee, fs::path const& _bcTestPath, TransactionPriority _txPrio, u256 _expectedAsk, u256 _expectedBid, eth::Network _sealEngineNetwork = eth::Network::TransitionnetTest)
{
	BasicGasPricer gp(u256(double(ether / 1000) / _etherPrice), u256(_blockFee * 1000));

	Json::Value vJson = test::loadJsonFromFile(test::getTestPath() / _bcTestPath);
	test::BlockChainLoader bcLoader(vJson[name], _sealEngineNetwork);
	BlockChain const& bc = bcLoader.bc();

	gp.update(bc);
	BOOST_CHECK_MESSAGE(abs(gp.ask(Block(Block::Null)) - _expectedAsk ) < 100000000, "ASK Got: " + toString(gp.ask(Block(Block::Null))) + " Expected: " + toString(_expectedAsk));
	BOOST_CHECK_MESSAGE(abs(gp.bid(_txPrio) - _expectedBid ) < 100000000, "BID Got: " + toString(gp.bid(_txPrio)) + " Expected: " + toString(_expectedBid));
}
} }

BOOST_FIXTURE_TEST_SUITE(GasPricer, TestOutputHelperFixture)

BOOST_AUTO_TEST_CASE(trivialGasPricer)
{
	std::shared_ptr<dev::eth::GasPricer> gp(new TrivialGasPricer);
	BOOST_CHECK_EQUAL(gp->ask(Block(Block::Null)), DefaultGasPrice);
	BOOST_CHECK_EQUAL(gp->bid(), DefaultGasPrice);

	gp->update(BlockChain(eth::ChainParams(), "", WithExisting::Kill));
	BOOST_CHECK_EQUAL(gp->ask(Block(Block::Null)), DefaultGasPrice);
	BOOST_CHECK_EQUAL(gp->bid(), DefaultGasPrice);
}

BOOST_AUTO_TEST_CASE(basicGasPricerNoUpdate)
{
	BasicGasPricer gp(u256(double(ether / 1000) / 30.679), u256(15.0 * 1000));
	BOOST_CHECK_EQUAL(gp.ask(Block(Block::Null)), 103754996057);
	BOOST_CHECK_EQUAL(gp.bid(), 103754996057);

	gp.setRefPrice(u256(0));
	BOOST_CHECK_EQUAL(gp.ask(Block(Block::Null)), 0);
	BOOST_CHECK_EQUAL(gp.bid(), 0);

	gp.setRefPrice(u256(1));
	gp.setRefBlockFees(u256(0));
	BOOST_CHECK_EQUAL(gp.ask(Block(Block::Null)), 0);
	BOOST_CHECK_EQUAL(gp.bid(), 0);

	gp.setRefPrice(u256("0x100000000000000000000000000000000"));
	BOOST_CHECK_THROW(gp.setRefBlockFees(u256("0x100000000000000000000000000000000")), Overflow);
	BOOST_CHECK_EQUAL(gp.ask(Block(Block::Null)), 0);
	BOOST_CHECK_EQUAL(gp.bid(), 0);

	gp.setRefPrice(1);
	gp.setRefBlockFees(u256("0x100000000000000000000000000000000"));
	BOOST_CHECK_THROW(gp.setRefPrice(u256("0x100000000000000000000000000000000")), Overflow);
	BOOST_CHECK_EQUAL(gp.ask(Block(Block::Null)), u256("72210176012870430758964373780717"));
	BOOST_CHECK_EQUAL(gp.bid(), u256("72210176012870430758964373780717"));
}

BOOST_AUTO_TEST_CASE(basicGasPricer_RPC_API_Test_Frontier)
{
	u256 _expectedAsk = 16056883295;
	u256 _expectedBid = 1;
	dev::test::executeGasPricerTest("RPC_API_Test_Frontier", 30.679, 15.0, "/BlockchainTests/bcGasPricerTest/RPC_API_Test.json", TransactionPriority::Medium, _expectedAsk, _expectedBid, eth::Network::FrontierTest);
}

BOOST_AUTO_TEST_CASE(basicGasPricer_RPC_API_Test_Homestead)
{
	u256 _expectedAsk = 16056864311;
	u256 _expectedBid = 1;
	dev::test::executeGasPricerTest("RPC_API_Test_Homestead", 30.679, 15.0, "/BlockchainTests/bcGasPricerTest/RPC_API_Test.json", TransactionPriority::Medium, _expectedAsk, _expectedBid, eth::Network::HomesteadTest);
}

BOOST_AUTO_TEST_CASE(basicGasPricer_bcValidBlockTest)
{
	dev::test::executeGasPricerTest("SimpleTx_Frontier", 30.679, 15.0, "/BlockchainTests/bcValidBlockTest/SimpleTx.json", TransactionPriority::Medium, 155632494086, 10);
}

BOOST_AUTO_TEST_CASE(basicGasPricer_bcUncleTest_Frontier)
{
	u256 _expectedAsk = 155632494086;
	u256 _expectedBid = 1;
	dev::test::executeGasPricerTest("twoUncle_Frontier", 30.679, 15.0, "/BlockchainTests/bcUncleTest/twoUncle.json", TransactionPriority::Medium, _expectedAsk, _expectedBid, eth::Network::FrontierTest);
}

BOOST_AUTO_TEST_CASE(basicGasPricer_bcUncleTest_Homestead)
{
    u256 _expectedAsk = 155632494086;
    u256 _expectedBid = 1;
	dev::test::executeGasPricerTest("twoUncle_Homestead", 30.679, 15.0, "/BlockchainTests/bcUncleTest/twoUncle.json", TransactionPriority::Medium, _expectedAsk, _expectedBid, eth::Network::HomesteadTest);
}

BOOST_AUTO_TEST_CASE(basicGasPricer_bcUncleHeaderValidity_Frontier)
{
	u256 _expectedAsk = 155632494086;
	u256 _expectedBid = 1;
	dev::test::executeGasPricerTest("correct_Frontier", 30.679, 15.0, "/BlockchainTests/bcUncleHeaderValidity/correct.json", TransactionPriority::Medium, _expectedAsk, _expectedBid, eth::Network::FrontierTest);
}

BOOST_AUTO_TEST_CASE(basicGasPricer_bcUncleHeaderValidity_Homestead)
{
	u256 _expectedAsk = 155633980282;
	u256 _expectedBid = 1;
	dev::test::executeGasPricerTest("correct_Homestead", 30.679, 15.0, "/BlockchainTests/bcUncleHeaderValidity/correct.json", TransactionPriority::Medium, _expectedAsk, _expectedBid, eth::Network::HomesteadTest);
}

BOOST_AUTO_TEST_CASE(basicGasPricer_notxs_frontier)
{
	u256 _expectedAsk = 155632494086;
	u256 _expectedBid = 155632494086;
	dev::test::executeGasPricerTest("notxs_Frontier", 30.679, 15.0, "/BlockchainTests/bcGasPricerTest/notxs.json", TransactionPriority::Medium, _expectedAsk, _expectedBid, eth::Network::FrontierTest);
}

BOOST_AUTO_TEST_CASE(basicGasPricer_notxs_homestead)
{
    u256 _expectedAsk = 155632494086;
    u256 _expectedBid = 155632494086;
    dev::test::executeGasPricerTest("notxs_Homestead", 30.679, 15.0, "/BlockchainTests/bcGasPricerTest/notxs.json", TransactionPriority::Medium, _expectedAsk, _expectedBid, eth::Network::HomesteadTest);
}

BOOST_AUTO_TEST_CASE(basicGasPricer_highGasUsage_LowestPrio)
{
    u256 _expectedAsk = 15731408053;
    u256 _expectedBid = 10000000000000;
	dev::test::executeGasPricerTest("highGasUsage_Frontier", 30.679, 15.0, "/BlockchainTests/bcGasPricerTest/highGasUsage.json", TransactionPriority::Lowest, _expectedAsk, _expectedBid, eth::Network::FrontierTest);
}

BOOST_AUTO_TEST_CASE(basicGasPricer_highGasUsage_LowPrio)
{
    u256 _expectedAsk = 15731408053;
    u256 _expectedBid = 15734152261884;
	dev::test::executeGasPricerTest("highGasUsage_Frontier", 30.679, 15.0, "/BlockchainTests/bcGasPricerTest/highGasUsage.json", TransactionPriority::Low, _expectedAsk, _expectedBid, eth::Network::FrontierTest);
}

BOOST_AUTO_TEST_CASE(basicGasPricer_highGasUsage_MediumPrio)
{
    u256 _expectedAsk = 15731408053;
    u256 _expectedBid = 20000000000000;
	dev::test::executeGasPricerTest("highGasUsage_Frontier", 30.679, 15.0, "/BlockchainTests/bcGasPricerTest/highGasUsage.json", TransactionPriority::Medium, _expectedAsk, _expectedBid, eth::Network::FrontierTest);
}

BOOST_AUTO_TEST_CASE(basicGasPricer_highGasUsage_HighPrio)
{
    u256 _expectedAsk = 15731408053;
    u256 _expectedBid = 24265847738115;
	dev::test::executeGasPricerTest("highGasUsage_Frontier", 30.679, 15.0, "/BlockchainTests/bcGasPricerTest/highGasUsage.json", TransactionPriority::High, _expectedAsk, _expectedBid, eth::Network::FrontierTest);
}

BOOST_AUTO_TEST_CASE(basicGasPricer_highGasUsage_HighestPrio)
{
    u256 _expectedAsk = 15731408053;
    u256 _expectedBid = 30000000000000;
	dev::test::executeGasPricerTest("highGasUsage_Frontier", 30.679, 15.0, "/BlockchainTests/bcGasPricerTest/highGasUsage.json", TransactionPriority::Highest, _expectedAsk, _expectedBid, eth::Network::FrontierTest);
}
BOOST_AUTO_TEST_SUITE_END()
