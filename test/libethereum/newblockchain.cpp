#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <test/TestHelper.h>
#include <test/BlockChainHelper.h>

#include <libethereum/BlockChain.h>
#include <libethcore/BasicAuthority.h>
#include <test/JsonSpiritHeaders.h>
#include <libethereum/TransactionQueue.h>
#include <libdevcore/TransientDirectory.h>
#include <libethereum/Block.h>

#include "test/fuzzTesting/fuzzHelper.h"
#include <libdevcore/FileSystem.h>
#include <libethcore/Params.h>

using namespace std;
using namespace json_spirit;
using namespace dev;
using namespace dev::eth;
namespace dev {  namespace test {

TestTransaction::TestTransaction(json_spirit::mObject const& _o): m_jsonTransaction(_o)
{
	ImportTest::importTransaction(_o, m_transaction); //check that json structure is valid
}

TestBlock::TestBlock(TestBlock const& _original)
{
	populateFrom(_original);
}

TestBlock& TestBlock::operator = (TestBlock const& _original)
{
	populateFrom(_original);
	return *this;
}

TestBlock::TestBlock(json_spirit::mObject& _blockObj, json_spirit::mObject& _stateObj) //genesis block
{
	m_state = State(OverlayDB(State::openDB(m_tempDirState.path(), h256{}, WithExisting::Kill)), BaseState::Empty);
	ImportTest::importState(_stateObj, m_state);
	m_state.commit();

	m_blockHeader = constructBlock(_blockObj, m_state.rootHash());
	dev::eth::mine(m_blockHeader); //update PoW
	m_blockHeader.noteDirty();
	recalcBlockHeaderBytes();
}

void TestBlock::addTransaction(TestTransaction const& _tr)
{
	m_testTransactions.push_back(_tr);
	if (m_transactionQueue.import(_tr.getTransaction().rlp()) != ImportResult::Success)
		cnote << "Test block failed importing transaction\n";
}

void TestBlock::mine(TestBlockChain const& bc)
{
	TestBlock const& genesisBlock = bc.getTestGenesis();
	OverlayDB const& genesisDB = genesisBlock.getState().db();

	FullBlockChain<Ethash> const& blockchain = bc.getInterface();
	Block block = (blockchain.genesisBlock(genesisDB));
	block.setBeneficiary(genesisBlock.getBeneficiary());
	try
	{
		ZeroGasPricer gp;
		block.sync(blockchain);
		block.sync(blockchain, m_transactionQueue, gp);
		dev::eth::mine(block, blockchain);
	}
	catch (Exception const& _e)
	{
		cnote << "block sync or mining did throw an exception: " << diagnostic_information(_e);
		return;
	}
	catch (std::exception const& _e)
	{
		cnote << "block sync or mining did throw an exception: " << _e.what();
		return;
	}

	m_blockHeader = BlockHeader(block.blockData());
	//copyStateFrom(block.state());  ???
	recalcBlockHeaderBytes();
}

void TestBlock::overwriteBlockHeader(mObject const& _blObj, const BlockHeader& _parent)
{
	BlockHeader& header = m_blockHeader;
	auto ho = _blObj.at("blockHeader").get_obj();
	if (ho.size() != 14)
	{
		BlockHeader tmp = constructHeader(
			ho.count("parentHash") ? h256(ho["parentHash"].get_str()) : header.parentHash(),
			ho.count("uncleHash") ? h256(ho["uncleHash"].get_str()) : header.sha3Uncles(),
			ho.count("coinbase") ? Address(ho["coinbase"].get_str()) : header.beneficiary(),
			ho.count("stateRoot") ? h256(ho["stateRoot"].get_str()): header.stateRoot(),
			ho.count("transactionsTrie") ? h256(ho["transactionsTrie"].get_str()) : header.transactionsRoot(),
			ho.count("receiptTrie") ? h256(ho["receiptTrie"].get_str()) : header.receiptsRoot(),
			ho.count("bloom") ? LogBloom(ho["bloom"].get_str()) : header.logBloom(),
			ho.count("difficulty") ? toInt(ho["difficulty"]) : header.difficulty(),
			ho.count("number") ? toInt(ho["number"]) : header.number(),
			ho.count("gasLimit") ? toInt(ho["gasLimit"]) : header.gasLimit(),
			ho.count("gasUsed") ? toInt(ho["gasUsed"]) : header.gasUsed(),
			ho.count("timestamp") ? toInt(ho["timestamp"]) : header.timestamp(),
			ho.count("extraData") ? importByteArray(ho["extraData"].get_str()) : header.extraData());

		if (ho.count("RelTimestamp"))
		{
			tmp.setTimestamp(toInt(ho["RelTimestamp"]) +_parent.timestamp());
			tmp.setDifficulty(tmp.calculateDifficulty(_parent));
			this_thread::sleep_for(chrono::seconds((int)toInt(ho["RelTimestamp"])));
		}

		// find new valid nonce
		if (static_cast<BlockInfo>(tmp) != static_cast<BlockInfo>(header) && tmp.difficulty())
			dev::eth::mine(tmp);


		if (ho.count("mixHash"))
			updateEthashSeal(tmp, h256(ho["mixHash"].get_str()), tmp.nonce());
		if (ho.count("nonce"))
			updateEthashSeal(tmp, tmp.mixHash(), Nonce(ho["nonce"].get_str()));

		tmp.noteDirty();
		header = tmp;
	}
	else
	{
		// take the blockheader as is
		const bytes c_blockRLP = createBlockRLPFromFields(ho);
		const RLP c_bRLP(c_blockRLP);
		header.populateFromHeader(c_bRLP, IgnoreSeal);
	}

	recalcBlockHeaderBytes();
}

///Test Block Private

TestBlock::BlockHeader TestBlock::constructBlock(mObject& _o, h256 const& _stateRoot)
{
	BlockHeader ret;
	try
	{
		const bytes c_blockRLP = createBlockRLPFromFields(_o, _stateRoot);
		const RLP c_bRLP(c_blockRLP);
		ret.populateFromHeader(c_bRLP, IgnoreSeal);
	}
	catch (Exception const& _e)
	{
		cnote << "block population did throw an exception: " << diagnostic_information(_e);
	}
	catch (std::exception const& _e)
	{
		TBOOST_ERROR("Failed block population with Exception: " << _e.what());
	}
	catch(...)
	{
		TBOOST_ERROR("block population did throw an unknown exception\n");
	}
	return ret;
}

bytes TestBlock::createBlockRLPFromFields(mObject& _tObj, h256 const& _stateRoot)
{
	RLPStream rlpStream;
	rlpStream.appendList(_tObj.count("hash") > 0 ? (_tObj.size() - 1) : _tObj.size());

	if (_tObj.count("parentHash"))
		rlpStream << importByteArray(_tObj["parentHash"].get_str());

	if (_tObj.count("uncleHash"))
		rlpStream << importByteArray(_tObj["uncleHash"].get_str());

	if (_tObj.count("coinbase"))
		rlpStream << importByteArray(_tObj["coinbase"].get_str());

	if (_stateRoot)
		rlpStream << _stateRoot;
	else if (_tObj.count("stateRoot"))
		rlpStream << importByteArray(_tObj["stateRoot"].get_str());

	if (_tObj.count("transactionsTrie"))
		rlpStream << importByteArray(_tObj["transactionsTrie"].get_str());

	if (_tObj.count("receiptTrie"))
		rlpStream << importByteArray(_tObj["receiptTrie"].get_str());

	if (_tObj.count("bloom"))
		rlpStream << importByteArray(_tObj["bloom"].get_str());

	if (_tObj.count("difficulty"))
		rlpStream << bigint(_tObj["difficulty"].get_str());

	if (_tObj.count("number"))
		rlpStream << bigint(_tObj["number"].get_str());

	if (_tObj.count("gasLimit"))
		rlpStream << bigint(_tObj["gasLimit"].get_str());

	if (_tObj.count("gasUsed"))
		rlpStream << bigint(_tObj["gasUsed"].get_str());

	if (_tObj.count("timestamp"))
		rlpStream << bigint(_tObj["timestamp"].get_str());

	if (_tObj.count("extraData"))
		rlpStream << fromHex(_tObj["extraData"].get_str());

	if (_tObj.count("mixHash"))
		rlpStream << importByteArray(_tObj["mixHash"].get_str());

	if (_tObj.count("nonce"))
		rlpStream << importByteArray(_tObj["nonce"].get_str());

	return rlpStream.out();
}

void TestBlock::recalcBlockHeaderBytes()
{
	RLPStream blHeaderStream;
	m_blockHeader.streamRLP(blHeaderStream, WithProof);

	Transactions txList;
	for (auto const& txi: m_transactionQueue.topTransactions(std::numeric_limits<unsigned>::max()))
		txList.push_back(txi);
	RLPStream txStream;
	txStream.appendList(txList.size());
	for (unsigned i = 0; i < txList.size(); ++i)
	{
		RLPStream txrlp;
		txList[i].streamRLP(txrlp);
		txStream.appendRaw(txrlp.out());
	}

	RLPStream ret(3);
	ret.appendRaw(blHeaderStream.out()); //block header
	ret.appendRaw(txStream.out());		 //transactions
	ret.appendRaw(RLPEmptyList);		 //uncles

	m_blockHeader.verifyInternals(&ret.out());
	m_bytes = ret.out();
}

void TestBlock::copyStateFrom(State const& _state)
{
	//WEIRD WAY TO COPY STATE AS COPY CONSTRUCTOR FOR STATE NOT IMPLEMENTED CORRECTLY (they would share the same DB)
	m_tempDirState = TransientDirectory();
	m_state = State(OverlayDB(State::openDB(m_tempDirState.path(), h256{}, WithExisting::Kill)), BaseState::Empty);
	json_spirit::mObject obj = fillJsonWithState(_state);
	ImportTest::importState(obj, m_state);
}

void TestBlock::populateFrom(TestBlock const& _original)
{
	copyStateFrom(_original.getState());
	m_testTransactions = _original.getTestTransactions();
	m_transactionQueue.clear();
	TransactionQueue const& trQueue = _original.getTransactionQueue();
	for (auto const& txi: trQueue.topTransactions(std::numeric_limits<unsigned>::max()))
		m_transactionQueue.import(txi.rlp());

	m_blockHeader = _original.getBlockHeader();
	m_bytes = _original.getBytes();
}

///

TestBlockChain::TestBlockChain(TestBlock const& _genesisBlock)
{
	m_blockChain = std::unique_ptr<FullBlockChainEthash>(
				new FullBlockChainEthash(_genesisBlock.getBytes(), AccountMap(), m_tempDirBlockchain.path(), WithExisting::Kill));
	m_genesisBlock = _genesisBlock;
}

void TestBlockChain::addBlock(TestBlock& _block)
{
	m_blockChain.get()->import(_block.getBytes(), m_genesisBlock.getState().db());
}

void compareBlocks(TestBlock const& _a, TestBlock const& _b);
void doBlockchainTests2(json_spirit::mValue& _v, bool _fillin)
{
	string testname;
	for (auto& i: _v.get_obj())
	{
		mObject& o = i.second.get_obj();
		if (test::Options::get().singleTest && test::Options::get().singleTestName != i.first)
		{
			o.clear();
			continue;
		}

		cnote << i.first;
		testname = "(" + i.first + ") ";

		TBOOST_REQUIRE(o.count("genesisBlockHeader"));
		TBOOST_REQUIRE(o.count("pre"));

		TestBlock genesisBlock(o["genesisBlockHeader"].get_obj(), o["pre"].get_obj()); //pTestBlock (new TestBlock(o["genesisBlockHeader"].get_obj(), o["pre"].get_obj()));
		TestBlockChain trueBc(genesisBlock);

		//writeBlockHeaderToJson(o["genesisBlockHeader"].get_obj(), biGenesisBlock)
		//o["pre"] = fillJsonWithState(trueState); //convert all fields to hex
		//o["genesisRLP"] = toHex(rlpGenesisBlock.out(), 2, HexPrefix::Add);

		if (_fillin)
		{
			TBOOST_REQUIRE(o.count("blocks"));

			TestBlockChain blockchain(genesisBlock);
			size_t importBlockNumber = 0;
			for (auto const& bl: o["blocks"].get_array())
			{
				mObject blObj = bl.get_obj();
				if (blObj.count("blocknumber") > 0)
					importBlockNumber = max((int)toInt(blObj["blocknumber"]), 1);
				else
					importBlockNumber++;

				TestBlock block;

				TBOOST_REQUIRE(blObj.count("transactions"));
				for (auto const& txObj: blObj["transactions"].get_array())
				{
					TestTransaction transaction(txObj.get_obj());
					block.addTransaction(transaction);
				}

				//IMPORT UNCLES

				block.mine(blockchain);
				TestBlock alterBlock(block);

				//blObj["rlp"] = toHex(block.blockData(), 2, HexPrefix::Add);
				//blObj["transactions"] = writeTransactionsToJson(txList);

				if (blObj.count("blockHeader"))
					alterBlock.overwriteBlockHeader(blObj, blockchain.getInterface().info());

				//if (blObj.count("blockHeader") && blObj["blockHeader"].get_obj().count("bruncle"))
				//	current_BlockHeader.populateFromParent(vBiBlocks[vBiBlocks.size() -1]);

				/*RLPStream uncleStream;
				uncleStream.appendList(vBiUncles.size());
				for (unsigned i = 0; i < vBiUncles.size(); ++i)
				{
					RLPStream uncleRlp;
					vBiUncles[i].streamRLP(uncleRlp);
					uncleStream.appendRaw(uncleRlp.out());
				}

				if (vBiUncles.size())
				{
					// update unclehash in case of invalid uncles
					current_BlockHeader.setSha3Uncles(sha3(uncleStream.out()));
					updatePoW(current_BlockHeader);
				}*/

				// write block header
				//mObject oBlockHeader;
				//writeBlockHeaderToJson(oBlockHeader, current_BlockHeader);
				//blObj["blockHeader"] = oBlockHeader;
				//vBiBlocks.push_back(current_BlockHeader);

				//blObj["rlp"] = toHex(blockRLP.out(), 2, HexPrefix::Add);
				compareBlocks(block, alterBlock);


					blockchain.addBlock(block);


/*
					//there we get new blockchain status in state which could have more difficulty than we have in trueState
					//attempt to import new block to the true blockchain
					trueBc.sync(uncleBlockQueue, trueState.db(), 4);
					trueBc.attemptImport(blockRLP.out(), trueState.db());

					if (block.blockData() == trueBc.block())
						trueState = block.state();

					blockSet newBlock;
					newBlock.first = blockRLP.out();
					newBlock.second = uncleBlockQueueList;
					if (importBlockNumber < blockSets.size())
					{
						//make new correct history of imported blocks
						blockSets[importBlockNumber] = newBlock;
						for (size_t i = importBlockNumber + 1; i < blockSets.size(); i++)
							blockSets.pop_back();
					}
					else
						blockSets.push_back(newBlock);
				}
				// if exception is thrown, RLP is invalid and no blockHeader, Transaction list, or Uncle list should be given
				catch (...)
				{
					cnote << "block is invalid!\n";
					blObj.erase(blObj.find("blockHeader"));
					blObj.erase(blObj.find("uncleHeaders"));
					blObj.erase(blObj.find("transactions"));
				}
				blArray.push_back(blObj);
				this_thread::sleep_for(chrono::seconds(1));
*/
			}//each blocks
		}//fillin
	}//for tests
}

void compareBlocks(TestBlock const& _a, TestBlock const& _b)
{
	if (sha3(RLP(_a.getBytes())[0].data()) != sha3(RLP(_b.getBytes())[0].data()))
	{
		cnote << "block header mismatch\n";
		cnote << toHex(RLP(_a.getBytes())[0].data()) << "vs" << toHex(RLP(_b.getBytes())[0].data());
	}

	if (sha3(RLP(_a.getBytes())[1].data()) != sha3(RLP(_b.getBytes())[1].data()))
		cnote << "txs mismatch\n";

	if (sha3(RLP(_a.getBytes())[2].data()) != sha3(RLP(_b.getBytes())[2].data()))
		cnote << "uncle list mismatch\n" << RLP(_a.getBytes())[2].data() << "\n" << RLP(_b.getBytes())[2].data();
}

}}//namespaces


BOOST_AUTO_TEST_SUITE(BlockChainTests)

BOOST_AUTO_TEST_CASE(bc2)
{
	dev::test::executeTests("bcTotalDifficultyTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests2);
}

BOOST_AUTO_TEST_SUITE_END()
