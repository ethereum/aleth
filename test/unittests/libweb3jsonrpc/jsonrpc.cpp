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

#pragma GCC diagnostic ignored "-Wdeprecated"

#include "WebThreeStubClient.h"
#include <jsonrpccpp/server/abstractserverconnector.h>
#include <libdevcore/CommonIO.h>
#include <libethcore/CommonJS.h>
#include <libethcore/KeyManager.h>
#include <libweb3jsonrpc/AccountHolder.h>
#include <libweb3jsonrpc/AdminEth.h>
#include <libweb3jsonrpc/AdminNet.h>
#include <libweb3jsonrpc/Debug.h>
#include <libweb3jsonrpc/Eth.h>
#include <libweb3jsonrpc/ModularServer.h>
#include <libweb3jsonrpc/Net.h>
#include <libweb3jsonrpc/Test.h>
#include <libweb3jsonrpc/Web3.h>
#include <libwebthree/WebThree.h>
#include <test/tools/libtesteth/TestHelper.h>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>

// This is defined by some weird windows header - workaround for now.
#undef GetMessage

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

static std::string const c_genesisConfigString = R"(
{
    "sealEngine": "NoProof",
    "params": {
         "accountStartNonce": "0x00",
         "maximumExtraDataSize": "0x1000000",
         "blockReward": "0x",
         "allowFutureBlocks": true,
         "homesteadForkBlock": "0x00",
         "EIP150ForkBlock": "0x00",
         "EIP158ForkBlock": "0x00"
    },
    "genesis": {
        "author" : "0x2adc25665018aa1fe0e6bc666dac8fc2697ff9ba",
        "difficulty" : "0x20000",
        "gasLimit" : "0x0f4240",
        "nonce" : "0x00",
        "extraData" : "0x00",
        "timestamp" : "0x00",
        "mixHash" : "0x00"
    },
    "accounts": {
        "0000000000000000000000000000000000000001": { "precompiled": { "name": "ecrecover", "linear": { "base": 3000, "word": 0 } } },
        "0000000000000000000000000000000000000002": { "precompiled": { "name": "sha256", "linear": { "base": 60, "word": 12 } } },
        "0000000000000000000000000000000000000003": { "precompiled": { "name": "ripemd160", "linear": { "base": 600, "word": 120 } } },
        "0000000000000000000000000000000000000004": { "precompiled": { "name": "identity", "linear": { "base": 15, "word": 3 } } },
        "0x095e7baea6a6c7c4c2dfeb977efac326af552d87" : {
            "balance" : "0x0de0b6b3a7640000",
            "code" : "0x6001600101600055",
            "nonce" : "0x00",
            "storage" : {
            }
        },
        "0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b" : {
            "balance" : "0x0de0b6b3a7640000",
            "code" : "0x",
            "nonce" : "0x00",
            "storage" : {
            }
        }
    }
}
)";


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
        dev::p2p::NetworkConfig nprefs;
        ChainParams chainParams;
        chainParams.sealEngineName = NoProof::name();
        chainParams.allowFutureBlocks = true;
        chainParams.difficulty = chainParams.minimumDifficulty;
        chainParams.gasLimit = chainParams.maxGasLimit;
        // add random extra data to randomize genesis hash and get random DB path,
        // so that tests can be run in parallel
        // TODO: better make it use ethemeral in-memory databases
        chainParams.extraData = h256::random().asBytes();
        web3.reset(new WebThreeDirect(
            "eth tests", "", "", chainParams, WithExisting::Kill, nprefs, bytesConstRef(), true));

        web3->setIdealPeerCount(5);

        web3->ethereum()->setAuthor(coinbase.address());

        using FullServer = ModularServer<rpc::EthFace, rpc::NetFace, rpc::Web3Face,
            rpc::AdminEthFace, rpc::AdminNetFace, rpc::DebugFace, rpc::TestFace>;

        accountHolder.reset(new FixedAccountHolder([&]() { return web3->ethereum(); }, {}));
        accountHolder->setAccounts({coinbase});

        sessionManager.reset(new rpc::SessionManager());
        adminSession = sessionManager->newSession(rpc::SessionPermissions{{rpc::Privilege::Admin}});

        auto ethFace = new rpc::Eth(*web3->ethereum(), *accountHolder.get());

        gasPricer = make_shared<eth::TrivialGasPricer>(0, DefaultGasPrice);

        rpcServer.reset(
            new FullServer(ethFace, new rpc::Net(*web3), new rpc::Web3(web3->clientVersion()),
                new rpc::AdminEth(*web3->ethereum(), *gasPricer, keyManager, *sessionManager.get()),
                new rpc::AdminNet(*web3, *sessionManager), new rpc::Debug(*web3->ethereum()),
                new rpc::Test(*web3->ethereum())));
        auto ipcServer = new TestIpcServer;
        rpcServer->addConnector(ipcServer);
        ipcServer->StartListening();

        client = unique_ptr<TestIpcClient>(new TestIpcClient{*ipcServer});
        rpcClient = unique_ptr<WebThreeStubClient>(new WebThreeStubClient(*client));
    }

    string sendingRawShouldFail(string const& _t)
    {
        try
        {
            rpcClient->eth_sendRawTransaction(_t);
            BOOST_FAIL("Exception expected.");
        }
        catch (jsonrpc::JsonRpcException const& _e)
        {
            return _e.GetMessage();
        }
        return string();
    }

    unique_ptr<WebThreeDirect> web3;
    dev::KeyPair coinbase{KeyPair::create()};
    unique_ptr<FixedAccountHolder> accountHolder;
    unique_ptr<rpc::SessionManager> sessionManager;
    std::shared_ptr<eth::TrivialGasPricer> gasPricer;
    KeyManager keyManager{KeyManager::defaultPath(), SecretStore::defaultPath()};
    unique_ptr<ModularServer<>> rpcServer;
    unique_ptr<TestIpcClient> client;
    unique_ptr<WebThreeStubClient> rpcClient;
    std::string adminSession;
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
    string gasPrice = rpcClient->eth_gasPrice();
    BOOST_CHECK_EQUAL(gasPrice, toJS(20 * dev::eth::shannon));
}

