// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2018-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include <libethereum/ChainParams.h>
#include <libethereum/ClientTest.h>
#include <libp2p/Network.h>
#include <libwebthree/WebThree.h>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;
using namespace dev::p2p;
namespace fs = boost::filesystem;

class TestClientFixture : public TestOutputHelperFixture
{
public:
    TestClientFixture()
    {
        ChainParams chainParams;
        chainParams.sealEngineName = NoProof::name();
        chainParams.allowFutureBlocks = true;

        fs::path dir = fs::temp_directory_path();

        string listenIP = "127.0.0.1";
        unsigned short listenPort = 30303;
        auto netPrefs = NetworkConfig(listenIP, listenPort, false);
        netPrefs.discovery = false;
        netPrefs.pin = false;

        auto nodesState = contents(dir / fs::path("network.rlp"));
        bool testingMode = true;
        m_web3.reset(new dev::WebThreeDirect(WebThreeDirect::composeClientVersion("eth"), dir, dir,
            chainParams, WithExisting::Kill, netPrefs, &nodesState, testingMode));
    }

    dev::WebThreeDirect* getWeb3() { return m_web3.get(); }

    private:
    std::unique_ptr<dev::WebThreeDirect> m_web3;
};

// genesis config string from solidity
static std::string const c_configString = R"(
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
        "nonce": "0x0000000000000042",
        "author": "0000000000000010000000000000000000000000",
        "timestamp": "0x00",
        "extraData": "0x",
        "gasLimit": "0x1000000000000",
        "difficulty": "0x020000",
        "mixHash": "0x0000000000000000000000000000000000000000000000000000000000000000"
    }
}
)";


BOOST_FIXTURE_TEST_SUITE(ClientTestSuite, TestClientFixture)

BOOST_AUTO_TEST_CASE(ClientTest_setChainParamsAuthor)
{
    ClientTest* testClient = asClientTest(getWeb3()->ethereum());
    BOOST_CHECK_EQUAL(testClient->author(), Address("0000000000000000000000000000000000000000"));
    testClient->setChainParams(c_configString);
    BOOST_CHECK_EQUAL(testClient->author(), Address("0000000000000010000000000000000000000000"));
}

BOOST_AUTO_TEST_CASE(ClientTest_setChainParamsPrecompilesAreIgnored)
{
    ClientTest* testClient = asClientTest(getWeb3()->ethereum());
    testClient->setChainParams(c_configString);

    auto const ecrecoverAddress = Address{0x01};
    auto const sha256Address = Address{0x02};

    BOOST_CHECK_EQUAL(
        testClient->chainParams().precompiled.at(ecrecoverAddress).startingBlock(), 0);
    BOOST_CHECK(contains(testClient->chainParams().precompiled, sha256Address));

    std::string const configWithCustomPrecompiles = R"({
        "sealEngine": "NoProof",
        "params": {
            "accountStartNonce": "0x00",
            "maximumExtraDataSize": "0x1000000",
            "blockReward": "0x",
            "allowFutureBlocks": true,
            "homesteadForkBlock": "0x118c30",
            "daoHardforkBlock": "0x1d4c00",
            "EIP150ForkBlock": "0x259518",
            "EIP158ForkBlock": "0x28d138"
        },
        "genesis": {
            "nonce": "0x0000000000000042",
            "author": "0000000000000010000000000000000000000000",
            "timestamp": "0x00",
            "extraData": "0x",
            "gasLimit": "0x1000000000000",
            "difficulty": "0x020000",
            "mixHash": "0x0000000000000000000000000000000000000000000000000000000000000000"
        },
        "accounts": {
            "0000000000000000000000000000000000000001": { "precompiled": { "name": "ecrecover", "linear": { "base": 3000, "word": 0 }, "startingBlock": "0x28d138" } }
        }
    })";

    testClient->setChainParams(configWithCustomPrecompiles);

    BOOST_CHECK_EQUAL(
        testClient->chainParams().precompiled.at(ecrecoverAddress).startingBlock(), 0);
    BOOST_CHECK(contains(testClient->chainParams().precompiled, sha256Address));
}

BOOST_AUTO_TEST_SUITE_END()
