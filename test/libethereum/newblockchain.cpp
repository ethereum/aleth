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

void TestBlock::addUncle(TestBlock const& _uncle)
{
	m_uncles.push_back(_uncle);
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
	copyStateFrom(block.state());
	recalcBlockHeaderBytes();
}

void TestBlock::setBlockHeader(Ethash::BlockHeader const& _header)
{
	m_blockHeader = _header;
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

//Form bytestream of a block with [header transactions uncles]
void TestBlock::recalcBlockHeaderBytes()
{
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

	RLPStream uncleStream;
	uncleStream.appendList(m_uncles.size());
	for (unsigned i = 0; i < m_uncles.size(); ++i)
	{
		RLPStream uncleRlp;
		m_uncles[i].getBlockHeader().streamRLP(uncleRlp);
		uncleStream.appendRaw(uncleRlp.out());
	}

	if (m_uncles.size()) // update unclehash in case of invalid uncles
	{
		m_blockHeader.setSha3Uncles(sha3(uncleStream.out()));
		dev::eth::mine(m_blockHeader);
		m_blockHeader.noteDirty();
	}
	RLPStream blHeaderStream;
	m_blockHeader.streamRLP(blHeaderStream, WithProof);

	RLPStream ret(3);
	ret.appendRaw(blHeaderStream.out()); //block header
	ret.appendRaw(txStream.out());		 //transactions
	ret.appendRaw(uncleStream.out());	 //uncles

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
	m_lastBlock = m_genesisBlock;
}

void TestBlockChain::addBlock(TestBlock const& _block)
{	
	m_blockChain.get()->import(_block.getBytes(), m_genesisBlock.getState().db());

	//Imported and best
	if (_block.getBytes() == m_blockChain.get()->block())
		m_lastBlock = _block;
}

//Functions that working with test json
void compareBlocks(TestBlock const& _a, TestBlock const& _b);
mArray writeTransactionsToJson(TransactionQueue const& _txsQueue);
mObject& writeBlockHeaderToJson2(mObject& _o, Ethash::BlockHeader const& _bi);
void overwriteBlockHeaderForTest(mObject const& _blObj, TestBlock& _block, Ethash::BlockHeader const& _parentHeader);
void overwriteUncleHeaderForTest(mObject& _uncleHeaderObj, TestBlock& _uncle, std::vector<TestBlock> const& _uncles, std::vector<TestBlock> const& _importedBlocks);

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

		if (_fillin)
		{
			writeBlockHeaderToJson2(o["genesisBlockHeader"].get_obj(), genesisBlock.getBlockHeader());
			o["genesisRLP"] = toHex(genesisBlock.getBytes(), 2, HexPrefix::Add);
			TBOOST_REQUIRE(o.count("blocks"));

			mArray blArray;
			size_t importBlockNumber = 0;
			vector<TestBlock> importedBlocks;
			for (auto const& bl: o["blocks"].get_array())
			{
				TestBlockChain blockchain(genesisBlock);

				mObject blObj = bl.get_obj();
				if (blObj.count("blocknumber") > 0)
					importBlockNumber = max((int)toInt(blObj["blocknumber"]), 1);
				else
					importBlockNumber++;

				//Restore blockchain up to block.number to start import from it
				// importBlockNumber > importedBlocks.size ???? Check!!!!
				for (size_t i = 1; i < importBlockNumber; i++) //0 block is genesis
					blockchain.addBlock(importedBlocks.at(i));

				TestBlock block;

				//Import Transactions
				TBOOST_REQUIRE(blObj.count("transactions"));
				for (auto const& txObj: blObj["transactions"].get_array())
				{
					TestTransaction transaction(txObj.get_obj());
					block.addTransaction(transaction);
				}

				//Import Uncles
				vector<TestBlock> uncles; //really need this? ==block.getuncles()
				for (auto const& uHObj: blObj.at("uncleHeaders").get_array())
				{
					mObject emptyState;
					mObject uncleHeaderObj = uHObj.get_obj();
					TestBlock uncle(uncleHeaderObj, emptyState);
					overwriteUncleHeaderForTest(uncleHeaderObj, uncle, uncles, importedBlocks);
					uncles.push_back(uncle);
					block.addUncle(uncle);
				}

				block.mine(blockchain);
				TestBlock alterBlock(block);

				if (blObj.count("blockHeader"))
					overwriteBlockHeaderForTest(blObj, alterBlock, blockchain.getInterface().info());

				blObj["transactions"] = writeTransactionsToJson(alterBlock.getTransactionQueue());
				blObj["rlp"] = toHex(alterBlock.getBytes(), 2, HexPrefix::Add);
				mObject hobj;
				writeBlockHeaderToJson2(hobj, alterBlock.getBlockHeader());
				blObj["blockHeader"] = hobj;

				compareBlocks(block, alterBlock);
				try
				{
					blockchain.addBlock(alterBlock);
					trueBc.addBlock(alterBlock);					

					if (importBlockNumber < importedBlocks.size())
					{
						importedBlocks[importBlockNumber] = alterBlock;
						for (size_t i = importBlockNumber + 1; i < importedBlocks.size(); i++)
							importedBlocks.pop_back();
					}
					else
						importedBlocks.push_back(alterBlock);
				}
				catch (...)
				{	// if exception is thrown, RLP is invalid and no blockHeader, Transaction list, or Uncle list should be given
					cnote << "block is invalid!\n";
					blObj.erase(blObj.find("blockHeader"));
					blObj.erase(blObj.find("uncleHeaders"));
					blObj.erase(blObj.find("transactions"));
				}
				blArray.push_back(blObj);  //json data
				this_thread::sleep_for(chrono::seconds(1));
			}//each blocks

			if (o.count("expect") > 0)
			{
				AccountMaskMap expectStateMap;
				State stateExpect(OverlayDB(), BaseState::Empty);
				ImportTest::importState(o["expect"].get_obj(), stateExpect, expectStateMap);
				if (ImportTest::compareStates(stateExpect, trueBc.getTopBlock().getState(), expectStateMap, Options::get().checkState ? WhenError::Throw : WhenError::DontThrow))
					cerr << testname << endl;
				o.erase(o.find("expect"));
			}

			o["blocks"] = blArray;
			o["postState"] = fillJsonWithState(trueBc.getTopBlock().getState());
			o["lastblockhash"] = toString(trueBc.getTopBlock().getBlockHeader().hash());

			//make all values hex in pre section
			State prestate(OverlayDB(), BaseState::Empty);
			ImportTest::importState(o["pre"].get_obj(), prestate);
			o["pre"] = fillJsonWithState(prestate);
		}//fillin
	}//for tests
}


