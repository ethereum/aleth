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
/** @file block.cpp
 * @author Dimitry Khokhlov <dimitry@ethdev.com>
 * @date 2015
 * Block test functions.
 */

#include <libethereum/BlockQueue.h>
#include <libethereum/Block.h>
#include <test/libtesteth/TestHelper.h>
#include <test/libtesteth/BlockChainHelper.h>
#include <test/libtesteth/JsonSpiritHeaders.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

BOOST_FIXTURE_TEST_SUITE(BlockSuite, TestOutputHelper)

#if !defined(_WIN32)
BOOST_AUTO_TEST_CASE(bStructures)
{
	BlockChat chat;
	BlockTrace trace;
	BlockDetail details;
	BlockSafeExceptions exeptions;

	BOOST_REQUIRE(string(chat.name()).find("◌") != string::npos);
	BOOST_REQUIRE(string(trace.name()).find("◎") != string::npos);
	BOOST_REQUIRE(string(details.name()).find("◌") != string::npos);
	BOOST_REQUIRE(string(exeptions.name()).find("ℹ") != string::npos);
}
#endif

BOOST_AUTO_TEST_CASE(bStates)
{
	if (!dev::test::Options::get().quadratic)
		return;
	try
	{
		TestBlockChain testBlockchain(TestBlockChain::defaultGenesisBlock());
		TestBlock const& genesisBlock = testBlockchain.testGenesis();
		OverlayDB const& genesisDB = genesisBlock.state().db();
		BlockChain const& blockchain = testBlockchain.interface();

		State stateBofore = testBlockchain.topBlock().state();

		TestBlock testBlock;
		TestTransaction transaction1 = TestTransaction::defaultTransaction(1);
		testBlock.addTransaction(transaction1);
		TestTransaction transaction2 = TestTransaction::defaultTransaction(2);
		testBlock.addTransaction(transaction2);

		testBlock.mine(testBlockchain);
		testBlockchain.addBlock(testBlock);

		//Block2 is synced to latest blockchain block
		Block block1 = blockchain.genesisBlock(genesisDB);
		block1.populateFromChain(blockchain, testBlock.blockHeader().hash());

		Block block2 = blockchain.genesisBlock(genesisDB);
		block2.populateFromChain(blockchain, testBlock.blockHeader().hash());
		State stateAfterInsert = block2.fromPending(0); //get the state of blockchain on previous block
		BOOST_REQUIRE(ImportTest::compareStates(stateBofore, stateAfterInsert) == 0);

		State stateAfterInsert1 = block2.fromPending(1); //get the state of blockchain on current block executed
		BOOST_REQUIRE(ImportTest::compareStates(stateAfterInsert, stateAfterInsert1, eth::AccountMaskMap(), WhenError::DontThrow) == 1);

		State stateAfterInsert2 = block2.fromPending(2); //get the state of blockchain on current block executed
		BOOST_REQUIRE(ImportTest::compareStates(stateBofore, stateAfterInsert2, eth::AccountMaskMap(), WhenError::DontThrow) == 1);
		BOOST_REQUIRE(ImportTest::compareStates(stateAfterInsert1, stateAfterInsert2, eth::AccountMaskMap(), WhenError::DontThrow) == 1);

		//Block2 will start a new block on top of blockchain
		BOOST_REQUIRE(block1.info() == block2.info());
		block2.sync(blockchain);
		BOOST_REQUIRE(block1.info() != block2.info());

		try
		{
			Block block(Block::Null);
			//Invalid state root exception if block not initialized by genesis root
			block.populateFromChain(blockchain, testBlock.blockHeader().hash());
		}
		catch (std::exception const& _e)
		{
			BOOST_REQUIRE(string(_e.what()).find("InvalidStateRoot") != string::npos);
		}
	}
	catch (Exception const& _e)
	{
		BOOST_ERROR("Failed test with Exception: " << diagnostic_information(_e));
	}
	catch (std::exception const& _e)
	{
		BOOST_ERROR("Failed test with Exception: " << _e.what());
	}
	catch(...)
	{
		BOOST_ERROR("Exception thrown when trying to mine or import a block!");
	}
}

