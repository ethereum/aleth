// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2017-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include <test/tools/libtesteth/BlockChainHelper.h>
#include <test/tools/libtesteth/TestHelper.h>
#include <test/tools/libtestutils/TestLastBlockHashes.h>

#include <libethereum/Block.h>
#include <libethereum/ExtVM.h>

using namespace dev;
using namespace dev::eth;
using namespace dev::test;

class ExtVMExperimentalTestFixture : public TestOutputHelperFixture
{
public:
    ExtVMExperimentalTestFixture()
      : networkSelector(eth::Network::ExperimentalTransitionTest),
        testBlockchain(TestBlockChain::defaultGenesisBlock()),
        genesisBlock(testBlockchain.testGenesis()),
        genesisDB(genesisBlock.state().db()),
        blockchain(testBlockchain.getInterface())
    {
        TestBlock testBlock;
        // block 1 - before Experimental
        testBlock.mine(testBlockchain);
        testBlockchain.addBlock(testBlock);
        preExperimentalBlockHash = testBlock.blockHeader().hash();

        // block 2 - first Experimental block
        testBlock.mine(testBlockchain);
        testBlockchain.addBlock(testBlock);
        experimentalBlockHash = testBlock.blockHeader().hash();
    }

    EnvInfo createEnvInfo(BlockHeader const& _header) const
    {
        return {_header, lastBlockHashes, 0, blockchain.chainID()};
    }

    NetworkSelector networkSelector;
    TestBlockChain testBlockchain;
    TestBlock const& genesisBlock;
    OverlayDB const& genesisDB;
    BlockChain const& blockchain;
    h256 preExperimentalBlockHash;
    h256 experimentalBlockHash;
    TestLastBlockHashes lastBlockHashes{{}};
};

BOOST_FIXTURE_TEST_SUITE(ExtVmExperimentalSuite, ExtVMExperimentalTestFixture)

BOOST_AUTO_TEST_CASE(BlockhashOutOfBoundsRetunsZero)
{
    Block block = blockchain.genesisBlock(genesisDB);
    block.sync(blockchain);

    EnvInfo envInfo(createEnvInfo(block.info()));
    Address addr("0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b");
    ExtVM extVM(block.mutableState(), envInfo, *blockchain.sealEngine(), addr, addr, addr, 0, 0, {},
        {}, {}, 0, 0, false, false);

    BOOST_CHECK_EQUAL(extVM.blockHash(100), h256());
}

BOOST_AUTO_TEST_CASE(BlockhashBeforeExperimentalReliesOnLastHashes)
{
    Block block = blockchain.genesisBlock(genesisDB);
    block.sync(blockchain);

    h256s lastHashes{h256("0xaaabbbccc"), h256("0xdddeeefff")};
    TestLastBlockHashes lastBlockHashes(lastHashes);
    EnvInfo envInfo{block.info(), lastBlockHashes, 0, blockchain.chainID()};
    Address addr("0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b");
    ExtVM extVM(block.mutableState(), envInfo, *blockchain.sealEngine(), addr, addr, addr, 0, 0, {},
        {}, {}, 0, 0, false, false);
    h256 hash = extVM.blockHash(1);
    BOOST_REQUIRE_EQUAL(hash, lastHashes[0]);
}

BOOST_AUTO_TEST_CASE(BlockhashDoesntNeedLastHashesInExperimental)
{
    // BLOCKHASH starts to work through the call to a contract 256 block after Experimental fork
    // block
    TestBlock testBlock;
    for (int i = 0; i < 256; ++i)
    {
        testBlock.mine(testBlockchain);
        testBlockchain.addBlock(testBlock);
    }

    Block block = blockchain.genesisBlock(genesisDB);
    block.sync(blockchain);

    EnvInfo envInfo(createEnvInfo(block.info()));
    Address addr("0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b");
    ExtVM extVM(block.mutableState(), envInfo, *blockchain.sealEngine(), addr, addr, addr, 0, 0, {},
        {}, {}, 0, 0, false, false);

    // older than 256 not available
    BOOST_CHECK_EQUAL(extVM.blockHash(1), h256());

    h256 hash = extVM.blockHash(200);
    BOOST_REQUIRE_EQUAL(hash, blockchain.numberHash(200));
}

BOOST_AUTO_TEST_CASE(ScheduleAccordingToForkBeforeExperimental)
{
    Block block = blockchain.genesisBlock(genesisDB);
    block.sync(blockchain, preExperimentalBlockHash);

    EnvInfo envInfo(createEnvInfo(block.info()));
    Address addr("0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b");
    ExtVM extVM(block.mutableState(), envInfo, *blockchain.sealEngine(), addr, addr, addr, 0, 0, {},
        {}, {}, 0, 0, false, false);

    BOOST_CHECK_EQUAL(extVM.evmSchedule().accountVersion, 0);
    BOOST_CHECK(
        extVM.evmSchedule().txDataNonZeroGas == 16 && extVM.evmSchedule().blockhashGas == 20);
}

BOOST_AUTO_TEST_CASE(IstanbulScheduleForVersionZeroInExperimental)
{
    Block block = blockchain.genesisBlock(genesisDB);
    block.sync(blockchain, experimentalBlockHash);

    EnvInfo envInfo(createEnvInfo(block.info()));
    Address addr("0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b");
    u256 const version = 0;
    ExtVM extVM(block.mutableState(), envInfo, *blockchain.sealEngine(), addr, addr, addr, 0, 0, {},
        {}, {}, version, 0, false, false);

    BOOST_CHECK_EQUAL(extVM.evmSchedule().accountVersion, version);
    BOOST_CHECK(
        extVM.evmSchedule().txDataNonZeroGas == 16 && extVM.evmSchedule().blockhashGas == 20);
}

BOOST_AUTO_TEST_CASE(ExperimentalScheduleForVersionOneInExperimental)
{
    Block block = blockchain.genesisBlock(genesisDB);
    block.sync(blockchain, experimentalBlockHash);

    EnvInfo envInfo(createEnvInfo(block.info()));
    Address addr("0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b");
    u256 const version = 1;
    ExtVM extVM(block.mutableState(), envInfo, *blockchain.sealEngine(), addr, addr, addr, 0, 0, {},
        {}, {}, version, 0, false, false);

    BOOST_CHECK_EQUAL(extVM.evmSchedule().accountVersion, version);
}


BOOST_AUTO_TEST_SUITE_END()
