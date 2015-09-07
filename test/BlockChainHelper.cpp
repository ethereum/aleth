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
/** @file BlockChainHelper.cpp
 * @author Dimitry Khokhlov <dimitry@ethdev.com>
 * @date 2015
 */

#include <libethereum/Block.h>
#include <libethereum/BlockChain.h>
#include <libethereum/TransactionQueue.h>
#include <libdevcore/TransientDirectory.h>
#include <test/BlockChainHelper.h>
#include <test/TestHelper.h>

using namespace std;
using namespace json_spirit;
using namespace dev;
using namespace dev::eth;

namespace dev {  namespace test {

TestTransaction::TestTransaction(mObject const& _o): m_jsonTransaction(_o)
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

TestBlock::TestBlock(mObject const& _blockObj, mObject const& _stateObj, RecalcBlockHeader _verify)
{
	m_tempDirState = std::unique_ptr<TransientDirectory>(new TransientDirectory());
	m_state = std::unique_ptr<State>(new State(OverlayDB(State::openDB(m_tempDirState.get()->path(), h256{}, WithExisting::Kill)), BaseState::Empty));
	ImportTest::importState(_stateObj, *m_state.get());
	m_state.get()->commit();

	m_blockHeader = constructBlock(_blockObj, _stateObj.size() ? m_state.get()->rootHash() : h256{});
	recalcBlockHeaderBytes(_verify);
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

	for (auto const& uRLP: root[2])
	{
		BlockHeader uBl;
		uBl.populateFromHeader(uRLP);
		TestBlock uncle;
		//uncle goes without transactions and uncles but
		//it's hash could contain hashsum of transactions/uncles
		//thus it won't need verification
		uncle.setBlockHeader(uBl, RecalcBlockHeader::SkipVerify);
		m_uncles.push_back(uncle);
	}	
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
		if (m_transactionQueue.import(_tr.getTransaction().rlp()) != ImportResult::Success)
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

	Block block = blockchain.genesisBlock(genesisDB);
	block.setBeneficiary(genesisBlock.getBeneficiary());

	//set some header data before mining from original blockheader
	BlockInfo& blockInfo = *const_cast<BlockInfo*>(&block.info());

	if (m_premineUpdate.count("parentHash") > 0)
		blockInfo.setParentHash(m_blockHeader.parentHash());
	if (m_premineUpdate.count("coinbase") > 0)
		blockInfo.setCoinbaseAddress(m_blockHeader.beneficiary());

	if (m_premineUpdate.count("uncleHash") > 0 || m_premineUpdate.count("stateRoot") > 0 ||
		m_premineUpdate.count("transactionsTrie") > 0 || m_premineUpdate.count("receiptTrie") > 0)
		blockInfo.setRoots(m_premineUpdate.count("transactionsTrie") > 0 ? m_blockHeader.transactionsRoot() : blockInfo.transactionsRoot(),
						   m_premineUpdate.count("receiptTrie") > 0 ? m_blockHeader.receiptsRoot() : blockInfo.receiptsRoot(),
						   m_premineUpdate.count("uncleHash") > 0 ? m_blockHeader.sha3Uncles() : blockInfo.sha3Uncles(),
						   m_premineUpdate.count("stateRoot") > 0 ? m_blockHeader.stateRoot() : blockInfo.stateRoot());

	if (m_premineUpdate.count("bloom") > 0)
		blockInfo.setLogBloom(m_blockHeader.logBloom());
	if (m_premineUpdate.count("difficulty") > 0)
		blockInfo.setDifficulty(m_blockHeader.difficulty());
	if (m_premineUpdate.count("number") > 0)
		blockInfo.setNumber(m_blockHeader.number());
	if (m_premineUpdate.count("gasLimit") > 0)
		blockInfo.setGasLimit(m_blockHeader.gasLimit());
	if (m_premineUpdate.count("gasUsed") > 0)
		blockInfo.setGasUsed(m_blockHeader.gasUsed());
	if (m_premineUpdate.count("timestamp") > 0)
		blockInfo.setTimestamp(m_blockHeader.timestamp());
	if (m_premineUpdate.count("extraData") > 0)
		blockInfo.setExtraData(m_blockHeader.extraData());

	try
	{
		ZeroGasPricer gp;
		block.sync(blockchain);
		block.sync(blockchain, m_transactionQueue, gp);

		//Get only valid transactions
		//Transactions const& trs = block.pending();
		//m_transactionQueue.clear();
		//for (auto const& tr : trs)
		//	m_transactionQueue.import(tr.rlp());

		dev::eth::mine(block, blockchain);
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

	m_blockHeader = BlockHeader(block.blockData());
	copyStateFrom(block.state());

	//Update block hashes cause we would fill block with uncles and transactions that
	//actually might have been dropped because they are invalid
	recalcBlockHeaderBytes(RecalcBlockHeader::UpdateAndVerify);
}

void TestBlock::setBlockHeader(Ethash::BlockHeader const& _header, RecalcBlockHeader _recalculate)
{
	m_blockHeader = _header;
	recalcBlockHeaderBytes(_recalculate);
}

///Test Block Private
TestBlock::BlockHeader TestBlock::constructBlock(mObject const& _o, h256 const& _stateRoot)
{
	BlockHeader ret;
	try
	{
		const bytes c_blockRLP = createBlockRLPFromFields(_o, _stateRoot);

		RLPStream header(3);
		header.appendRaw(c_blockRLP);			//block header
		header.appendRaw(RLPStream(0).out());	//transactions
		header.appendRaw(RLPStream(0).out());	//uncles

		ret = BlockHeader(header.out(), Strictness::CheckNothing);
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
void TestBlock::recalcBlockHeaderBytes(RecalcBlockHeader _recalculate)
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

	if (_recalculate == RecalcBlockHeader::Update || _recalculate == RecalcBlockHeader::UpdateAndVerify)
	{
		//update hashes correspong to block contents
		if (m_uncles.size())
			m_blockHeader.setSha3Uncles(sha3(uncleStream.out()));

		//if (txList.size())
		//	m_blockHeader.setRoots(sha3(txStream.out()), m_blockHeader.receiptsRoot(), m_blockHeader.sha3Uncles(), m_blockHeader.stateRoot());

		dev::eth::mine(m_blockHeader);
		m_blockHeader.noteDirty();
	}

	RLPStream blHeaderStream;
	m_blockHeader.streamRLP(blHeaderStream, WithProof);

	RLPStream ret(3);
	ret.appendRaw(blHeaderStream.out()); //block header
	ret.appendRaw(txStream.out());		 //transactions
	ret.appendRaw(uncleStream.out());	 //uncles

	if (_recalculate == RecalcBlockHeader::Verify || _recalculate == RecalcBlockHeader::UpdateAndVerify)
	{
		try
		{
			m_blockHeader.verifyInternals(&ret.out());
		}
		catch (Exception const& _e)
		{
			BOOST_ERROR(TestOutputHelper::testName() + "BlockHeader Verification failed: " << diagnostic_information(_e));
		}
		catch(...)
		{
			BOOST_ERROR(TestOutputHelper::testName() + "BlockHeader Verification failed");
		}
	}
	m_bytes = ret.out();
}

void TestBlock::copyStateFrom(State const& _state)
{
	//WEIRD WAY TO COPY STATE AS COPY CONSTRUCTOR FOR STATE NOT IMPLEMENTED CORRECTLY (they would share the same DB)
	m_tempDirState.reset(new TransientDirectory());
	m_state.reset(new State(OverlayDB(State::openDB(m_tempDirState.get()->path(), h256{}, WithExisting::Kill)), BaseState::Empty));
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

void TestBlock::setPremine(std::string const& _parameter)
{
	m_premineUpdate[_parameter] = true;
}

void TestBlock::populateFrom(TestBlock const& _original)
{
	try
	{
		copyStateFrom(_original.getState()); //copy state if it is defined in _original
	}
	catch (BlockStateUndefined const& _ex)
	{
		cnote << _ex.what() << "copying block with null state";
	}
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
	m_tempDirBlockchain = std::unique_ptr<TransientDirectory>(new TransientDirectory());
	m_blockChain = std::unique_ptr<FullBlockChainEthash>(
				new FullBlockChainEthash(_genesisBlock.getBytes(), AccountMap(), m_tempDirBlockchain.get()->path(), WithExisting::Kill));
	m_genesisBlock = _genesisBlock;
	m_lastBlock = m_genesisBlock;
}

void TestBlockChain::reset(TestBlock const& _genesisBlock)
{
	m_tempDirBlockchain.reset(new TransientDirectory());
	m_blockChain.reset(new FullBlockChainEthash(_genesisBlock.getBytes(), AccountMap(), m_tempDirBlockchain.get()->path(), WithExisting::Kill));
	m_genesisBlock = _genesisBlock;
	m_lastBlock = m_genesisBlock;
}

void TestBlockChain::addBlock(TestBlock const& _block)
{
	m_blockChain.get()->import(_block.getBytes(), m_genesisBlock.getState().db());

	//Imported and best
	if (_block.getBytes() == m_blockChain.get()->block())
	{
		m_lastBlock = _block;

		//overwrite state in case _block had no State defined (e.x. created from RLP)
		OverlayDB const& genesisDB = m_genesisBlock.getState().db();
		FullBlockChain<Ethash> const& blockchain = getInterface();
		Block block = (blockchain.genesisBlock(genesisDB));
		block.sync(blockchain);
		m_lastBlock.setState(block.state());
	}
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

}}
