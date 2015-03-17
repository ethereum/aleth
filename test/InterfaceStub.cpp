
#include <boost/test/unit_test.hpp>
#include <libdevcore/CommonJS.h>
#include "TestUtils.h"

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

BOOST_FIXTURE_TEST_SUITE(InterfaceStub, InterfaceStubFixture)

BOOST_AUTO_TEST_CASE(postState)
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
	});
}

BOOST_AUTO_TEST_CASE(blocks)
{
	enumerateInterfaces([](Json::Value const& _json, dev::eth::InterfaceStub& _client) -> void
	{
		// number
		unsigned expectedNumber = _json["blocks"].size();
		unsigned number = _client.number();
		BOOST_CHECK_EQUAL(expectedNumber, number);
		
		for (Json::Value const& block: _json["blocks"])
		{
			Json::Value blockHeader = block["blockHeader"];
			
			// hashFromNumber
			h256 expectedHashFromNumber = h256(toBytes(blockHeader["hash"].asString()));
			h256 hashFromNumber = _client.hashFromNumber(jsToInt(blockHeader["number"].asString()));
			BOOST_CHECK_EQUAL(expectedHashFromNumber, hashFromNumber);
			
			// blockInfo
			LogBloom expectedBlockInfoBloom = LogBloom(toBytes(blockHeader["bloom"].asString()));
			Address expectedBlockInfoCoinbase = Address(toBytes(blockHeader["coinbase"].asString()));
			u256 expectedBlockInfoDifficulty = u256(blockHeader["difficulty"].asString());
			bytes expectedBlockInfoExtraData = toBytes(blockHeader["extraData"].asString());
			u256 expectedBlockInfoGasLimit = u256(blockHeader["gasLimit"].asString());
			u256 expectedBlockInfoGasUsed = u256(blockHeader["gasUsed"].asString());
			h256 expectedBlockInfoHash = h256(toBytes(blockHeader["hash"].asString()));
			h256 expectedBlockInfoMixHash = h256(toBytes(blockHeader["mixHash"].asString()));
			Nonce expectedBlockInfoNonce = Nonce(toBytes(blockHeader["nonce"].asString()));
			u256 expectedBlockInfoNumber = u256(blockHeader["number"].asString());
			h256 expectedBlockInfoParentHash = h256(toBytes(blockHeader["parentHash"].asString()));
			h256 expectedBlockInfoReceiptsRoot = h256(toBytes(blockHeader["receiptTrie"].asString()));
			u256 expectedBlockInfoTimestamp = u256(blockHeader["timestamp"].asString());
			h256 expectedBlockInfoTransactionsRoot = h256(toBytes(blockHeader["transactionsTrie"].asString()));
			h256 expectedBlockInfoUncldeHash = h256(toBytes(blockHeader["uncleHash"].asString()));
			BlockInfo blockInfo = _client.blockInfo(expectedBlockInfoHash);
			BOOST_CHECK_EQUAL(expectedBlockInfoBloom, blockInfo.logBloom);
			BOOST_CHECK_EQUAL(expectedBlockInfoCoinbase, blockInfo.coinbaseAddress);
			BOOST_CHECK_EQUAL(expectedBlockInfoDifficulty, blockInfo.difficulty);
			BOOST_CHECK_EQUAL_COLLECTIONS(expectedBlockInfoExtraData.begin(), expectedBlockInfoExtraData.end(),
										  blockInfo.extraData.begin(), blockInfo.extraData.end());
			BOOST_CHECK_EQUAL(expectedBlockInfoGasLimit, blockInfo.gasLimit);
			BOOST_CHECK_EQUAL(expectedBlockInfoGasUsed, blockInfo.gasUsed);
			BOOST_CHECK_EQUAL(expectedBlockInfoHash, blockInfo.hash);
			BOOST_CHECK_EQUAL(expectedBlockInfoMixHash, blockInfo.mixHash);
			BOOST_CHECK_EQUAL(expectedBlockInfoNonce, blockInfo.nonce);
			BOOST_CHECK_EQUAL(expectedBlockInfoNumber, blockInfo.number);
			BOOST_CHECK_EQUAL(expectedBlockInfoParentHash, blockInfo.parentHash);
			BOOST_CHECK_EQUAL(expectedBlockInfoReceiptsRoot, blockInfo.receiptsRoot);
			BOOST_CHECK_EQUAL(expectedBlockInfoTimestamp, blockInfo.timestamp);
			BOOST_CHECK_EQUAL(expectedBlockInfoTransactionsRoot, blockInfo.transactionsRoot);
			BOOST_CHECK_EQUAL(expectedBlockInfoUncldeHash, blockInfo.sha3Uncles);

		}
		
		
		
	});
}

BOOST_AUTO_TEST_SUITE_END()
