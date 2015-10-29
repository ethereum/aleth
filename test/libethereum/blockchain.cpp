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
/** @file blockchain.cpp
 * @author Christoph Jentzsch <cj@ethdev.com>, Dimitry Khokhlov <dimitry@ethdev.com>
 * @date 2015
 * BlockChain test functions.
 */

#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <libdevcore/FileSystem.h>
#include <libethcore/Params.h>

#include <test/TestHelper.h>
#include <test/BlockChainHelper.h>
#include <test/JsonSpiritHeaders.h>
#include "test/fuzzTesting/fuzzHelper.h"

using namespace std;
using namespace json_spirit;
using namespace dev;
using namespace dev::eth;

namespace dev {  namespace test {

//Functions that working with test json
void compareBlocks(TestBlock const& _a, TestBlock const& _b);
mArray writeTransactionsToJson(TransactionQueue const& _txsQueue);
mObject writeBlockHeaderToJson(Ethash::BlockHeader const& _bi);
void overwriteBlockHeaderForTest(mObject const& _blObj, TestBlock& _block, vector<TestBlock> const& importedBlocks, RecalcBlockHeader _verification);
void overwriteUncleHeaderForTest(mObject& _uncleHeaderObj, TestBlock& _uncle, vector<TestBlock> const& _uncles, vector<TestBlock> const& _importedBlocks);
void eraseJsonSectionForInvalidBlock(mObject& _blObj);
void checkJsonSectionForInvalidBlock(mObject& _blObj);
void checkExpectedException(mObject& _blObj, Exception const& _e);
void checkBlocks(TestBlock const& _blockFromFields, TestBlock const& _blockFromRlp, string const& _testname);
struct chainBranch
{
	chainBranch(TestBlock const& _genesis):
		blockchain(_genesis) { importedBlocks.push_back(_genesis); }
	void reset() { blockchain.reset(importedBlocks.at(0)); }
	void restoreFromHistory(size_t _importBlockNumber)
	{
		//Restore blockchain up to block.number to start new block mining after specific block
		for (size_t i = 1; i < importedBlocks.size(); i++) //0 block is genesis
			if (i < _importBlockNumber)
				blockchain.addBlock(importedBlocks.at(i));
			else
				break;

		//Drop old blocks as we would construct new history
		size_t originalSize = importedBlocks.size();
		for (size_t i = _importBlockNumber; i < originalSize; i++)
			importedBlocks.pop_back();
	}
	TestBlockChain blockchain;
	vector<TestBlock> importedBlocks;
};

void doBlockchainTests(json_spirit::mValue& _v, bool _fillin)
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

		TestBlock genesisBlock(o["genesisBlockHeader"].get_obj(), o["pre"].get_obj(), RecalcBlockHeader::Verify);
		if (_fillin)
			genesisBlock.setBlockHeader(genesisBlock.getBlockHeader(), RecalcBlockHeader::UpdateAndVerify); //update PoW
		TestBlockChain trueBc(genesisBlock);

