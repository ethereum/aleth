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
/** @file
 * Incapsulation of libetehreum blockchain logic into test classes
 * that manage block/transaction import and test mining
 */

#include <libdevcore/TransientDirectory.h>
#include <libethereum/Block.h>
#include <libethereum/BlockChain.h>
#include <libethereum/TransactionQueue.h>
#include <libethereum/GenesisInfo.h>
#include <libethashseal/GenesisInfo.h>
#include <test/libtesteth/BlockChainHelper.h>
#include <test/libtesteth/TestHelper.h>
using namespace std;
using namespace json_spirit;
using namespace dev;
using namespace dev::eth;

namespace dev
{
namespace test
{

TestTransaction::TestTransaction(mObject const& _o):
	m_jsonTransaction(_o)
{
	ImportTest::importTransaction(_o, m_transaction); //check that json structure is valid
}

TestBlock::TestBlock()
{
	m_dirty = false;
}

TestBlock::TestBlock(mObject const& _blockObj, mObject const& _stateObj):
	TestBlock()
{
	m_tempDirState = std::unique_ptr<TransientDirectory>(new TransientDirectory());

	m_state = std::unique_ptr<State>(new State(0, OverlayDB(State::openDB(m_tempDirState.get()->path(), h256{}, WithExisting::Kill)), BaseState::Empty));
	ImportTest::importState(_stateObj, *m_state.get());
	m_state.get()->commit(State::CommitBehaviour::KeepEmptyAccounts);

	json_spirit::mObject state = _stateObj;
	dev::test::replaceLLLinState(state);
	m_accountMap = jsonToAccountMap(json_spirit::write_string(json_spirit::mValue(state), false));

	m_blockHeader = constructBlock(_blockObj, _stateObj.size() ? m_state.get()->rootHash() : h256{});
	recalcBlockHeaderBytes();
}

TestBlock::TestBlock(std::string const& _blockRLP):
	TestBlock()
{
	m_bytes = importByteArray(_blockRLP);

	RLP root(m_bytes);
	m_blockHeader = BlockHeader(m_bytes);

	m_transactionQueue.clear();
	m_testTransactions.clear();
	for (auto const& tr: root[1])
	{
		Transaction tx(tr.data(), CheckTransaction::Everything);
		TestTransaction testTx(tx);
		m_transactionQueue.import(tx.rlp());
		m_testTransactions.push_back(testTx);
	}

	for (auto const& uRLP: root[2])
	{
		BlockHeader uBl(uRLP.data(), HeaderData);
		TestBlock uncle;
		uncle.setBlockHeader(uBl);
		m_uncles.push_back(uncle);
	}
}

TestBlock::TestBlock(TestBlock const& _original)
{
	populateFrom(_original);
}

TestBlock& TestBlock::operator=(TestBlock const& _original)
{
	populateFrom(_original);
	return *this;
}

void TestBlock::setState(State const& _state)
{
	copyStateFrom(_state);
}

void TestBlock::addTransaction(TestTransaction const& _tr)
{
	try
	{
		m_testTransactions.push_back(_tr);
		if (m_transactionQueue.import(_tr.transaction().rlp()) != ImportResult::Success)
			cnote << TestOutputHelper::testName() + "Test block failed importing transaction\n";
	}
	catch (Exception const& _e)
	{
		BOOST_ERROR(TestOutputHelper::testName() + "Failed transaction constructor with Exception: " << diagnostic_information(_e));
	}
	catch (exception const& _e)
	{
		cnote << _e.what();
	}

	recalcBlockHeaderBytes();
}

void TestBlock::addUncle(TestBlock const& _uncle)
{
	m_uncles.push_back(_uncle);
	recalcBlockHeaderBytes();
}

void TestBlock::setUncles(vector<TestBlock> const& _uncles)
{
	m_uncles.clear();
	m_uncles = _uncles;
	recalcBlockHeaderBytes();
}

void TestBlock::premineUpdate(BlockHeader& _blockInfo)
{
	//alter blockheader with defined fields before actual mining

	if (m_premineUpdate.count("parentHash") > 0)
		_blockInfo.setParentHash(m_blockHeader.parentHash());
	if (m_premineUpdate.count("coinbase") > 0)
		_blockInfo.setAuthor(m_blockHeader.author());

	if (m_premineUpdate.count("uncleHash") > 0 || m_premineUpdate.count("stateRoot") > 0 ||
		m_premineUpdate.count("transactionsTrie") > 0 || m_premineUpdate.count("receiptTrie") > 0)
		_blockInfo.setRoots(m_premineUpdate.count("transactionsTrie") > 0 ? m_blockHeader.transactionsRoot() : _blockInfo.transactionsRoot(),
						m_premineUpdate.count("receiptTrie") > 0 ? m_blockHeader.receiptsRoot() : _blockInfo.receiptsRoot(),
						m_premineUpdate.count("uncleHash") > 0 ? m_blockHeader.sha3Uncles() : _blockInfo.sha3Uncles(),
						m_premineUpdate.count("stateRoot") > 0 ? m_blockHeader.stateRoot() : _blockInfo.stateRoot());

	if (m_premineUpdate.count("bloom") > 0)
		_blockInfo.setLogBloom(m_blockHeader.logBloom());
	if (m_premineUpdate.count("difficulty") > 0)
		_blockInfo.setDifficulty(m_blockHeader.difficulty());
	if (m_premineUpdate.count("number") > 0)
		_blockInfo.setNumber(m_blockHeader.number());
	if (m_premineUpdate.count("gasLimit") > 0)
		_blockInfo.setGasLimit(m_blockHeader.gasLimit());
	if (m_premineUpdate.count("gasUsed") > 0)
		_blockInfo.setGasUsed(m_blockHeader.gasUsed());
	if (m_premineUpdate.count("timestamp") > 0)
		_blockInfo.setTimestamp(m_blockHeader.timestamp());
	if (m_premineUpdate.count("extraData") > 0)
		_blockInfo.setExtraData(m_blockHeader.extraData());

	m_premineHeader = _blockInfo; //needed for check that any altered fields are altered in mined block as well
}

void TestBlock::mine(TestBlockChain const& _bc)
{
	TestBlock const& genesisBlock = _bc.testGenesis();
	OverlayDB const& genesisDB = genesisBlock.state().db();

	BlockChain const& blockchain = _bc.interface();

	Block block = blockchain.genesisBlock(genesisDB);
	block.setAuthor(genesisBlock.beneficiary());

	//set some header data before mining from original blockheader
	BlockHeader& blockInfo = *const_cast<BlockHeader*>(&block.info());

	try
	{
		ZeroGasPricer gp;
		block.sync(blockchain);  //sync block with blockchain

		premineUpdate(blockInfo);

		size_t transactionsOnImport = m_transactionQueue.topTransactions(100).size();
		block.sync(blockchain, m_transactionQueue, gp); //!!! Invalid transactions are dropped here
		if (transactionsOnImport >  m_transactionQueue.topTransactions(100).size())
			cnote << "Dropped invalid Transactions when mining!";

		dev::eth::mine(block, blockchain, blockchain.sealEngine());
		blockchain.sealEngine()->verify(JustSeal, block.info());
	}
	catch (Exception const& _e)
	{
		cnote << TestOutputHelper::testName() + "block sync or mining did throw an exception: " << diagnostic_information(_e);
		return;
	}
	catch (std::exception const& _e)
	{
		cnote << TestOutputHelper::testName() + "block sync or mining did throw an exception: " << _e.what();
		return;
	}

	size_t validTransactions = m_transactionQueue.topTransactions(100).size();
	m_receipts = RLPStream(validTransactions);
	for (size_t i = 0; i < validTransactions; i++)
	{
		const dev::bytes receipt = block.receipt(i).rlp();
		m_receipts.appendRaw(receipt);
	}

	m_blockHeader = BlockHeader(block.blockData());		// NOTE no longer checked at this point in new API. looks like it was unimportant anyway
	cnote << "Mined TrRoot: " << m_blockHeader.transactionsRoot();
	copyStateFrom(block.state());

	//Invalid uncles are dropped when mining. but we restore the hash to produce block with invalid uncles (for later test when importing to blockchain)
	if (m_uncles.size())
	{
		//Fill info with uncles
		RLPStream uncleStream;
		uncleStream.appendList(m_uncles.size());
		for (unsigned i = 0; i < m_uncles.size(); ++i)
		{
			RLPStream uncleRlp;
			m_uncles[i].blockHeader().streamRLP(uncleRlp);
			uncleStream.appendRaw(uncleRlp.out());
		}

		m_blockHeader.setSha3Uncles(sha3(uncleStream.out()));
		updateNonce(_bc);
	}
	else
		recalcBlockHeaderBytes();
}

void TestBlock::setBlockHeader(BlockHeader const& _header)
{
	m_blockHeader = _header;
	recalcBlockHeaderBytes();
}

///Test Block Private
BlockHeader TestBlock::constructBlock(mObject const& _o, h256 const& _stateRoot)
{
	BlockHeader ret;
	try
	{
		const dev::bytes c_blockRLP = createBlockRLPFromFields(_o, _stateRoot);
		ret = BlockHeader(c_blockRLP, HeaderData);
//		cdebug << "Block constructed of hash" << ret.hash() << "(without:" << ret.hash(WithoutSeal) << ")";
	}
	catch (Exception const& _e)
	{
		cnote << TestOutputHelper::testName() + "block population did throw an exception: " << diagnostic_information(_e);
	}
	catch (std::exception const& _e)
	{
		BOOST_ERROR(TestOutputHelper::testName() + "Failed block population with Exception: " << _e.what());
	}
	catch(...)
	{
		BOOST_ERROR(TestOutputHelper::testName() + "block population did throw an unknown exception\n");
	}
	return ret;
}

dev::bytes TestBlock::createBlockRLPFromFields(mObject const& _tObj, h256 const& _stateRoot)
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

void TestBlock::updateNonce(TestBlockChain const& _bc)
{
	if (((BlockHeader)m_blockHeader).difficulty() == 0)
		BOOST_TEST_MESSAGE("Trying to mine a block with 0 difficulty! " + TestOutputHelper::testName());
	else
	{
		//do not verify blockheader for validity here
		dev::eth::mine(m_blockHeader, _bc.interface().sealEngine(), false);
	}

	recalcBlockHeaderBytes();
}

void TestBlock::verify(TestBlockChain const& _bc) const
{
	if (m_dirty) //TestBlock have incorrect blockheader for testing purposes
		return;

	try
	{
		_bc.interface().sealEngine()->verify(CheckNothingNew, m_blockHeader, BlockHeader(), &m_bytes);
	}
	catch (Exception const& _e)
	{
		u256 daoHardfork = _bc.interface().sealEngine()->chainParams().u256Param("daoHardforkBlock");
		if ((m_blockHeader.number() >= daoHardfork && m_blockHeader.number() <= daoHardfork + 9) || m_blockHeader.number() == 0)
		{
			string exWhat {	_e.what() };
			string exExpect = "InvalidTransactionsRoot";
			BOOST_REQUIRE_MESSAGE(exWhat.find(exExpect) != string::npos, TestOutputHelper::testName() + "block import expected another exeption: " + exExpect);
		}
		else
			BOOST_ERROR(TestOutputHelper::testName() + toString(m_blockHeader.number()) + " BlockHeader Verification failed: " <<  boost::current_exception_diagnostic_information());
	}
	catch (...)
	{
		BOOST_ERROR(TestOutputHelper::testName() + "BlockHeader Verification failed: " <<  boost::current_exception_diagnostic_information());
	}
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
		m_uncles[i].blockHeader().streamRLP(uncleRlp);
		uncleStream.appendRaw(uncleRlp.out());
	}