BOOST_AUTO_TEST_CASE(jsonrpc_isListening)
{
    web3->startNetwork();
    bool listeningOn = rpcClient->net_listening();
    BOOST_CHECK_EQUAL(listeningOn, web3->isNetworkStarted());

    web3->stopNetwork();
    bool listeningOff = rpcClient->net_listening();
    BOOST_CHECK_EQUAL(listeningOff, web3->isNetworkStarted());
}

BOOST_AUTO_TEST_CASE(jsonrpc_accounts)
{
    std::vector <dev::KeyPair> keys = {KeyPair::create(), KeyPair::create()};
    accountHolder->setAccounts(keys);
    Json::Value k = rpcClient->eth_accounts();
    accountHolder->setAccounts({});
    BOOST_CHECK_EQUAL(k.isArray(), true);
    BOOST_CHECK_EQUAL(k.size(),  keys.size());
    for (auto &i:k)
    {
        auto it = std::find_if(keys.begin(), keys.end(), [i](dev::KeyPair const& keyPair) {
            return jsToAddress(i.asString()) == keyPair.address();
        });
        BOOST_CHECK_EQUAL(it != keys.end(), true);
    }
}

BOOST_AUTO_TEST_CASE(jsonrpc_number)
{
    auto number = jsToU256(rpcClient->eth_blockNumber());
    BOOST_CHECK_EQUAL(number, web3->ethereum()->number());
    dev::eth::mine(*(web3->ethereum()), 1);
    auto numberAfter = jsToU256(rpcClient->eth_blockNumber());
    BOOST_CHECK_GE(numberAfter, number + 1);
    BOOST_CHECK_EQUAL(numberAfter, web3->ethereum()->number());
}

BOOST_AUTO_TEST_CASE(jsonrpc_peerCount)
{
    auto peerCount = jsToU256(rpcClient->net_peerCount());
    BOOST_CHECK_EQUAL(web3->peerCount(), peerCount);
}

BOOST_AUTO_TEST_CASE(jsonrpc_setMining)
{
    rpcClient->admin_eth_setMining(true, adminSession);
    BOOST_CHECK_EQUAL(web3->ethereum()->wouldSeal(), true);

    rpcClient->admin_eth_setMining(false, adminSession);
    BOOST_CHECK_EQUAL(web3->ethereum()->wouldSeal(), false);
}

