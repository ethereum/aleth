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
/** @file ClientBase.cpp
 * @author Marek Kotewicz <marek@ethdev.com>
 * @date 2015
 */

#include <libdevcore/CommonJS.h>
#include <libdevcore/TransientDirectory.h>
#include <libethashseal/Ethash.h>
#include <libethashseal/GenesisInfo.h>
#include <libethereum/BlockChain.h>
#include <libethereum/ClientBase.h>

#include <boost/test/unit_test.hpp>

#include <json/json.h>

using namespace std;
using namespace dev;
using namespace dev::eth;

namespace
{


// should be used for multithread tests
static SharedMutex x_boostTest;
#define ETH_CHECK_EQUAL(x, y) { dev::WriteGuard wg(x_boostTest); BOOST_CHECK_EQUAL(x, y); }
#define ETH_CHECK_EQUAL_COLLECTIONS(xb, xe, yb, ye) { dev::WriteGuard wg(x_boostTest); BOOST_CHECK_EQUAL_COLLECTIONS(xb, xe, yb, ye); }
#define ETH_REQUIRE(x) { dev::WriteGuard wg(x_boostTest); BOOST_REQUIRE(x); }

struct LoadTestFileFixture
{
	LoadTestFileFixture();

protected:
	Json::Value m_json;
};

struct ParallelFixture
{
	void enumerateThreads(std::function<void()> callback) const;
};

struct BlockChainFixture: public LoadTestFileFixture
{
	void enumerateBlockchains(std::function<void(Json::Value const&, dev::eth::BlockChain const&, dev::eth::State state)> callback) const;
};

struct ClientBaseFixture: public BlockChainFixture
{
	void enumerateClients(std::function<void(Json::Value const&, dev::eth::ClientBase&)> callback) const;
};

struct JsonRpcFixture: public ClientBaseFixture
{

};
// important BOOST TEST do have problems with thread safety!!!
// BOOST_CHECK is not thread safe
// BOOST_MESSAGE is not thread safe
// http://boost.2283326.n4.nabble.com/Is-boost-test-thread-safe-td3471644.html
// http://lists.boost.org/boost-users/2010/03/57691.php
// worth reading
// https://codecrafter.wordpress.com/2012/11/01/c-unit-test-framework-adapter-part-3/
struct ParallelClientBaseFixture : public ClientBaseFixture, public ParallelFixture
{
	void enumerateClients(
		std::function<void(Json::Value const &, dev::eth::ClientBase &)> callback) const;
};

class BlockChainLoader
{
public:
	explicit BlockChainLoader(Json::Value const& _json, eth::Network _sealEngineNetwork = eth::Network::TransitionnetTest);
	eth::BlockChain const& bc() const { return *m_bc; }
	eth::State const& state() const { return m_block.state(); }	// TODO remove?
	eth::Block const& block() const { return m_block; }

private:
	TransientDirectory m_dir;
	std::unique_ptr<eth::BlockChain> m_bc;
	eth::Block m_block;
};


BlockChainLoader::BlockChainLoader(Json::Value const& _json, eth::Network _sealEngineNetwork):
	m_block(Block::Null)
{
	// load genesisBlock
	bytes genesisBlock = fromHex(_json["genesisRLP"].asString());

	Json::FastWriter a;
	m_bc.reset(new BlockChain(ChainParams(genesisInfo(_sealEngineNetwork), genesisBlock, jsonToAccountMap(a.write(_json["pre"]))), m_dir.path(), WithExisting::Kill));

	// load pre state
	m_block = m_bc->genesisBlock(State::openDB(m_dir.path(), m_bc->genesisHash(), WithExisting::Kill));

	assert(m_block.rootHash() == m_bc->info().stateRoot());

	// load blocks
	for (auto const& block: _json["blocks"])
	{
		bytes rlp = fromHex(block["rlp"].asString());
		m_bc->import(rlp, state().db());
	}

	// sync state
	m_block.sync(*m_bc);
}


bool getCommandLineOption(string const& _name)
{
	auto argc = boost::unit_test::framework::master_test_suite().argc;
	auto argv = boost::unit_test::framework::master_test_suite().argv;
	bool result = false;
	for (auto i = 0; !result && i < argc; ++i)
		result = _name == argv[i];
	return result;
}

std::string getCommandLineArgument(string const& _name)
{
	auto argc = boost::unit_test::framework::master_test_suite().argc;
	auto argv = boost::unit_test::framework::master_test_suite().argv;
	for (auto i = 1; i < argc; ++i)
	{
		string str = argv[i];
		if (_name == str.substr(0, _name.size()))
			return str.substr(str.find("=") + 1);
	}
	return "";
}

Json::Value loadJsonFromFile(fs::path const& _path)
{
	Json::Reader reader;
	Json::Value result;
	string s = dev::contentsString(_path);
	if (!s.length())
		clog(VerbosityWarning, "test")
			<< "Contents of " << _path.string()
			<< " is empty. Have you cloned the 'tests' repo branch develop and "
			   "set ETHEREUM_TEST_PATH to its path?";
	else
		clog(VerbosityWarning, "test") << "FIXTURE: loaded test from file: " << _path.string();

	reader.parse(s, result);
	return result;
}

LoadTestFileFixture::LoadTestFileFixture()
{
	m_json = loadJsonFromFile(getCommandLineArgument("--eth_testfile"));
}

void ParallelFixture::enumerateThreads(std::function<void()> callback) const
{
	size_t threadsCount = std::stoul(getCommandLineArgument("--eth_threads"), nullptr, 10);

	std::vector<std::thread> workers;
	for (size_t i = 0; i < threadsCount; i++)
		workers.emplace_back(callback);

	for (std::thread& t : workers)
		t.join();
}

void BlockChainFixture::enumerateBlockchains(std::function<void(Json::Value const&, dev::eth::BlockChain const&, State state)> callback) const
{
	for (string const& name: m_json.getMemberNames())
	{
		BlockChainLoader bcl(m_json[name]);
		callback(m_json[name], bcl.bc(), bcl.state());
	}
}

void ClientBaseFixture::enumerateClients(std::function<void(Json::Value const&, dev::eth::ClientBase&)> callback) const
{
	enumerateBlockchains([&callback](Json::Value const& _json, BlockChain const& _bc, State _state) -> void
						 {
							 cerr << "void ClientBaseFixture::enumerateClients. FixedClient now accepts block not sate!" << endl;
							 _state.commit(State::CommitBehaviour::KeepEmptyAccounts); //unused variable. remove this line
							 eth::Block b(Block::Null);
							 b.noteChain(_bc);
							 FixedClient client(_bc, b);
							 callback(_json, client);
						 });
}

void ParallelClientBaseFixture::enumerateClients(std::function<void(Json::Value const&, dev::eth::ClientBase&)> callback) const
{
	ClientBaseFixture::enumerateClients([this, &callback](Json::Value const& _json, dev::eth::ClientBase& _client) -> void
										{
											// json is being copied here
											enumerateThreads([callback, _json, &_client]() -> void
															 {
																 callback(_json, _client);
															 });
										});
}

}

