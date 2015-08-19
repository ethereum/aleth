#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>
#include <test/TestHelper.h>
#include <libethereum/BlockChain.h>
#include <libethereum/CanonBlockChain.h>
#include <libethcore/BasicAuthority.h>
#include "../JsonSpiritHeaders.h"
#include <libethereum/TransactionQueue.h>
#include <libdevcore/TransientDirectory.h>
#include <libethereum/Block.h>

#include "test/fuzzTesting/fuzzHelper.h"
#include <boost/filesystem.hpp>
#include <libdevcore/FileSystem.h>
#include <libethcore/Params.h>
#include <libethereum/CanonBlockChain.h>

using namespace std;
using namespace json_spirit;
using namespace dev;
using namespace dev::eth;
namespace dev {  namespace test {

class TestTransaction
{
public:
	TestTransaction(json_spirit::mObject const& _o): m_jsonTransaction(_o)
	{
		ImportTest::importTransaction(_o, m_transaction); //check that json structure is valid
	}
	json_spirit::mObject const&  getJson() const{ return m_jsonTransaction; }
	eth::Transaction const& getTransaction() const { return m_transaction; }

private:
	json_spirit::mObject m_jsonTransaction;
	eth::Transaction m_transaction;
};

class TestBlock
{
private:
	typedef Ethash::BlockHeader BlockHeader;

public:
	TestBlock(){} //empty block
	TestBlock(json_spirit::mObject& _blockObj, json_spirit::mObject& _stateObj) //genesis block
	{
		m_state = State(OverlayDB(State::openDB(m_tempDirState.path(), h256{}, WithExisting::Kill)), BaseState::Empty);
		ImportTest::importState(_stateObj, m_state);
		m_state.commit();

		m_blockHeader = constructBlock(_blockObj, m_state.rootHash());
		mine(m_blockHeader); //update PoW
		m_blockHeader.noteDirty();
	}
	void addTransaction(TestTransaction const& _tr)
	{
		m_testTransactions.push_back(_tr);
		if (m_transactionQueue.import(_tr.getTransaction().rlp()) != ImportResult::Success)
			cnote << "Test block failed importing transaction\n";
	}
	Address getBeneficiary() { return m_blockHeader.beneficiary();}
	State const& getState() { return m_state; }
	bytes const& getBytes() {
		RLPStream rlpStream;
		m_blockHeader.streamRLP(rlpStream, WithProof);

		RLPStream ret(3);
		ret.appendRaw(rlpStream.out()); //block header
		ret.appendRaw(RLPEmptyList);	//transactions
		ret.appendRaw(RLPEmptyList);	//uncles

		m_blockHeader.verifyInternals(&ret.out());
		m_bytes = ret.out();
		return m_bytes;
	}
	TransactionQueue& getTransactionQueue() { return m_transactionQueue;}

private:
	BlockHeader constructBlock(mObject& _o, h256 const& _stateRoot)
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
	bytes createBlockRLPFromFields(mObject& _tObj, h256 const& _stateRoot)
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

	TransientDirectory m_tempDirState;
	std::vector<TestTransaction> m_testTransactions;
	TransactionQueue m_transactionQueue;
	BlockHeader m_blockHeader;
	State m_state;
	bytes m_bytes;
};


class TestBlockChain
{
private:
	typedef FullBlockChain<Ethash> FullBlockChainEthash;
	typedef Ethash::BlockHeader BlockHeader;

public:
	TestBlockChain(TestBlock& _genesisBlock)
	{
		m_genesisBlockState = _genesisBlock.getState();
		m_genesisCoinbase = _genesisBlock.getBeneficiary();
		m_blockChain = std::unique_ptr<FullBlockChainEthash>(
					new FullBlockChainEthash(_genesisBlock.getBytes(), AccountMap(), m_tempDirBlockchain.path(), WithExisting::Kill));

	}

	void addBlock(TestBlock& _block)
	{
		Block block = (*m_blockChain.get()).genesisBlock(m_genesisBlockState.db());
		block.setBeneficiary(m_genesisCoinbase);
		try
		{
			ZeroGasPricer gp;
			block.sync(*m_blockChain.get());
			block.sync(*m_blockChain.get(), _block.getTransactionQueue(), gp);
			mine(block, *m_blockChain.get());
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
	}
private:
	std::unique_ptr<FullBlockChainEthash> m_blockChain;
	TransientDirectory m_tempDirBlockchain;
	State m_genesisBlockState;
	Address m_genesisCoinbase;
};



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

		TestBlock genesisBlock(o["genesisBlockHeader"].get_obj(), o["pre"].get_obj());
		TestBlockChain trueBc(genesisBlock);

		//writeBlockHeaderToJson(o["genesisBlockHeader"].get_obj(), biGenesisBlock)
		//o["pre"] = fillJsonWithState(trueState); //convert all fields to hex
		//o["genesisRLP"] = toHex(rlpGenesisBlock.out(), 2, HexPrefix::Add);

		if (_fillin)
		{
			TBOOST_REQUIRE(o.count("blocks"));

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

				trueBc.addBlock(block); //mining


			}//each blocks
		}//fillin
	}
}

}}//namespaces


BOOST_AUTO_TEST_SUITE(BlockChainTests)

BOOST_AUTO_TEST_CASE(bc2)
{
	dev::test::executeTests("bcTotalDifficultyTest", "/BlockchainTests",dev::test::getFolder(__FILE__) + "/BlockchainTestsFiller", dev::test::doBlockchainTests2);
}

BOOST_AUTO_TEST_SUITE_END()
