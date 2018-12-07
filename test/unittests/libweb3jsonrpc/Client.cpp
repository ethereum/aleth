
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
/**
 * @author Christian <chris@ethereum.org>
 * @date 2016
 */

#include <thread>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <jsonrpccpp/common/exception.h>
#include <libweb3jsonrpc/AccountHolder.h>
#include <libweb3jsonrpc/Personal.h>
#include <libweb3jsonrpc/Eth.h>
#include <libdevcore/TransientDirectory.h>
#include <libdevcore/FileSystem.h>
#include <libethcore/KeyManager.h>
#include <libwebthree/WebThree.h>
#include <libp2p/Network.h>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <test/tools/libtesteth/TestHelper.h>

// This is defined by some weird windows header - workaround for now.
#undef GetMessage

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::p2p;

namespace dev
{
namespace test
{

BOOST_FIXTURE_TEST_SUITE(ClientTests, TestOutputHelperFixture)

BOOST_AUTO_TEST_CASE(Personal)
{
    TransientDirectory tempDir;
    boost::filesystem::create_directories(tempDir.path() + "/keys");

    KeyManager keyManager(tempDir.path(), tempDir.path() + "/keys");
    setDataDir(tempDir.path());

    // 'allowFutureBlocks = true' is required to mine multiple blocks,
    // otherwise mining will hang after the first block
    ChainParams chainParams;
    chainParams.sealEngineName = NoProof::name();
    chainParams.allowFutureBlocks = true;
    chainParams.difficulty = chainParams.minimumDifficulty;
    chainParams.gasLimit = chainParams.maxGasLimit;

    dev::WebThreeDirect web3(WebThreeDirect::composeClientVersion("eth"), getDataDir(), string(),
        chainParams, WithExisting::Kill, p2p::NetworkConfig(0));
    web3.stopNetwork();
    web3.ethereum()->stopSealing();
    
    bool userShouldEnterPassword = false;
    string passwordUserWillEnter;

    SimpleAccountHolder accountHolder(
        [&](){ return web3.ethereum(); },
        [&](Address) { if (!userShouldEnterPassword) BOOST_FAIL("Password input requested"); return passwordUserWillEnter; },
        keyManager,
        [](TransactionSkeleton const&, bool) -> bool {
            return false; // user input goes here
        }
    );
    rpc::Personal personal(keyManager, accountHolder, *web3.ethereum());
    rpc::Eth eth(*web3.ethereum(), accountHolder);

    // Create account

    string password = "12345";
    string address = personal.personal_newAccount(password);

    // Try to send transaction

    Json::Value tx;
    tx["from"] = address;
    tx["to"] = string("0x0000000000000000000000000000000000000000");
    tx["value"] = string("0x5208");
    auto sendingShouldFail = [&]() -> string
    {
        try
        {
            eth.eth_sendTransaction(tx);
            BOOST_FAIL("Exception expected.");
        }
        catch (jsonrpc::JsonRpcException const& _e)
        {
            return _e.GetMessage();
        }
        return string();
    };
    auto sendingShouldSucceed = [&]()
    {
        BOOST_CHECK(!eth.eth_sendTransaction(tx).empty());
    };

    BOOST_TEST_CHECKPOINT("Account is locked at the start.");
    BOOST_CHECK_EQUAL(sendingShouldFail(), "Transaction rejected by user.");

    BOOST_TEST_CHECKPOINT("Unlocking without password should not work.");
    BOOST_CHECK(!personal.personal_unlockAccount(address, string(), 2));
    BOOST_CHECK_EQUAL(sendingShouldFail(), "Transaction rejected by user.");

    BOOST_TEST_CHECKPOINT("Unlocking with wrong password should not work.");
    BOOST_CHECK(!personal.personal_unlockAccount(address, "abcd", 2));
    BOOST_CHECK_EQUAL(sendingShouldFail(), "Transaction rejected by user.");

    BOOST_TEST_CHECKPOINT("Unlocking with correct password should work.");
    BOOST_CHECK(personal.personal_unlockAccount(address, password, 2));
    // Mine 1 block so the account will have a non-zero balance
    // and transactions can be sent successfully
    web3.ethereum()->setAuthor(jsToAddress(address));
    dev::eth::mine(*(web3.ethereum()), 1 /* # blocks to mine */);
    sendingShouldSucceed();

    BOOST_TEST_CHECKPOINT("Transaction should be sendable multiple times in unlocked mode.");
    // Mine so the previous successful transaction is included in a block and Client
    // nonce validation for the next transaction will pass
    dev::eth::mine(*(web3.ethereum()), 1 /* # blocks to mine */);
    sendingShouldSucceed();

    this_thread::sleep_for(chrono::seconds(2));
    BOOST_TEST_CHECKPOINT("After unlock time, account should be locked again.");
    BOOST_CHECK_EQUAL(sendingShouldFail(), "Transaction rejected by user.");

    BOOST_TEST_CHECKPOINT("Unlocking again with empty password should not work.");
    BOOST_CHECK(!personal.personal_unlockAccount(address, string(), 2));
    BOOST_CHECK_EQUAL(sendingShouldFail(), "Transaction rejected by user.");
}

BOOST_AUTO_TEST_SUITE_END()

}
}
