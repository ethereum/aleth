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
#include "TestUtils.h"

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;
namespace js = json_spirit;

namespace dev { namespace test {
RLPStream createFullBlockFromHeader(const BlockInfo& _bi, const bytes& _txs = RLPEmptyList, const bytes& _uncles = RLPEmptyList);
}}

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
	(void)_fillin;
	for (auto& i: v.get_obj())
	{
		cerr << i.first << endl;
		js::mObject& o = i.second.get_obj();
		
		BOOST_REQUIRE(o.count("genesisBlockHeader"));
		BlockInfo biGenesisBlock = constructBlock(o["genesisBlockHeader"].get_obj());
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
	dev::test::executeTests("bcJS_API_Test", "/BlockTests", doJsonrpcTests);
}

BOOST_AUTO_TEST_SUITE_END()

