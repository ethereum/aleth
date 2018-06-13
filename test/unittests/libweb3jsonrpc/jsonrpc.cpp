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

#include "WebThreeStubClient.h"
#include <jsonrpccpp/server/abstractserverconnector.h>
#include <libdevcore/CommonIO.h>
#include <libethcore/CommonJS.h>
#include <libweb3jsonrpc/AccountHolder.h>
#include <libweb3jsonrpc/Eth.h>
#include <libweb3jsonrpc/ModularServer.h>
#include <libwebthree/WebThree.h>
#include <test/tools/libtesteth/TestHelper.h>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;
namespace js = json_spirit;

namespace
{
class TestIpcServer : public jsonrpc::AbstractServerConnector
{
public:
    bool StartListening() override { return true; }
    bool StopListening() override { return true; }
    bool SendResponse(std::string const& _response, void* _addInfo = nullptr) override
    {
        *static_cast<std::string*>(_addInfo) = _response;
        return true;
    }
};

class TestIpcClient : public jsonrpc::IClientConnector
{
public:
    explicit TestIpcClient(TestIpcServer& _server) : m_server{_server} {}

    void SendRPCMessage(const std::string& _message, std::string& _result) throw(
        jsonrpc::JsonRpcException) override
    {
        m_server.OnRequest(_message, &_result);
    }

private:
    TestIpcServer& m_server;
};

struct JsonRpcFixture : public TestOutputHelperFixture
{
    JsonRpcFixture()
    {
        dev::p2p::NetworkPreferences nprefs("30303", std::string(), false);
        ChainParams chainParams;
        chainParams.sealEngineName = "NoProof";
        web3.reset(new WebThreeDirect(
            "eth tests", "", "", chainParams, WithExisting::Trust, {"eth"}, nprefs));

        web3->setIdealPeerCount(5);
        using FullServer = ModularServer<rpc::EthFace>;

        accountHolder.reset(new FixedAccountHolder([&]() { return web3->ethereum(); }, {}));
        auto ethFace = new rpc::Eth(*web3->ethereum(), *accountHolder.get());

        jsonrpcIpcServer.reset(new FullServer(ethFace));
        auto ipcServer = new TestIpcServer;
        jsonrpcIpcServer->addConnector(ipcServer);
        ipcServer->StartListening();

        auto client = new TestIpcClient{*ipcServer};
        jsonrpcClient = unique_ptr<WebThreeStubClient>(new WebThreeStubClient(*client));
    }

    unique_ptr<WebThreeDirect> web3;
    unique_ptr<FixedAccountHolder> accountHolder;
    unique_ptr<ModularServer<>> jsonrpcIpcServer;
    unique_ptr<WebThreeStubClient> jsonrpcClient;
};

string fromAscii(string _s)
{
    bytes b = asBytes(_s);
    return toHexPrefixed(b);
}
}

BOOST_FIXTURE_TEST_SUITE(JsonRpcSuite, JsonRpcFixture)


BOOST_AUTO_TEST_CASE(jsonrpc_gasPrice)
{
    string gasPrice = jsonrpcClient->eth_gasPrice();
    BOOST_CHECK_EQUAL(gasPrice, toJS(10 * dev::eth::szabo));
}

BOOST_AUTO_TEST_CASE(jsonrpc_isListening)
{
    web3->startNetwork();
    bool listeningOn = jsonrpcClient->net_listening();
    BOOST_CHECK_EQUAL(listeningOn, web3->isNetworkStarted());

    web3->stopNetwork();
    bool listeningOff = jsonrpcClient->net_listening();
    BOOST_CHECK_EQUAL(listeningOff, web3->isNetworkStarted());
}

BOOST_AUTO_TEST_CASE(jsonrpc_isMining)
{
    web3->ethereum()->startSealing();
    bool miningOn = jsonrpcClient->eth_mining();
    BOOST_CHECK_EQUAL(miningOn, web3->ethereum()->wouldSeal());

    web3->ethereum()->stopSealing();
    bool miningOff = jsonrpcClient->eth_mining();
    BOOST_CHECK_EQUAL(miningOff, web3->ethereum()->wouldSeal());
}

BOOST_AUTO_TEST_CASE(jsonrpc_accounts)
{
    std::vector <dev::KeyPair> keys = {KeyPair::create(), KeyPair::create()};
    accountHolder->setAccounts(keys);
    Json::Value k = jsonrpcClient->eth_accounts();
    accountHolder->setAccounts({});
    BOOST_CHECK_EQUAL(k.isArray(), true);
    BOOST_CHECK_EQUAL(k.size(),  keys.size());
    for (auto &i:k)
    {
        auto it = std::find_if(keys.begin(), keys.end(), [i](dev::KeyPair const& keyPair)
        {
            return jsToAddress(i.asString()) == keyPair.address();
        });
        BOOST_CHECK_EQUAL(it != keys.end(), true);
    }
}