//TestFunction
void overwriteBlockHeaderForTest(mObject const& _blObj, TestBlock& _block, Ethash::BlockHeader const& _parentHeader)
{
	//_blObj  - json object with header data
	//_block  - which header would be overwritten
	//_parentHeader - parent blockheader

	Ethash::BlockHeader header = _block.getBlockHeader();
	auto ho = _blObj.at("blockHeader").get_obj();
	if (ho.size() != 14)
	{
		Ethash::BlockHeader tmp = constructHeader(
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
			tmp.setTimestamp(toInt(ho["RelTimestamp"]) +_parentHeader.timestamp());
			tmp.setDifficulty(tmp.calculateDifficulty(_parentHeader));
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
		mObject emptyState;
		header = TestBlock(ho, emptyState).getBlockHeader();
	}

	if (_blObj.at("blockHeader").get_obj().count("bruncle"))
		header.populateFromParent(_parentHeader); //importedBlocks.at(importedBlocks.size() -1).getBlockHeader()

	_block.setBlockHeader(header);
}

void overwriteUncleHeaderForTest(mObject& uncleHeaderObj, TestBlock& uncle, std::vector<TestBlock> const& uncles, std::vector<TestBlock> const& importedBlocks)
{
	//uncleHeaderObj - json Uncle header with additional option fields
	//uncle			 - uncle Block to overwrite
	//uncles		 - previously imported uncles
	//importedBlocks - blocks already included in BlockChain

	if (uncleHeaderObj.count("sameAsPreviousSibling"))
	{
		uncleHeaderObj.erase("sameAsPreviousSibling");
		if (uncles.size())			
			uncle = uncles.at(uncles.size() - 1);
		else
			cerr << "Could not create uncle sameAsPreviousSibling: there are no siblings!";
		return;
	}

	if (uncleHeaderObj.count("sameAsBlock"))
	{
		uncleHeaderObj.erase("sameAsBlock");
		size_t number = (size_t)toInt(uncleHeaderObj.at("sameAsBlock"));

		if (number < importedBlocks.size())
			uncle = importedBlocks.at(number);
		else
			cerr << "Could not create uncle sameAsBlock: there are no block imported!";
		return;
	}

	if (uncleHeaderObj.count("sameAsPreviousBlockUncle"))
	{
		uncleHeaderObj.erase("sameAsPreviousBlockUncle");
		size_t number = (size_t)toInt(uncleHeaderObj.at("sameAsPreviousBlockUncle"));
		if (number < importedBlocks.size())
		{
			vector<TestBlock> prevBlockUncles = importedBlocks.at(number).getUncles();
			if (prevBlockUncles.size())
				uncle = prevBlockUncles[0];
			else
				cerr << "Could not create uncle sameAsPreviousBlockUncle: previous block has no uncles!";
		}
		else
			cerr << "Could not create uncle sameAsPreviousBlockUncle: there are no block imported!";
		return;
	}

	string overwrite = "false";
	if (uncleHeaderObj.count("overwriteAndRedoPoW"))
	{
		overwrite = uncleHeaderObj.at("overwriteAndRedoPoW").get_str();
		uncleHeaderObj.erase("overwriteAndRedoPoW");
	}

	// make uncle header valid
	// from this point naughty part will start with blockheader
	//Ethash::BlockHeader& uncleHeader = *const_cast<Ethash::BlockHeader*> (&uncle.getBlockHeader());

	Ethash::BlockHeader uncleHeader = uncle.getBlockHeader();
	uncleHeader.setTimestamp((u256)time(0));
	if (importedBlocks.size() > 2)
	{
		if (uncleHeader.number() - 1 < importedBlocks.size())
			uncleHeader.populateFromParent(importedBlocks.at((size_t)uncleHeader.number() - 1).getBlockHeader());
		else
			uncleHeader.populateFromParent(importedBlocks.at(importedBlocks.size() - 2).getBlockHeader());

	}
	else
	{
		uncle.setBlockHeader(uncleHeader);
		return;
	}

	if (overwrite != "false")
	{
		uncleHeader = constructHeader(
			overwrite == "parentHash" ? h256(uncleHeaderObj.at("parentHash").get_str()) : uncleHeader.parentHash(),
			uncleHeader.sha3Uncles(),
			uncleHeader.beneficiary(),
			overwrite == "stateRoot" ? h256(uncleHeaderObj.at("stateRoot").get_str()) : uncleHeader.stateRoot(),
			uncleHeader.transactionsRoot(),
			uncleHeader.receiptsRoot(),
			uncleHeader.logBloom(),
			overwrite == "difficulty" ? toInt(uncleHeaderObj.at("difficulty"))
									  :	overwrite == "timestamp" ? uncleHeader.calculateDifficulty(importedBlocks.at((size_t)uncleHeader.number() - 1).getBlockHeader())
																 : uncleHeader.difficulty(),
			uncleHeader.number(),
			overwrite == "gasLimit" ? toInt(uncleHeaderObj.at("gasLimit")) : uncleHeader.gasLimit(),
			overwrite == "gasUsed" ? toInt(uncleHeaderObj.at("gasUsed")) : uncleHeader.gasUsed(),
			overwrite == "timestamp" ? toInt(uncleHeaderObj.at("timestamp")) : uncleHeader.timestamp(),
			uncleHeader.extraData());

		if (overwrite == "parentHashIsBlocksParent")
			uncleHeader.populateFromParent(importedBlocks.at(importedBlocks.size() - 1).getBlockHeader());
	}

	dev::eth::mine(uncleHeader);
	uncleHeader.noteDirty();

	if (overwrite == "nonce")
		updateEthashSeal(uncleHeader, uncleHeader.mixHash(), Nonce(uncleHeaderObj["nonce"].get_str()));

	if (overwrite == "mixHash")
		updateEthashSeal(uncleHeader, h256(uncleHeaderObj["mixHash"].get_str()), uncleHeader.nonce());

	uncle.setBlockHeader(uncleHeader);
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

mArray writeTransactionsToJson(TransactionQueue const& _txsQueue)
{
	Transactions const& txs = _txsQueue.topTransactions(std::numeric_limits<unsigned>::max());
	mArray txArray;
	for (auto const& txi: txs)
	{
		mObject txObject = fillJsonWithTransaction(txi);
		txArray.push_back(txObject);
	}
	return txArray;
}

mObject& writeBlockHeaderToJson2(mObject& _o, Ethash::BlockHeader const& _bi)
{
	_o["parentHash"] = toString(_bi.parentHash());
	_o["uncleHash"] = toString(_bi.sha3Uncles());
	_o["coinbase"] = toString(_bi.beneficiary());
	_o["stateRoot"] = toString(_bi.stateRoot());
	_o["transactionsTrie"] = toString(_bi.transactionsRoot());
	_o["receiptTrie"] = toString(_bi.receiptsRoot());
	_o["bloom"] = toString(_bi.logBloom());
	_o["difficulty"] = toCompactHex(_bi.difficulty(), HexPrefix::Add, 1);
	_o["number"] = toCompactHex(_bi.number(), HexPrefix::Add, 1);
	_o["gasLimit"] = toCompactHex(_bi.gasLimit(), HexPrefix::Add, 1);
	_o["gasUsed"] = toCompactHex(_bi.gasUsed(), HexPrefix::Add, 1);
	_o["timestamp"] = toCompactHex(_bi.timestamp(), HexPrefix::Add, 1);
	_o["extraData"] = toHex(_bi.extraData(), 2, HexPrefix::Add);
	_o["mixHash"] = toString(_bi.mixHash());
	_o["nonce"] = toString(_bi.nonce());
	_o["hash"] = toString(_bi.hash());
	return _o;
}
}}//namespaces


BOOST_AUTO_TEST_SUITE(BlockChainTests)

BOOST_AUTO_TEST_CASE(bc2)
{
	dev::test::executeTests("bcValidBlockTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests2);
}

BOOST_AUTO_TEST_SUITE_END()
