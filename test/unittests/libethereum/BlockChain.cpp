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
/** @file fullblockchain.cpp
 * @author Dimitry Khokhlov <dimitry@ethdev.com>
 * @date 2015
 * Blockchain test functions.
 */

#include <libethereum/Block.h>
#include <libethereum/BlockChain.h>
#include <libdevcore/DBFactory.h>
#include <test/tools/libtesteth/TestHelper.h>
#include <test/tools/libtesteth/BlockChainHelper.h>
#include <libethereum/GenesisInfo.h>
#include <libethereum/ChainParams.h>

using namespace std;
using namespace dev;
using namespace dev::db;
using namespace dev::eth;
using namespace dev::test;
namespace utf = boost::unit_test;

BOOST_FIXTURE_TEST_SUITE(BlockChainFrontierSuite, FrontierNoProofTestFixture)

BOOST_AUTO_TEST_CASE(output)
{
    TestBlock genesis = TestBlockChain::defaultGenesisBlock();
    TestBlockChain bc(genesis);

    TestBlock block;
    block.mine(bc);
    bc.addBlock(block);

    std::stringstream buffer;
    buffer << bc.getInterface();
    BOOST_REQUIRE(buffer.str().size() == 139);
    buffer.str(std::string());
}

// We always run this test using a disk-backed database since that's the configuration that
// users are running.
BOOST_AUTO_TEST_CASE(opendb)
{
    // Need to set database kind before creating a genesis block since that creates a state
    // database
    auto const preDatabaseKind = databaseKind();
    setDatabaseKind(DatabaseKind::LevelDB);

    TestBlock genesis = TestBlockChain::defaultGenesisBlock();
    TransientDirectory tempDirBlockchain;
    ChainParams p(genesisInfo(eth::Network::TransitionnetTest), genesis.bytes(), genesis.accountMap());
    BlockChain bc(p, tempDirBlockchain.path(), WithExisting::Kill);
    auto is_critical = []( std::exception const& _e) { return string(_e.what()).find("DatabaseAlreadyOpen") != string::npos; };
    BOOST_CHECK_EXCEPTION(BlockChain bc2(p, tempDirBlockchain.path(), WithExisting::Verify), DatabaseAlreadyOpen, is_critical);

    setDatabaseKind(preDatabaseKind);
}

BOOST_AUTO_TEST_CASE(Mining_1_mineBlockWithTransaction)
{
    TestBlockChain bc(TestBlockChain::defaultGenesisBlock());
    TestTransaction tr = TestTransaction::defaultTransaction(1); //nonce = 1
    TestBlock block;
    block.addTransaction(tr);
    block.mine(bc);
    bc.addBlock(block);
    BOOST_REQUIRE(bc.getInterface().transactions().size() > 0);
}

BOOST_AUTO_TEST_CASE(Mining_2_mineUncles)
{
    TestBlockChain bc(TestBlockChain::defaultGenesisBlock());
    TestTransaction tr = TestTransaction::defaultTransaction(1); //nonce = 1
    TestBlock block;
    block.addTransaction(tr);
    block.mine(bc);
    bc.addBlock(block);

    TestBlock uncleBlock;
    uncleBlock.mine(bc);
    TestBlock uncleBlock2;
    uncleBlock2.mine(bc);

    TestTransaction tr2 = TestTransaction::defaultTransaction(2);
    TestBlock block2;
    block2.addTransaction(tr2);
    block2.mine(bc);
    bc.addBlock(block2);
}


/*

Often broken test disabled 5th September 2016, until we have time to
troubleshoot the root cause.

See https://github.com/ethereum/cpp-ethereum/issues/3256.

(Mining_3_mineBlockWithUncles)
{
    TestBlockChain bc(TestBlockChain::defaultGenesisBlock());
    TestTransaction tr = TestTransaction::defaultTransaction(1); //nonce = 1
    TestBlock block;
    block.addTransaction(tr);
    block.mine(bc);
    bc.addBlock(block);

    TestBlock uncleBlock;
    uncleBlock.mine(bc);
    TestBlock uncleBlock2;
    uncleBlock2.mine(bc);

    TestTransaction tr2 = TestTransaction::defaultTransaction(2);
    TestBlock block2;
    block2.addTransaction(tr2);
    block2.mine(bc);
    bc.addBlock(block2);

    TestTransaction tr3 = TestTransaction::defaultTransaction(3);
    TestBlock block3;
    block3.addUncle(uncleBlock);
    bc.syncUncles(block3.uncles());
    block3.addTransaction(tr3);
    block3.mine(bc);
    bc.addBlock(block3);
    BOOST_REQUIRE(bc.interface().info().number() == 3);
    BOOST_REQUIRE(bc.interface().info(uncleBlock.blockHeader().hash()) == uncleBlock.blockHeader());
}
*/