BOOST_AUTO_TEST_CASE(jsonrpc_stateAt)
{
    dev::KeyPair key = KeyPair::create();
    auto address = key.address();
    string stateAt = rpcClient->eth_getStorageAt(toJS(address), "0", "latest");
    BOOST_CHECK_EQUAL(web3->ethereum()->stateAt(address, 0, 0), jsToU256(stateAt));
}

BOOST_AUTO_TEST_CASE(eth_coinbase)
{
    string coinbase = rpcClient->eth_coinbase();
    BOOST_REQUIRE_EQUAL(jsToAddress(coinbase), web3->ethereum()->author());
}

BOOST_AUTO_TEST_CASE(eth_sendTransaction)
{
    auto address = coinbase.address();
    auto countAt = jsToU256(rpcClient->eth_getTransactionCount(toJS(address), "latest"));

    BOOST_CHECK_EQUAL(countAt, web3->ethereum()->countAt(address));
    BOOST_CHECK_EQUAL(countAt, 0);
    auto balance = web3->ethereum()->balanceAt(address, 0);
    string balanceString = rpcClient->eth_getBalance(toJS(address), "latest");
    BOOST_CHECK_EQUAL(toJS(balance), balanceString);
    BOOST_CHECK_EQUAL(jsToDecimal(balanceString), "0");

    dev::eth::mine(*(web3->ethereum()), 1);
    BOOST_CHECK_EQUAL(web3->ethereum()->blockByNumber(LatestBlock).author(), address);
    balance = web3->ethereum()->balanceAt(address, LatestBlock);
    balanceString = rpcClient->eth_getBalance(toJS(address), "latest");

    BOOST_REQUIRE_GT(balance, 0);
    BOOST_CHECK_EQUAL(toJS(balance), balanceString);


    auto txAmount = balance / 2u;
    auto gasPrice = 10 * dev::eth::szabo;
    auto gas = EVMSchedule().txGas;

    auto receiver = KeyPair::create();

    Json::Value t;
    t["from"] = toJS(address);
    t["value"] = jsToDecimal(toJS(txAmount));
    t["to"] = toJS(receiver.address());
    t["data"] = toJS(bytes());
    t["gas"] = toJS(gas);
    t["gasPrice"] = toJS(gasPrice);

    std::string txHash = rpcClient->eth_sendTransaction(t);
    BOOST_REQUIRE(!txHash.empty());

    accountHolder->setAccounts({});
    dev::eth::mine(*(web3->ethereum()), 1);

    countAt = jsToU256(
        rpcClient->eth_getTransactionCount(toJS(address), "latest"));
    auto balance2 = web3->ethereum()->balanceAt(receiver.address());
    string balanceString2 = rpcClient->eth_getBalance(toJS(receiver.address()), "latest");

    BOOST_CHECK_EQUAL(countAt, web3->ethereum()->countAt(address));
    BOOST_CHECK_EQUAL(countAt, 1);
    BOOST_CHECK_EQUAL(toJS(balance2), balanceString2);
    BOOST_CHECK_EQUAL(jsToU256(balanceString2), txAmount);
    BOOST_CHECK_EQUAL(txAmount, balance2);
}

BOOST_AUTO_TEST_CASE(eth_sendRawTransaction_validTransaction)
{
    auto senderAddress = coinbase.address();
    auto receiver = KeyPair::create();

    Json::Value t;
    t["from"] = toJS(senderAddress);
    t["to"] = toJS(receiver.address());
    t["value"] = jsToDecimal(toJS(10000 * dev::eth::szabo));

    // Mine to generate a non-zero account balance
    const int blocksToMine = 1;
    const int blockNumber = 1;
    const u256 blockReward = 5 * dev::eth::ether;
    dev::eth::mine(*(web3->ethereum()), blocksToMine);
    BOOST_CHECK_EQUAL(blockReward, web3->ethereum()->balanceAt(senderAddress, blockNumber));

    auto signedTx = rpcClient->eth_signTransaction(t);
    BOOST_REQUIRE(!signedTx["raw"].empty());

    auto txHash = rpcClient->eth_sendRawTransaction(signedTx["raw"].asString());
    BOOST_REQUIRE(!txHash.empty());
}

