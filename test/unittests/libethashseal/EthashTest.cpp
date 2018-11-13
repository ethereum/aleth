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

#include <libethashseal/Ethash.h>

#include <gtest/gtest.h>

using namespace dev;
using namespace dev::eth;

// FIXME: Add a helper function here, because the test cases are almost identical.
// TODO: Add tests for Homestead difficulty change.

TEST(Ethash, calculateDifficultyByzantiumWithoutUncles)
{
    ChainOperationParams params;
    params.homesteadForkBlock = 0;
    params.byzantiumForkBlock = u256(0x1000);

    Ethash ethash;
    ethash.setChainParams(params);

    BlockHeader parentHeader;
    parentHeader.clear();
    parentHeader.setNumber(0x2000);
    parentHeader.setTimestamp(100);
    parentHeader.setDifficulty(1000000);

    BlockHeader header;
    header.clear();
    header.setNumber(0x2001);
    header.setTimestamp(130);

    EXPECT_EQ(calculateEthashDifficulty(ethash.chainParams(), header, parentHeader), 999024);
}

TEST(Ethash, calculateDifficultyByzantiumWithUncles)
{
    ChainOperationParams params;
    params.homesteadForkBlock = 0;
    params.byzantiumForkBlock = u256(0x1000);

    Ethash ethash;
    ethash.setChainParams(params);

    BlockHeader parentHeader;
    parentHeader.clear();
    parentHeader.setNumber(0x2000);
    parentHeader.setTimestamp(100);
    parentHeader.setDifficulty(1000000);
    parentHeader.setSha3Uncles(
        h256("0x949d991d685738352398dff73219ab19c62c06e6f8ce899fbae755d5127ed1ef"));

    BlockHeader header;
    header.clear();
    header.setNumber(0x2001);
    header.setTimestamp(130);

    EXPECT_EQ(calculateEthashDifficulty(ethash.chainParams(), header, parentHeader), 999512);
}

TEST(Ethash, calculateDifficultyByzantiumMaxAdjustment)
{
    ChainOperationParams params;
    params.homesteadForkBlock = 0;
    params.byzantiumForkBlock = u256(0x1000);

    Ethash ethash;
    ethash.setChainParams(params);

    BlockHeader parentHeader;
    parentHeader.clear();
    parentHeader.setNumber(0x2000);
    parentHeader.setTimestamp(100);
    parentHeader.setDifficulty(1000000);

    BlockHeader header;
    header.clear();
    header.setNumber(0x2001);
    header.setTimestamp(1100);

    EXPECT_EQ(calculateEthashDifficulty(ethash.chainParams(), header, parentHeader), 951688);
}

class IceAgeDelay : public testing::Test
{
public:
    IceAgeDelay()
    {
        params.homesteadForkBlock = 0;
        params.byzantiumForkBlock = 4000000;
        params.constantinopleForkBlock = 6000000;

        ethash.setChainParams(params);
    }

    u256 calculateDifficulty(int64_t _blockNumber)
    {
        BlockHeader parentHeader;
        parentHeader.clear();
        parentHeader.setNumber(_blockNumber - 1);
        parentHeader.setTimestamp(100);
        parentHeader.setDifficulty(1000000);

        BlockHeader header;
        header.clear();
        header.setNumber(_blockNumber);
        header.setTimestamp(1100);

        return dev::eth::calculateEthashDifficulty(ethash.chainParams(), header, parentHeader);
    }

    ChainOperationParams params;
    Ethash ethash;
};

TEST_F(IceAgeDelay, ByzantiumIceAgeDelay)
{
    EXPECT_EQ(calculateDifficulty(4500000), calculateDifficulty(1500000));
}

TEST_F(IceAgeDelay, ConstantinopleIceAgeDelay)
{
    EXPECT_EQ(calculateDifficulty(6500000), calculateDifficulty(1500000));
}

TEST(Ethash, epochSeed)
{
    BlockHeader header;
    header.setNumber(0);
    h256 seed = Ethash::seedHash(header);
    EXPECT_EQ(seed, h256{});

    header.setNumber(30000);
    seed = Ethash::seedHash(header);
    EXPECT_EQ(seed, h256{"290decd9548b62a8d60345a988386fc84ba6bc95484008f6362f93160ef3e563"});

    header.setNumber(2048 * 30000);
    seed = Ethash::seedHash(header);
    EXPECT_EQ(seed, h256{"20a7678ca7b50829183baac2e1e3c43fa3c4bcbc171b11cf5a9f30bebd172920"});
}

