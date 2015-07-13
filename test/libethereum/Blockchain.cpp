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
/** @file Blockchain.cpp
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
#include <memory>

#include <libethereum/GenesisInfo.h>
#include <libdevcore/TransientDirectory.h>
#include <libethereum/TransactionQueue.h>

using namespace dev;
using namespace eth;

class TestTransaction
{
public:
	TestTransaction(json_spirit::mObject& _o): m_jsonTransaction(_o)	{}
	json_spirit::mObject&  getJson(){ return m_jsonTransaction; }

private:
	json_spirit::mObject m_jsonTransaction;
};

class TestBlock
{
public:
	void addTransaction(TestTransaction& _tr)
	{
		m_testTransactions.push_back(_tr);
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
	TestBlockChain(BaseState _baseState = BaseState::CanonGenesis)
	{
		m_blockchain = std::unique_ptr<dev::eth::CanonBlockChain>(new dev::eth::CanonBlockChain(m_td.path(), WithExisting::Kill));
		m_state = dev::eth::State(OverlayDB(State::openDB(m_td2.path())), _baseState, m_blockchain->info().coinbaseAddress);
	}

	unsigned number() { return m_blockchain->number(); }

	void addBlock(TestBlock& _bl)
	{
		m_testBlocks.push_back(_bl);

		TransactionQueue txs;
		dev::test::ZeroGasPricer gp;

		//import all txs from block in a queue
		for (size_t i = 0; i < _bl.getTransactions().size(); i++)
		{
			Transaction tr;
			dev::test::ImportTest::importTransaction(_bl.getTransactions().at(i).getJson(), tr);

			if (txs.import(tr.rlp()) != ImportResult::Success)
				cnote << "failed importing transaction\n";
		}

		try
		{
			//calculate new state
			m_state.sync(*m_blockchain.get());
			m_state.sync(*m_blockchain.get(), txs, gp);
			mine(m_state, *m_blockchain.get());
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
		for (auto const& txi: txs.topTransactions(std::numeric_limits<unsigned>::max()))
			txList.push_back(txi);			//get valid transactions

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
			m_state.sync(*m_blockchain.get());
		}
		catch (Exception const& _e)
		{
			cnote << "block sync or import did throw an exception: " << diagnostic_information(_e);
			m_testBlocks.pop_back();
			return;
		}
		catch (std::exception const& _e)
		{
			cnote << "block sync or import did throw an exception: " << _e.what();
			m_testBlocks.pop_back();
			return;
		}
	}

	dev::eth::BlockChain& get()
	{
		return *m_blockchain.get();
	}

	void rebuild(WithExisting _withExisiting = WithExisting::Verify)
	{
		m_blockchain.reset();
		m_blockchain.reset(new dev::eth::CanonBlockChain(m_td.path(), _withExisiting));
	}	

private:
	dev::TransientDirectory m_td;				//BlockChain Dir
	dev::TransientDirectory m_td2;				//State Dir
	std::unique_ptr<dev::eth::BlockChain> m_blockchain;
	dev::eth::State m_state;	
	std::vector<TestBlock> m_testBlocks;
};

std::string const c_genesisInfoTest =
R"ETHEREUM(
{
	"a94f5374fce5edbc8e2a8697c15331677e6ebf0b": { "wei": "1606938044258990275541962092341162602522202993782792835301376" },
	"0000000000000000000000000000000000000001": { "wei": "1" },
	"0000000000000000000000000000000000000002": { "wei": "1" },
	"0000000000000000000000000000000000000003": { "wei": "1" },
	"0000000000000000000000000000000000000004": { "wei": "1" },
	"dbdbdb2cbd23b783741e8d7fcf51e459b497e4a6": { "wei": "1606938044258990275541962092341162602522202993782792835301376" },
	"e6716f9544a56c530d868e4bfbacb172315bdead": { "wei": "1606938044258990275541962092341162602522202993782792835301376" },
	"b9c015918bdaba24b4ff057a92a3873d6eb201be": { "wei": "1606938044258990275541962092341162602522202993782792835301376" },
	"1a26338f0d905e295fccb71fa9ea849ffa12aaf4": { "wei": "1606938044258990275541962092341162602522202993782792835301376" },
	"2ef47100e0787b915105fd5e3f4ff6752079d5cb": { "wei": "1606938044258990275541962092341162602522202993782792835301376" },
	"cd2a3d9f938e13cd947ec05abc7fe734df8dd826": { "wei": "1606938044258990275541962092341162602522202993782792835301376" },
	"6c386a4b26f73c802f34673f7248bb118f97424a": { "wei": "1606938044258990275541962092341162602522202993782792835301376" },
	"e4157b34ea9615cfbde6b4fda419828124b70c78": { "wei": "1606938044258990275541962092341162602522202993782792835301376" },
}
)ETHEREUM";

BOOST_AUTO_TEST_SUITE(libethereum)

BOOST_AUTO_TEST_CASE(canonBlockChain)
{
	std::string genesisTemp = dev::eth::c_genesisInfo;

	//A trick for blockchain to load changed genesis state
	std::string *genesis_ptr;
	genesis_ptr = const_cast<std::string*>( &dev::eth::c_genesisInfo );
	*genesis_ptr = c_genesisInfoTest;

	TestBlockChain blChain;

	//Test Nonce
	Nonce nonce(u64(42));
	BlockInfo a = blChain.get().info();
	BOOST_CHECK_MESSAGE(a.nonce == nonce, "Blockchain created with diffrent genesis nonce! (u64(42))");

	nonce = Nonce(u64(142));
	dev::eth::CanonBlockChain::setGenesisNonce(nonce);
	blChain.rebuild(WithExisting::Kill);
	a = blChain.get().info();
	BOOST_CHECK_MESSAGE(a.nonce == nonce, "Blockchain setGenesisNonce error!");

	//restore original nonce
	nonce = Nonce(u64(42));
	dev::eth::CanonBlockChain::setGenesisNonce(nonce);
	blChain.rebuild(WithExisting::Kill);

	json_spirit::mObject trJson;
	trJson["data"] = "";
	trJson["gasLimit"] = "0x59d8";
	trJson["gasPrice"] = "0x01";
	trJson["nonce"] = "0x00";
	trJson["secretKey"] = "45a915e4d060149eb4365960e6a7a45f334393093061116b197e3240065ff2d8";
	trJson["to"] = "d2571607e241ecf590ed94b12d87c94babe36db6";
	trJson["value"] = "0x0a";

	TestBlock block;
	TestTransaction tr(trJson);
	block.addTransaction(tr);
	blChain.addBlock(block);

	json_spirit::mObject trJson2;
	trJson2["data"] = "";
	trJson2["gasLimit"] = "0x59d8";
	trJson2["gasPrice"] = "0x01";
	trJson2["nonce"] = "0x01";
	trJson2["secretKey"] = "45a915e4d060149eb4365960e6a7a45f334393093061116b197e3240065ff2d8";
	trJson2["to"] = "d2571607e241ecf590ed94b12d87c94babe36db6";
	trJson2["value"] = "0x0a";

	TestBlock block2;
	TestTransaction tr2(trJson2);
	block2.addTransaction(tr2);
	blChain.addBlock(block2);

	BOOST_CHECK_MESSAGE(blChain.number() == 2, "Block chain simple block import error!");

	blChain.rebuild();

	BOOST_CHECK_MESSAGE(blChain.number() == 2, "Block chain size mismatch after rebuild!");

	*genesis_ptr = genesisTemp;
}

BOOST_AUTO_TEST_SUITE_END()
