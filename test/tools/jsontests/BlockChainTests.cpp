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
 * Generation of a blockchain tests. Parse blockchain test fillers.
 * Simulating block import checking the post state.
 */

#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <libdevcore/FileSystem.h>
#include <libethashseal/Ethash.h>
#include <test/tools/libtesteth/TestHelper.h>
#include <test/tools/libtesteth/BlockChainHelper.h>
#include <test/tools/libtesteth/JsonSpiritHeaders.h>
#include <test/tools/fuzzTesting/fuzzHelper.h>
using namespace std;
using namespace json_spirit;
using namespace dev;
using namespace dev::eth;


namespace dev {

namespace test {

eth::Network TestBlockChain::s_sealEngineNetwork = eth::Network::FrontierTest;

struct ChainBranch
{
	ChainBranch(TestBlock const& _genesis): blockchain(_genesis) { importedBlocks.push_back(_genesis); }
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

	static void forceBlockchain(string const& chainname)
	{
		s_tempBlockchainNetwork = dev::test::TestBlockChain::s_sealEngineNetwork;
		if (chainname == "Frontier")
			dev::test::TestBlockChain::s_sealEngineNetwork = eth::Network::FrontierTest;
		else if (chainname == "Homestead")
			dev::test::TestBlockChain::s_sealEngineNetwork = eth::Network::HomesteadTest;
		else if (chainname == "EIP150")
			dev::test::TestBlockChain::s_sealEngineNetwork = eth::Network::EIP150Test;
		else if (chainname == "TestFtoH5")
			dev::test::TestBlockChain::s_sealEngineNetwork = eth::Network::TransitionnetTest;
		else if (chainname == "Byzantium")
			dev::test::TestBlockChain::s_sealEngineNetwork = eth::Network::ByzantiumTest;
		else if (chainname == "Constantinople")
			dev::test::TestBlockChain::s_sealEngineNetwork = eth::Network::ConstantinopleTest;
	}