	RLPStream blHeaderStream;
	m_blockHeader.streamRLP(blHeaderStream, WithSeal);

	RLPStream ret(3);
	ret.appendRaw(blHeaderStream.out()); //block header
	ret.appendRaw(txStream.out());		 //transactions
	ret.appendRaw(uncleStream.out());	 //uncles
	m_bytes = ret.out();
}

void TestBlock::copyStateFrom(State const& _state)
{
	//WEIRD WAY TO COPY STATE AS COPY CONSTRUCTOR FOR STATE NOT IMPLEMENTED CORRECTLY (they would share the same DB)
	m_tempDirState.reset(new TransientDirectory());
	m_state.reset(new State(0, OverlayDB(State::openDB(m_tempDirState.get()->path(), h256{}, WithExisting::Kill)), BaseState::Empty));
	json_spirit::mObject obj = fillJsonWithState(_state);
	ImportTest::importState(obj, *m_state.get());
}

void TestBlock::clearState()
{
	m_state.reset(0);
	m_tempDirState.reset(0);
	for (size_t i = 0; i < m_uncles.size(); i++)
		m_uncles.at(i).clearState();
}

void TestBlock::populateFrom(TestBlock const& _original)
{
	try
	{
		copyStateFrom(_original.state()); //copy state if it is defined in _original
	}
	catch (BlockStateUndefined const& _ex)
	{
		if (g_logVerbosity > 6)
			cnote << _ex.what() << "copying block with null state";
	}
	m_testTransactions = _original.testTransactions();
	m_transactionQueue.clear();
	TransactionQueue const& trQueue = _original.transactionQueue();
	for (auto const& txi: trQueue.topTransactions(std::numeric_limits<unsigned>::max()))
		m_transactionQueue.import(txi.rlp());

	m_uncles = _original.uncles();
	m_blockHeader = _original.blockHeader();
	m_bytes = _original.bytes();
	m_accountMap = _original.accountMap();
	m_dirty = false;
}

TestBlockChain::TestBlockChain(TestBlock const& _genesisBlock)
{
	reset(_genesisBlock);
}

void TestBlockChain::reset(TestBlock const& _genesisBlock)
{
	m_tempDirBlockchain.reset(new TransientDirectory);
	ChainParams p = ChainParams(genesisInfo(TestBlockChain::s_sealEngineNetwork), _genesisBlock.bytes(), _genesisBlock.accountMap());

	m_blockChain.reset(new BlockChain(p, m_tempDirBlockchain.get()->path(), WithExisting::Kill));
	if (!m_blockChain->isKnown(BlockHeader::headerHashFromBlock(_genesisBlock.bytes())))
	{
		cdebug << "Not known:" << BlockHeader::headerHashFromBlock(_genesisBlock.bytes()) << BlockHeader(p.genesisBlock()).hash();
		cdebug << "Genesis block not known!";
		cdebug << "This should never happen.";
		assert(false);
	}
	m_lastBlock = m_genesisBlock = _genesisBlock;
}

bool TestBlockChain::addBlock(TestBlock const& _block)
{
	while (true)
	{
		try
		{
			_block.verify(*this); //check that block header match TestBlock contents
			m_blockChain.get()->import(_block.bytes(), m_genesisBlock.state().db());
			break;
		}
		catch (FutureTime)
		{
			this_thread::sleep_for(chrono::milliseconds(100));
			break;
		}
	}

	//Imported and best
	if (_block.bytes() == m_blockChain.get()->block())
	{
		m_lastBlock = _block;

		//overwrite state in case _block had no State defined (e.x. created from RLP)
		OverlayDB const& genesisDB = m_genesisBlock.state().db();		
		Block block = (m_blockChain.get()->genesisBlock(genesisDB));
		block.sync(*m_blockChain.get());

		//BOOST_REQUIRE(m_lastBlock.blockHeader().hash() == BlockHeader(block.blockData()).hash());
		m_lastBlock.setState(block.fromPending(10000));
		return true;
	}

	return false;
}

vector<TestBlock> TestBlockChain::syncUncles(vector<TestBlock> const& uncles)
{	
	vector<TestBlock> validUncles;
	if (uncles.size() == 0)
		return validUncles;

	BlockQueue uncleBlockQueue;
	BlockChain& blockchain = *m_blockChain.get();
	uncleBlockQueue.setChain(blockchain);

	for (size_t i = 0; i < uncles.size(); i++)
	{
		try
		{
			uncleBlockQueue.import(&uncles.at(i).bytes(), false);
			this_thread::sleep_for(chrono::seconds(1)); // wait until block is verified
			validUncles.push_back(uncles.at(i));
		}
		catch(...)
		{
			cnote << "error in importing uncle! This produces an invalid block (May be by purpose for testing).";
		}
	}

	blockchain.sync(uncleBlockQueue, m_genesisBlock.state().db(), (unsigned)4);
	return validUncles;
}

TestTransaction TestTransaction::defaultTransaction(u256 const& _nonce, u256 const& _gasPrice, u256 const& _gasLimit, bytes const& _data)
{
	json_spirit::mObject txObj;
	txObj["data"] = toHex(_data);
	txObj["gasLimit"] = toString(_gasLimit);
	txObj["gasPrice"] = toString(_gasPrice);
	txObj["nonce"] = toString(_nonce);
	txObj["secretKey"] = "45a915e4d060149eb4365960e6a7a45f334393093061116b197e3240065ff2d8";
	txObj["to"] = "095e7baea6a6c7c4c2dfeb977efac326af552d87";
	txObj["value"] = "100";

	return TestTransaction(txObj);
}

AccountMap TestBlockChain::defaultAccountMap()
{
	AccountMap ret;
	ret[Address("a94f5374fce5edbc8e2a8697c15331677e6ebf0b")] = Account(0, 10000000000);
	return ret;
}

json_spirit::mObject TestBlockChain::defaultGenesisBlockJson()
{
	json_spirit::mObject blockObj;
	blockObj["bloom"] = "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
	blockObj["coinbase"] = "0x8888f1f195afa192cfee860698584c030f4c9db1";
	blockObj["difficulty"] = "131072";
	blockObj["extraData"] = "0x42";
	blockObj["gasLimit"] = "3141592";
	blockObj["gasUsed"] = "0";
	blockObj["mixHash"] = "0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421";
	blockObj["nonce"] = "0x0102030405060708";
	blockObj["number"] = "0";
	blockObj["parentHash"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
	blockObj["receiptTrie"] = "0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421";
	blockObj["stateRoot"] = "0xf99eb1626cfa6db435c0836235942d7ccaa935f1ae247d3f1c21e495685f903a";
	blockObj["timestamp"] = "0x54c98c81";
	blockObj["transactionsTrie"] = "0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421";
	blockObj["uncleHash"] = "0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347";
	return blockObj;
}

TestBlock TestBlockChain::defaultGenesisBlock(u256 const& _gasLimit)
{
	json_spirit::mObject blockObj = defaultGenesisBlockJson();
	blockObj["gasLimit"] = toString(_gasLimit);

	json_spirit::mObject accountObj;
	accountObj["balance"] = "10000000000";
	accountObj["nonce"] = "1";					//=1for nonce too low exception check
	accountObj["code"] = "";
	accountObj["storage"] = json_spirit::mObject();

	json_spirit::mObject accountMapObj;
	accountMapObj["a94f5374fce5edbc8e2a8697c15331677e6ebf0b"] = accountObj;

	return TestBlock(blockObj, accountMapObj);
}

}}
