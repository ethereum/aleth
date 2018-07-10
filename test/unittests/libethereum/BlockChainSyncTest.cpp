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

#include <libdevcore/TransientDirectory.h>
#include <libwebthree/WebThree.h>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <test/tools/libtesteth/TestHelper.h>
#include <boost/test/unit_test.hpp>

namespace dev
{
namespace test
{
namespace
{
struct BlockChainSyncFixture: public TestOutputHelperFixture
{
    BlockChainSyncFixture()
    {
        // TODO this will not work wth parallel tests
        p2p::NetworkPreferences nprefs1("127.0.0.1", p2p::c_defaultListenPort);
        nprefs1.discovery = false;
        eth::ChainParams chainParams;
        chainParams.sealEngineName = eth::NoProof::name();
        chainParams.allowFutureBlocks = true;
        chainParams.daoHardforkBlock = 0;
        chainParams.difficulty = chainParams.minimumDifficulty;
        chainParams.gasLimit = chainParams.maxGasLimit;
        // add random extra data to randomize genesis hash and get random DB path,
        // so that tests can be run in parallel
        // TODO: better make it use ethemeral in-memory databases
        chainParams.extraData = h256::random().asBytes();
        web3Node1.reset(new WebThreeDirect(
            "testclient1", dbDir1.path(), "", chainParams, WithExisting::Kill, {"eth"}, nprefs1));

        //web3->setIdealPeerCount(5);

        web3Node1->ethereum()->setAuthor(coinbase.address());

        web3Node1->startNetwork();

        p2p::NetworkPreferences nprefs2("127.0.0.1", p2p::c_defaultListenPort + 1);
        nprefs2.discovery = false;
        web3Node2.reset(new WebThreeDirect(
            "testclient2", dbDir2.path(), "", chainParams, WithExisting::Kill, {"eth"}, nprefs2));

        web3Node2->startNetwork();
    }

    TransientDirectory dbDir1;
    std::unique_ptr<WebThreeDirect> web3Node1;
    TransientDirectory dbDir2;
    std::unique_ptr<WebThreeDirect> web3Node2;
    KeyPair coinbase{KeyPair::create()};
};

}

BOOST_FIXTURE_TEST_SUITE(BlockChainSyncSuite, BlockChainSyncFixture)


BOOST_AUTO_TEST_CASE(basicSync)
{
    dev::eth::mine(*(web3Node1->ethereum()), 10);

    int blocksImported = 0;
    std::promise<void> allBlocksImported;
    auto importHandler =
        web3Node2->ethereum()->setOnBlockImport([&blocksImported, &allBlocksImported](eth::BlockHeader const&) {
        if (++blocksImported == 10)
            allBlocksImported.set_value();
    });
    
    //    p2p::NodeSpec node2spec{"127.0.0.1", p2p::c_defaultListenPort + 1};
//    web3Node1->addPeer(/*web3Node2->enode()*/node2spec, p2p::PeerType::Required);
    web3Node1->requirePeer(web3Node2->id(), "127.0.0.1:30304");

    allBlocksImported.get_future().wait_for(std::chrono::minutes(1));

    BOOST_REQUIRE_EQUAL(blocksImported, 10);
}

BOOST_AUTO_TEST_CASE(syncFromMiner)
{
    int sealedBlocks = 0;
    auto sealHandler =
        web3Node1->ethereum()->setOnBlockSealed([&sealedBlocks](bytes const&) { ++sealedBlocks; });

    int importedBlocks = 0;
    auto importHandler = web3Node1->ethereum()->setOnBlockImport(
        [&importedBlocks](eth::BlockHeader const&) { ++importedBlocks; });

    web3Node1->ethereum()->startSealing();

    /*    int blocksImported = 0;
        std::promise<void> allBlocksImported;
        auto importHandler =
            web3Node2->ethereum()->setOnBlockImport([&blocksImported,
       &allBlocksImported](eth::BlockHeader const&) { if (++blocksImported == 10)
                allBlocksImported.set_value();
        });
    */
    web3Node1->requirePeer(web3Node2->id(), "127.0.0.1:30304");


    // allBlocksImported.get_future().wait_for(std::chrono::minutes(1));

    BOOST_REQUIRE(web3Node1->ethereum()->wouldSeal());
}

BOOST_AUTO_TEST_SUITE_END()

}
}