		if (_fillin)
		{
			o["genesisBlockHeader"] = writeBlockHeaderToJson(genesisBlock.getBlockHeader());
			o["genesisRLP"] = toHex(genesisBlock.getBytes(), 2, HexPrefix::Add);
			BOOST_REQUIRE(o.count("blocks"));

			mArray blArray;
			size_t importBlockNumber = 0;
			string chainname = "default";
			std::map<string, chainBranch*> chainMap = { {chainname , new chainBranch(genesisBlock)}};

			for (auto const& bl: o["blocks"].get_array())
			{
				mObject blObj = bl.get_obj();
				if (blObj.count("blocknumber") > 0)
					importBlockNumber = max((int)toInt(blObj["blocknumber"]), 1);
				else
					importBlockNumber++;

				if (blObj.count("chainname") > 0)
					chainname = blObj["chainname"].get_str();
				else
					chainname = "default";

				if (chainMap.count(chainname) > 0)
				{
					if (o.count("noBlockChainHistory") == 0)
					{
						chainMap[chainname]->reset();
						chainMap[chainname]->restoreFromHistory(importBlockNumber);
					}
				}
				else
					chainMap[chainname] = new chainBranch(genesisBlock);

				TestBlock block;
				TestBlockChain& blockchain = chainMap[chainname]->blockchain;
				vector<TestBlock>& importedBlocks = chainMap[chainname]->importedBlocks;

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
					string uncleChainName = chainname;
					if (uncleHeaderObj.count("chainname") > 0)
						uncleChainName = uncleHeaderObj["chainname"].get_str();

					vector<TestBlock>& importedBlocksForUncle = chainMap[uncleChainName]->importedBlocks;
					overwriteUncleHeaderForTest(uncleHeaderObj, uncle, block.getUncles(), importedBlocksForUncle);
					block.addUncle(uncle);
				}

				vector<TestBlock> validUncles = blockchain.syncUncles(block.getUncles());
				block.setUncles(validUncles);

				//read premining parameters
				if (blObj.count("blockHeaderPremine"))
				{
					overwriteBlockHeaderForTest(blObj.at("blockHeaderPremine").get_obj(), block, importedBlocks, RecalcBlockHeader::SkipVerify);
					blObj.erase("blockHeaderPremine");
				}

				cnote << "Mining block at test " << testname;
				block.mine(blockchain);

				TestBlock alterBlock(block);

				if (blObj.count("blockHeader"))
					overwriteBlockHeaderForTest(blObj.at("blockHeader").get_obj(), alterBlock, importedBlocks, RecalcBlockHeader::Verify);

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
					if (test::Options::get().checkState == true)
						BOOST_REQUIRE_MESSAGE(blObj.count("expectException") == 0, "block import expected exception, but no exeption was thrown!");
					if (o.count("noBlockChainHistory") == 0)
					{
						importedBlocks.push_back(alterBlock);
						importedBlocks.at(importedBlocks.size()-1).clearState(); //close the state as it wont be needed. too many open states would lead to exception.
					}
				}
				catch (Exception const& _e)
				{
					cnote << testname + "block import throw an exception: " << diagnostic_information(_e);
					checkExpectedException(blObj, _e);
					eraseJsonSectionForInvalidBlock(blObj);
				}
				catch (std::exception const& _e)
				{
					cnote << testname + "block import throw an exception: " << _e.what();
					cout << testname + "block import thrown std exeption" << std::endl;
					eraseJsonSectionForInvalidBlock(blObj);
				}
				catch (...)
				{
					cout << testname + "block import thrown unknown exeption" << std::endl;
					eraseJsonSectionForInvalidBlock(blObj);
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

			for (auto iterator = chainMap.begin(); iterator != chainMap.end(); iterator++)
				delete iterator->second;

		}//fillin
		else
		{
			TestBlockChain blockchain(genesisBlock);
			for (auto const& bl: o["blocks"].get_array())
			{
				mObject blObj = bl.get_obj();
				TestBlock blockFromRlp;
				try
				{
					TestBlock blRlp(blObj["rlp"].get_str());
					blockFromRlp = blRlp;
					trueBc.addBlock(blRlp);
				}
				// if exception is thrown, RLP is invalid and no blockHeader, Transaction list, or Uncle list should be given
				catch (Exception const& _e)
				{
					cnote << testname + "state sync or block import did throw an exception: " << diagnostic_information(_e);
					checkJsonSectionForInvalidBlock(blObj);
					continue;
				}
				catch (std::exception const& _e)
				{
					cnote << testname + "state sync or block import did throw an exception: " << _e.what();
					checkJsonSectionForInvalidBlock(blObj);
					continue;
				}
				catch (...)
				{
					cnote << testname + "state sync or block import did throw an exception\n";
					checkJsonSectionForInvalidBlock(blObj);
					continue;
				}

				//block from RLP successfully imported. now compare this rlp to test sections
				BOOST_REQUIRE(blObj.count("blockHeader"));

				//Check Provided Header against block in RLP
				mObject emptyState;
				TestBlock blockFromFields(blObj["blockHeader"].get_obj(), emptyState, RecalcBlockHeader::SkipVerify); //not verify as we havent imported transactions and uncles yet

				//ImportTransactions
				BOOST_REQUIRE(blObj.count("transactions"));
				for (auto const& txObj: blObj["transactions"].get_array())
				{
					TestTransaction transaction(txObj.get_obj());
					blockFromFields.addTransaction(transaction);
				}

				// ImportUncles
				if (blObj["uncleHeaders"].type() != json_spirit::null_type)
					for (auto const& uBlHeaderObj: blObj["uncleHeaders"].get_array())
					{
						mObject uBlH = uBlHeaderObj.get_obj();
						BOOST_REQUIRE((uBlH.size() == 16));

						mObject emptyState;
						TestBlock uncle(uBlH, emptyState, RecalcBlockHeader::Verify);
						blockFromFields.addUncle(uncle);
					}

				checkBlocks(blockFromFields, blockFromRlp, testname);

				try
				{
					blockFromFields.setBlockHeader(blockFromFields.getBlockHeader(), RecalcBlockHeader::Verify); //recalculateBytes
					blockchain.addBlock(blockFromFields);
				}
				catch (Exception const& _e)
				{
					cerr << testname + "Error importing block from fields to blockchain: " << diagnostic_information(_e);
					break;
				}

				//Check that imported block to the chain is equal to declared block from test
				bytes importedblock = trueBc.getInterface().block(blockFromFields.getBlockHeader().hash());
				TestBlock inchainBlock(toHex(importedblock));
				checkBlocks(inchainBlock, blockFromFields, testname);

				//Check that trueBc is rearanged correctrly after importing this block
				string blockNumber;
				string blockChainName = "default";
				if (blObj.count("chainname") > 0)
					blockChainName = blObj["chainname"].get_str();
				if (blObj.count("blocknumber") > 0)
					blockNumber = blObj["blocknumber"].get_str();

				cnote << "Tested topblock number" << blockNumber << "for chain " << blockChainName << testname;

			}//allBlocks

			//Check lastblock hash
			BOOST_REQUIRE((o.count("lastblockhash") > 0));
			string lastTrueBlockHash = toString(trueBc.getTopBlock().getBlockHeader().hash());
			BOOST_CHECK_MESSAGE(lastTrueBlockHash == o["lastblockhash"].get_str(),
					testname + "Boost check: lastblockhash does not match " + lastTrueBlockHash + " expected: " + o["lastblockhash"].get_str());

			//Check final state (just to be sure)
			BOOST_CHECK_MESSAGE(toString(trueBc.getTopBlock().getState().rootHash()) ==
								toString(blockchain.getTopBlock().getState().rootHash()),
								testname + "State root in chain from RLP blocks != State root in chain from Field blocks!");

			State postState; //Compare post states
			BOOST_REQUIRE((o.count("postState") > 0));
			ImportTest::importState(o["postState"].get_obj(), postState);
			ImportTest::compareStates(postState, trueBc.getTopBlock().getState());
			ImportTest::compareStates(postState, blockchain.getTopBlock().getState());
		}
	}//for tests
}

