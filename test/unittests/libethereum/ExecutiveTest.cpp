// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include <libethashseal/Ethash.h>
#include <libethashseal/GenesisInfo.h>
#include <libethereum/ChainParams.h>
#include <libethereum/Executive.h>
#include <libethereum/ExtVM.h>
#include <libethereum/State.h>
#include <test/tools/libtestutils/TestLastBlockHashes.h>
#include <gtest/gtest.h>

using namespace dev;
using namespace dev::eth;
using namespace dev::test;

class ExecutiveTest : public testing::Test
{
public:
    ExecutiveTest()
    {
        ethash.setChainParams(ChainParams{genesisInfo(eth::Network::IstanbulTransitionTest)});
    }

    Ethash ethash;
    BlockHeader blockHeader;
    TestLastBlockHashes lastBlockHashes{{}};
    State state{0};

    Address receiveAddress{"0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b"};
    Address txSender{"0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"};
    u256 txValue;
    u256 gasPrice;
    bytesConstRef txData{};
    u256 gas = 1000000;
    bytes code = {1, 2, 3, 4};
};

TEST_F(ExecutiveTest, callUsesAccountVersion)
{
    EnvInfo envInfo(blockHeader, lastBlockHashes, 0);

    state.createContract(receiveAddress);
    u256 version = 1;
    state.setCode(receiveAddress, bytes{code}, version);
    state.commit(State::CommitBehaviour::RemoveEmptyAccounts);

    Executive executive(state, envInfo, ethash);

    bool done = executive.call(receiveAddress, txSender, txValue, gasPrice, txData, gas);

    EXPECT_FALSE(done);
    EXPECT_EQ(executive.extVM().version, version);
}

TEST_F(ExecutiveTest, createUsesLatestForkVersion)
{
    // block in Istanbul fork
    blockHeader.setNumber(10);

    EnvInfo envInfo(blockHeader, lastBlockHashes, 0);

    Executive executive(state, envInfo, ethash);

    bool done = executive.create(txSender, txValue, gasPrice, gas, ref(code), txSender);

    EXPECT_FALSE(done);
    EXPECT_EQ(executive.extVM().version, IstanbulSchedule.accountVersion);
}

TEST_F(ExecutiveTest, createOpcodeUsesParentVersion)
{
    EnvInfo envInfo(blockHeader, lastBlockHashes, 0);

    state.createContract(txSender);
    u256 version = 1;
    state.setCode(txSender, bytes{code}, version);
    state.commit(State::CommitBehaviour::RemoveEmptyAccounts);

    Executive executive(state, envInfo, ethash);

    bool done = executive.createOpcode(txSender, txValue, gasPrice, gas, ref(code), txSender);

    EXPECT_FALSE(done);
    EXPECT_EQ(executive.extVM().version, version);
}

TEST_F(ExecutiveTest, create2OpcodeUsesParentVersion)
{
    EnvInfo envInfo(blockHeader, lastBlockHashes, 0);

    state.createContract(txSender);
    u256 version = 1;
    state.setCode(txSender, bytes{code}, version);
    state.commit(State::CommitBehaviour::RemoveEmptyAccounts);

    Executive executive(state, envInfo, ethash);

    bool done = executive.create2Opcode(txSender, txValue, gasPrice, gas, ref(code), txSender, 0);

    EXPECT_FALSE(done);
    EXPECT_EQ(executive.extVM().version, version);
}

TEST_F(ExecutiveTest, emptyInitCodeSetsParentVersion)
{
    EnvInfo envInfo(blockHeader, lastBlockHashes, 0);

    state.createContract(txSender);
    u256 version = 1;
    state.setCode(txSender, bytes{code}, version);
    state.commit(State::CommitBehaviour::RemoveEmptyAccounts);

    Executive executive(state, envInfo, ethash);

    bytes initCode;
    bool done = executive.createOpcode(txSender, txValue, gasPrice, gas, ref(initCode), txSender);

    EXPECT_TRUE(done);
    EXPECT_FALSE(state.addressHasCode(executive.newAddress()));
    EXPECT_EQ(state.version(executive.newAddress()), version);
}

TEST_F(ExecutiveTest, createdContractHasParentVersion)
{
    EnvInfo envInfo(blockHeader, lastBlockHashes, 0);

    state.createContract(txSender);
    u256 version = 1;
    state.setCode(txSender, bytes{code}, version);
    state.commit(State::CommitBehaviour::RemoveEmptyAccounts);

    Executive executive(state, envInfo, ethash);

    // mstore(0, 0x60)
    // return(0, 0x20)
    bytes initCode = fromHex("606060005260206000f3");

    bool done = executive.createOpcode(txSender, txValue, gasPrice, gas, ref(initCode), txSender);
    EXPECT_FALSE(done);

    done = executive.go();
    EXPECT_TRUE(done);

    EXPECT_TRUE(state.addressHasCode(executive.newAddress()));
    EXPECT_EQ(state.version(executive.newAddress()), version);
}