BOOST_AUTO_TEST_CASE(eth_sendRawTransaction_errorZeroBalance)
{
    auto senderAddress = coinbase.address();
    auto receiver = KeyPair::create();

    const int blockNumber = 0;
    BOOST_CHECK_EQUAL(0, web3->ethereum()->balanceAt(senderAddress, blockNumber));

    Json::Value t;
    t["from"] = toJS(senderAddress);
    t["to"] = toJS(receiver.address());
    t["value"] = jsToDecimal(toJS(10000 * dev::eth::szabo));
    
    auto signedTx = rpcClient->eth_signTransaction(t);
    BOOST_REQUIRE(signedTx["raw"]);

    BOOST_CHECK_EQUAL(sendingRawShouldFail(signedTx["raw"].asString()), "Account balance is too low (balance < value + gas * gas price).");
}

BOOST_AUTO_TEST_CASE(eth_sendRawTransaction_errorInvalidNonce)
{
    auto senderAddress = coinbase.address();
    auto receiver = KeyPair::create();

    // Mine to generate a non-zero account balance
    const int blocksToMine = 1;
    const int blockNumber = 1;
    const u256 blockReward = 5 * dev::eth::ether;
    dev::eth::mine(*(web3->ethereum()), blocksToMine);
    BOOST_CHECK_EQUAL(blockReward, web3->ethereum()->balanceAt(senderAddress, blockNumber));

    Json::Value t;
    t["from"] = toJS(senderAddress);
    t["to"] = toJS(receiver.address());
    t["value"] = jsToDecimal(toJS(10000 * dev::eth::szabo));

    auto signedTx = rpcClient->eth_signTransaction(t);
    BOOST_REQUIRE(!signedTx["raw"].empty());

    auto txHash = rpcClient->eth_sendRawTransaction(signedTx["raw"].asString());
    BOOST_REQUIRE(!txHash.empty());

    auto invalidNonce = jsToU256(rpcClient->eth_getTransactionCount(toJS(senderAddress), "latest")) - 1;
    t["nonce"] = jsToDecimal(toJS(invalidNonce));
    
    signedTx = rpcClient->eth_signTransaction(t);
    BOOST_REQUIRE(!signedTx["raw"].empty());

    BOOST_CHECK_EQUAL(sendingRawShouldFail(signedTx["raw"].asString()), "Invalid transaction nonce.");

}

BOOST_AUTO_TEST_CASE(eth_sendRawTransaction_errorInsufficientGas)
{
    auto senderAddress = coinbase.address();
    auto receiver = KeyPair::create();

    // Mine to generate a non-zero account balance
    const int blocksToMine = 1;
    const int blockNumber = 1;
    const u256 blockReward = 5 * dev::eth::ether;
    dev::eth::mine(*(web3->ethereum()), blocksToMine);
    BOOST_CHECK_EQUAL(blockReward, web3->ethereum()->balanceAt(senderAddress, blockNumber));

    Json::Value t;
    t["from"] = toJS(senderAddress);
    t["to"] = toJS(receiver.address());
    t["value"] = jsToDecimal(toJS(10000 * dev::eth::szabo));

    const int minGasForValueTransferTx = 21000;
    t["gas"] = jsToDecimal(toJS(minGasForValueTransferTx - 1));
    
    auto signedTx = rpcClient->eth_signTransaction(t);
    BOOST_REQUIRE(!signedTx["raw"].empty());

    BOOST_CHECK_EQUAL(sendingRawShouldFail(signedTx["raw"].asString()), "Transaction gas amount is less than the intrinsic gas amount for this transaction type.");
}

BOOST_AUTO_TEST_CASE(eth_sendRawTransaction_errorDuplicateTransaction)
{
    auto senderAddress = coinbase.address();
    auto receiver = KeyPair::create();

    // Mine to generate a non-zero account balance
    const int blocksToMine = 1;
    const int blockNumber = 1;
    const u256 blockReward = 5 * dev::eth::ether;
    dev::eth::mine(*(web3->ethereum()), blocksToMine);
    BOOST_CHECK_EQUAL(blockReward, web3->ethereum()->balanceAt(senderAddress, blockNumber));

    Json::Value t;
    t["from"] = toJS(senderAddress);
    t["to"] = toJS(receiver.address());
    t["value"] = jsToDecimal(toJS(10000 * dev::eth::szabo));

    auto signedTx = rpcClient->eth_signTransaction(t);
    BOOST_REQUIRE(!signedTx["raw"].empty());

    auto txHash = rpcClient->eth_sendRawTransaction(signedTx["raw"].asString());
    BOOST_REQUIRE(!txHash.empty());
    
    auto txNonce = jsToU256(rpcClient->eth_getTransactionCount(toJS(senderAddress), "latest"));
    t["nonce"] = jsToDecimal(toJS(txNonce));
    
    signedTx = rpcClient->eth_signTransaction(t);
    BOOST_REQUIRE(!signedTx["raw"].empty());
    
    BOOST_CHECK_EQUAL(sendingRawShouldFail(signedTx["raw"].asString()), "Same transaction already exists in the pending transaction queue.");
}