	static void resetBlockchain()
	{
		dev::test::TestBlockChain::s_sealEngineNetwork = s_tempBlockchainNetwork;
	}

private:
	static eth::Network s_tempBlockchainNetwork;
};

eth::Network ChainBranch::s_tempBlockchainNetwork = eth::Network::MainNetwork;

//Functions that working with test json
void compareBlocks(TestBlock const& _a, TestBlock const& _b);
mArray writeTransactionsToJson(TransactionQueue const& _txsQueue);
mObject writeBlockHeaderToJson(BlockHeader const& _bi);
void overwriteBlockHeaderForTest(mObject const& _blObj, TestBlock& _block, ChainBranch const& _chainBranch);
void overwriteUncleHeaderForTest(mObject& _uncleHeaderObj, TestBlock& _uncle, vector<TestBlock> const& _uncles, ChainBranch const& _chainBranch);
void eraseJsonSectionForInvalidBlock(mObject& _blObj);
void checkJsonSectionForInvalidBlock(mObject& _blObj);
void checkExpectedException(mObject& _blObj, Exception const& _e);
void checkBlocks(TestBlock const& _blockFromFields, TestBlock const& _blockFromRlp, string const& _testname);
bigint calculateMiningReward(u256 const& _blNumber, u256 const& _unNumber1, u256 const& _unNumber2, SealEngineFace const& _sealEngine);
json_spirit::mObject fillBCTest(json_spirit::mObject const& _input);
void testBCTest(json_spirit::mObject& _o);

//percent output for many tests in one file
json_spirit::mValue doBlockchainTests(json_spirit::mValue const& _v, bool _fillin)
{
	TestOutputHelper::initTest(_v.get_obj().size());	//Count how many tests in the json object (read from .json file)
	json_spirit::mValue ret = doBlockchainTestNoLog(_v, _fillin); //Do the test / test generation
	TestOutputHelper::finishTest(); //Calculate the time of test execution and add it to the log
	return ret;
}

json_spirit::mValue doTransitionTest(json_spirit::mValue const& _input, bool _fillin)
{
	json_spirit::mValue output = _input;
	for (auto& i: output.get_obj())
	{
		string testname = i.first;
		json_spirit::mObject& o = i.second.get_obj();

		BOOST_REQUIRE(o.count("genesisBlockHeader"));
		BOOST_REQUIRE(o.count("pre"));
		BOOST_REQUIRE(o.count("network"));

		dev::test::TestBlockChain::s_sealEngineNetwork = stringToNetId(o["network"].get_str());
		if (test::isDisabledNetwork(dev::test::TestBlockChain::s_sealEngineNetwork))
			continue;

		if (!TestOutputHelper::passTest(testname))
		{
			o.clear(); //don't add irrelevant tests to the final file when filling
			continue;
		}

		if (_fillin)
			o = fillBCTest(o);
		else
			testBCTest(o);
	}
	return output;
}

json_spirit::mValue doBlockchainTestNoLog(json_spirit::mValue const& _input, bool _fillin)
{
	json_spirit::mValue v = _input; // TODO: avoid copying and add only valid fields to the new object.
	map<string, json_spirit::mObject> tests;
	vector<decltype(v.get_obj().begin())> erase_list;

	// range-for is not used because iterators are necessary for removing elements later.
	for (auto i = v.get_obj().begin(); i != v.get_obj().end(); i++)
	{
		string testname = i->first;
		json_spirit::mObject& o = i->second.get_obj();

		//Select test by name if --singletest is set and not filling state tests as blockchain
		if (!Options::get().fillchain && !TestOutputHelper::passTest(testname))
		{
			o.clear(); //don't add irrelevant tests to the final file when filling
			continue;
		}

		BOOST_REQUIRE_MESSAGE(o.count("genesisBlockHeader"),
			"\"genesisBlockHeader\" field is not found. filename: " + TestOutputHelper::testFileName() +
			" testname: " + TestOutputHelper::testName()
		);
		BOOST_REQUIRE_MESSAGE(o.count("pre"),
			"\"pre\" field is not found. filename: " + TestOutputHelper::testFileName() +
			" testname: " + TestOutputHelper::testName()
		);

		if (_fillin)
		{
			//create a blockchain test for each network
			for (auto& network : test::getNetworks())
			{
				if (!Options::get().singleTestNet.empty() && Options::get().singleTestNet != test::netIdToString(network))
					continue;

				dev::test::TestBlockChain::s_sealEngineNetwork = network;
				string newtestname = testname + "_" + test::netIdToString(network);

				json_spirit::mObject jObj = o;
				if (o.count("expect"))
				{
					//prepare the corresponding expect section for the test
					json_spirit::mArray& expects = o["expect"].get_array();
					bool found = false;

					for (auto& expect : expects)
					{
						vector<string> netlist;
						json_spirit::mObject& expectObj = expect.get_obj();
						ImportTest::parseJsonStrValueIntoVector(expectObj["network"], netlist);
						if (std::find(netlist.begin(), netlist.end(), test::netIdToString(network)) != netlist.end() ||
							std::find(netlist.begin(), netlist.end(), "ALL") != netlist.end())
						{
							jObj["expect"] = expectObj["result"];
							found = true;
							break;
						}
					}
					if (!found)
						jObj.erase(jObj.find("expect"));
				}
				TestOutputHelper::setCurrentTestName(newtestname);
				jObj = fillBCTest(jObj);
				jObj["network"] = test::netIdToString(network);
				tests[newtestname] = jObj;
			}

			// will be deleted once after the loop.
			// removing an element while in this loop causes memory corruption.
			erase_list.push_back(i);
		}
		else
		{
			BOOST_REQUIRE_MESSAGE(o.count("network"),
				"\"network\" field is not found. filename: " + TestOutputHelper::testFileName() +
				" testname: " + TestOutputHelper::testName()
			);
			dev::test::TestBlockChain::s_sealEngineNetwork = stringToNetId(o["network"].get_str());
			if (test::isDisabledNetwork(dev::test::TestBlockChain::s_sealEngineNetwork))
				continue;
			testBCTest(o);
		}
	}

	//Delete source test from the json
	for (auto i: erase_list)
		v.get_obj().erase(i);

	//Add generated tests to the result file
	if (_fillin)
	{
		BOOST_CHECK_MESSAGE(v.get_obj().size() == 0, " Test Filler is incorrect. Still having the test source when generating from filler " + TestOutputHelper::testName());
		json_spirit::mObject& obj = v.get_obj();
		for (auto& test : tests)
			obj[test.first] = test.second;
	}
	return v;
}

json_spirit::mObject fillBCTest(json_spirit::mObject const& _input)
{
	json_spirit::mObject output;
	string const& testName = TestOutputHelper::testName();
	TestBlock genesisBlock(_input.at("genesisBlockHeader").get_obj(), _input.at("pre").get_obj());
	genesisBlock.setBlockHeader(genesisBlock.blockHeader());

	TestBlockChain testChain(genesisBlock);
	assert(testChain.interface().isKnown(genesisBlock.blockHeader().hash(WithSeal)));

	output["genesisBlockHeader"] = writeBlockHeaderToJson(genesisBlock.blockHeader());
	output["genesisRLP"] = toHexPrefixed(genesisBlock.bytes());
	BOOST_REQUIRE(_input.count("blocks"));

	mArray blArray;
	size_t importBlockNumber = 0;
	string chainname = "default";
	string chainnetwork = "default";
	std::map<string, ChainBranch*> chainMap = { {chainname , new ChainBranch(genesisBlock)}};

	if (_input.count("network") > 0)
		output["network"] = _input.at("network");

	for (auto const& bl: _input.at("blocks").get_array())
	{
		mObject const& blObjInput = bl.get_obj();
		mObject blObj;
		if (blObjInput.count("blocknumber") > 0)
		{
			importBlockNumber = max((int)toInt(blObjInput.at("blocknumber")), 1);
			blObj["blocknumber"] = blObjInput.at("blocknumber");
		}
		else
			importBlockNumber++;

		if (blObjInput.count("chainname") > 0)
		{
			chainname = blObjInput.at("chainname").get_str();
			blObj["chainname"] = blObjInput.at("chainname");
		}
		else
			chainname = "default";

		if (blObjInput.count("chainnetwork") > 0)
		{
			chainnetwork = blObjInput.at("chainnetwork").get_str();
			blObj["chainnetwork"] = blObjInput.at("chainnetwork");
		}
		else
			chainnetwork = "default";

		// Copy expectException* fields
		for (auto const& field: blObjInput)
			if (field.first.substr(0, 15) == "expectException")
				blObj[field.first] = field.second;

		if (chainMap.count(chainname) > 0)
		{
			if (_input.count("noBlockChainHistory") == 0)
			{
				ChainBranch::forceBlockchain(chainnetwork);
				chainMap[chainname]->reset();
				ChainBranch::resetBlockchain();
				chainMap[chainname]->restoreFromHistory(importBlockNumber);
			}
		}
		else
		{
			ChainBranch::forceBlockchain(chainnetwork);
			chainMap[chainname] = new ChainBranch(genesisBlock);
			ChainBranch::resetBlockchain();
		}

		TestBlock block;
		TestBlockChain& blockchain = chainMap[chainname]->blockchain;
		vector<TestBlock>& importedBlocks = chainMap[chainname]->importedBlocks;

		//Import Transactions
		BOOST_REQUIRE(blObjInput.count("transactions"));
		for (auto const& txObj: blObjInput.at("transactions").get_array())
		{
			TestTransaction transaction(txObj.get_obj());
			block.addTransaction(transaction);
		}

		//Import Uncles
		for (auto const& uHObj: blObjInput.at("uncleHeaders").get_array())
		{
			cnote << "Generating uncle block at test " << testName;
			TestBlock uncle;
			mObject uncleHeaderObj = uHObj.get_obj();
			string uncleChainName = chainname;
			if (uncleHeaderObj.count("chainname") > 0)
				uncleChainName = uncleHeaderObj["chainname"].get_str();

			overwriteUncleHeaderForTest(uncleHeaderObj, uncle, block.uncles(), *chainMap[uncleChainName]);
			block.addUncle(uncle);
		}

		vector<TestBlock> validUncles = blockchain.syncUncles(block.uncles());
		block.setUncles(validUncles);

		if (blObjInput.count("blockHeaderPremine"))
			overwriteBlockHeaderForTest(blObjInput.at("blockHeaderPremine").get_obj(), block, *chainMap[chainname]);

		cnote << "Mining block" <<  importBlockNumber << "for chain" << chainname << "at test " << testName;
		block.mine(blockchain);
		cnote << "Block mined with...";
		cnote << "Transactions: " << block.transactionQueue().topTransactions(100).size();
		cnote << "Uncles: " << block.uncles().size();

		TestBlock alterBlock(block);
		checkBlocks(block, alterBlock, testName);

		if (blObjInput.count("blockHeader"))
			overwriteBlockHeaderForTest(blObjInput.at("blockHeader").get_obj(), alterBlock, *chainMap[chainname]);

		blObj["rlp"] = toHexPrefixed(alterBlock.bytes());
		blObj["blockHeader"] = writeBlockHeaderToJson(alterBlock.blockHeader());

		mArray aUncleList;
		for (auto const& uncle : alterBlock.uncles())
		{
			mObject uncleHeaderObj = writeBlockHeaderToJson(uncle.blockHeader());
			aUncleList.push_back(uncleHeaderObj);
		}
		blObj["uncleHeaders"] = aUncleList;
		blObj["transactions"] = writeTransactionsToJson(alterBlock.transactionQueue());

		compareBlocks(block, alterBlock);
		try
		{
			if (blObjInput.count("expectException"))
				BOOST_ERROR("Deprecated expectException field! " + testName);

			blockchain.addBlock(alterBlock);
			if (testChain.addBlock(alterBlock))
				cnote << "The most recent best Block now is " <<  importBlockNumber << "in chain" << chainname << "at test " << testName;

			bool isException = (blObjInput.count("expectException"+test::netIdToString(test::TestBlockChain::s_sealEngineNetwork))
								|| blObjInput.count("expectExceptionALL"));
			BOOST_REQUIRE_MESSAGE(!isException, "block import expected exception, but no exception was thrown!");

			if (_input.count("noBlockChainHistory") == 0)
			{
				importedBlocks.push_back(alterBlock);
				importedBlocks.back().clearState(); //close the state as it wont be needed. too many open states would lead to exception.
			}
		}
		catch (Exception const& _e)
		{
			cnote << testName + "block import throw an exception: " << diagnostic_information(_e);
			checkExpectedException(blObj, _e);
			eraseJsonSectionForInvalidBlock(blObj);
		}
		catch (std::exception const& _e)
		{
			cnote << testName + "block import throw an exception: " << _e.what();
			cout << testName + "block import thrown std exeption\n";
			eraseJsonSectionForInvalidBlock(blObj);
		}
		catch (...)
		{
			cout << testName + "block import thrown unknown exeption\n";
			eraseJsonSectionForInvalidBlock(blObj);
		}

		blArray.push_back(blObj);  //json data
		this_thread::sleep_for(chrono::seconds(1));
	}//each blocks

	if (_input.count("expect") > 0)
	{
		AccountMaskMap expectStateMap;
		State stateExpect(State::Null);
		ImportTest::importState(_input.at("expect").get_obj(), stateExpect, expectStateMap);
		if (ImportTest::compareStates(stateExpect, testChain.topBlock().state(), expectStateMap, WhenError::Throw))
				cerr << testName << "\n";
	}

	output["blocks"] = blArray;
	output["postState"] = fillJsonWithState(testChain.topBlock().state());
	output["lastblockhash"] = toHexPrefixed(testChain.topBlock().blockHeader().hash(WithSeal));

	//make all values hex in pre section
	State prestate(State::Null);
	ImportTest::importState(_input.at("pre").get_obj(), prestate);
	output["pre"] = fillJsonWithState(prestate);

	for (auto iterator = chainMap.begin(); iterator != chainMap.end(); iterator++)
		delete iterator->second;

	return output;
}

void testBCTest(json_spirit::mObject& _o)
{
	string testName = TestOutputHelper::testName();
	TestBlock genesisBlock(_o["genesisBlockHeader"].get_obj(), _o["pre"].get_obj());
	TestBlockChain blockchain(genesisBlock);

	TestBlockChain testChain(genesisBlock);
	assert(testChain.interface().isKnown(genesisBlock.blockHeader().hash(WithSeal)));

	if (_o.count("genesisRLP") > 0)
	{
		TestBlock genesisFromRLP(_o["genesisRLP"].get_str());
		checkBlocks(genesisBlock, genesisFromRLP, testName);
	}

	for (auto const& bl: _o["blocks"].get_array())
	{
		mObject blObj = bl.get_obj();
		TestBlock blockFromRlp;
		State const preState = testChain.topBlock().state();
		h256 const preHash = testChain.topBlock().blockHeader().hash();
		try
		{
			TestBlock blRlp(blObj["rlp"].get_str());
			blockFromRlp = blRlp;
			if (blObj.count("blockHeader") == 0)
				blockFromRlp.noteDirty();			//disable blockHeader check in TestBlock
			testChain.addBlock(blockFromRlp);
		}
		// if exception is thrown, RLP is invalid and no blockHeader, Transaction list, or Uncle list should be given
		catch (Exception const& _e)
		{
			cnote << testName + "state sync or block import did throw an exception: " << diagnostic_information(_e);
			checkJsonSectionForInvalidBlock(blObj);
			continue;
		}
		catch (std::exception const& _e)
		{
			cnote << testName + "state sync or block import did throw an exception: " << _e.what();
			checkJsonSectionForInvalidBlock(blObj);
			continue;
		}
		catch (...)
		{
			cnote << testName + "state sync or block import did throw an exception\n";
			checkJsonSectionForInvalidBlock(blObj);
			continue;
		}

		//block from RLP successfully imported. now compare this rlp to test sections
		BOOST_REQUIRE_MESSAGE(blObj.count("blockHeader"),
			"blockHeader field is not found. "
			"filename: " + TestOutputHelper::testFileName() +
			" testname: " + TestOutputHelper::testName()
		);

		//Check Provided Header against block in RLP
		TestBlock blockFromFields(blObj["blockHeader"].get_obj());

		//ImportTransactions
		BOOST_REQUIRE_MESSAGE(blObj.count("transactions"),
			"transactions field is not found. "
			"filename: " + TestOutputHelper::testFileName() +
			" testname: " + TestOutputHelper::testName()
		);
		for (auto const& txObj: blObj["transactions"].get_array())
		{
			TestTransaction transaction(txObj.get_obj());
			blockFromFields.addTransaction(transaction);
		}

		// ImportUncles
		vector<u256> uncleNumbers;
		if (blObj["uncleHeaders"].type() != json_spirit::null_type)
		{
			BOOST_REQUIRE_MESSAGE(blObj["uncleHeaders"].get_array().size() <= 2, "Too many uncle headers in block! " + TestOutputHelper::testName());
			for (auto const& uBlHeaderObj: blObj["uncleHeaders"].get_array())
			{
				mObject uBlH = uBlHeaderObj.get_obj();
				BOOST_REQUIRE((uBlH.size() == 16));

				TestBlock uncle(uBlH);
				blockFromFields.addUncle(uncle);
				uncleNumbers.push_back(uncle.blockHeader().number());
			}
		}

		checkBlocks(blockFromFields, blockFromRlp, testName);

		try
		{
			blockchain.addBlock(blockFromFields);
		}
		catch (Exception const& _e)
		{
			cerr << testName + "Error importing block from fields to blockchain: " << diagnostic_information(_e);
			break;
		}

		//Check that imported block to the chain is equal to declared block from test
		bytes importedblock = testChain.interface().block(blockFromFields.blockHeader().hash(WithSeal));
		TestBlock inchainBlock(toHex(importedblock));
		checkBlocks(inchainBlock, blockFromFields, testName);

		string blockNumber = toString(testChain.interface().number());
		string blockChainName = "default";
		if (blObj.count("chainname") > 0)
			blockChainName = blObj["chainname"].get_str();
		if (blObj.count("blocknumber") > 0)
			blockNumber = blObj["blocknumber"].get_str();

		//check the balance before and after the block according to mining rules
		if (blockFromFields.blockHeader().parentHash() == preHash)
		{
			State const postState = testChain.topBlock().state();
			assert(testChain.interface().sealEngine());
			bigint reward = calculateMiningReward(testChain.topBlock().blockHeader().number(), uncleNumbers.size() >= 1 ? uncleNumbers[0] : 0, uncleNumbers.size() >= 2 ? uncleNumbers[1] : 0, *testChain.interface().sealEngine());
			ImportTest::checkBalance(preState, postState, reward);
		}
		else
		{
			cnote << "Block Number " << testChain.topBlock().blockHeader().number();
			cnote << "Skipping the balance validation of potential correct block: " << TestOutputHelper::testName();
		}

		cnote << "Tested topblock number" << blockNumber << "for chain " << blockChainName << testName;

	}//allBlocks

	//Check lastblock hash
	BOOST_REQUIRE((_o.count("lastblockhash") > 0));
	string lastTrueBlockHash = toHexPrefixed(testChain.topBlock().blockHeader().hash(WithSeal));
	BOOST_CHECK_MESSAGE(lastTrueBlockHash == _o["lastblockhash"].get_str(),
			testName + "Boost check: lastblockhash does not match " + lastTrueBlockHash + " expected: " + _o["lastblockhash"].get_str());

	//Check final state (just to be sure)
	BOOST_CHECK_MESSAGE(toString(testChain.topBlock().state().rootHash()) ==
						toString(blockchain.topBlock().state().rootHash()),
						testName + "State root in chain from RLP blocks != State root in chain from Field blocks!");

	State postState(State::Null); //Compare post states
	BOOST_REQUIRE((_o.count("postState") > 0));
	ImportTest::importState(_o["postState"].get_obj(), postState);
	ImportTest::compareStates(postState, testChain.topBlock().state());
	ImportTest::compareStates(postState, blockchain.topBlock().state());
}

bigint calculateMiningReward(u256 const& _blNumber, u256 const& _unNumber1, u256 const& _unNumber2, SealEngineFace const& _sealEngine)
{
	bigint const baseReward = _sealEngine.blockReward(_blNumber);
	bigint reward = baseReward;
	//INCLUDE_UNCLE = BASE_REWARD / 32
	//UNCLE_REWARD  = BASE_REWARD * (8 - Bn + Un) / 8
	if (_unNumber1 != 0)
	{
		reward += baseReward * (8 - _blNumber + _unNumber1) / 8;
		reward += baseReward / 32;
	}
	if (_unNumber2 != 0)
	{
		reward += baseReward * (8 - _blNumber + _unNumber2) / 8;
		reward += baseReward / 32;
	}
	return reward;
}

//TestFunction
void overwriteBlockHeaderForTest(mObject const& _blObj, TestBlock& _block, ChainBranch const& _chainBranch)
{
	//_blObj  - json object with header data
	//_block  - which header would be overwritten
	//_parentHeader - parent blockheader

	vector<TestBlock> const& importedBlocks = _chainBranch.importedBlocks;
	const SealEngineFace* sealEngine = _chainBranch.blockchain.interface().sealEngine();

	BlockHeader tmp;
	BlockHeader const& header = _block.blockHeader();
	auto ho = _blObj;
	if (ho.size() != 14)
	{
		tmp = constructHeader(
			ho.count("parentHash") ? h256(ho["parentHash"].get_str()) : header.parentHash(),
			ho.count("uncleHash") ? h256(ho["uncleHash"].get_str()) : header.sha3Uncles(),
			ho.count("coinbase") ? Address(ho["coinbase"].get_str()) : header.author(),
			ho.count("stateRoot") ? h256(ho["stateRoot"].get_str()): header.stateRoot(),
			ho.count("transactionsTrie") ? h256(ho["transactionsTrie"].get_str()) : header.transactionsRoot(),
			ho.count("receiptTrie") ? h256(ho["receiptTrie"].get_str()) : header.receiptsRoot(),
			ho.count("bloom") ? LogBloom(ho["bloom"].get_str()) : header.logBloom(),
			ho.count("difficulty") ? toInt(ho["difficulty"]) :
									 ho.count("relDifficulty") ? header.difficulty() + toInt(ho["relDifficulty"]) : header.difficulty(),
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
			BlockHeader parentHeader = importedBlocks.at(importedBlocks.size() - 1).blockHeader();
			tmp.setTimestamp(toInt(ho["RelTimestamp"]) + parentHeader.timestamp());
			tmp.setDifficulty(((const Ethash*)sealEngine)->calculateDifficulty(tmp, parentHeader));
		}

		Ethash::setMixHash(tmp, ho.count("mixHash") ? h256(ho["mixHash"].get_str()) : Ethash::mixHash(header));
		Ethash::setNonce(tmp, ho.count("nonce") ? Nonce(ho["nonce"].get_str()) : Ethash::nonce(header));
		tmp.noteDirty();
	}
	else
		tmp = TestBlock(ho).blockHeader();	// take the blockheader as is

	if (ho.count("populateFromBlock"))
	{
		size_t number = (size_t)toInt(ho.at("populateFromBlock"));
		ho.erase("populateFromBlock");
		if (number < importedBlocks.size())
		{
			BlockHeader parentHeader = importedBlocks.at(number).blockHeader();
			sealEngine->populateFromParent(tmp, parentHeader);
		}
		else
		{
			cerr << TestOutputHelper::testName() + "Could not populate blockHeader from block: there are no block with number!" << TestOutputHelper::testName() << "\n";
		}
	}

	if (ho.count("mixHash") || ho.count("nonce") || !ho.count("updatePoW"))
	{
		_block.setBlockHeader(tmp);
		_block.noteDirty(); //disable integrity check in test block
	}
	else
	{
		_block.setBlockHeader(tmp);
		_block.updateNonce(_chainBranch.blockchain);
	}
}

void overwriteUncleHeaderForTest(mObject& uncleHeaderObj, TestBlock& uncle, std::vector<TestBlock> const& uncles, ChainBranch const& _chainBranch)
{
	//uncleHeaderObj - json Uncle header with additional option fields
	//uncle			 - uncle Block to overwrite
	//uncles		 - previously imported uncles
	//importedBlocks - blocks already included in BlockChain
	vector<TestBlock> const& importedBlocks = _chainBranch.importedBlocks;
	const SealEngineFace* sealEngine = _chainBranch.blockchain.interface().sealEngine();

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
			vector<TestBlock> prevBlockUncles = importedBlocks.at(number).uncles();
			if (prevBlockUncles.size())
				uncle = prevBlockUncles[0];  //exact uncle??
			else
				cerr << TestOutputHelper::testName() + "Could not create uncle sameAsPreviousBlockUncle: previous block has no uncles!" << TestOutputHelper::testName() << "\n";
		}
		else
			cerr << TestOutputHelper::testName() + "Could not create uncle sameAsPreviousBlockUncle: there are no block imported!" << TestOutputHelper::testName() << "\n";
		return;
	}

