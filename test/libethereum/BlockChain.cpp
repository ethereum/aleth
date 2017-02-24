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
#include <test/libtesteth/TestHelper.h>
#include <test/libtesteth/BlockChainHelper.h>
#include <libethereum/GenesisInfo.h>
#include <libethashseal/GenesisInfo.h>


using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

BOOST_FIXTURE_TEST_SUITE(BlockChainSuite, TestOutputHelper)

BOOST_AUTO_TEST_CASE(output)
{
	try
	{
		BOOST_WARN(string(BlockChainDebug::name()) == string(EthBlue "☍" EthWhite " ◇"));
		BOOST_WARN(string(BlockChainWarn::name()) == string(EthBlue "☍" EthOnRed EthBlackBold " ✘"));
		BOOST_WARN(string(BlockChainNote::name()) == string(EthBlue "☍" EthBlue " ℹ"));
		BOOST_WARN(string(BlockChainChat::name()) == string(EthBlue "☍" EthWhite " ◌"));

		TestBlock genesis = TestBlockChain::defaultGenesisBlock();
		TestBlockChain bc(genesis);

		TestBlock block;
		block.mine(bc);
		bc.addBlock(block);

		std::stringstream buffer;
		buffer << bc.interface();
		BOOST_REQUIRE(buffer.str().size() == 139);
		buffer.str(std::string());
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

BOOST_AUTO_TEST_CASE(opendb)
{
	TestBlock genesis = TestBlockChain::defaultGenesisBlock();
	TransientDirectory tempDirBlockchain;
	ChainParams p(genesisInfo(Network::TransitionnetTest), genesis.bytes(), genesis.accountMap());
	BlockChain bc(p, tempDirBlockchain.path(), WithExisting::Kill);
	auto is_critical = []( std::exception const& _e) { return string(_e.what()).find("DatabaseAlreadyOpen") != string::npos; };
	BOOST_CHECK_EXCEPTION(BlockChain bc2(p, tempDirBlockchain.path(), WithExisting::Verify), DatabaseAlreadyOpen, is_critical);
}

//BOOST_AUTO_TEST_CASE(rebuild)
//{
//	string dbPath;
//	TestBlock genesisCopy;
//	{
//		TestBlock genesis = TestBlockChain::getDefaultGenesisBlock();
//		genesisCopy = genesis;
//		TransientDirectory tempDirBlockchain;
//		dbPath = tempDirBlockchain.path();
//		FullBlockChain<Ethash> bc(genesis.getBytes(), AccountMap(), tempDirBlockchain.path(), WithExisting::Kill);

//		TestTransaction testTr = TestTransaction::getDefaultTransaction();
//		TransactionQueue trQueue;
//		trQueue.import(testTr.getTransaction().rlp());

//		ZeroGasPricer gp;
//		Block block = bc.genesisBlock(genesis.getState().db());
//		block.sync(bc);
//		block.sync(bc, trQueue, gp);
//		dev::eth::mine(block, bc);
//		bc.import(block.blockData(), block.state().db());
//		BOOST_REQUIRE(bc.number() == 1);

//		bc.rebuild(tempDirBlockchain.path());
//		BOOST_REQUIRE(bc.number() == 1);
//	}

//	FullBlockChain<Ethash> bc(genesisCopy.getBytes(), AccountMap(), dbPath, WithExisting::Verify);
//	BOOST_REQUIRE(bc.number() == 0);
//	bc.rebuild(dbPath);
//	BOOST_REQUIRE(bc.number() == 1);
//}

BOOST_AUTO_TEST_CASE(Mining_1_mineBlockWithTransaction)
{
	try
	{
		dev::test::TestBlockChain::s_sealEngineNetwork = eth::Network::FrontierTest;
		TestBlockChain bc(TestBlockChain::defaultGenesisBlock());
		TestTransaction tr = TestTransaction::defaultTransaction(1); //nonce = 1
		TestBlock block;
		block.addTransaction(tr);
		block.mine(bc);
		bc.addBlock(block);
		BOOST_REQUIRE(bc.interface().transactions().size() > 0);
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

BOOST_AUTO_TEST_CASE(Mining_2_mineUncles)
{
	try
	{
		dev::test::TestBlockChain::s_sealEngineNetwork = eth::Network::FrontierTest;
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


/*

Often broken test disabled 5th September 2016, until we have time to
troubleshoot the root cause.

See https://github.com/ethereum/cpp-ethereum/issues/3256.

BOOST_AUTO_TEST_CASE(Mining_3_mineBlockWithUncles)
{
	try
	{
		dev::test::TestBlockChain::s_sealEngineNetwork = eth::Network::FrontierTest;
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
*/


/*

Often broken test disabled 5th September 2016, until we have time to
troubleshoot the root cause.

See https://github.com/ethereum/cpp-ethereum/issues/3059.

BOOST_AUTO_TEST_CASE(Mining_4_BlockQueueSyncing)
{
	try
	{
		dev::test::TestBlockChain::s_sealEngineNetwork = eth::Network::FrontierTest;
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
*/


BOOST_AUTO_TEST_CASE(Mining_5_BlockFutureTime)
{
	try
	{
		dev::test::TestBlockChain::s_sealEngineNetwork = eth::Network::FrontierTest;
		TestBlockChain bc(TestBlockChain::defaultGenesisBlock());

		TestBlock uncleBlock;
		uncleBlock.mine(bc);

		BlockHeader uncleHeader = uncleBlock.blockHeader();
		uncleHeader.setTimestamp(uncleHeader.timestamp() + 10000);
		uncleBlock.setBlockHeader(uncleHeader);
		uncleBlock.updateNonce(bc);

		BlockQueue uncleBlockQueue;
		uncleBlockQueue.setChain(bc.interface());
		uncleBlockQueue.import(&uncleBlock.bytes(), false);
		this_thread::sleep_for(chrono::seconds(2));

		BlockChain& bcRef = bc.interfaceUnsafe();
		bcRef.sync(uncleBlockQueue, bc.testGenesis().state().db(), unsigned(4));
		BOOST_REQUIRE(uncleBlockQueue.blockStatus(uncleBlock.blockHeader().hash()) == QueueStatus::Unknown);

		pair<ImportResult, ImportRoute> importAttempt;
		importAttempt = bcRef.attemptImport(uncleBlock.bytes(), bc.testGenesis().state().db());
		BOOST_REQUIRE(importAttempt.first == ImportResult::FutureTimeKnown);

		auto is_critical = []( std::exception const& _e) { cnote << _e.what(); return true; };
		BOOST_CHECK_EXCEPTION(bcRef.insert(uncleBlock.bytes(), uncleBlock.receipts()), FutureTime, is_critical);
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

bool onBadwasCalled = false;
void onBad(Exception& _ex)
{
	cout << _ex.what();
	onBadwasCalled = true;
}

BOOST_AUTO_TEST_CASE(attemptImport)
{
	//UnknownParent
	//Success
	//AlreadyKnown
	//FutureTimeKnown
	//Malformed

	try
	{
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
		cout << endl;
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

BOOST_AUTO_TEST_CASE(insert)
{
	try
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

		auto is_critical = []( std::exception const& _e) { return string(_e.what()).find("InvalidBlockFormat") != string::npos; };
		BOOST_CHECK_EXCEPTION(bcRef.insert(bl.blockData(), receiptRef), InvalidBlockFormat, is_critical);
		auto is_critical2 = []( std::exception const& _e) { return string(_e.what()).find("InvalidReceiptsStateRoot") != string::npos; };
		BOOST_CHECK_EXCEPTION(bcRef.insert(block.bytes(), receiptRef), InvalidReceiptsStateRoot, is_critical2);

		BOOST_REQUIRE(bcRef.number() == 0);

		try
		{
			bcRef.insert(block.bytes(), block.receipts());
		}
		catch(...)
		{
			BOOST_ERROR("Unexpected Exception!");
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

BOOST_AUTO_TEST_CASE(insertException)
{
	try
	{
		TestBlockChain bc(TestBlockChain::defaultGenesisBlock());
		BlockChain& bcRef = bc.interfaceUnsafe();

		TestTransaction tr = TestTransaction::defaultTransaction();
		TestBlock block;
		block.addTransaction(tr);
		block.mine(bc);
		bc.addBlock(block);

		auto is_critical = []( std::exception const& _e) { cnote << _e.what(); return true; };
		BOOST_CHECK_EXCEPTION(bcRef.insert(block.bytes(), block.receipts()), AlreadyHaveBlock, is_critical);
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

BOOST_AUTO_TEST_CASE(rescue)
{
	try
	{
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

		// Temporary disable this assertion, which is failing in TravisCI for OS X Mavericks
		// See https://travis-ci.org/ethereum/cpp-ethereum/jobs/156083698.
		#if !defined(DISABLE_BROKEN_UNIT_TESTS_UNTIL_WE_FIX_THEM)
			try
			{
				BlockChain& bcRef = bc.interfaceUnsafe();
				std::this_thread::sleep_for(std::chrono::seconds(10)); //try wait for block verification before rescue
				bcRef.rescue(bc.testGenesis().state().db());
				BOOST_CHECK_MESSAGE(bcRef.number() == 3, "Rescued Blockchain missing some blocks!");
			}
			catch(...)
			{
				BOOST_ERROR("Unexpected Exception!");
			}
		#endif // !defined(DISABLE_BROKEN_UNIT_TESTS_UNTIL_WE_FIX_THEM)

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

BOOST_AUTO_TEST_CASE(updateStats)
{
	try
	{
		TestBlockChain bc(TestBlockChain::defaultGenesisBlock());
		BlockChain& bcRef = bc.interfaceUnsafe();

		BlockChain::Statistics stat = bcRef.usage();
		//Absolutely random values here!
		//BOOST_REQUIRE(stat.memBlockHashes == 0);
		//BOOST_REQUIRE(stat.memBlocks == 1); //incorrect value here
		//BOOST_REQUIRE(stat.memDetails == 0);
		//BOOST_REQUIRE(stat.memLogBlooms == 0);
		//BOOST_REQUIRE(stat.memReceipts == 0);
		//BOOST_REQUIRE(stat.memTotal() == 0);
		//BOOST_REQUIRE(stat.memTransactionAddresses == 0); //incorrect value here

		TestTransaction tr = TestTransaction::defaultTransaction();
		TestBlock block;
		block.addTransaction(tr);
		block.mine(bc);
		bc.addBlock(block);

		stat = bcRef.usage(true);
		BOOST_REQUIRE(stat.memBlockHashes == 0);
		BOOST_REQUIRE(stat.memBlocks == 675);
		BOOST_REQUIRE(stat.memDetails == 138);
		BOOST_REQUIRE(stat.memLogBlooms == 8422);
		BOOST_REQUIRE(stat.memReceipts == 0);
		BOOST_REQUIRE(stat.memTotal() == 9235);
		BOOST_REQUIRE(stat.memTransactionAddresses == 0);

		//memchache size 33554432 - 3500 blocks before cache to be cleared
		bcRef.garbageCollect(true);
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