BOOST_AUTO_TEST_CASE(eth_signTransaction)
{
    auto address = coinbase.address();
    auto countAtBeforeSign = jsToU256(rpcClient->eth_getTransactionCount(toJS(address), "latest"));
    auto receiver = KeyPair::create();

    Json::Value t;
    t["from"] = toJS(address);
    t["value"] = jsToDecimal(toJS(1));
    t["to"] = toJS(receiver.address());
    
    Json::Value res = rpcClient->eth_signTransaction(t);
    BOOST_REQUIRE(res["raw"]);
    BOOST_REQUIRE(res["tx"]);

    accountHolder->setAccounts({});
    dev::eth::mine(*(web3->ethereum()), 1);

    auto countAtAfterSign = jsToU256(
        rpcClient->eth_getTransactionCount(toJS(address), "latest"));
    
    BOOST_CHECK_EQUAL(countAtBeforeSign, countAtAfterSign);
}

BOOST_AUTO_TEST_CASE(simple_contract)
{
    dev::eth::mine(*(web3->ethereum()), 1);


    // contract test {
    //  function f(uint a) returns(uint d) { return a * 7; }
    // }

    string compiled =
        "6080604052341561000f57600080fd5b60b98061001d6000396000f300"
        "608060405260043610603f576000357c01000000000000000000000000"
        "00000000000000000000000000000000900463ffffffff168063b3de64"
        "8b146044575b600080fd5b3415604e57600080fd5b606a600480360381"
        "019080803590602001909291905050506080565b604051808281526020"
        "0191505060405180910390f35b60006007820290509190505600a16562"
        "7a7a72305820f294e834212334e2978c6dd090355312a3f0f9476b8eb9"
        "8fb480406fc2728a960029";

    Json::Value create;
    create["code"] = compiled;
    string txHash = rpcClient->eth_sendTransaction(create);
    dev::eth::mine(*(web3->ethereum()), 1);

    Json::Value receipt = rpcClient->eth_getTransactionReceipt(txHash);
    string contractAddress = receipt["contractAddress"].asString();

    Json::Value call;
    call["to"] = contractAddress;
    call["data"] = "0xb3de648b0000000000000000000000000000000000000000000000000000000000000001";
    call["gas"] = "1000000";
    call["gasPrice"] = "0";
    string result = rpcClient->eth_call(call, "latest");
    BOOST_CHECK_EQUAL(result, "0x0000000000000000000000000000000000000000000000000000000000000007");
}

BOOST_AUTO_TEST_CASE(contract_storage)
{
    dev::eth::mine(*(web3->ethereum()), 1);


     // pragma solidity ^0.4.22;
        
     // contract test
     // {
     //     uint hello;
     //     function writeHello(uint value) returns(bool d)
     //     {
     //       hello = value;
     //       return true;
     //     }
     // }


    const string compiled =
        "6080604052341561000f57600080fd5b60c28061001d6000396000f3006"
        "08060405260043610603f576000357c0100000000000000000000000000"
        "000000000000000000000000000000900463ffffffff16806315b2eec31"
        "46044575b600080fd5b3415604e57600080fd5b606a6004803603810190"
        "80803590602001909291905050506084565b60405180821515151581526"
        "0200191505060405180910390f35b600081600081905550600190509190"
        "505600a165627a7a72305820d8407d9cdaaf82966f3fa7a3e665b8cf4e6"
        "5ee8909b83094a3f856b9051274500029";

    const string runtimeCode = compiled.substr(58);

    Json::Value create;
    create["code"] = compiled;
    string txHash = rpcClient->eth_sendTransaction(create);
    dev::eth::mine(*(web3->ethereum()), 1);

    Json::Value receipt = rpcClient->eth_getTransactionReceipt(txHash);
    string contractAddress = receipt["contractAddress"].asString();
    
    Json::Value transact;
    transact["to"] = contractAddress;
    transact["data"] = "0x15b2eec30000000000000000000000000000000000000000000000000000000000000003";
    rpcClient->eth_sendTransaction(transact);
    dev::eth::mine(*(web3->ethereum()), 1);

    string storage = rpcClient->eth_getStorageAt(contractAddress, "0", "latest");
    BOOST_CHECK_EQUAL(storage, "0x0000000000000000000000000000000000000000000000000000000000000003");

    auto code = rpcClient->eth_getCode(contractAddress, "latest");
    BOOST_CHECK_EQUAL(code, "0x" + runtimeCode);
}