	string overwrite = "false";
	if (uncleHeaderObj.count("overwriteAndRedoPoW"))
	{
		overwrite = uncleHeaderObj.at("overwriteAndRedoPoW").get_str();
		uncleHeaderObj.erase("overwriteAndRedoPoW");
	}

	//construct actual block
	BlockHeader uncleHeader;
	if (uncleHeaderObj.count("populateFromBlock"))
	{
		uncleHeader.setTimestamp((u256)time(0));
		size_t number = (size_t)toInt(uncleHeaderObj.at("populateFromBlock"));
		uncleHeaderObj.erase("populateFromBlock");
		if (number < importedBlocks.size())
		{
			sealEngine->populateFromParent(uncleHeader, importedBlocks.at(number).blockHeader());
			//Set Default roots for empty block
			//m_transactionsRoot = _t; m_receiptsRoot = _r; m_sha3Uncles = _u; m_stateRoot = _s;
			uncleHeader.setRoots((h256)fromHex("56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421"),
								 (h256)fromHex("56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421"),
								 (h256)fromHex("1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347"),
								 uncleHeader.stateRoot()
								 );

			if (uncleHeaderObj.count("RelTimestamp"))
			{
				BlockHeader parentHeader = importedBlocks.at(number).blockHeader();
				uncleHeader.setTimestamp(toInt(uncleHeaderObj["RelTimestamp"]) + parentHeader.timestamp());
				uncleHeader.setDifficulty(((const Ethash*)sealEngine)->calculateDifficulty(uncleHeader, parentHeader));
				uncleHeaderObj.erase("RelTimestamp");
			}
		}
		else
			cerr << TestOutputHelper::testName() + "Could not create uncle populateFromBlock: there are no block with number " << number << TestOutputHelper::testName() << "\n";
	}
	else
	{
		uncle = TestBlock(uncleHeaderObj);
		uncleHeader = uncle.blockHeader();
	}

