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
#include <test/tools/libtesteth/TestHelper.h>
#include <test/tools/libtesteth/BlockChainHelper.h>
#include <test/tools/libtesteth/JsonSpiritHeaders.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

BOOST_FIXTURE_TEST_SUITE(BlockSuite, TestOutputHelperFixture)

BOOST_FIXTURE_TEST_SUITE(FrontierBlockSuite, FrontierNoProofTestFixture)

BOOST_AUTO_TEST_CASE(bStates)
{
    TestBlockChain testBlockchain(TestBlockChain::defaultGenesisBlock());
    TestBlock const& genesisBlock = testBlockchain.testGenesis();
    OverlayDB const& genesisDB = genesisBlock.state().db();
    BlockChain const& blockchain = testBlockchain.getInterface();

    h256 stateRootBefore = testBlockchain.topBlock().state().rootHash();
    BOOST_REQUIRE(stateRootBefore != h256());

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
    h256 stateRootAfterInsert = block2.stateRootBeforeTx(0); //get the state of blockchain on previous block
    BOOST_REQUIRE(stateRootAfterInsert != h256());
    BOOST_REQUIRE_EQUAL(stateRootBefore, stateRootAfterInsert);

    h256 stateRootAfterInsert1 = block2.stateRootBeforeTx(1); //get the state of blockchain on current block executed
    BOOST_REQUIRE(stateRootAfterInsert1 != h256());
    BOOST_REQUIRE(stateRootAfterInsert != stateRootAfterInsert1);

    h256 stateRootAfterInsert2 = block2.stateRootBeforeTx(2); //get the state of blockchain on current block executed
    BOOST_REQUIRE(stateRootAfterInsert2 != h256());
    BOOST_REQUIRE(stateRootBefore != stateRootAfterInsert2);
    BOOST_REQUIRE(stateRootAfterInsert1 != stateRootAfterInsert2);

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

BOOST_AUTO_TEST_CASE(bCopyOperator)
{
    TestBlockChain testBlockchain(TestBlockChain::defaultGenesisBlock());
    TestBlock const& genesisBlock = testBlockchain.testGenesis();

    OverlayDB const& genesisDB = genesisBlock.state().db();
    BlockChain const& blockchain = testBlockchain.getInterface();
    Block block = blockchain.genesisBlock(genesisDB);
    block.setAuthor(genesisBlock.beneficiary());

    auto& blockRef = block;  // Hide itself, compilers can complain about direct self-assignments.
    block = blockRef;        // Assign to itself.
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
    auto is_critical = [](std::exception const& _e) { return string(_e.what()).find("BlockNotFound") != string::npos; };
    BOOST_CHECK_EXCEPTION(block32.populateFromChain(blockchain, h256("0x0000000000000000000000000000000000000000000000000000000000000001")), BlockNotFound, is_critical);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(bGasPricer)
{
    TestBlockChain testBlockchain(TestBlockChain::defaultGenesisBlock(63000));
    TestBlock const& genesisBlock = testBlockchain.testGenesis();
    OverlayDB const& genesisDB = genesisBlock.state().db();
    BlockChain const& blockchain = testBlockchain.getInterface();

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

BOOST_AUTO_TEST_CASE(bGetReceiptOverflow)
{
    TestBlockChain bc;
    TestBlock const& genesisBlock = bc.testGenesis();
    OverlayDB const& genesisDB = genesisBlock.state().db();
    BlockChain const& blockchain = bc.getInterface();
    Block block = blockchain.genesisBlock(genesisDB);
    BOOST_CHECK_THROW(block.receipt(123), std::out_of_range);
}


BOOST_FIXTURE_TEST_SUITE(ByzantiumBlockSuite, ByzantiumTestFixture)

BOOST_AUTO_TEST_CASE(bByzantiumBlockReward)
{
    TestBlockChain testBlockchain;
    TestBlock testBlock;
    testBlock.mine(testBlockchain);
    testBlockchain.addBlock(testBlock);

    TestBlock const& topBlock = testBlockchain.topBlock();
    BOOST_REQUIRE_EQUAL(topBlock.state().balance(topBlock.beneficiary()), 3 * ether);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(ConstantinopleBlockSuite, ConstantinopleTestFixture)

BOOST_AUTO_TEST_CASE(bConstantinopleBlockReward)
{
    TestBlockChain testBlockchain;
    TestBlock testBlock;
    testBlock.mine(testBlockchain);
    testBlockchain.addBlock(testBlock);

    TestBlock const& topBlock = testBlockchain.topBlock();
    BOOST_REQUIRE_EQUAL(topBlock.state().balance(topBlock.beneficiary()), 2 * ether);
}

BOOST_AUTO_TEST_SUITE_END()

class ExperimentalTransitionTestFixture : public TestOutputHelperFixture
{
public:
    ExperimentalTransitionTestFixture()
      : networkSelector(eth::Network::ExperimentalTransitionTest),
        testBlockchain(TestBlockChain::defaultGenesisBlock()),
        genesisBlock(testBlockchain.testGenesis()),
        genesisDB(genesisBlock.state().db()),
        blockchain(testBlockchain.getInterface())
    {
        TestBlock testBlock;
        // block 1 - before Experimental
        testBlock.mine(testBlockchain);
        testBlockchain.addBlock(testBlock);
        block1hash = blockchain.currentHash();

        // block 2 - first Experimental block
        testBlock.mine(testBlockchain);
        testBlockchain.addBlock(testBlock);
        block2hash = blockchain.currentHash();
    }

    NetworkSelector networkSelector;
    TestBlockChain testBlockchain;
    TestBlock const& genesisBlock;
    OverlayDB const& genesisDB;
    BlockChain const& blockchain;

    h256 block1hash;
    h256 block2hash;
};

BOOST_FIXTURE_TEST_SUITE(ExperimentalBlockSuite, ExperimentalTransitionTestFixture)

BOOST_AUTO_TEST_CASE(bBlockhashContractIsCreated)
{
    Block block = blockchain.genesisBlock(genesisDB);
    BOOST_CHECK(!block.state().addressHasCode(Address(0xf0)));

    block.sync(blockchain);
    BOOST_REQUIRE(block.state().addressHasCode(Address(0xf0)));
}

BOOST_AUTO_TEST_CASE(bBlockhashContractIsUpdated)
{
    Block block = blockchain.genesisBlock(genesisDB);
    block.sync(blockchain, block1hash); // sync to the beginning of block 2

    h256 storageRoot2 = block.state().storageRoot(Address(0xf0));
    BOOST_CHECK(storageRoot2 != EmptyTrie);

    block.sync(blockchain); // sync to the beginning of block 3
    h256 storageRoot3 = block.state().storageRoot(Address(0xf0));
    BOOST_CHECK(storageRoot3 != EmptyTrie);

    BOOST_REQUIRE(storageRoot2 != storageRoot3);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