BOOST_AUTO_TEST_CASE(eth_getCode_emptyAccount)
{
    auto code = rpcClient->eth_getCode(toJS(coinbase.address()), "latest");
    BOOST_CHECK_EQUAL(code, "");

    code = rpcClient->eth_getCode("0xaabbccddeeff0000000011223344556677889900", "pending");
    BOOST_CHECK_EQUAL(code, "");
}

BOOST_AUTO_TEST_CASE(web3_sha3)
{
    string testString = "multiply(uint256)";
    h256 expected = dev::sha3(testString);

    auto hexValue = fromAscii(testString);
    string result = rpcClient->web3_sha3(hexValue);
    BOOST_CHECK_EQUAL(toJS(expected), result);
    BOOST_CHECK_EQUAL("0xc6888fa159d67f77c2f3d7a402e199802766bd7e8d4d1ecd2274fc920265d56a", result);
}

BOOST_AUTO_TEST_CASE(debugAccountRangeAtFinalBlockState)
{
    // mine to get some balance at coinbase
    dev::eth::mine(*(web3->ethereum()), 1);

    // send transaction to have non-emtpy block
    Address receiver = Address::random();
    Json::Value tx;
    tx["from"] = toJS(coinbase.address());
    tx["value"] = toJS(10);
    tx["to"] = toJS(receiver);
    tx["gas"] = toJS(EVMSchedule().txGas);
    tx["gasPrice"] = toJS(10 * dev::eth::szabo);
    string txHash = rpcClient->eth_sendTransaction(tx);
    BOOST_REQUIRE(!txHash.empty());

    dev::eth::mine(*(web3->ethereum()), 1);

    string receiverHash = toString(sha3(receiver));

    // receiver doesn't exist in the beginning of the 2nd block
    Json::Value result = rpcClient->debug_accountRangeAt("2", 0, "0", 100);
    BOOST_CHECK(!result["addressMap"].isMember(receiverHash));

    // receiver exists in the end of the 2nd block
    result = rpcClient->debug_accountRangeAt("2", 1, "0", 100);
    BOOST_CHECK(result["addressMap"].isMember(receiverHash));
    BOOST_CHECK_EQUAL(result["addressMap"][receiverHash], toString(receiver));
}

BOOST_AUTO_TEST_CASE(debugStorageRangeAtFinalBlockState)
{
    // mine to get some balance at coinbase
    dev::eth::mine(*(web3->ethereum()), 1);

    //pragma solidity ^0.4.22;
    //contract test
    //{
    //    uint hello = 7;
    //}
    string initCode =
        "608060405260076000553415601357600080fd5b60358060206000396000"
        "f3006080604052600080fd00a165627a7a7230582006db0551577963b544"
        "3e9501b4b10880e186cff876cd360e9ad6e4181731fcdd0029";

    Json::Value tx;
    tx["code"] = initCode;
    tx["from"] = toJS(coinbase.address());
    string txHash = rpcClient->eth_sendTransaction(tx);

    dev::eth::mine(*(web3->ethereum()), 1);

    Json::Value receipt = rpcClient->eth_getTransactionReceipt(txHash);
    string contractAddress = receipt["contractAddress"].asString();

    // contract doesn't exist in the beginning of the 2nd block
    Json::Value result = rpcClient->debug_storageRangeAt("2", 0, contractAddress, "0", 100);
    BOOST_CHECK(result["storage"].empty());

    // contracts exists in the end of the 2nd block
    result = rpcClient->debug_storageRangeAt("2", 1, contractAddress, "0", 100);
    BOOST_CHECK(!result["storage"].empty());
    string keyHash = toJS(sha3(u256{0}));
    BOOST_CHECK(!result["storage"][keyHash].empty());
    BOOST_CHECK_EQUAL(result["storage"][keyHash]["key"].asString(), "0x00");
    BOOST_CHECK_EQUAL(result["storage"][keyHash]["value"].asString(), "0x07");
}