/*

Often broken test disabled 5th September 2016, until we have time to
troubleshoot the root cause.

See https://github.com/ethereum/cpp-ethereum/issues/3059.

(Mining_4_BlockQueueSyncing)
{
    TestBlockChain bc(TestBlockChain::defaultGenesisBlock());
    TestBlockChain bc2(TestBlockChain::defaultGenesisBlock());

    TestBlock block;
    block.mine(bc2);
    bc2.addBlock(block);
    TestBlock block2;
    block2.mine(bc2);

    BlockQueue uncleBlockQueue;
    uncleBlockQueue.setChain(bc2.interface());
    ImportResult importIntoQueue = uncleBlockQueue.import(&block2.bytes(), false);
    BOOST_REQUIRE(importIntoQueue == ImportResult::Success);
    this_thread::sleep_for(chrono::seconds(2));

    BlockChain& bcRef = bc.interfaceUnsafe();
    bcRef.sync(uncleBlockQueue, bc.testGenesis().state().db(), unsigned(4));

    //Attempt import block5 to another blockchain
    pair<ImportResult, ImportRoute> importAttempt;
    importAttempt = bcRef.attemptImport(block2.bytes(), bc.testGenesis().state().db());
    BOOST_REQUIRE(importAttempt.first == ImportResult::UnknownParent);

    //Insert block5 to another blockchain
    auto is_critical = []( std::exception const& _e) { cnote << _e.what(); return true; };
    BOOST_CHECK_EXCEPTION(bcRef.insert(block2.bytes(), block2.receipts()), UnknownParent, is_critical);

    //Get status of block5 in the block queue based on block5's chain (block5 imported into queue but not imported into chain)
    //BlockQueue(bc2) changed by sync function of original bc
    QueueStatus status = uncleBlockQueue.blockStatus(block2.blockHeader().hash());
    BOOST_REQUIRE_MESSAGE(status == QueueStatus::Bad, "Received Queue Status: " + toString(status) + " Expected Queue Status: " + toString(QueueStatus::Bad));
}
*/

BOOST_AUTO_TEST_CASE(insertWithoutParent)
{
    TestBlockChain bc(TestBlockChain::defaultGenesisBlock());
    TestTransaction tr = TestTransaction::defaultTransaction();
    TestBlock block;
    block.mine(bc);

    BlockHeader header = block.blockHeader();
    header.setNumber(10);
    block.setBlockHeader(header);

    BlockChain& bcRef = bc.interfaceUnsafe();

    bcRef.insertWithoutParent(block.bytes(), block.receipts(), 0x040000);
    BOOST_CHECK_EQUAL(bcRef.number(), 10);

    bcRef.setChainStartBlockNumber(10);
    BOOST_REQUIRE_EQUAL(bcRef.chainStartBlockNumber(), 10);
}


BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(BlockChainMainNetworkSuite, MainNetworkNoProofTestFixture)