//TestFunction
void overwriteBlockHeaderForTest(mObject const& _blObj, TestBlock& _block, std::vector<TestBlock> const& _importedBlocks, RecalcBlockHeader _verification)
{
	//_blObj  - json object with header data
	//_block  - which header would be overwritten
	//_parentHeader - parent blockheader

	RecalcBlockHeader findNewValidNonce = _verification;
	Ethash::BlockHeader tmp;
	Ethash::BlockHeader const& header = _block.getBlockHeader();
	auto ho = _blObj;
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

		//Set block to update this parameters before mining the actual block
		_block.setPremine(ho.count("parentHash") ? "parentHash" : "");
		_block.setPremine(ho.count("uncleHash") ? "uncleHash" : "");
		_block.setPremine(ho.count("coinbase") ? "coinbase" : "");
		_block.setPremine(ho.count("stateRoot") ? "stateRoot" : "");
		_block.setPremine(ho.count("transactionsTrie") ? "transactionsTrie" : "");
		_block.setPremine(ho.count("receiptTrie") ? "receiptTrie" : "");
		_block.setPremine(ho.count("bloom") ? "bloom" : "");
		_block.setPremine(ho.count("difficulty") ? "difficulty" : "");
		_block.setPremine(ho.count("number") ? "number" : "");
		_block.setPremine(ho.count("gasLimit") ? "gasLimit" : "");
		_block.setPremine(ho.count("gasUsed") ? "gasUsed" : "");
		_block.setPremine(ho.count("timestamp") ? "timestamp" : "");
		_block.setPremine(ho.count("extraData") ? "extraData" : "");

		if (ho.count("RelTimestamp"))
		{
			Ethash::BlockHeader parentHeader = _importedBlocks.at(_importedBlocks.size() - 1).getBlockHeader();
			tmp.setTimestamp(toInt(ho["RelTimestamp"]) + parentHeader.timestamp());
			tmp.setDifficulty(tmp.calculateDifficulty(parentHeader));
			this_thread::sleep_for(chrono::seconds((int)toInt(ho["RelTimestamp"])));
		}

		// find new valid nonce
		if (static_cast<BlockInfo>(tmp) != static_cast<BlockInfo>(header) && tmp.difficulty())
			findNewValidNonce = RecalcBlockHeader::Update;

		if (ho.count("updatePoW"))
			findNewValidNonce = RecalcBlockHeader::UpdateAndVerify;

		tmp.setNonce(header.nonce());
		tmp.setMixHash(header.mixHash());

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
		tmp = TestBlock(ho, emptyState, RecalcBlockHeader::SkipVerify).getBlockHeader();
	}

	if (ho.count("populateFromBlock"))
	{
		size_t number = (size_t)toInt(ho.at("populateFromBlock"));
		ho.erase("populateFromBlock");
		if (number < _importedBlocks.size())
		{
			Ethash::BlockHeader parentHeader = _importedBlocks.at(number).getBlockHeader();
			tmp.populateFromParent(parentHeader);
			findNewValidNonce = RecalcBlockHeader::UpdateAndVerify;
		}
		else
		{
			cerr << TestOutputHelper::testName() + "Could not populate blockHeader from block: there are no block with number!" << TestOutputHelper::testName() << endl;
		}
	}

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
			cerr << TestOutputHelper::testName() + "Could not create uncle sameAsPreviousSibling: there are no siblings!";
		return;
	}

	if (uncleHeaderObj.count("sameAsBlock"))
	{
		size_t number = (size_t)toInt(uncleHeaderObj.at("sameAsBlock"));
		uncleHeaderObj.erase("sameAsBlock");

		if (number < importedBlocks.size())
			uncle = importedBlocks.at(number);
		else
			cerr << TestOutputHelper::testName() + "Could not create uncle sameAsBlock: there are no block with number " << number;
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
				cerr << TestOutputHelper::testName() + "Could not create uncle sameAsPreviousBlockUncle: previous block has no uncles!" << TestOutputHelper::testName() << endl;
		}
		else
			cerr << TestOutputHelper::testName() + "Could not create uncle sameAsPreviousBlockUncle: there are no block imported!" << TestOutputHelper::testName() << endl;
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
			cerr << TestOutputHelper::testName() + "Could not create uncle populateFromBlock: there are no block with number " << number << TestOutputHelper::testName() << endl;
	}
	else
	{
		mObject emptyState;
		uncle = TestBlock(uncleHeaderObj, emptyState, RecalcBlockHeader::SkipVerify);
		uncleHeader = uncle.getBlockHeader();
	}

	if (overwrite != "false")
	{
		uncleHeader = constructHeader(
			overwrite == "parentHash" ? h256(uncleHeaderObj.at("parentHash").get_str()) : uncleHeader.parentHash(),
			uncleHeader.sha3Uncles(),
			overwrite == "coinbase" ? Address(uncleHeaderObj.at("coinbase").get_str()) : uncleHeader.beneficiary(),
			overwrite == "stateRoot" ? h256(uncleHeaderObj.at("stateRoot").get_str()) : uncleHeader.stateRoot(),
			uncleHeader.transactionsRoot(),
			uncleHeader.receiptsRoot(),
			uncleHeader.logBloom(),
			overwrite == "difficulty" ? toInt(uncleHeaderObj.at("difficulty"))
									  :	overwrite == "timestamp" ? uncleHeader.calculateDifficulty(importedBlocks.at((size_t)uncleHeader.number() - 1).getBlockHeader())
																 : uncleHeader.difficulty(),
			overwrite == "number" ? toInt(uncleHeaderObj.at("number")) : uncleHeader.number(),
			overwrite == "gasLimit" ? toInt(uncleHeaderObj.at("gasLimit")) : uncleHeader.gasLimit(),
			overwrite == "gasUsed" ? toInt(uncleHeaderObj.at("gasUsed")) : uncleHeader.gasUsed(),
			overwrite == "timestamp" ? toInt(uncleHeaderObj.at("timestamp")) : uncleHeader.timestamp(),
			uncleHeader.extraData());
	}

	if (overwrite == "nonce" || overwrite == "mixHash")
	{
		if (overwrite == "nonce")
			updateEthashSeal(uncleHeader, uncleHeader.mixHash(), Nonce(uncleHeaderObj["nonce"].get_str()));

		if (overwrite == "mixHash")
			updateEthashSeal(uncleHeader, h256(uncleHeaderObj["mixHash"].get_str()), uncleHeader.nonce());

		uncle.setBlockHeader(uncleHeader, RecalcBlockHeader::Verify);
	}
	else
		uncle.setBlockHeader(uncleHeader, RecalcBlockHeader::UpdateAndVerify);
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