TEST(Ethash, etashQuickVerify)
{
    BlockHeader header;
    header.setParentHash(h256{"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"});
    header.setDifficulty(h256{"0000000000000000000000000000000000000000000000000000000000ffffff"});
    header.setGasLimit(1000000);
    Ethash::setMixHash(header, {});

    Ethash etash;
    Ethash::setNonce(header, Nonce{3272400});
    etash.verify(QuickNonce, header, {}, {});
    Ethash::setNonce(header, Nonce{3272401});
    EXPECT_THROW(etash.verify(QuickNonce, header, {}, {}), InvalidBlockNonce);
}

TEST(Ethash, etashVerify)
{
    BlockHeader header;
    header.setParentHash(h256{"aff00eb20f8a48450b9ea5307e2737287854f357c9022280772e995cc22affd3"});
    header.setAuthor(Address{"8888f1f195afa192cfee860698584c030f4c9db1"});
    header.setRoots(h256{"fcfe9f2203bd98342867117fa3de299a09578371efd04fc9e76a46f7f1fda4bb"},
        h256{"1751f772ba1fdb3ad31fa04c39144ea3b523f10604a5a09a19cb4c1d0b56992c"},
        h256{"1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347"},
        h256{"1cd69d76c84ea914e746833b7a31d9bfe210f75929893f1da0748efaeb31fe27"});
    header.setLogBloom({});
    header.setDifficulty(h256{131072});
    header.setNumber(1);
    header.setGasLimit(3141562);
    header.setGasUsed(55179);
    header.setTimestamp(1507291743);

    EXPECT_EQ(header.hash(WithoutSeal),
        h256{"57c5cfb8fe8a70a24ea81f12398e8a074ac25dd32b6dba8cd1f2bf85680fbfce"});

    Ethash::setMixHash(
        header, h256{"d8ada7ff7720ebc9700c170c046352b8ee6fb4630cf6a285489896daac7a40eb"});

    Ethash etash;
    Ethash::setNonce(header, Nonce{"81c3f9bfae230a8e"});
    etash.verify(CheckEverything, header, {}, {});

    // Break nonce.
    Ethash::setNonce(header, Nonce{"71c3f9bfae230a8e"});
    try
    {
        etash.verify(CheckEverything, header, {}, {});
        ADD_FAILURE();
    }
    catch (InvalidBlockNonce const& ex)
    {
        std::tuple<h256, h256> ethashResult = *boost::get_error_info<errinfo_ethashResult>(ex);
        EXPECT_EQ(std::get<0>(ethashResult),
            h256{"07a4017237d933aa1ff4f62650f68ea2118c8bd741575e97c2867fb41d5b832d"});
        EXPECT_EQ(std::get<1>(ethashResult),
            h256{"a842d613f0b8ad1266e507bb1845b2db75673caf593596d1de4951ecd9620a93"});
    }
    Ethash::setNonce(header, Nonce{"81c3f9bfae230a8e"});

    // Break mix hash.
    Ethash::setMixHash(
        header, h256{"e8ada7ff7720ebc9700c170c046352b8ee6fb4630cf6a285489896daac7a40eb"});
    EXPECT_THROW(etash.verify(CheckEverything, header, {}, {}), InvalidBlockNonce);

    // Break nonce & mix hash.
    Ethash::setNonce(header, Nonce{"71c3f9bfae230a8e"});
    EXPECT_THROW(etash.verify(CheckEverything, header, {}, {}), InvalidBlockNonce);
    Ethash::setNonce(header, Nonce{"81c3f9bfae230a8e"});
    Ethash::setMixHash(
        header, h256{"d8ada7ff7720ebc9700c170c046352b8ee6fb4630cf6a285489896daac7a40eb"});

    // Recheck the valid header.
    etash.verify(CheckEverything, header, {}, {});
}

TEST(Ethash, boundary)
{
    BlockHeader header;

    header.setDifficulty(0);
    EXPECT_EQ(Ethash::boundary(header),
        h256{"ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"});

    header.setDifficulty(1);
    EXPECT_EQ(Ethash::boundary(header),
        h256{"ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"});

    header.setDifficulty(2);
    EXPECT_EQ(Ethash::boundary(header),
        h256{"8000000000000000000000000000000000000000000000000000000000000000"});

    header.setDifficulty(31);
    EXPECT_EQ(Ethash::boundary(header),
        h256{"0842108421084210842108421084210842108421084210842108421084210842"});

    header.setDifficulty(32);
    EXPECT_EQ(Ethash::boundary(header),
        h256{"0800000000000000000000000000000000000000000000000000000000000000"});
}