BOOST_AUTO_TEST_CASE(Mining_5_BlockFutureTime)
{
    TestBlockChain bc(TestBlockChain::defaultGenesisBlock());

    TestBlock uncleBlock;
    uncleBlock.mine(bc);

    BlockHeader uncleHeader = uncleBlock.blockHeader();
    uncleHeader.setTimestamp(uncleHeader.timestamp() + 10000);
    uncleBlock.setBlockHeader(uncleHeader);
    uncleBlock.updateNonce(bc);

    BlockQueue uncleBlockQueue;
    uncleBlockQueue.setChain(bc.getInterface());
    uncleBlockQueue.import(&uncleBlock.bytes(), false);
    std::this_thread::sleep_for(std::chrono::seconds(2));

    BlockChain& bcRef = bc.interfaceUnsafe();
    bcRef.sync(uncleBlockQueue, bc.testGenesis().state().db(), unsigned(4));
    BOOST_REQUIRE(uncleBlockQueue.blockStatus(uncleBlock.blockHeader().hash()) == QueueStatus::Unknown);

    pair<ImportResult, ImportRoute> importAttempt;
    importAttempt = bcRef.attemptImport(uncleBlock.bytes(), bc.testGenesis().state().db());
    BOOST_REQUIRE(importAttempt.first == ImportResult::FutureTimeKnown);

    auto is_critical = []( std::exception const& _e) { cnote << _e.what(); return true; };
    BOOST_CHECK_EXCEPTION(bcRef.insert(uncleBlock.bytes(), uncleBlock.receipts()), FutureTime, is_critical);
}

bool onBadwasCalled = false;
void onBad(Exception&)
{
    onBadwasCalled = true;
}

BOOST_AUTO_TEST_CASE(attemptImport)
{
    //UnknownParent
    //Success
    //AlreadyKnown
    //FutureTimeKnown
    //Malformed

    TestBlockChain bc(TestBlockChain::defaultGenesisBlock());

    TestTransaction tr = TestTransaction::defaultTransaction();
    TestBlock block;
    block.addTransaction(tr);
    block.mine(bc);

    pair<ImportResult, ImportRoute> importAttempt;
    BlockChain& bcRef = bc.interfaceUnsafe();
    bcRef.setOnBad(onBad);

    importAttempt = bcRef.attemptImport(block.bytes(), bc.testGenesis().state().db());
    BOOST_REQUIRE(importAttempt.first == ImportResult::Success);

    importAttempt = bcRef.attemptImport(block.bytes(), bc.testGenesis().state().db());
    BOOST_REQUIRE(importAttempt.first == ImportResult::AlreadyKnown);

    bytes blockBytes = block.bytes();
    blockBytes[0] = 0;
    importAttempt = bcRef.attemptImport(blockBytes, bc.testGenesis().state().db());
    BOOST_REQUIRE(importAttempt.first == ImportResult::Malformed);
    BOOST_REQUIRE(onBadwasCalled == true);
}

BOOST_AUTO_TEST_CASE(insert)
{
    TestBlockChain bc(TestBlockChain::defaultGenesisBlock());
    TestTransaction tr = TestTransaction::defaultTransaction();
    TestBlock block;
    block.addTransaction(tr);
    block.mine(bc);

    BlockChain& bcRef = bc.interfaceUnsafe();

    //Incorrect Receipt
    ZeroGasPricer gp;
    Block bl = bcRef.genesisBlock(bc.testGenesis().state().db());
    bl.sync(bcRef);
    bl.sync(bcRef, block.transactionQueue(), gp);

    //Receipt should be RLPStream
    const bytes receipt = bl.receipt(0).rlp();
    bytesConstRef receiptRef(&receipt[0], receipt.size());

    auto is_critical = [](std::exception const& _e) { return string(_e.what()).find("InvalidBlockFormat") != string::npos; };
    BOOST_CHECK_EXCEPTION(bcRef.insert(bl.blockData(), receiptRef), InvalidBlockFormat, is_critical);
    auto is_critical2 = [](std::exception const& _e) { return string(_e.what()).find("InvalidReceiptsStateRoot") != string::npos; };
    BOOST_CHECK_EXCEPTION(bcRef.insert(block.bytes(), receiptRef), InvalidReceiptsStateRoot, is_critical2);

    BOOST_REQUIRE(bcRef.number() == 0);

    bcRef.insert(block.bytes(), block.receipts());
}

BOOST_AUTO_TEST_CASE(insertException)
{
    TestBlockChain bc(TestBlockChain::defaultGenesisBlock());
    BlockChain& bcRef = bc.interfaceUnsafe();

    TestTransaction tr = TestTransaction::defaultTransaction();
    TestBlock block;
    block.addTransaction(tr);
    block.mine(bc);
    bc.addBlock(block);

    auto is_critical = [](std::exception const& _e) { cnote << _e.what(); return true; };
    BOOST_CHECK_EXCEPTION(bcRef.insert(block.bytes(), block.receipts()), AlreadyHaveBlock, is_critical);
}

