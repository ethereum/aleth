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
/** @file jsonrpc.cpp
 * @author Marek Kotewicz <marek@ethdev.com>
 * @date 2014
 */

// @debris disabled as tests fail with:
// unknown location(0): fatal error in "jsonrpc_setMining": std::exception: Exception -32003 : Client connector error: : libcurl error: 28
// /home/gav/Eth/cpp-ethereum/test/jsonrpc.cpp(169): last checkpoint
//#if ETH_JSONRPC

#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <libdevcore/Log.h>
#include <libdevcore/CommonIO.h>
#include <libethcore/CommonJS.h>
#include <libwebthree/WebThree.h>
#include <libweb3jsonrpc/WebThreeStubServer.h>
#include <jsonrpccpp/server/connectors/httpserver.h>
#include <jsonrpccpp/client/connectors/httpclient.h>
#include <libethereum/InterfaceStub.h>
#include <libdevcore/CommonJS.h>
#include <set>
#include "JsonSpiritHeaders.h"
#include "TestHelper.h"
#include "webthreestubclient.h"

//BOOST_AUTO_TEST_SUITE(jsonrpc)

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;
namespace js = json_spirit;

namespace dev { namespace test {
RLPStream createFullBlockFromHeader(const BlockInfo& _bi, const bytes& _txs = RLPEmptyList, const bytes& _uncles = RLPEmptyList);
}};

// helping functions

bytes createBlockRLPFromFieldsX(js::mObject& _tObj)
{
	RLPStream rlpStream;
	rlpStream.appendList(_tObj.count("hash") > 0 ? (_tObj.size() - 1) : _tObj.size());
	
	if (_tObj.count("parentHash"))
		rlpStream << importByteArray(_tObj["parentHash"].get_str());
	
	if (_tObj.count("uncleHash"))
		rlpStream << importByteArray(_tObj["uncleHash"].get_str());
	
	if (_tObj.count("coinbase"))
		rlpStream << importByteArray(_tObj["coinbase"].get_str());
	
	if (_tObj.count("stateRoot"))
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
	
	if (_tObj.count("seedHash"))
		rlpStream << importByteArray(_tObj["seedHash"].get_str());
	
	if (_tObj.count("mixHash"))
		rlpStream << importByteArray(_tObj["mixHash"].get_str());
	
	if (_tObj.count("nonce"))
		rlpStream << importByteArray(_tObj["nonce"].get_str());
	
	return rlpStream.out();
}


BlockInfo constructBlockX(js::mObject& _o)
{
	
	BlockInfo ret;
	try
	{
		// construct genesis block
		const bytes c_blockRLP = createBlockRLPFromFieldsX(_o);
		const RLP c_bRLP(c_blockRLP);
		ret.populateFromHeader(c_bRLP, IgnoreNonce);
	}
	catch (Exception const& _e)
	{
		cnote << "block population did throw an exception: " << diagnostic_information(_e);
		BOOST_ERROR("Failed block population with Exception: " << _e.what());
	}
	catch (std::exception const& _e)
	{
		BOOST_ERROR("Failed block population with Exception: " << _e.what());
	}
	catch(...)
	{
		BOOST_ERROR("block population did throw an unknown exception\n");
	}
	return ret;
}

class FixedStateServer: public dev::WebThreeStubServerBase, public dev::WebThreeStubDatabaseFace
{
public:
	FixedStateServer(jsonrpc::AbstractServerConnector& _conn, std::vector<dev::KeyPair> const& _accounts, dev::eth::Interface* _client): WebThreeStubServerBase(_conn, _accounts), m_client(_client) {};
	
private:
	dev::eth::Interface* client() override { return m_client; }
	std::shared_ptr<dev::shh::Interface> face() override {	BOOST_THROW_EXCEPTION(InterfaceNotSupported("dev::shh::Interface")); }
	dev::WebThreeNetworkFace* network() override { BOOST_THROW_EXCEPTION(InterfaceNotSupported("dev::WebThreeNetworkFace")); }
	dev::WebThreeStubDatabaseFace* db() override { return this; }
	std::string get(std::string const& _name, std::string const& _key) override
	{
		std::string k(_name + "/" + _key);
		return m_db[k];
	}
	void put(std::string const& _name, std::string const& _key, std::string const& _value) override
	{
		std::string k(_name + "/" + _key);
		m_db[k] = _value;
	}

private:
	dev::eth::Interface* m_client;
	std::map<std::string, std::string> m_db;
};

class FixedStateInterface: public dev::eth::InterfaceStub
{
public:
	FixedStateInterface(BlockChain const& _bc, State _state) :  m_bc(_bc), m_state(_state) {}
	virtual ~FixedStateInterface() {}
	
	// stub
	virtual void flushTransactions() override {}
	virtual BlockChain const& bc() const override { return m_bc; }
	virtual State asOf(int _h) const override { (void)_h; return m_state; }
	virtual State asOf(h256 _h) const override { (void)_h; return m_state; }
	virtual State preMine() const override { return m_state; }
	virtual State postMine() const override { return m_state; }
	virtual void prepareForTransaction() override {}
	
private:
	BlockChain const& m_bc;
	State m_state;
};

string fromAscii(string _s)
{
	bytes b = asBytes(_s);
	return "0x" + toHex(b);
}