BOOST_AUTO_TEST_CASE(bGasPricer)
{
	TestBlockChain testBlockchain(TestBlockChain::defaultGenesisBlock(63000));
	TestBlock const& genesisBlock = testBlockchain.testGenesis();
	OverlayDB const& genesisDB = genesisBlock.state().db();
	BlockChain const& blockchain = testBlockchain.interface();

	TestBlock testBlock;
	TestTransaction transaction1 = TestTransaction::defaultTransaction(1, 1, 21000);
	testBlock.addTransaction(transaction1);
	TestTransaction transaction2 = TestTransaction::defaultTransaction(2, 1, 21000);
	testBlock.addTransaction(transaction2);

	{
		//Normal transaction input
		ZeroGasPricer gp;
		Block block = blockchain.genesisBlock(genesisDB);
		block.sync(blockchain);
		TestBlock testBlockT = testBlock;
		block.sync(blockchain, testBlockT.transactionQueue(), gp);
		BOOST_REQUIRE(testBlockT.transactionQueue().topTransactions(4).size() == 2);
	}

	{
		//Block not synced to blockchain
		TestBlock testBlockT = testBlock;
		ZeroGasPricer gp;
		Block block = blockchain.genesisBlock(genesisDB);
		block.sync(blockchain, testBlockT.transactionQueue(), gp);
		BOOST_REQUIRE(testBlockT.transactionQueue().topTransactions(4).size() == 2);
	}

	{
		//Transactions valid but exceed block gasLimit - BlockGasLimitReached
		TestBlock testBlockT = testBlock;
		TestTransaction transaction = TestTransaction::defaultTransaction(3, 1, 1500000);
		testBlockT.addTransaction(transaction);

		ZeroGasPricer gp;
		Block block = blockchain.genesisBlock(genesisDB);
		block.sync(blockchain);
		block.sync(blockchain, testBlockT.transactionQueue(), gp);
		BOOST_REQUIRE(testBlockT.transactionQueue().topTransactions(4).size() == 2);
	}

	{
		//Temporary no gas left in the block
		TestBlock testBlockT = testBlock;
		TestTransaction transaction = TestTransaction::defaultTransaction(3, 1, 25000, importByteArray("238479601324597364057623047523945623847562387450234857263485723459273645345234689563486749"));
		testBlockT.addTransaction(transaction);

		ZeroGasPricer gp;
		Block block = blockchain.genesisBlock(genesisDB);
		block.sync(blockchain);
		block.sync(blockchain, testBlockT.transactionQueue(), gp);
		BOOST_REQUIRE(testBlockT.transactionQueue().topTransactions(4).size() == 3);
	}

	{
		//Invalid nonce - nonces ahead
		TestBlock testBlockT = testBlock;
		TestTransaction transaction = TestTransaction::defaultTransaction(12, 1, 21000);
		testBlockT.addTransaction(transaction);

		ZeroGasPricer gp;
		Block block = blockchain.genesisBlock(genesisDB);
		block.sync(blockchain);
		block.sync(blockchain, testBlockT.transactionQueue(), gp);
		BOOST_REQUIRE(testBlockT.transactionQueue().topTransactions(4).size() == 2);
	}

	{
		//Invalid nonce - nonce too low
		TestBlock testBlockT = testBlock;
		TestTransaction transaction = TestTransaction::defaultTransaction(0, 1, 21000);
		testBlockT.addTransaction(transaction);

		ZeroGasPricer gp;
		Block block = blockchain.genesisBlock(genesisDB);
		block.sync(blockchain);
		block.sync(blockchain, testBlockT.transactionQueue(), gp);
		BOOST_REQUIRE(testBlockT.transactionQueue().topTransactions(4).size() == 2);
	}
}

BOOST_AUTO_TEST_CASE(bCopyOperator)
{
	try
	{
		TestBlockChain testBlockchain(TestBlockChain::defaultGenesisBlock());
		TestBlock const& genesisBlock = testBlockchain.testGenesis();

		OverlayDB const& genesisDB = genesisBlock.state().db();
		BlockChain const& blockchain = testBlockchain.interface();
		Block block = blockchain.genesisBlock(genesisDB);
		block.setAuthor(genesisBlock.beneficiary());

		block = block;
		Block block2 = block;
		BOOST_REQUIRE(ImportTest::compareStates(block.state(), block2.state()) == 0);
		BOOST_REQUIRE(block2.pending() == block.pending());
		BOOST_REQUIRE(block2.author() == block.author());
		BOOST_REQUIRE(block2.info() == block.info());

		TestBlock testBlock;
		TestTransaction transaction1 = TestTransaction::defaultTransaction(1);
		testBlock.addTransaction(transaction1);
		testBlock.mine(testBlockchain);
		testBlockchain.addBlock(testBlock);

		Block block3 = blockchain.genesisBlock(genesisDB);
		block3.populateFromChain(blockchain, testBlock.blockHeader().hash());
		BOOST_REQUIRE(block3.info() == testBlock.blockHeader());

		//Genesis is populating wrong???
		//Block block31 = blockchain.genesisBlock(genesisDB);
		//block31.populateFromChain(blockchain, genesisBlock.getBlockHeader().hash());
		//BOOST_REQUIRE(block31.info() == (BlockInfo)genesisBlock.getBlockHeader());

		Block block32 = blockchain.genesisBlock(genesisDB);
		auto is_critical = []( std::exception const& _e) { return string(_e.what()).find("BlockNotFound") != string::npos; };
		BOOST_CHECK_EXCEPTION(block32.populateFromChain(blockchain, h256("0x0000000000000000000000000000000000000000000000000000000000000001")), BlockNotFound, is_critical);
	}
	catch (Exception const& _e)
	{
		BOOST_ERROR("Failed test with Exception: " << diagnostic_information(_e));
	}
	catch (std::exception const& _e)
	{
		BOOST_ERROR("Failed test with Exception: " << _e.what());
	}
	catch(...)
	{
		BOOST_ERROR("Exception thrown when trying to mine or import a block!");
	}
}

BOOST_AUTO_TEST_SUITE_END()
