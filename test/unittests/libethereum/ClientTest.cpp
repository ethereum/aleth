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
/** @date 2018
 */

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
    },
    "accounts": {
        "0000000000000000000000000000000000000001": { "wei": "1", "precompiled": { "name": "ecrecover", "linear": { "base": 3000, "word": 0 } } },
        "0000000000000000000000000000000000000002": { "wei": "1", "precompiled": { "name": "sha256", "linear": { "base": 60, "word": 12 } } },
        "0000000000000000000000000000000000000003": { "wei": "1", "precompiled": { "name": "ripemd160", "linear": { "base": 600, "word": 120 } } },
        "0000000000000000000000000000000000000004": { "wei": "1", "precompiled": { "name": "identity", "linear": { "base": 15, "word": 3 } } },
        "0000000000000000000000000000000000000005": { "wei": "1", "precompiled": { "name": "modexp" } },
        "0000000000000000000000000000000000000006": { "wei": "1", "precompiled": { "name": "alt_bn128_G1_add", "linear": { "base": 500, "word": 0 } } },
        "0000000000000000000000000000000000000007": { "wei": "1", "precompiled": { "name": "alt_bn128_G1_mul", "linear": { "base": 40000, "word": 0 } } },
        "0000000000000000000000000000000000000008": { "wei": "1", "precompiled": { "name": "alt_bn128_pairing_product" } }
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

BOOST_AUTO_TEST_SUITE_END()