	if (overwrite != "false")
	{
		uncleHeader = constructHeader(
			overwrite == "parentHash" ? h256(uncleHeaderObj.at("parentHash").get_str()) : uncleHeader.parentHash(),
			uncleHeader.sha3Uncles(),
			overwrite == "coinbase" ? Address(uncleHeaderObj.at("coinbase").get_str()) : uncleHeader.author(),
			overwrite == "stateRoot" ? h256(uncleHeaderObj.at("stateRoot").get_str()) : uncleHeader.stateRoot(),
			uncleHeader.transactionsRoot(),
			uncleHeader.receiptsRoot(),
			uncleHeader.logBloom(),
			overwrite == "difficulty" ? toInt(uncleHeaderObj.at("difficulty"))
									  :	overwrite == "timestamp" ? ((const Ethash*)sealEngine)->calculateDifficulty(uncleHeader, importedBlocks.at((size_t)uncleHeader.number() - 1).blockHeader())
																 : uncleHeader.difficulty(),
			overwrite == "number" ? toInt(uncleHeaderObj.at("number")) : uncleHeader.number(),
			overwrite == "gasLimit" ? toInt(uncleHeaderObj.at("gasLimit")) : uncleHeader.gasLimit(),
			overwrite == "gasUsed" ? toInt(uncleHeaderObj.at("gasUsed")) : uncleHeader.gasUsed(),
			overwrite == "timestamp" ? toInt(uncleHeaderObj.at("timestamp")) : uncleHeader.timestamp(),
			overwrite == "extraData" ? fromHex(uncleHeaderObj.at("extraData").get_str()) : uncleHeader.extraData());
	}