BOOST_AUTO_TEST_CASE(rescue, *utf::expected_failures(1))
{
    cout << "BlockChainMainNetworkSuite/rescue test - failure is expected\n";

    TestBlockChain bc(TestBlockChain::defaultGenesisBlock());

    {
        TestTransaction tr = TestTransaction::defaultTransaction();
        TestBlock block;
        block.addTransaction(tr);
        block.mine(bc);
        bc.addBlock(block);
    }

    {
        TestTransaction tr = TestTransaction::defaultTransaction(1);
        TestBlock block;
        block.addTransaction(tr);
        block.mine(bc);
        bc.addBlock(block);
    }

    {
        TestTransaction tr = TestTransaction::defaultTransaction(2);
        TestBlock block;
        block.addTransaction(tr);
        block.mine(bc);
        bc.addBlock(block);
    }

    BlockChain& bcRef = bc.interfaceUnsafe();
    bcRef.rescue(bc.testGenesis().state().db());
    BOOST_CHECK_EQUAL(bcRef.number(), 3);
}

BOOST_AUTO_TEST_CASE(updateStats)
{
    TestBlockChain bc(TestBlockChain::defaultGenesisBlock());
    BlockChain& bcRef = bc.interfaceUnsafe();

    BlockChain::Statistics stat = bcRef.usage();
    BOOST_CHECK_EQUAL(stat.memBlockHashes, 0);
    BOOST_CHECK_EQUAL(stat.memBlocks, 0);
    BOOST_CHECK_EQUAL(stat.memDetails, 0);
    BOOST_CHECK_EQUAL(stat.memLogBlooms, 0);
    BOOST_CHECK_EQUAL(stat.memReceipts, 0);
    BOOST_CHECK_EQUAL(stat.memTotal(), 0);
    BOOST_CHECK_EQUAL(stat.memTransactionAddresses, 0);

    TestTransaction tr = TestTransaction::defaultTransaction();
    TestBlock block;
    block.addTransaction(tr);
    block.mine(bc);
    bc.addBlock(block);

    stat = bcRef.usage(true);
    BOOST_CHECK_EQUAL(stat.memBlockHashes, 0);

    unsigned const memBlocksExpected = block.bytes().size() + 64;
    BOOST_CHECK_EQUAL(stat.memBlocks, memBlocksExpected);
    unsigned totalExpected = memBlocksExpected;

    h256 const genesisHash = bc.testGenesis().blockHeader().hash();
    unsigned const memDetailsExpected = bcRef.details(genesisHash).size + 64;
    BOOST_CHECK_EQUAL(stat.memDetails, memDetailsExpected);
    totalExpected += memDetailsExpected;

    unsigned const memLogBloomsExpected =
        bcRef.blocksBlooms(0, 0).size + 64 + bcRef.blocksBlooms(1, 0).size + 64;
    BOOST_CHECK_EQUAL(stat.memLogBlooms, memLogBloomsExpected);
    totalExpected += memLogBloomsExpected;

    BOOST_CHECK_EQUAL(stat.memReceipts, 0);
    BOOST_CHECK_EQUAL(stat.memTotal(), totalExpected);
    BOOST_CHECK_EQUAL(stat.memTransactionAddresses, 0);

    //memchache size 33554432 - 3500 blocks before cache to be cleared
    bcRef.garbageCollect(true);
}

BOOST_AUTO_TEST_CASE(invalidJsonThrows)
{
    h256 emptyStateRoot;
    /* Below, a comma is missing between fields. */
    BOOST_CHECK_THROW(ChainParams("{ \"sealEngine\" : \"unknown\" \"accountStartNonce\" : \"3\" }", emptyStateRoot), json_spirit::Error_position);
}

BOOST_AUTO_TEST_CASE(unknownFieldThrows)
{
    h256 emptyStateRoot;
    /* Below, an unknown field is passed. */
    BOOST_CHECK_THROW(ChainParams("{ \"usuallyNotThere\" : \"unknown\" }", emptyStateRoot), UnknownField);
}

BOOST_AUTO_TEST_SUITE_END()
