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
#include <test/TestHelper.h>
#include <test/BlockChainHelper.h>
#include <test/JsonSpiritHeaders.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

BOOST_AUTO_TEST_SUITE(BlockSuite)

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

BOOST_AUTO_TEST_CASE(bStates)
{
	TestBlockChain testBlockchain(TestBlockChain::getDefaultGenesisBlock());
	TestBlock const& genesisBlock = testBlockchain.getTestGenesis();
	OverlayDB const& genesisDB = genesisBlock.getState().db();
	FullBlockChain<Ethash> const& blockchain = testBlockchain.getInterface();

	State stateBofore = testBlockchain.getTopBlock().getState();

	TestBlock testBlock;
	TestTransaction transaction1 = TestTransaction::getDefaultTransaction();
	testBlock.addTransaction(transaction1);
	TestTransaction transaction2 = TestTransaction::getDefaultTransaction("1");
	testBlock.addTransaction(transaction2);

	testBlock.mine(testBlockchain);
	testBlockchain.addBlock(testBlock);

	//Block2 is synced to latest blockchain block
	Block block1 = blockchain.genesisBlock(genesisDB);
	block1.populateFromChain(blockchain, testBlock.getBlockHeader().hash());

	Block block2 = blockchain.genesisBlock(genesisDB);
	block2.populateFromChain(blockchain, testBlock.getBlockHeader().hash());
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
		Block block;
		//Invalid state root exception if block not initialized by genesis root
		block.populateFromChain(blockchain, testBlock.getBlockHeader().hash());
	}
	catch (std::exception const& _e)
	{
		BOOST_REQUIRE(string(_e.what()).find("InvalidStateRoot") != string::npos);
	}
}

BOOST_AUTO_TEST_CASE(bGasPricer)
{
	TestBlockChain testBlockchain(TestBlockChain::getDefaultGenesisBlock("63000"));
	TestBlock const& genesisBlock = testBlockchain.getTestGenesis();
	OverlayDB const& genesisDB = genesisBlock.getState().db();
	FullBlockChain<Ethash> const& blockchain = testBlockchain.getInterface();

	TestBlock testBlock;
	TestTransaction transaction1 = TestTransaction::getDefaultTransaction("1", "21000");
	testBlock.addTransaction(transaction1);
	TestTransaction transaction2 = TestTransaction::getDefaultTransaction("2", "21000");
	testBlock.addTransaction(transaction2);

	{
		//Normal transaction input
		ZeroGasPricer gp;
		Block block = blockchain.genesisBlock(genesisDB);
		block.sync(blockchain);
		TestBlock testBlockT = testBlock;
		block.sync(blockchain, testBlockT.getTransactionQueue(), gp);
		BOOST_REQUIRE(testBlockT.getTransactionQueue().topTransactions(4).size() == 2);
	}

	{
		//Block not synced to blockchain - BlockGasLimitReached
		TestBlock testBlockT = testBlock;
		ZeroGasPricer gp;
		Block block = blockchain.genesisBlock(genesisDB);
		block.sync(blockchain, testBlockT.getTransactionQueue(), gp);
		BOOST_REQUIRE(testBlockT.getTransactionQueue().topTransactions(4).size() == 0);
	}

	{
		//Transactions valid but exceed block gasLimit - BlockGasLimitReached
		TestBlock testBlockT = testBlock;
		TestTransaction transaction = TestTransaction::getDefaultTransaction("3", "1500000");
		testBlockT.addTransaction(transaction);

		ZeroGasPricer gp;
		Block block = blockchain.genesisBlock(genesisDB);
		block.sync(blockchain);
		block.sync(blockchain, testBlockT.getTransactionQueue(), gp);
		BOOST_REQUIRE(testBlockT.getTransactionQueue().topTransactions(4).size() == 2);
	}

	{
		//Temporary no gas left in the block
		TestBlock testBlockT = testBlock;
		TestTransaction transaction = TestTransaction::getDefaultTransaction("3", "25000", "238479601324597364057623047523945623847562387450234857263485723459273645345234689563486749");
		testBlockT.addTransaction(transaction);

		ZeroGasPricer gp;
		Block block = blockchain.genesisBlock(genesisDB);
		block.sync(blockchain);
		block.sync(blockchain, testBlockT.getTransactionQueue(), gp);
		BOOST_REQUIRE(testBlockT.getTransactionQueue().topTransactions(4).size() == 3);
	}

	{
		//Invalid nonce - nonces ahead
		TestBlock testBlockT = testBlock;
		TestTransaction transaction = TestTransaction::getDefaultTransaction("12", "21000");
		testBlockT.addTransaction(transaction);

		ZeroGasPricer gp;
		Block block = blockchain.genesisBlock(genesisDB);
		block.sync(blockchain);
		block.sync(blockchain, testBlockT.getTransactionQueue(), gp);
		BOOST_REQUIRE(testBlockT.getTransactionQueue().topTransactions(4).size() == 2);
	}

	{
		//Invalid nonce - nonce too low
		TestBlock testBlockT = testBlock;
		TestTransaction transaction = TestTransaction::getDefaultTransaction("0", "21000");
		testBlockT.addTransaction(transaction);

		ZeroGasPricer gp;
		Block block = blockchain.genesisBlock(genesisDB);
		block.sync(blockchain);
		block.sync(blockchain, testBlockT.getTransactionQueue(), gp);
		BOOST_REQUIRE(testBlockT.getTransactionQueue().topTransactions(4).size() == 2);
	}
}

