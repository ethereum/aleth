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

	Block block = blockchain.genesisBlock(genesisDB);
	block.setBeneficiary(genesisBlock.getBeneficiary());
	TestBlock testBlock;
	TestTransaction transaction1 = TestTransaction::getDefaultTransaction();
	testBlock.addTransaction(transaction1);
	testBlock.mine(testBlockchain);
	testBlockchain.addBlock(testBlock);
	block.sync(blockchain);

	//Block2 is synced to latest blockchain block
	Block block2 = blockchain.genesisBlock(genesisDB);
	block2.populateFromChain(blockchain, testBlock.getBlockHeader().hash());
	State stateAfterInsert = block2.fromPending(0); //get the state of blockchain on previous block
	BOOST_REQUIRE(ImportTest::compareStates(stateBofore, stateAfterInsert) == 0);

	State stateAfterInsert2 = block2.fromPending(2); //get the state of blockchain on current block executed
	BOOST_REQUIRE(ImportTest::compareStates(stateBofore, stateAfterInsert2, eth::AccountMaskMap(), WhenError::DontThrow) == 1);

	//test the state on transaction in the middle

}

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

	//Block block31 = blockchain.genesisBlock(genesisDB);
	//block31.populateFromChain(blockchain, genesisBlock.getBlockHeader().hash());
	//BOOST_REQUIRE(block31.info() == (BlockInfo)genesisBlock.getBlockHeader());
}

BOOST_AUTO_TEST_SUITE_END()
