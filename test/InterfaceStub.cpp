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
/** @file InterfaceStub.cpp
 * @author Marek Kotewicz <marek@ethdev.com>
 * @date 2015
 */

#include <boost/test/unit_test.hpp>
#include <libdevcore/CommonJS.h>
#include "TestUtils.h"

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

BOOST_FIXTURE_TEST_SUITE(InterfaceStub, ParallelInterfaceStubFixture)

BOOST_AUTO_TEST_CASE(blocks)
{
	enumerateInterfaces([](Json::Value const& _json, dev::eth::InterfaceStub& _client) -> void
	{
		for (string const& name: _json["postState"].getMemberNames())
		{
			Json::Value o = _json["postState"][name];
			Address address(name);

			// balanceAt
			u256 expectedBalance = u256(o["balance"].asString());
			u256 balance = _client.balanceAt(address);
			BOOST_CHECK_EQUAL(expectedBalance, balance);

			// countAt
			u256 expectedCount = u256(o["nonce"].asString());
			u256 count = _client.countAt(address);
			BOOST_CHECK_EQUAL(expectedCount, count);

			// stateAt
			for (string const& pos: o["storage"].getMemberNames())
			{
				u256 expectedState = u256(o["storage"][pos].asString());
				u256 state = _client.stateAt(address, u256(pos));
				BOOST_CHECK_EQUAL(expectedState, state);
			}

			// codeAt
			bytes expectedCode = toBytes(o["code"].asString());
			bytes code = _client.codeAt(address);
			BOOST_CHECK_EQUAL_COLLECTIONS(expectedCode.begin(), expectedCode.end(),
										  code.begin(), code.end());
		}

		// number
		unsigned expectedNumber = _json["blocks"].size();
		unsigned number = _client.number();
		BOOST_CHECK_EQUAL(expectedNumber, number);
		
		u256 totalDifficulty = u256(_json["genesisBlockHeader"]["difficulty"].asString());
		for (Json::Value const& block: _json["blocks"])
		{
			Json::Value blockHeader = block["blockHeader"];
			Json::Value uncles = block["uncleHeaders"];
			Json::Value transactions = block["transactions"];
			h256 blockHash = h256(toBytes(blockHeader["hash"].asString()));
			
			// just update the difficulty
			for (Json::Value const& uncle: uncles)
			{
				totalDifficulty += u256(uncle["difficulty"].asString());
			}
			
			// hashFromNumber
			h256 expectedHashFromNumber = h256(toBytes(blockHeader["hash"].asString()));
			h256 hashFromNumber = _client.hashFromNumber(jsToInt(blockHeader["number"].asString()));
			BOOST_CHECK_EQUAL(expectedHashFromNumber, hashFromNumber);
			
			// blockInfo
			auto compareBlockInfos = [](Json::Value const& _b, BlockInfo _blockInfo) -> void
			{
				LogBloom expectedBlockInfoBloom = LogBloom(toBytes(_b["bloom"].asString()));
				Address expectedBlockInfoCoinbase = Address(toBytes(_b["coinbase"].asString()));
				u256 expectedBlockInfoDifficulty = u256(_b["difficulty"].asString());
				bytes expectedBlockInfoExtraData = toBytes(_b["extraData"].asString());
				u256 expectedBlockInfoGasLimit = u256(_b["gasLimit"].asString());
				u256 expectedBlockInfoGasUsed = u256(_b["gasUsed"].asString());
				h256 expectedBlockInfoHash = h256(toBytes(_b["hash"].asString()));
				h256 expectedBlockInfoMixHash = h256(toBytes(_b["mixHash"].asString()));
				Nonce expectedBlockInfoNonce = Nonce(toBytes(_b["nonce"].asString()));
				u256 expectedBlockInfoNumber = u256(_b["number"].asString());
				h256 expectedBlockInfoParentHash = h256(toBytes(_b["parentHash"].asString()));
				h256 expectedBlockInfoReceiptsRoot = h256(toBytes(_b["receiptTrie"].asString()));
				u256 expectedBlockInfoTimestamp = u256(_b["timestamp"].asString());
				h256 expectedBlockInfoTransactionsRoot = h256(toBytes(_b["transactionsTrie"].asString()));
				h256 expectedBlockInfoUncldeHash = h256(toBytes(_b["uncleHash"].asString()));
				BOOST_CHECK_EQUAL(expectedBlockInfoBloom, _blockInfo.logBloom);
				BOOST_CHECK_EQUAL(expectedBlockInfoCoinbase, _blockInfo.coinbaseAddress);
				BOOST_CHECK_EQUAL(expectedBlockInfoDifficulty, _blockInfo.difficulty);
				BOOST_CHECK_EQUAL_COLLECTIONS(expectedBlockInfoExtraData.begin(), expectedBlockInfoExtraData.end(),
											  _blockInfo.extraData.begin(), _blockInfo.extraData.end());
				BOOST_CHECK_EQUAL(expectedBlockInfoGasLimit, _blockInfo.gasLimit);
				BOOST_CHECK_EQUAL(expectedBlockInfoGasUsed, _blockInfo.gasUsed);
				BOOST_CHECK_EQUAL(expectedBlockInfoHash, _blockInfo.hash);
				BOOST_CHECK_EQUAL(expectedBlockInfoMixHash, _blockInfo.mixHash);
				BOOST_CHECK_EQUAL(expectedBlockInfoNonce, _blockInfo.nonce);
				BOOST_CHECK_EQUAL(expectedBlockInfoNumber, _blockInfo.number);
				BOOST_CHECK_EQUAL(expectedBlockInfoParentHash, _blockInfo.parentHash);
				BOOST_CHECK_EQUAL(expectedBlockInfoReceiptsRoot, _blockInfo.receiptsRoot);
				BOOST_CHECK_EQUAL(expectedBlockInfoTimestamp, _blockInfo.timestamp);
				BOOST_CHECK_EQUAL(expectedBlockInfoTransactionsRoot, _blockInfo.transactionsRoot);
				BOOST_CHECK_EQUAL(expectedBlockInfoUncldeHash, _blockInfo.sha3Uncles);
			};
			

			BlockInfo blockInfo = _client.blockInfo(blockHash);
			compareBlockInfos(blockHeader, blockInfo);

			// blockDetails
			unsigned expectedBlockDetailsNumber = jsToInt(blockHeader["number"].asString());
			totalDifficulty += u256(blockHeader["difficulty"].asString());
			BlockDetails blockDetails = _client.blockDetails(blockHash);
			BOOST_CHECK_EQUAL(expectedBlockDetailsNumber, blockDetails.number);
			BOOST_CHECK_EQUAL(totalDifficulty, blockDetails.totalDifficulty);
			
			auto compareTransactions = [](Json::Value const& _t, Transaction _transaction) -> void
			{
				bytes expectedTransactionData = toBytes(_t["data"].asString());
				u256 expectedTransactionGasLimit = u256(_t["gasLimit"].asString());
				u256 expectedTransactionGasPrice = u256(_t["gasPrice"].asString());
				u256 expectedTransactionNonce = u256(_t["nonce"].asString());
				u256 expectedTransactionSignatureR = h256(toBytes(_t["r"].asString()));
				u256 expectedTransactionSignatureS = h256(toBytes(_t["s"].asString()));
//				unsigned expectedTransactionSignatureV = jsToInt(t["v"].asString());
				
				BOOST_CHECK_EQUAL_COLLECTIONS(expectedTransactionData.begin(), expectedTransactionData.end(),
											  _transaction.data().begin(), _transaction.data().end());
				BOOST_CHECK_EQUAL(expectedTransactionGasLimit, _transaction.gas());
				BOOST_CHECK_EQUAL(expectedTransactionGasPrice, _transaction.gasPrice());
				BOOST_CHECK_EQUAL(expectedTransactionNonce, _transaction.nonce());
				BOOST_CHECK_EQUAL(expectedTransactionSignatureR, _transaction.signature().r);
				BOOST_CHECK_EQUAL(expectedTransactionSignatureS, _transaction.signature().s);
//				BOOST_CHECK_EQUAL(expectedTransactionSignatureV, _transaction.signature().v); // 27 === 0x0, 28 === 0x1, not sure why
			};

			Transactions ts = _client.transactions(blockHash);
			TransactionHashes tHashes = _client.transactionHashes(blockHash);
			unsigned tsCount = _client.transactionCount(blockHash);
			
			BOOST_REQUIRE(transactions.size() == ts.size());
			BOOST_REQUIRE(transactions.size() == tHashes.size());
			
			// transactionCount
			BOOST_CHECK_EQUAL(transactions.size(), tsCount);
			
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
				BOOST_CHECK_EQUAL(transaction.sha3(), tHashes[i]);
			}
			
			// uncleCount
			unsigned usCount = _client.uncleCount(blockHash);
			BOOST_CHECK_EQUAL(uncles.size(), usCount);
			
			for (unsigned i = 0; i < usCount; i++)
			{
				Json::Value u = uncles[i];
				
				// uncle (by hash)
				BlockInfo uncle = _client.uncle(blockHash, i);
				compareBlockInfos(u, uncle);
			}
		}
	});
}

BOOST_AUTO_TEST_SUITE_END()