//move expect exception under checkstate option
//try to get into transaction exception in sync

BOOST_AUTO_TEST_CASE(bCopyOperator)
{
	TestBlockChain testBlockchain(TestBlockChain::getDefaultGenesisBlock());
	TestBlock const& genesisBlock = testBlockchain.getTestGenesis();

	OverlayDB const& genesisDB = genesisBlock.getState().db();
	FullBlockChain<Ethash> const& blockchain = testBlockchain.getInterface();
	Block block = blockchain.genesisBlock(genesisDB);
	block.setBeneficiary(genesisBlock.getBeneficiary());

	block = block;
	Block block2 = block;
	BOOST_REQUIRE(ImportTest::compareStates(block.state(), block2.state()) == 0);
	BOOST_REQUIRE(block2.pending() == block.pending());
	BOOST_REQUIRE(block2.beneficiary() == block.beneficiary());
	BOOST_REQUIRE(block2.info() == block.info());

	TestBlock testBlock;
	TestTransaction transaction1 = TestTransaction::getDefaultTransaction();
	testBlock.addTransaction(transaction1);
	testBlock.mine(testBlockchain);
	testBlockchain.addBlock(testBlock);

	Block block3 = blockchain.genesisBlock(genesisDB);
	block3.populateFromChain(blockchain, testBlock.getBlockHeader().hash());
	BOOST_REQUIRE(block3.info() == (BlockInfo)testBlock.getBlockHeader());

	//Genesis is populating wrong???
	//Block block31 = blockchain.genesisBlock(genesisDB);
	//block31.populateFromChain(blockchain, genesisBlock.getBlockHeader().hash());
	//BOOST_REQUIRE(block31.info() == (BlockInfo)genesisBlock.getBlockHeader());

	try
	{
		Block block32 = blockchain.genesisBlock(genesisDB);
		block32.populateFromChain(blockchain, h256("0x0000000000000000000000000000000000000000000000000000000000000001"));
		BOOST_ERROR("Expected BlockNotFound exception!");
	}
	catch (std::exception const& _e)
	{
		BOOST_REQUIRE(string(_e.what()).find("BlockNotFound") != string::npos);
	}
}

BOOST_AUTO_TEST_SUITE_END()