BOOST_FIXTURE_TEST_SUITE(ClientBase, ParallelClientBaseFixture)

BOOST_AUTO_TEST_CASE(blocks)
{
	enumerateClients([](Json::Value const& _json, dev::eth::ClientBase& _client) -> void
	{
		auto compareState = [&_client](Json::Value const& _o, string const& _name, BlockNumber _blockNumber) -> void
		{
			Address address(_name);

			// balanceAt
			u256 expectedBalance = u256(_o["balance"].asString());
			u256 balance = _client.balanceAt(address, _blockNumber);
			ETH_CHECK_EQUAL(expectedBalance, balance);

			// countAt
			u256 expectedCount = u256(_o["nonce"].asString());
			u256 count = _client.countAt(address,  _blockNumber);
			ETH_CHECK_EQUAL(expectedCount, count);

			// stateAt
			for (string const& pos: _o["storage"].getMemberNames())
			{
				u256 expectedState = u256(_o["storage"][pos].asString());
				u256 state = _client.stateAt(address, u256(pos), _blockNumber);
				ETH_CHECK_EQUAL(expectedState, state);
			}

			// codeAt
			bytes expectedCode = fromHex(_o["code"].asString());
			bytes code = _client.codeAt(address, _blockNumber);
			ETH_CHECK_EQUAL_COLLECTIONS(expectedCode.begin(), expectedCode.end(),
										code.begin(), code.end());
		};

		for (string const& name: _json["postState"].getMemberNames())
		{
			Json::Value o = _json["postState"][name];
			compareState(o, name, PendingBlock);
		}

		for (string const& name: _json["pre"].getMemberNames())
		{
			Json::Value o = _json["pre"][name];
			compareState(o, name, 0);
		}

		// number
		unsigned expectedNumber = _json["blocks"].size();
		unsigned number = _client.number();
		ETH_CHECK_EQUAL(expectedNumber, number);
		
		u256 totalDifficulty = u256(_json["genesisBlockHeader"]["difficulty"].asString());
		for (Json::Value const& block: _json["blocks"])
		{
			Json::Value blockHeader = block["blockHeader"];
			Json::Value uncles = block["uncleHeaders"];
			Json::Value transactions = block["transactions"];
			h256 blockHash = h256(fromHex(blockHeader["hash"].asString()));
			
			// just update the difficulty
			for (Json::Value const& uncle: uncles)
			{
				totalDifficulty += u256(uncle["difficulty"].asString());
			}
			
			// hashFromNumber
			h256 expectedHashFromNumber = h256(fromHex(blockHeader["hash"].asString()));
			h256 hashFromNumber = _client.hashFromNumber(jsToInt(blockHeader["number"].asString()));
			ETH_CHECK_EQUAL(expectedHashFromNumber, hashFromNumber);
			
			// blockInfo
			auto compareBlockInfos = [](Json::Value const& _b, BlockHeader _blockInfo) -> void
			{
				LogBloom expectedBlockInfoBloom = LogBloom(fromHex(_b["bloom"].asString()));
				Address expectedBlockInfoCoinbase = Address(fromHex(_b["coinbase"].asString()));
				u256 expectedBlockInfoDifficulty = u256(_b["difficulty"].asString());
				bytes expectedBlockInfoExtraData = fromHex(_b["extraData"].asString());
				u256 expectedBlockInfoGasLimit = u256(_b["gasLimit"].asString());
				u256 expectedBlockInfoGasUsed = u256(_b["gasUsed"].asString());
				h256 expectedBlockInfoHash = h256(fromHex(_b["hash"].asString()));
				h256 expectedBlockInfoMixHash = h256(fromHex(_b["mixHash"].asString()));
				eth::Nonce expectedBlockInfoNonce = eth::Nonce(fromHex(_b["nonce"].asString()));
				u256 expectedBlockInfoNumber = u256(_b["number"].asString());
				h256 expectedBlockInfoParentHash = h256(fromHex(_b["parentHash"].asString()));
				h256 expectedBlockInfoReceiptsRoot = h256(fromHex(_b["receiptTrie"].asString()));
				u256 expectedBlockInfoTimestamp = u256(_b["timestamp"].asString());
				h256 expectedBlockInfoTransactionsRoot = h256(fromHex(_b["transactionsTrie"].asString()));
				h256 expectedBlockInfoUncldeHash = h256(fromHex(_b["uncleHash"].asString()));
				ETH_CHECK_EQUAL(expectedBlockInfoBloom, _blockInfo.logBloom());
				ETH_CHECK_EQUAL(expectedBlockInfoCoinbase, _blockInfo.author());
				ETH_CHECK_EQUAL(expectedBlockInfoDifficulty, _blockInfo.difficulty());
				ETH_CHECK_EQUAL_COLLECTIONS(
					expectedBlockInfoExtraData.begin(),
					expectedBlockInfoExtraData.end(),
					_blockInfo.extraData().begin(),
					_blockInfo.extraData().end()
				);
				ETH_CHECK_EQUAL(expectedBlockInfoGasLimit, _blockInfo.gasLimit());
				ETH_CHECK_EQUAL(expectedBlockInfoGasUsed, _blockInfo.gasUsed());
				ETH_CHECK_EQUAL(expectedBlockInfoHash, _blockInfo.hash());
				ETH_CHECK_EQUAL(expectedBlockInfoMixHash, Ethash::mixHash(_blockInfo));
				ETH_CHECK_EQUAL(expectedBlockInfoNonce, Ethash::nonce(_blockInfo));
				ETH_CHECK_EQUAL(expectedBlockInfoNumber, _blockInfo.number());
				ETH_CHECK_EQUAL(expectedBlockInfoParentHash, _blockInfo.parentHash());
				ETH_CHECK_EQUAL(expectedBlockInfoReceiptsRoot, _blockInfo.receiptsRoot());
				ETH_CHECK_EQUAL(expectedBlockInfoTimestamp, _blockInfo.timestamp());
				ETH_CHECK_EQUAL(expectedBlockInfoTransactionsRoot, _blockInfo.transactionsRoot());
				ETH_CHECK_EQUAL(expectedBlockInfoUncldeHash, _blockInfo.sha3Uncles());
			};

			BlockHeader blockInfo((static_cast<FixedClient&>(_client)).bc().headerData(blockHash), HeaderData);
			compareBlockInfos(blockHeader, blockInfo);

			// blockDetails
			unsigned expectedBlockDetailsNumber = jsToInt(blockHeader["number"].asString());
			totalDifficulty += u256(blockHeader["difficulty"].asString());
			BlockDetails blockDetails = _client.blockDetails(blockHash);
			ETH_CHECK_EQUAL(expectedBlockDetailsNumber, blockDetails.number);
			ETH_CHECK_EQUAL(totalDifficulty, blockDetails.totalDifficulty);
			
			auto compareTransactions = [](Json::Value const& _t, Transaction _transaction) -> void
			{
				bytes expectedTransactionData = fromHex(_t["data"].asString());
				u256 expectedTransactionGasLimit = u256(_t["gasLimit"].asString());
				u256 expectedTransactionGasPrice = u256(_t["gasPrice"].asString());
				u256 expectedTransactionNonce = u256(_t["nonce"].asString());
				u256 expectedTransactionSignatureR = h256(fromHex(_t["r"].asString()));
				u256 expectedTransactionSignatureS = h256(fromHex(_t["s"].asString()));
//				unsigned expectedTransactionSignatureV = jsToInt(t["v"].asString());
				
				ETH_CHECK_EQUAL_COLLECTIONS(
					expectedTransactionData.begin(),
					expectedTransactionData.end(),
					_transaction.data().begin(),
					_transaction.data().end()
				);
				ETH_CHECK_EQUAL(expectedTransactionGasLimit, _transaction.gas());
				ETH_CHECK_EQUAL(expectedTransactionGasPrice, _transaction.gasPrice());
				ETH_CHECK_EQUAL(expectedTransactionNonce, _transaction.nonce());
				ETH_CHECK_EQUAL(expectedTransactionSignatureR, _transaction.signature().r);
				ETH_CHECK_EQUAL(expectedTransactionSignatureS, _transaction.signature().s);
//				ETH_CHECK_EQUAL(expectedTransactionSignatureV, _transaction.signature().v); // 27 === 0x0, 28 === 0x1, not sure why
			};

			Transactions ts = _client.transactions(blockHash);
			TransactionHashes tHashes = _client.transactionHashes(blockHash);
			unsigned tsCount = _client.transactionCount(blockHash);
			
			ETH_REQUIRE(transactions.size() == ts.size());
			ETH_REQUIRE(transactions.size() == tHashes.size());
			
			// transactionCount
			ETH_CHECK_EQUAL(transactions.size(), tsCount);
			
			for (unsigned i = 0; i < tsCount; i++)
			{
				Json::Value t = transactions[i];
				
				// transaction (by block hash and transaction index)
				Transaction transaction = _client.transaction(blockHash, i);
				compareTransactions(t, transaction);

				// transaction (by hash)
				Transaction transactionByHash = _client.transaction(transaction.sha3());
				compareTransactions(t, transactionByHash);
				
				// transactions
				compareTransactions(t, ts[i]);
				
				// transactionHashes
				ETH_CHECK_EQUAL(transaction.sha3(), tHashes[i]);
			}
			
			// uncleCount
			unsigned usCount = _client.uncleCount(blockHash);
			ETH_CHECK_EQUAL(uncles.size(), usCount);
			
			for (unsigned i = 0; i < usCount; i++)
			{
				Json::Value u = uncles[i];
				
				// uncle (by hash)
				BlockHeader uncle = _client.uncle(blockHash, i);
				compareBlockInfos(u, uncle);
			}
		}
	});
}

BOOST_AUTO_TEST_SUITE_END()