	uncle.setBlockHeader(uncleHeader);
	uncle.updateNonce(_chainBranch.blockchain);

	if (overwrite == "nonce" || overwrite == "mixHash")
	{
		if (overwrite == "nonce")
			Ethash::setNonce(uncleHeader, Nonce(uncleHeaderObj["nonce"].get_str()));
		if (overwrite == "mixHash")
			Ethash::setMixHash(uncleHeader, h256(uncleHeaderObj["mixHash"].get_str()));

		uncle.setBlockHeader(uncleHeader);
	}
}

void compareBlocks(TestBlock const& _a, TestBlock const& _b)
{
	if (sha3(RLP(_a.bytes())[0].data()) != sha3(RLP(_b.bytes())[0].data()))
	{
		cnote << "block header mismatch\n";
		cnote << toHex(RLP(_a.bytes())[0].data()) << "vs" << toHex(RLP(_b.bytes())[0].data());
	}

	if (sha3(RLP(_a.bytes())[1].data()) != sha3(RLP(_b.bytes())[1].data()))
		cnote << "txs mismatch\n";

	if (sha3(RLP(_a.bytes())[2].data()) != sha3(RLP(_b.bytes())[2].data()))
		cnote << "uncle list mismatch\n" << RLP(_a.bytes())[2].data() << "\n" << RLP(_b.bytes())[2].data();
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

mObject writeBlockHeaderToJson(BlockHeader const& _bi)
{
	mObject o;
	o["parentHash"] = toString(_bi.parentHash());
	o["uncleHash"] = toString(_bi.sha3Uncles());
	o["coinbase"] = toString(_bi.author());
	o["stateRoot"] = toString(_bi.stateRoot());
	o["transactionsTrie"] = toString(_bi.transactionsRoot());
	o["receiptTrie"] = toString(_bi.receiptsRoot());
	o["bloom"] = toString(_bi.logBloom());
	o["difficulty"] = toCompactHexPrefixed(_bi.difficulty(), 1);
	o["number"] = toCompactHexPrefixed(_bi.number(), 1);
	o["gasLimit"] = toCompactHexPrefixed(_bi.gasLimit(), 1);
	o["gasUsed"] = toCompactHexPrefixed(_bi.gasUsed(), 1);
	o["timestamp"] = toCompactHexPrefixed(_bi.timestamp(), 1);
	o["extraData"] = (_bi.extraData().size()) ? toHexPrefixed(_bi.extraData()) : "";
	o["mixHash"] = toString(Ethash::mixHash(_bi));
	o["nonce"] = toString(Ethash::nonce(_bi));
	o["hash"] = toString(_bi.hash());
	o = ImportTest::makeAllFieldsHex(o, true);
	return o;
}

void checkExpectedException(mObject& _blObj, Exception const& _e)
{
	string exWhat {	_e.what() };
	bool isNetException = (_blObj.count("expectException"+test::netIdToString(test::TestBlockChain::s_sealEngineNetwork)) > 0);
	bool isAllNetException = (_blObj.count("expectExceptionALL") > 0);

	BOOST_REQUIRE_MESSAGE(isNetException || isAllNetException, TestOutputHelper::testName() + " block import thrown unexpected Excpetion! (" + exWhat + ")");

	string exExpect;
	if (isNetException)
		exExpect = _blObj.at("expectException"+test::netIdToString(test::TestBlockChain::s_sealEngineNetwork)).get_str();
	if (isAllNetException)
		exExpect = _blObj.at("expectExceptionALL").get_str();

	BOOST_REQUIRE_MESSAGE(exWhat.find(exExpect) != string::npos, TestOutputHelper::testName() + " block import expected another exeption: " + exExpect + ", but got: " + exWhat);
}

void checkJsonSectionForInvalidBlock(mObject& _blObj)
{
	BOOST_CHECK_MESSAGE(_blObj.count("blockHeader") == 0,
			"blockHeader field is found in the should-be invalid block. "
			"filename: " + TestOutputHelper::testFileName() +
			" testname: " + TestOutputHelper::testName()
	);
	BOOST_CHECK_MESSAGE(_blObj.count("transactions") == 0,
			"transactions field is found in the should-be invalid block. "
			"filename: " + TestOutputHelper::testFileName() +
			" testname: " + TestOutputHelper::testName()
	);
	BOOST_CHECK_MESSAGE(_blObj.count("uncleHeaders") == 0,
			"uncleHeaders field is found in the should-be invalid block. "
			"filename: " + TestOutputHelper::testFileName() +
			" testname: " + TestOutputHelper::testName()
	);
}

void eraseJsonSectionForInvalidBlock(mObject& _blObj)
{
	// if exception is thrown, RLP is invalid and no blockHeader, Transaction list, or Uncle list should be given
	cnote << TestOutputHelper::testName() + " block is invalid!\n";
	_blObj.erase(_blObj.find("blockHeader"));
	_blObj.erase(_blObj.find("uncleHeaders"));
	_blObj.erase(_blObj.find("transactions"));
}

void checkBlocks(TestBlock const& _blockFromFields, TestBlock const& _blockFromRlp, string const& _testname)
{
	BlockHeader const& blockHeaderFromFields = _blockFromFields.blockHeader();
	BlockHeader const& blockFromRlp = _blockFromRlp.blockHeader();

	BOOST_CHECK_MESSAGE(blockHeaderFromFields.hash(WithoutSeal) == blockFromRlp.hash(WithoutSeal), _testname + "hash in given RLP not matching the block hash!");
	BOOST_CHECK_MESSAGE(blockHeaderFromFields.parentHash() == blockFromRlp.parentHash(), _testname + "parentHash in given RLP not matching the block parentHash!");
	BOOST_CHECK_MESSAGE(blockHeaderFromFields.sha3Uncles() == blockFromRlp.sha3Uncles(), _testname + "sha3Uncles in given RLP not matching the block sha3Uncles!");
	BOOST_CHECK_MESSAGE(blockHeaderFromFields.author() == blockFromRlp.author(), _testname + "author in given RLP not matching the block author!");
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
	//BOOST_CHECK_MESSAGE(Ethash::mixHash(blockHeaderFromFields) == Ethash::mixHash(blockFromRlp), _testname + "mixHash in given RLP not matching the block mixHash!");
	//BOOST_CHECK_MESSAGE(Ethash::nonce(blockHeaderFromFields) == Ethash::nonce(blockFromRlp), _testname + "nonce in given RLP not matching the block nonce!");

	BOOST_CHECK_MESSAGE(blockHeaderFromFields == blockFromRlp, _testname + "However, blockHeaderFromFields != blockFromRlp!");

	vector<TestTransaction> const& txsFromField = _blockFromFields.testTransactions();
	vector<TestTransaction> const& txsFromRlp = _blockFromRlp.testTransactions();
	BOOST_CHECK_MESSAGE(txsFromRlp.size() == txsFromField.size(), _testname + "transaction list size does not match");

	for (size_t i = 0; i < txsFromField.size(); ++i)
	{
		Transaction const& trField = txsFromField.at(i).transaction();
		Transaction const& trRlp = txsFromRlp.at(i).transaction();

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

	vector<TestBlock> const& unclesFromField = _blockFromFields.uncles();
	vector<TestBlock> const& unclesFromRlp = _blockFromRlp.uncles();
	BOOST_REQUIRE_EQUAL(unclesFromField.size(), unclesFromRlp.size());
	for (size_t i = 0; i < unclesFromField.size(); ++i)
		BOOST_CHECK_MESSAGE(unclesFromField.at(i).blockHeader() == unclesFromRlp.at(i).blockHeader(), _testname + "block header in rlp and in field do not match at uncles");
}

//namespaces
}
}