void checkExpectedException(mObject& _blObj, Exception const& _e)
{
	if (!test::Options::get().checkState)
		return;

	BOOST_REQUIRE_MESSAGE(_blObj.count("expectException") > 0, TestOutputHelper::testName() + "block import thrown unexpected Excpetion!");
	string exWhat {	_e.what() };
	string exExpect = _blObj.at("expectException").get_str();

	BOOST_REQUIRE_MESSAGE(exWhat.find(exExpect) != string::npos, TestOutputHelper::testName() + "block import expected another exeption: " + exExpect);
	_blObj.erase(_blObj.find("expectException"));
}

void checkJsonSectionForInvalidBlock(mObject& _blObj)
{
	BOOST_CHECK(_blObj.count("blockHeader") == 0);
	BOOST_CHECK(_blObj.count("transactions") == 0);
	BOOST_CHECK(_blObj.count("uncleHeaders") == 0);
}

void eraseJsonSectionForInvalidBlock(mObject& _blObj)
{
	// if exception is thrown, RLP is invalid and no blockHeader, Transaction list, or Uncle list should be given
	cnote << TestOutputHelper::testName() + "block is invalid!\n";
	_blObj.erase(_blObj.find("blockHeader"));
	_blObj.erase(_blObj.find("uncleHeaders"));
	_blObj.erase(_blObj.find("transactions"));
}

