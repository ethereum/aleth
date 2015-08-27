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
	recalcBlockHeaderBytes(false);
}

TestBlock::TestBlock(std::string const& _blockRlp)
{		
	m_bytes = importByteArray(_blockRlp);

	RLP root(m_bytes);
	m_blockHeader.populateFromHeader(root[0], IgnoreSeal);

	m_transactionQueue.clear();
	m_testTransactions.clear();
	for (auto const& tr: root[1])
	{
		Transaction tx(tr.data(), CheckTransaction::Everything);
		TestTransaction testTx(tx);
		m_transactionQueue.import(tx.rlp());
		m_testTransactions.push_back(testTx);
	}

	for	(auto const& uRLP: root[2])
	{
		BlockHeader uBl;
		uBl.populateFromHeader(uRLP);
		TestBlock uncle;
		uncle.setBlockHeader(uBl, false);
		m_uncles.push_back(uncle);
	}

	//copyStateFrom(_original.getState());
}

void TestBlock::setBlockHeader(json_spirit::mObject& _blockHeader)
{
	try
	{
		const bytes c_rlpBytesBlockHeader = createBlockRLPFromFields(_blockHeader);
		const RLP c_blockHeaderRLP(c_rlpBytesBlockHeader);
		m_blockHeader.populateFromHeader(c_blockHeaderRLP, IgnoreSeal);
	}
	catch(...)
	{
		TBOOST_ERROR("invalid block header json!");
	}
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

void TestBlock::setUncles(vector<TestBlock> const& _uncles)
{
	m_uncles.clear();
	m_uncles = _uncles;
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
		//block.sync(blockchain);
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

void TestBlock::setBlockHeader(Ethash::BlockHeader const& _header, bool _recalculate)
{
	m_blockHeader = _header;
	recalcBlockHeaderBytes(_recalculate);
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

bytes TestBlock::createBlockRLPFromFields(mObject const& _tObj, h256 const& _stateRoot)
{
	RLPStream rlpStream;
	rlpStream.appendList(_tObj.count("hash") > 0 ? (_tObj.size() - 1) : _tObj.size());

	if (_tObj.count("parentHash"))
		rlpStream << importByteArray(_tObj.at("parentHash").get_str());

	if (_tObj.count("uncleHash"))
		rlpStream << importByteArray(_tObj.at("uncleHash").get_str());

	if (_tObj.count("coinbase"))
		rlpStream << importByteArray(_tObj.at("coinbase").get_str());

	if (_stateRoot)
		rlpStream << _stateRoot;
	else if (_tObj.count("stateRoot"))
		rlpStream << importByteArray(_tObj.at("stateRoot").get_str());

	if (_tObj.count("transactionsTrie"))
		rlpStream << importByteArray(_tObj.at("transactionsTrie").get_str());

	if (_tObj.count("receiptTrie"))
		rlpStream << importByteArray(_tObj.at("receiptTrie").get_str());

	if (_tObj.count("bloom"))
		rlpStream << importByteArray(_tObj.at("bloom").get_str());

	if (_tObj.count("difficulty"))
		rlpStream << bigint(_tObj.at("difficulty").get_str());

	if (_tObj.count("number"))
		rlpStream << bigint(_tObj.at("number").get_str());

	if (_tObj.count("gasLimit"))
		rlpStream << bigint(_tObj.at("gasLimit").get_str());

	if (_tObj.count("gasUsed"))
		rlpStream << bigint(_tObj.at("gasUsed").get_str());

	if (_tObj.count("timestamp"))
		rlpStream << bigint(_tObj.at("timestamp").get_str());

	if (_tObj.count("extraData"))
		rlpStream << fromHex(_tObj.at("extraData").get_str());

	if (_tObj.count("mixHash"))
		rlpStream << importByteArray(_tObj.at("mixHash").get_str());

	if (_tObj.count("nonce"))
		rlpStream << importByteArray(_tObj.at("nonce").get_str());

	return rlpStream.out();
}

//Form bytestream of a block with [header transactions uncles]
void TestBlock::recalcBlockHeaderBytes(bool _recalculate)
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

	if (_recalculate)
	{
		if (m_uncles.size()) // update unclehash in case of invalid uncles
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

	m_uncles = _original.getUncles();
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

vector<TestBlock> TestBlockChain::syncUncles(vector<TestBlock> const& uncles)
{
	BlockQueue uncleBlockQueue;
	vector<TestBlock> validUncles;
	FullBlockChain<Ethash>& blockchain = *m_blockChain.get();
	uncleBlockQueue.setChain(blockchain);

	for (size_t i = 0; i < uncles.size(); i++)
	{
		try
		{
			uncleBlockQueue.import(&uncles.at(i).getBytes(), false);
			this_thread::sleep_for(chrono::seconds(1)); // wait until block is verified
			validUncles.push_back(uncles.at(i));
		}
		catch(...)
		{
			cnote << "error in importing uncle! This produces an invalid block (May be by purpose for testing).";
		}
	}

	blockchain.sync(uncleBlockQueue, m_genesisBlock.getState().db(), (unsigned)4);
	return validUncles;
}

//Functions that working with test json
void compareBlocks(TestBlock const& _a, TestBlock const& _b);
mArray writeTransactionsToJson(TransactionQueue const& _txsQueue);
mObject writeBlockHeaderToJson(Ethash::BlockHeader const& _bi);
void overwriteBlockHeaderForTest(mObject const& _blObj, TestBlock& _block, Ethash::BlockHeader const& _parentHeader);
void overwriteUncleHeaderForTest(mObject& _uncleHeaderObj, TestBlock& _uncle, std::vector<TestBlock> const& _uncles, std::vector<TestBlock> const& _importedBlocks);
void checkJsonSectionForInvalidBlock(mObject& _blObj);
void checkBlocks(TestBlock const& _blockFromFields, TestBlock const& _blockFromRlp, string const& _testname);

void doBlockchainTests2(json_spirit::mValue& _v, bool _fillin)
{
	TestOutputHelper::initTest(_v);
	for (auto& i: _v.get_obj())
	{
		string testname = i.first;
		json_spirit::mObject& o = i.second.get_obj();

		if (!TestOutputHelper::passTest(o, testname))
			continue;

		BOOST_REQUIRE(o.count("genesisBlockHeader"));
		BOOST_REQUIRE(o.count("pre"));

		TestBlock genesisBlock(o["genesisBlockHeader"].get_obj(), o["pre"].get_obj());
		if (_fillin)
			genesisBlock.setBlockHeader(genesisBlock.getBlockHeader(), true); //update PoW
		TestBlockChain trueBc(genesisBlock);

		if (_fillin)
		{
			o["genesisBlockHeader"] = writeBlockHeaderToJson(genesisBlock.getBlockHeader());
			o["genesisRLP"] = toHex(genesisBlock.getBytes(), 2, HexPrefix::Add);
			BOOST_REQUIRE(o.count("blocks"));

			mArray blArray;
			size_t importBlockNumber = 0;
			vector<TestBlock> importedBlocks;
			importedBlocks.push_back(genesisBlock);

			for (auto const& bl: o["blocks"].get_array())
			{
				TestBlockChain blockchain(importedBlocks.at(0));
				mObject blObj = bl.get_obj();
				if (blObj.count("blocknumber") > 0)
					importBlockNumber = max((int)toInt(blObj["blocknumber"]), 1);
				else
					importBlockNumber++;

				//Restore blockchain up to block.number to start import from it
				for (size_t i = 1; i < importedBlocks.size(); i++) //0 block is genesis
					if (i < importBlockNumber)
						blockchain.addBlock(importedBlocks.at(i));
					else
						break;

				//Drop old blocks
				size_t originalSize = importedBlocks.size();
				for (size_t i = importBlockNumber; i < originalSize; i++)
					importedBlocks.pop_back();

				TestBlock block;

				//Import Transactions
				BOOST_REQUIRE(blObj.count("transactions"));
				for (auto const& txObj: blObj["transactions"].get_array())
				{
					TestTransaction transaction(txObj.get_obj());
					block.addTransaction(transaction);
				}

				//Import Uncles
				for (auto const& uHObj: blObj.at("uncleHeaders").get_array())
				{					
					TestBlock uncle;
					mObject uncleHeaderObj = uHObj.get_obj();
					overwriteUncleHeaderForTest(uncleHeaderObj, uncle, block.getUncles(), importedBlocks);
					block.addUncle(uncle);
				}

				vector<TestBlock> validUncles = blockchain.syncUncles(block.getUncles());
				block.setUncles(validUncles);
				block.mine(blockchain);

				TestBlock alterBlock(block);

				if (blObj.count("blockHeader"))
					overwriteBlockHeaderForTest(blObj, alterBlock, blockchain.getInterface().info());

				blObj["rlp"] = toHex(alterBlock.getBytes(), 2, HexPrefix::Add);
				blObj["blockHeader"] = writeBlockHeaderToJson(alterBlock.getBlockHeader());

				mArray aUncleList;
				for (size_t i = 0; i < alterBlock.getUncles().size(); i++)
				{
					mObject uncleHeaderObj = writeBlockHeaderToJson(alterBlock.getUncles().at(i).getBlockHeader());
					aUncleList.push_back(uncleHeaderObj);
				}
				blObj["uncleHeaders"] = aUncleList;

				blObj["transactions"] = writeTransactionsToJson(alterBlock.getTransactionQueue());

				compareBlocks(block, alterBlock);
				try
				{
					blockchain.addBlock(alterBlock);
					trueBc.addBlock(alterBlock);
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
		else
		{
			TestBlockChain blockchain(genesisBlock);
			for (auto const& bl: o["blocks"].get_array())
			{
				bool importedAndBest = true;
				mObject blObj = bl.get_obj();
				TestBlock blockFromRlp(blObj["rlp"].get_str());
				try
				{
					trueBc.addBlock(blockFromRlp);
					if (trueBc.getTopBlock().getBytes() != blockFromRlp.getBytes())
						importedAndBest  = false;
				}
				// if exception is thrown, RLP is invalid and no blockHeader, Transaction list, or Uncle list should be given
				catch (Exception const& _e)
				{
					cnote << "state sync or block import did throw an exception: " << diagnostic_information(_e);
					checkJsonSectionForInvalidBlock(blObj);
					continue;
				}
				catch (std::exception const& _e)
				{
					cnote << "state sync or block import did throw an exception: " << _e.what();
					checkJsonSectionForInvalidBlock(blObj);
					continue;
				}
				catch (...)
				{
					cnote << "state sync or block import did throw an exception\n";
					checkJsonSectionForInvalidBlock(blObj);
					continue;
				}

				//block from RLP successfully imported. now compare this rlp to test sections
				BOOST_REQUIRE(blObj.count("blockHeader"));

				//Check the fields restored from RLP to original fields
				if (importedAndBest)
				{
					//Check Provided Header against block in RLP
					TestBlock blockFromFields;
					blockFromFields.setBlockHeader(blObj["blockHeader"].get_obj());

					//ImportTransactions
					BOOST_REQUIRE(blObj.count("transactions"));
					for (auto const& txObj: blObj["transactions"].get_array())
					{
						try
						{
							TestTransaction transaction(txObj.get_obj());
							blockFromFields.addTransaction(transaction);
						}
						catch (Exception const& _e)
						{
							TBOOST_ERROR("Failed transaction constructor with Exception: " << diagnostic_information(_e));
						}
						catch (exception const& _e)
						{
							cnote << _e.what();
						}
					}

					// ImportUncles
					if (blObj["uncleHeaders"].type() != json_spirit::null_type)
						for (auto const& uBlHeaderObj: blObj["uncleHeaders"].get_array())
						{
							mObject uBlH = uBlHeaderObj.get_obj();
							BOOST_REQUIRE((uBlH.size() == 16));

							TestBlock uncle;
							uncle.setBlockHeader(blObj["blockHeader"].get_obj());
							blockFromFields.addUncle(uncle);
						}

					checkBlocks(blockFromFields, blockFromRlp, testname);

					blockFromFields.setBlockHeader(blockFromFields.getBlockHeader(), false); //recalculateBytes
					blockchain.addBlock(blockFromFields);
				}//importedAndBest
			}//allBlocks

			BOOST_REQUIRE((o.count("lastblockhash") > 0));
			string lastTrueBlockHash = toString(trueBc.getTopBlock().getBlockHeader().hash());
			BOOST_CHECK_MESSAGE(lastTrueBlockHash == o["lastblockhash"].get_str(),
					testname + "Boost check: lastblockhash does not match " + lastTrueBlockHash + " expected: " + o["lastblockhash"].get_str());

			//BOOST_CHECK_MESSAGE(toString(blockchain.getTopBlock().getState().rootHash()) == lastTrueBlockHash, testname + "State root in chain from RLP blocks != State root in chain from Field blocks!");
		}
	}//for tests
}

//TestFunction
void overwriteBlockHeaderForTest(mObject const& _blObj, TestBlock& _block, Ethash::BlockHeader const& _parentHeader)
{
	//_blObj  - json object with header data
	//_block  - which header would be overwritten
	//_parentHeader - parent blockheader

	bool findNewValidNonce = false;
	Ethash::BlockHeader tmp;
	Ethash::BlockHeader const& header = _block.getBlockHeader();
	auto ho = _blObj.at("blockHeader").get_obj();
	if (ho.size() != 14)
	{
		tmp = constructHeader(
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
			findNewValidNonce = true;

		if (ho.count("updatePoW"))
			findNewValidNonce = true;

		if (ho.count("mixHash"))
			updateEthashSeal(tmp, h256(ho["mixHash"].get_str()), tmp.nonce());
		if (ho.count("nonce"))
			updateEthashSeal(tmp, tmp.mixHash(), Nonce(ho["nonce"].get_str()));

		tmp.noteDirty();
	}
	else
	{
		// take the blockheader as is
		mObject emptyState;
		tmp = TestBlock(ho, emptyState).getBlockHeader();
	}

	if (_blObj.at("blockHeader").get_obj().count("bruncle"))
		tmp.populateFromParent(_parentHeader); //importedBlocks.at(importedBlocks.size() -1).getBlockHeader()

	tmp.setNonce(header.nonce());
	tmp.setMixHash(header.mixHash());
	_block.setBlockHeader(tmp, findNewValidNonce);
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
		size_t number = (size_t)toInt(uncleHeaderObj.at("sameAsBlock"));
		uncleHeaderObj.erase("sameAsBlock");

		if (number < importedBlocks.size())
			uncle = importedBlocks.at(number);
		else
			cerr << "Could not create uncle sameAsBlock: there are no block with number " << number;
		return;
	}

	if (uncleHeaderObj.count("sameAsPreviousBlockUncle"))
	{		
		size_t number = (size_t)toInt(uncleHeaderObj.at("sameAsPreviousBlockUncle"));
		uncleHeaderObj.erase("sameAsPreviousBlockUncle");
		if (number < importedBlocks.size())
		{
			vector<TestBlock> prevBlockUncles = importedBlocks.at(number).getUncles();
			if (prevBlockUncles.size())
				uncle = prevBlockUncles[0];  //exact uncle??
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

	//construct actual block
	Ethash::BlockHeader uncleHeader;
	if (uncleHeaderObj.count("populateFromBlock"))
	{
		uncleHeader.setTimestamp((u256)time(0));
		size_t number = (size_t)toInt(uncleHeaderObj.at("populateFromBlock"));
		uncleHeaderObj.erase("populateFromBlock");
		if (number < importedBlocks.size())
		{
			uncleHeader.populateFromParent(importedBlocks.at(number).getBlockHeader());
			//Set Default roots for empty block
			//m_transactionsRoot = _t; m_receiptsRoot = _r; m_sha3Uncles = _u; m_stateRoot = _s;
			uncleHeader.setRoots((h256)fromHex("56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421"),
								 (h256)fromHex("56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421"),
								 (h256)fromHex("1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347"),
								 uncleHeader.stateRoot()
								 );
		}
		else
			cerr << "Could not create uncle populateFromBlock: there are no block with number " << number;
	}
	else
	{
		mObject emptyState;
		uncle = TestBlock(uncleHeaderObj, emptyState);
		uncleHeader = uncle.getBlockHeader();
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
	uncle.setBlockHeader(uncleHeader); //updatePoW

	if (overwrite == "nonce" || overwrite == "mixHash")
	{
		uncleHeader = uncle.getBlockHeader();
		if (overwrite == "nonce")
			updateEthashSeal(uncleHeader, uncleHeader.mixHash(), Nonce(uncleHeaderObj["nonce"].get_str()));

		if (overwrite == "mixHash")
			updateEthashSeal(uncleHeader, h256(uncleHeaderObj["mixHash"].get_str()), uncleHeader.nonce());

		uncle.setBlockHeader(uncleHeader, false);
	}
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

mObject writeBlockHeaderToJson(Ethash::BlockHeader const& _bi)
{
	mObject o;
	o["parentHash"] = toString(_bi.parentHash());
	o["uncleHash"] = toString(_bi.sha3Uncles());
	o["coinbase"] = toString(_bi.beneficiary());
	o["stateRoot"] = toString(_bi.stateRoot());
	o["transactionsTrie"] = toString(_bi.transactionsRoot());
	o["receiptTrie"] = toString(_bi.receiptsRoot());
	o["bloom"] = toString(_bi.logBloom());
	o["difficulty"] = toCompactHex(_bi.difficulty(), HexPrefix::Add, 1);
	o["number"] = toCompactHex(_bi.number(), HexPrefix::Add, 1);
	o["gasLimit"] = toCompactHex(_bi.gasLimit(), HexPrefix::Add, 1);
	o["gasUsed"] = toCompactHex(_bi.gasUsed(), HexPrefix::Add, 1);
	o["timestamp"] = toCompactHex(_bi.timestamp(), HexPrefix::Add, 1);
	o["extraData"] = toHex(_bi.extraData(), 2, HexPrefix::Add);
	o["mixHash"] = toString(_bi.mixHash());
	o["nonce"] = toString(_bi.nonce());
	o["hash"] = toString(_bi.hash());
	return o;
}

void checkJsonSectionForInvalidBlock(mObject& _blObj)
{
	BOOST_CHECK(_blObj.count("blockHeader") == 0);
	BOOST_CHECK(_blObj.count("transactions") == 0);
	BOOST_CHECK(_blObj.count("uncleHeaders") == 0);
}

void checkBlocks(TestBlock const& _blockFromFields, TestBlock const& _blockFromRlp, string const& _testname)
{
	Ethash::BlockHeader blockHeaderFromFields = _blockFromFields.getBlockHeader();
	Ethash::BlockHeader blockFromRlp = _blockFromRlp.getBlockHeader();

	BOOST_CHECK_MESSAGE(blockHeaderFromFields.headerHash(WithProof) == blockFromRlp.headerHash(WithProof), _testname + "hash in given RLP not matching the block hash!");
	BOOST_CHECK_MESSAGE(blockHeaderFromFields.parentHash() == blockFromRlp.parentHash(), _testname + "parentHash in given RLP not matching the block parentHash!");
	BOOST_CHECK_MESSAGE(blockHeaderFromFields.sha3Uncles() == blockFromRlp.sha3Uncles(), _testname + "sha3Uncles in given RLP not matching the block sha3Uncles!");
	BOOST_CHECK_MESSAGE(blockHeaderFromFields.beneficiary() == blockFromRlp.beneficiary(), _testname + "beneficiary in given RLP not matching the block beneficiary!");
	BOOST_CHECK_MESSAGE(blockHeaderFromFields.stateRoot() == blockFromRlp.stateRoot(), _testname + "stateRoot in given RLP not matching the block stateRoot!");
	BOOST_CHECK_MESSAGE(blockHeaderFromFields.transactionsRoot() == blockFromRlp.transactionsRoot(), _testname + "transactionsRoot in given RLP not matching the block transactionsRoot!");
	BOOST_CHECK_MESSAGE(blockHeaderFromFields.receiptsRoot() == blockFromRlp.receiptsRoot(), _testname + "receiptsRoot in given RLP not matching the block receiptsRoot!");
	BOOST_CHECK_MESSAGE(blockHeaderFromFields.logBloom() == blockFromRlp.logBloom(), _testname + "logBloom in given RLP not matching the block logBloom!");
	BOOST_CHECK_MESSAGE(blockHeaderFromFields.difficulty() == blockFromRlp.difficulty(), _testname + "difficulty in given RLP not matching the block difficulty!");
	BOOST_CHECK_MESSAGE(blockHeaderFromFields.number() == blockFromRlp.number(), _testname + "number in given RLP not matching the block number!");
	BOOST_CHECK_MESSAGE(blockHeaderFromFields.gasLimit() == blockFromRlp.gasLimit(),"testname + gasLimit in given RLP not matching the block gasLimit!");
	BOOST_CHECK_MESSAGE(blockHeaderFromFields.gasUsed() == blockFromRlp.gasUsed(), _testname + "gasUsed in given RLP not matching the block gasUsed!");
	BOOST_CHECK_MESSAGE(blockHeaderFromFields.timestamp() == blockFromRlp.timestamp(), _testname + "timestamp in given RLP not matching the block timestamp!");
	BOOST_CHECK_MESSAGE(blockHeaderFromFields.extraData() == blockFromRlp.extraData(), _testname + "extraData in given RLP not matching the block extraData!");
	BOOST_CHECK_MESSAGE(blockHeaderFromFields.mixHash() == blockFromRlp.mixHash(), _testname + "mixHash in given RLP not matching the block mixHash!");
	BOOST_CHECK_MESSAGE(blockHeaderFromFields.nonce() == blockFromRlp.nonce(), _testname + "nonce in given RLP not matching the block nonce!");

	BOOST_CHECK_MESSAGE(blockHeaderFromFields == blockFromRlp, _testname + "However, blockHeaderFromFields != blockFromRlp!");

	vector<TestTransaction> const& txsFromField = _blockFromFields.getTestTransactions();
	vector<TestTransaction> const& txsFromRlp = _blockFromRlp.getTestTransactions();
	BOOST_CHECK_MESSAGE(txsFromRlp.size() == txsFromField.size(), _testname + "transaction list size does not match");

	for (size_t i = 0; i < txsFromField.size(); ++i)
	{
		Transaction const& trField = txsFromField.at(i).getTransaction();
		Transaction const& trRlp = txsFromRlp.at(i).getTransaction();

		BOOST_CHECK_MESSAGE(trField.data() == trRlp.data(), _testname + "transaction data in rlp and in field do not match");
		BOOST_CHECK_MESSAGE(trField.gas() == trRlp.gas(), _testname + "transaction gasLimit in rlp and in field do not match");
		BOOST_CHECK_MESSAGE(trField.gasPrice() == trRlp.gasPrice(), _testname + "transaction gasPrice in rlp and in field do not match");
		BOOST_CHECK_MESSAGE(trField.nonce() == trRlp.nonce(), _testname + "transaction nonce in rlp and in field do not match");
		BOOST_CHECK_MESSAGE(trField.signature().r == trRlp.signature().r, _testname + "transaction r in rlp and in field do not match");
		BOOST_CHECK_MESSAGE(trField.signature().s == trRlp.signature().s, _testname + "transaction s in rlp and in field do not match");
		BOOST_CHECK_MESSAGE(trField.signature().v == trRlp.signature().v, _testname + "transaction v in rlp and in field do not match");
		BOOST_CHECK_MESSAGE(trField.receiveAddress() == trRlp.receiveAddress(), _testname + "transaction receiveAddress in rlp and in field do not match");
		BOOST_CHECK_MESSAGE(trField.value() == trRlp.value(), _testname + "transaction receiveAddress in rlp and in field do not match");

		BOOST_CHECK_MESSAGE(trField == trRlp, _testname + "transactions from  rlp and transaction from field do not match");
		BOOST_CHECK_MESSAGE(trField.rlp() == trRlp.rlp(), _testname + "transactions rlp do not match");
	}

	BOOST_REQUIRE_EQUAL(_blockFromFields.getUncles().size(), _blockFromRlp.getUncles().size());

	vector<TestBlock> const& unclesFromField = _blockFromFields.getUncles();
	vector<TestBlock> const& unclesFromRlp = _blockFromRlp.getUncles();
	for (size_t i = 0; i < unclesFromField.size(); ++i)
		BOOST_CHECK_MESSAGE(unclesFromField.at(i).getBlockHeader() == unclesFromRlp.at(i).getBlockHeader(), _testname + "block header in rlp and in field do not match");
}

}}//namespaces


BOOST_AUTO_TEST_SUITE(BlockChainTests)

BOOST_AUTO_TEST_CASE(bc2)
{
	dev::test::executeTests("bcValidBlockTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests2);
}

BOOST_AUTO_TEST_SUITE_END()
