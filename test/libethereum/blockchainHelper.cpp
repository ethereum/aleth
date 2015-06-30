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
/** @file unit.cpp
 * @author Dimitry Khokhlov <Dimitry@ethdev.com>
 * @date 2015
 * libethereum unit test functions coverage.
 */

#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>
#include <test/TestHelper.h>
#include <libethereum/BlockChain.h>
#include <libethereum/CanonBlockChain.h>
#include "../JsonSpiritHeaders.h"

#include <libdevcore/TransientDirectory.h>

using namespace dev;
using namespace eth;
class TestTransaction
{
public:
	TestTransaction(json_spirit::mObject& o_): m_jsonTransaction(o_)	{}
	json_spirit::mObject&  getJson(){return m_jsonTransaction;}

private:
	json_spirit::mObject m_jsonTransaction;
};

class TestBlock
{
public:
	void addTransaction(TestTransaction& tr_)
	{
		m_testTransactions.push_back(tr_);
	}
	std::vector<TestTransaction>& getTransactions()
	{
		return m_testTransactions;
	}
private:
	std::vector<TestTransaction> m_testTransactions;
};

class TestBlockChain
{
public:
	TestBlockChain(BaseState baseState_ = BaseState::CanonGenesis)
	{
		//dev::bytes genesisBlockRLP = dev::test::importByteArray(m_genesisRLPString);
		m_blockchain = new dev::eth::CanonBlockChain(m_td.path(), WithExisting::Verify);
		m_state = dev::eth::State(OverlayDB(State::openDB(m_td2.path())), baseState_, m_blockchain->info().coinbaseAddress);
	}

	~TestBlockChain() {delete m_blockchain;}

	void addBlock(TestBlock& _bl)
	{
		m_testBlocks.push_back(_bl);

		TransactionQueue txs;
		dev::test::ZeroGasPricer gp;

		//import all txs from block in a queue
		unsigned imported = 0;
		for (unsigned i = 0; i < _bl.getTransactions().size(); i++)
		{
			Transaction tr;
			dev::test::ImportTest::importTransaction(_bl.getTransactions().at(i).getJson(), tr);

			if (txs.import(tr.rlp()) != ImportResult::Success)
				cnote << "failed importing transaction\n";
			else imported++;
		}

		cnote << "Transactions imported: " << imported;

		if (imported > 0)
		{
			try
			{
				//calculate new state
				m_state.sync(*m_blockchain);
				m_state.sync(*m_blockchain, txs, gp);
				mine(m_state, *m_blockchain);
			}
			catch (Exception const& _e)
			{
				cnote << "state sync or mining did throw an exception: " << diagnostic_information(_e);
				return;
			}
			catch (std::exception const& _e)
			{
				cnote << "state sync or mining did throw an exception: " << _e.what();
				return;
			}

			//make actual block from it			
			Transactions txList;
			for (auto const& txi: txs.transactions()) //get valid transactions
				txList.push_back(txi.second);

			RLPStream txStream;
			txStream.appendList(txList.size());
			for (unsigned i = 0; i < txList.size(); ++i)
			{
				RLPStream txrlp;
				txList[i].streamRLP(txrlp);
				txStream.appendRaw(txrlp.out());
			}

			//Uncles
			RLPStream uncleStream(0);
			BlockQueue uncleBlockQueue;

			//Make full block stream (with transactions and uncles)
			RLPStream rlpStream;
			m_state.info().streamRLP(rlpStream, WithNonce);

			RLPStream blockStream(3);
			blockStream.appendRaw(rlpStream.out());
			blockStream.appendRaw(txStream.out());
			blockStream.appendRaw(uncleStream.out());

			//store block to the blockchain
			try
			{
				m_blockchain->sync(uncleBlockQueue, m_state.db(), 4);
				m_blockchain->attemptImport(blockStream.out(), m_state.db());
				m_state.sync(*m_blockchain);
			}
			catch (Exception const& _e)
			{
				cnote << "block sync or import did throw an exception: " << diagnostic_information(_e);
				return;
			}
			catch (std::exception const& _e)
			{
				cnote << "block sync or import did throw an exception: " << _e.what();
				return;
			}
		}
	}

private:
	dev::TransientDirectory m_td; //BlockChain Dir
	dev::TransientDirectory m_td2; //State Dir
	std::string m_genesisRLPString = "0xf901fcf901f7a00000000000000000000000000000000000000000000000000000000000000000a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347948888f1f195afa192cfee860698584c030f4c9db1a07dba07d6b448a186e9612e5f737d1c909dce473e53199901a302c00646d523c1a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421b90100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000008302000080832fefd8808454c98c8142a0b28caac6b4c3c1f231ff6e4210a78d3b784988be77d89095038b843fb04440408821dbba66d491fb5ac0c0";
	dev::eth::BlockChain *m_blockchain;
	dev::eth::State m_state;	
	std::vector<TestBlock> m_testBlocks;
};


BOOST_AUTO_TEST_SUITE(blockchainhelper)

BOOST_AUTO_TEST_CASE(blockChainReload)
{
	TestBlockChain blChain;
	TestBlock block;

	json_spirit::mObject trJson;
	trJson["data"] = "";
	trJson["gasLimit"] = "0x59d8";
	trJson["gasPrice"] = "0x01";
	trJson["nonce"] = "0x00";
	trJson["secretKey"] = "45a915e4d060149eb4365960e6a7a45f334393093061116b197e3240065ff2d8";
	trJson["to"] = "d2571607e241ecf590ed94b12d87c94babe36db6";
	trJson["value"] = "0x0a";

	TestTransaction tr(trJson);
	block.addTransaction(tr);
	blChain.addBlock(block);
}

BOOST_AUTO_TEST_SUITE_END()