void checkBlocks(TestBlock const& _blockFromFields, TestBlock const& _blockFromRlp, string const& _testname)
{
	Ethash::BlockHeader const& blockHeaderFromFields = _blockFromFields.getBlockHeader();
	Ethash::BlockHeader const& blockFromRlp = _blockFromRlp.getBlockHeader();

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

	vector<TestBlock> const& unclesFromField = _blockFromFields.getUncles();
	vector<TestBlock> const& unclesFromRlp = _blockFromRlp.getUncles();
	BOOST_REQUIRE_EQUAL(unclesFromField.size(), unclesFromRlp.size());
	for (size_t i = 0; i < unclesFromField.size(); ++i)
		BOOST_CHECK_MESSAGE(unclesFromField.at(i).getBlockHeader() == unclesFromRlp.at(i).getBlockHeader(), _testname + "block header in rlp and in field do not match at uncles");
}

}}//namespaces

BOOST_AUTO_TEST_SUITE(BlockChainTests)

BOOST_AUTO_TEST_CASE(bcForkBlockTest)
{
	if (!dev::test::Options::get().fillTests)
		dev::test::executeTests("bcForkBlockTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests);
}

BOOST_AUTO_TEST_CASE(bcForkUncleTest)
{
	if (!dev::test::Options::get().fillTests)
		dev::test::executeTests("bcForkUncle", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests);
}

BOOST_AUTO_TEST_CASE(bcForkStressTest)
{
	dev::test::executeTests("bcForkStressTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests);
}

BOOST_AUTO_TEST_CASE(bcMultiChainTest)
{
	dev::test::executeTests("bcMultiChainTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests);
}

BOOST_AUTO_TEST_CASE(bcTotalDifficultyTest)
{
	dev::test::executeTests("bcTotalDifficultyTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests);
}

BOOST_AUTO_TEST_CASE(bcInvalidRLPTest)
{
	if (!dev::test::Options::get().fillTests)
		dev::test::executeTests("bcInvalidRLPTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests);
}

BOOST_AUTO_TEST_CASE(bcRPC_API_Test)
{
	dev::test::executeTests("bcRPC_API_Test", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests);
}

BOOST_AUTO_TEST_CASE(bcValidBlockTest)
{
	dev::test::executeTests("bcValidBlockTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests);
}

BOOST_AUTO_TEST_CASE(bcInvalidHeaderTest)
{
	dev::test::executeTests("bcInvalidHeaderTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests);
}

BOOST_AUTO_TEST_CASE(bcUncleTest)
{
	dev::test::executeTests("bcUncleTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests);
}

BOOST_AUTO_TEST_CASE(bcUncleHeaderValiditiy)
{
	dev::test::executeTests("bcUncleHeaderValiditiy", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests);
}

BOOST_AUTO_TEST_CASE(bcGasPricerTest)
{
	dev::test::executeTests("bcGasPricerTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests);
}

//BOOST_AUTO_TEST_CASE(bcBruncleTest)
//{
//	if (c_network != Network::Frontier)
//		dev::test::executeTests("bcBruncleTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests);
//}

BOOST_AUTO_TEST_CASE(bcBlockGasLimitTest)
{
	dev::test::executeTests("bcBlockGasLimitTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests);
}

BOOST_AUTO_TEST_CASE(bcWalletTest)
{
	if (test::Options::get().wallet)
		dev::test::executeTests("bcWalletTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests);
}

BOOST_AUTO_TEST_CASE(userDefinedFile)
{
	dev::test::userDefinedTest(dev::test::doBlockchainTests);
}

BOOST_AUTO_TEST_CASE(bcStateTest2)
{
	dev::test::executeTests("bcStateTest2", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests);
}

BOOST_AUTO_TEST_SUITE_END()