BOOST_AUTO_TEST_CASE(debugTraceTransaction)
{
    // mine to get some balance at coinbase
    dev::eth::mine(*(web3->ethereum()), 1);

    // send some transaction requiring execution
    string initCode =
        "608060405260076000553415601357600080fd5b60358060206000396000"
        "f3006080604052600080fd00a165627a7a7230582006db0551577963b544"
        "3e9501b4b10880e186cff876cd360e9ad6e4181731fcdd0029";

    Json::Value tx;
    tx["code"] = initCode;
    tx["from"] = toJS(coinbase.address());
    string txHash = rpcClient->eth_sendTransaction(tx);
    BOOST_REQUIRE(!txHash.empty());

    dev::eth::mine(*(web3->ethereum()), 1);

    Json::Value result = rpcClient->debug_traceTransaction(txHash, Json::Value(Json::objectValue));
    BOOST_REQUIRE(result.isObject());
    BOOST_REQUIRE(result["structLogs"].isArray());
    BOOST_REQUIRE_GT(result["structLogs"].size(), 0u);
}

BOOST_AUTO_TEST_CASE(adminEthVmTrace)
{
    // mine to get some balance at coinbase
    dev::eth::mine(*(web3->ethereum()), 1);

    // send some transaction requiring execution
    string initCode =
        "608060405260076000553415601357600080fd5b60358060206000396000"
        "f3006080604052600080fd00a165627a7a7230582006db0551577963b544"
        "3e9501b4b10880e186cff876cd360e9ad6e4181731fcdd0029";

    Json::Value tx;
    tx["code"] = initCode;
    tx["from"] = toJS(coinbase.address());
    string txHash = rpcClient->eth_sendTransaction(tx);
    BOOST_REQUIRE(!txHash.empty());

    dev::eth::mine(*(web3->ethereum()), 1);

    // get trace for 0th transaction in 2nd block
    Json::Value result = rpcClient->admin_eth_vmTrace("2", 0, adminSession);
    BOOST_REQUIRE(result.isObject());
    BOOST_REQUIRE(result["structLogs"].isArray());
    BOOST_REQUIRE_GT(result["structLogs"].size(), 0u);
}

BOOST_AUTO_TEST_CASE(test_setChainParams)
{
    Json::Value ret;
    Json::Reader().parse(c_genesisConfigString, ret);
    ret["genesis"]["extraData"] = toHexPrefixed(h256::random().asBytes());
    rpcClient->test_setChainParams(ret);
}


BOOST_AUTO_TEST_CASE(test_importRawBlock)
{
    Json::Value ret;
    Json::Reader().parse(c_genesisConfigString, ret);
    rpcClient->test_setChainParams(ret);
    string blockHash = rpcClient->test_importRawBlock(
        "0xf90279f9020ea0c92211c9cd49036c37568feedb8e518a24a77e9f6ca959931a19dcf186a8e1e6a01dcc4de8"
        "dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347942adc25665018aa1fe0e6bc666dac8fc2"
        "697ff9baa0328f16ca7b0259d7617b3ddf711c107efe6d5785cbeb11a8ed1614b484a6bc3aa093ca2a18d52e7c"
        "1846f7b104e2fc1e5fdc71ebe38187248f9437d39e74f43aaba0e151c94b824bded58346fa03fc91fa275bd0cf"
        "94caac0ea4ebb9c8d32a574644b901000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "00008304000001830f460f82a0348203e897d68094312e342e302b2b62383163726c696e7578676e75a08e2042"
        "e00086a18e2f095bc997dc11d1c93fcf34d0540a428ee95869a4a62264883f8fd3f43a3567c3f865f863800183"
        "061a8094095e7baea6a6c7c4c2dfeb977efac326af552d87830186a0801ca0e94818d1f3b0c69eb37720145a5e"
        "ad7fbf6f8d80139dd53953b4a782301050a3a01fcf46908c01576715411be0857e30027d6be3250a3653f049b3"
        "ff8d74d2540cc0");
    BOOST_CHECK_EQUAL(
        blockHash, "0xedef94eddd6002ae14803b91aa5138932f948026310144fc615d52d7d5ff29c7");
}

BOOST_AUTO_TEST_SUITE_END()