BOOST_AUTO_TEST_CASE(jsonrpc_number)
{
    auto number = boost::lexical_cast<unsigned>(jsonrpcClient->eth_blockNumber());
    BOOST_CHECK_EQUAL(number, web3->ethereum()->number() + 1);
    dev::eth::mine(*(web3->ethereum()), 1);
    auto numberAfter = boost::lexical_cast<unsigned>(jsonrpcClient->eth_blockNumber());
    BOOST_CHECK_EQUAL(number + 1, numberAfter);
    BOOST_CHECK_EQUAL(number, web3->ethereum()->number() + 1);
}

BOOST_AUTO_TEST_CASE(jsonrpc_peerCount)
{
    auto peerCount = boost::lexical_cast<size_t>(jsonrpcClient->net_peerCount());
    BOOST_CHECK_EQUAL(web3->peerCount(), peerCount);
}

BOOST_AUTO_TEST_CASE(jsonrpc_setListening)
{
    jsonrpcClient->admin_net_start("");
    BOOST_CHECK_EQUAL(web3->isNetworkStarted(), true);

    jsonrpcClient->admin_net_stop("");
    BOOST_CHECK_EQUAL(web3->isNetworkStarted(), false);
}

BOOST_AUTO_TEST_CASE(jsonrpc_setMining)
{
    jsonrpcClient->admin_eth_setMining(true, "");
    BOOST_CHECK_EQUAL(web3->ethereum()->wouldSeal(), true);

    jsonrpcClient->admin_eth_setMining(false, "");
    BOOST_CHECK_EQUAL(web3->ethereum()->wouldSeal(), false);
}

BOOST_AUTO_TEST_CASE(jsonrpc_stateAt)
{
    dev::KeyPair key = KeyPair::create();
    auto address = key.address();
    string stateAt = jsonrpcClient->eth_getStorageAt(toJS(address), "0", "latest");
    BOOST_CHECK_EQUAL(toJS(web3->ethereum()->stateAt(address, jsToU256("0"), 0)), stateAt);
}