void doJsonrpcTests(json_spirit::mValue& v, bool _fillin)
{
	for (auto& i: v.get_obj())
	{
		cerr << i.first << endl;
		js::mObject& o = i.second.get_obj();
		
		BOOST_REQUIRE(o.count("genesisBlockHeader"));
		BlockInfo biGenesisBlock = constructBlockX(o["genesisBlockHeader"].get_obj());
		BOOST_REQUIRE(o.count("pre"));
		ImportTest importer(o["pre"].get_obj());
		State state(Address(), OverlayDB(), BaseState::Empty);
		importer.importState(o["pre"].get_obj(), state);
		state.commit();
		
		// create new "genesis" block
		RLPStream rlpGenesisBlock = createFullBlockFromHeader(biGenesisBlock);
		biGenesisBlock.verifyInternals(&rlpGenesisBlock.out());
		
		// construct blockchain
		BlockChain bc(rlpGenesisBlock.out(), string(), true);
		
		for (auto const& bl: o["blocks"].get_array())
		{
			js::mObject blObj = bl.get_obj();
			bytes blockRLP;
			try
			{
				state.sync(bc);
				blockRLP = importByteArray(blObj["rlp"].get_str());
				bc.import(blockRLP, state.db());
				state.sync(bc);
			}
			// if exception is thrown, RLP is invalid and no blockHeader, Transaction list, or Uncle list should be given
			catch (Exception const& _e)
			{
				cnote << "state sync or block import did throw an exception: " << diagnostic_information(_e);
				BOOST_CHECK(blObj.count("blockHeader") == 0);
				BOOST_CHECK(blObj.count("transactions") == 0);
				BOOST_CHECK(blObj.count("uncleHeaders") == 0);
				continue;
			}
			catch (std::exception const& _e)
			{
				cnote << "state sync or block import did throw an exception: " << _e.what();
				BOOST_CHECK(blObj.count("blockHeader") == 0);
				BOOST_CHECK(blObj.count("transactions") == 0);
				BOOST_CHECK(blObj.count("uncleHeaders") == 0);
				continue;
			}
			catch(...)
			{
				cnote << "state sync or block import did throw an exception\n";
				BOOST_CHECK(blObj.count("blockHeader") == 0);
				BOOST_CHECK(blObj.count("transactions") == 0);
				BOOST_CHECK(blObj.count("uncleHeaders") == 0);
				continue;
			}
		}
		
		FixedStateInterface fsi(bc, state);
		unique_ptr<FixedStateServer> jsonrpcServer;
		auto server = new jsonrpc::HttpServer(8080);
		jsonrpcServer.reset(new FixedStateServer(*server, {}, &fsi));
		jsonrpcServer->StartListening();
		
		unique_ptr<WebThreeStubClient> jsonrpcClient;
		auto client = new jsonrpc::HttpClient("http://localhost:8080");
		jsonrpcClient.reset(new WebThreeStubClient(*client));
		
		// eth_coinbase
		
		// eth_mining
		
		// eth_gasPrice
		
		// eth_accounts
		
		// eth_blockNumber
		string number = jsonrpcClient->eth_blockNumber();
		string expectedNumber = toJS(o["blocks"].get_array().size());
		BOOST_CHECK_EQUAL(number, expectedNumber);
		
		// eth_getBalance
		for (auto& acc: o["pre"].get_obj())
		{
			string balance = jsonrpcClient->eth_getBalance(acc.first, "latest");
			js::mObject& a = acc.second.get_obj();
			string expectedBalance = toJS(jsToBytes(a["balance"].get_str()));
//			BOOST_CHECK_EQUAL(balance, expectedBalance);
		}
		
		// eth_getStorage
		
		// eth_getStorageAt
		
		// eth_getTransactionCount
		
		// eth_getBlockTransactionCountByHash
		// eth_getBlockTransactionCountByNumber
		// eth_getUncleCountByBlockHash
		// eth_getUncleCountByBlockNumber
		for (auto const& bl: o["blocks"].get_array())
		{
			js::mObject blObj = bl.get_obj();
			string blockNumber = blObj["blockHeader"].get_obj()["number"].get_str();
			string blockHash = "0x" + blObj["blockHeader"].get_obj()["hash"].get_str();
			
			string tcByNumber = jsonrpcClient->eth_getBlockTransactionCountByNumber(blockNumber);
			string tcByHash = jsonrpcClient->eth_getBlockTransactionCountByHash(blockHash);
			string expectedTc = toJS(blObj["transactions"].get_array().size());
			BOOST_CHECK_EQUAL(tcByNumber, expectedTc);
			BOOST_CHECK_EQUAL(tcByHash, expectedTc);
			
			string ucByNumber = jsonrpcClient->eth_getUncleCountByBlockNumber(blockNumber);
			string ucByHash = jsonrpcClient->eth_getUncleCountByBlockHash(blockHash);
			string expectedUc = toJS(blObj["uncleHeaders"].get_array().size());
			BOOST_CHECK_EQUAL(ucByNumber, expectedUc);
			BOOST_CHECK_EQUAL(ucByHash, expectedUc);
		}
		
		
		
	}
}

BOOST_AUTO_TEST_SUITE(jsonrpc)

BOOST_AUTO_TEST_CASE(bcBlockChainTest)
{
	dev::test::executeTests("bcBlockChainTest", "/BlockTests", doJsonrpcTests);
}

//BOOST_AUTO_TEST_CASE(bcValidBlockTest)
//{
//	dev::test::executeTests("bcValidBlockTest", "/BlockTests", doJsonrpcTests);
//}
//
//BOOST_AUTO_TEST_CASE(bcInvalidHeaderTest)
//{
//	dev::test::executeTests("bcInvalidHeaderTest", "/BlockTests", doJsonrpcTests);
//}
//
//BOOST_AUTO_TEST_CASE(bcUncleTest)
//{
//	dev::test::executeTests("bcUncleTest", "/BlockTests", doJsonrpcTests);
//}

BOOST_AUTO_TEST_SUITE_END()