BOOST_AUTO_TEST_CASE(jsonrpc_transact)
{
    string coinbase = jsonrpcClient->eth_coinbase();
    BOOST_CHECK_EQUAL(jsToAddress(coinbase), web3->ethereum()->author());
    
    dev::KeyPair key = KeyPair::create();
    auto address = key.address();
    auto receiver = KeyPair::create();
    web3->ethereum()->setAuthor(address);

    coinbase = jsonrpcClient->eth_coinbase();
    BOOST_CHECK_EQUAL(jsToAddress(coinbase), web3->ethereum()->author());
    BOOST_CHECK_EQUAL(jsToAddress(coinbase), address);

    accountHolder->setAccounts({key});
    auto balance = web3->ethereum()->balanceAt(address, 0);
    string balanceString = jsonrpcClient->eth_getBalance(toJS(address), "latest");
    auto countAt = boost::lexical_cast<uint64_t>(
        jsonrpcClient->eth_getTransactionCount(toJS(address), "latest"));

    BOOST_CHECK_EQUAL(countAt, (uint64_t)web3->ethereum()->countAt(address));
    BOOST_CHECK_EQUAL(countAt, 0);
    BOOST_CHECK_EQUAL(toJS(balance), balanceString);
    BOOST_CHECK_EQUAL(jsToDecimal(balanceString), "0");
    
    dev::eth::mine(*(web3->ethereum()), 1);
    balance = web3->ethereum()->balanceAt(address, 0);
    balanceString = jsonrpcClient->eth_getBalance(toJS(address), "latest");

    BOOST_CHECK_EQUAL(toJS(balance), balanceString);
    BOOST_CHECK_EQUAL(jsToDecimal(balanceString), "1500000000000000000");
    
    auto txAmount = balance / 2u;
    auto gasPrice = 10 * dev::eth::szabo;
    auto gas = EVMSchedule().txGas;
    
    Json::Value t;
    t["from"] = toJS(address);
    t["value"] = jsToDecimal(toJS(txAmount));
    t["to"] = toJS(receiver.address());
    t["data"] = toJS(bytes());
    t["gas"] = toJS(gas);
    t["gasPrice"] = toJS(gasPrice);

    jsonrpcClient->eth_sendTransaction(t);
    accountHolder->setAccounts({});
    dev::eth::mine(*(web3->ethereum()), 1);

    countAt = boost::lexical_cast<uint64_t>(
        jsonrpcClient->eth_getTransactionCount(toJS(address), "latest"));
    auto balance2 = web3->ethereum()->balanceAt(receiver.address());
    string balanceString2 = jsonrpcClient->eth_getBalance(toJS(receiver.address()), "latest");

    BOOST_CHECK_EQUAL(countAt, (uint64_t)web3->ethereum()->countAt(address));
    BOOST_CHECK_EQUAL(countAt, 1);
    BOOST_CHECK_EQUAL(toJS(balance2), balanceString2);
    BOOST_CHECK_EQUAL(jsToDecimal(balanceString2), "750000000000000000");
    BOOST_CHECK_EQUAL(txAmount, balance2);
}
/*
BOOST_AUTO_TEST_CASE(simple_contract)
{
    KeyPair kp = KeyPair::create();
    web3->ethereum()->setAuthor(kp.address());
    accountHolder->setAccounts({kp});

    dev::eth::mine(*(web3->ethereum()), 1);
    


    //char const* sourceCode = "contract test {\n"
    //"  function f(uint a) returns(uint d) { return a * 7; }\n"
    //"}\n";

    string compiled =
"6080604052341561000f57600080fd5b60b98061001d6000396000f300608060405260043610603f576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff168063b3de648b146044575b600080fd5b3415604e57600080fd5b606a600480360381019080803590602001909291905050506080565b6040518082815260200191505060405180910390f35b60006007820290509190505600a165627a7a72305820f294e834212334e2978c6dd090355312a3f0f9476b8eb98fb480406fc2728a960029";

    Json::Value create;
    create["code"] = compiled;
    string contractAddress = jsonrpcClient->eth_sendTransaction(create);
    dev::eth::mine(*(web3->ethereum()), 1);
    


    Json::Value call;
    call["to"] = contractAddress;
    call["data"] = "0x00000000000000000000000000000000000000000000000000000000000000001";
    string result = jsonrpcClient->eth_call(call, "latest");
    BOOST_CHECK_EQUAL(result, "0x0000000000000000000000000000000000000000000000000000000000000007");
}

BOOST_AUTO_TEST_CASE(contract_storage)
{
    KeyPair kp = KeyPair::create();
    web3->ethereum()->setAuthor(kp.address());
    accountHolder->setAccounts({kp});

    dev::eth::mine(*(web3->ethereum()), 1);
    


 //   char const* sourceCode = R"(
 //       pragma solidity ^0.4.22;

 //       contract test
 //       {
 //           uint hello;
 //           function writeHello(uint value) returns(bool d)
 //           {
 //           hello = value;
 //           return true;
 //           }
 //       }
    //)";
    


    string compiled =
"6080604052341561000f57600080fd5b60c28061001d6000396000f300608060405260043610603f576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806315b2eec3146044575b600080fd5b3415604e57600080fd5b606a600480360381019080803590602001909291905050506084565b604051808215151515815260200191505060405180910390f35b600081600081905550600190509190505600a165627a7a72305820d8407d9cdaaf82966f3fa7a3e665b8cf4e65ee8909b83094a3f856b9051274500029";

    Json::Value create;
    create["code"] = compiled;
    string contractAddress = jsonrpcClient->eth_sendTransaction(create);
    dev::eth::mine(*(web3->ethereum()), 1);
    


    Json::Value transact;
    transact["to"] = contractAddress;
    transact["data"] = "0x00000000000000000000000000000000000000000000000000000000000000003";
    jsonrpcClient->eth_sendTransaction(transact);
    dev::eth::mine(*(web3->ethereum()), 1);
    


    string storage = jsonrpcClient->eth_getStorageAt(contractAddress, "0", "latest");
    BOOST_CHECK_EQUAL(storage, "0x03");
}
*/
BOOST_AUTO_TEST_CASE(sha3)
{
    string testString = "multiply(uint256)";
    h256 expected = dev::sha3(testString);

    auto hexValue = fromAscii(testString);
    string result = jsonrpcClient->web3_sha3(hexValue);
    BOOST_CHECK_EQUAL(toJS(expected), result);
    BOOST_CHECK_EQUAL("0xc6888fa159d67f77c2f3d7a402e199802766bd7e8d4d1ecd2274fc920265d56a", result);
}

BOOST_AUTO_TEST_SUITE_END()
