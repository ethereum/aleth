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
/** @file ethash.cpp
 * Ethash class testing.
 */

#include <boost/test/unit_test.hpp>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <libethash/internal.h>
#include <libethashseal/Ethash.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

BOOST_FIXTURE_TEST_SUITE(EthashTests, TestOutputHelperFixture)

// FIXME: Add a helper function here, because the test cases are almost identical.
// TODO: Add tests for Homestead difficulty change.

BOOST_AUTO_TEST_CASE(calculateDifficultyByzantiumWithoutUncles)
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

	BOOST_CHECK_EQUAL(ethash.calculateDifficulty(header, parentHeader), 999024);
}

BOOST_AUTO_TEST_CASE(calculateDifficultyByzantiumWithUncles)
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
	parentHeader.setSha3Uncles(h256("0x949d991d685738352398dff73219ab19c62c06e6f8ce899fbae755d5127ed1ef"));

	BlockHeader header;
	header.clear();
	header.setNumber(0x2001);
	header.setTimestamp(130);

	BOOST_CHECK_EQUAL(ethash.calculateDifficulty(header, parentHeader), 999512);
}

BOOST_AUTO_TEST_CASE(calculateDifficultyByzantiumMaxAdjustment)
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

	BOOST_CHECK_EQUAL(ethash.calculateDifficulty(header, parentHeader), 951688);
}

BOOST_AUTO_TEST_CASE(ethash_light_cache)
{
    struct ItemTest
    {
        size_t index;
        const char* hash;
    };

    struct EpochTest
    {
        uint64_t epoch;
        std::vector<ItemTest> test;
    };

    // clang-format off
    EpochTest testCases[] = {
        {0, {
            {0,      "5e493e76a1318e50815c6ce77950425532964ebbb8dcf94718991fa9a82eaf37658de68ca6fe078884e803da3a26a4aa56420a6867ebcd9ab0f29b08d1c48fed"},
            {1,      "47bcbf5825d4ba95ed8d9074291fa2242cd6aabab201fd923565086c6d19b93c6936bba8def3c8e40f891288f9926eb87318c30b22611653439801d2a596a78c"},
            {52459,  "a1cfe59d6fcb93fe54a090f4d440472253b1d18962acc64a949c955ee5785c0a577bbbe8e02acdcc2d829df28357ec6ba7bb37d5026a6bd2c43e2f990b416fa9"},
            {176162, "b67128c2b592f834aaa17dad7990596f62cb85b11b3ee7072fc9f8d054366d30bed498a1d2e296920ce01ec0e32559c434a6a20329e23020ecce615ff7938c23"},
            {194590, "05ed082c5cb2c511aceb8ecd2dc785b46f77122e788e3dacbc76ecee5050b83be076758ca9aa6528e384edbf7c3ccb96ca0622fcbdd643380bd9d4dd2e293a59"},
            {262137, "4615b9c6e721f9c7b36ab317e8f125cc4edc42fabf391535da5ef15090182c2523a007d04aeca6fd1d3f926f5ae27a65400404fcdc80dd7da1d46fdaac030d06"},
            {262138, "724f2f86c24c487809dc3897acbbd32d5d791e4536aa1520e65e93891a40dde5887899ffc556cbd174f426e32ae2ab711be859601c024d1514b29a27370b662e"},
        }},
        {171, {
            {0,      "bb7f72d3813a9a9ad14e6396a3be9edba40197f8c6b60a26ca188260227f5d287616e9c93da7de35fd237c191c36cdcc00abd98dfcacd11d1f2544aa52000917"},
            {1,      "460036cc618bec3abd406c0cb53182c4b4dbbcc0266995d0f7dd3511b5a4d599405cb6cc3aff5874a7cfb87e71688ba0bb63c4aa756fd36676aa947d53c9a1b1"},
            {173473, "d5cd1e7ced40ad920d977ff3a28e11d8b42b556c44e3cf48e4cce0a0854a188182f971db468756f951ed9b76c9ecf3dbbdd0209e89febe893a88785e3c714e37"},
            {202503, "609227c6156334b0633526c032ab68d82414aee92461e60e75c5751e09e5c4d645e13e1332d2cddd993da5b5b872b18b5c26eabef39acf4fc610120685aa4b24"},
            {227478, "fb5bd4942d7da4c80dc4075e5dc121a1fb1d42a7feae844d5fe321cda1db3dbdd4035dadc5b69585f77580a61260dc36a75549995f6bcba5539da8fa726c1dd5"},
            {612347, "7a3373b5c50b8950de5172b0f2b7565ecaf00e3543de972e21122fb31505085b196c6be11738d6fce828dac744159bbd62381beddfcbd00586b8a84c6c4468c8"},
            {612348, "ec45073bd7820fe58ea29fa89375050cfb1da7bdb17b79f20f8e427bef1cdc0976d1291597fece7f538e5281a9d8df3f0b842bb691ade89d3864dfa965c7e187"},
        }},
        {2047, {
            {0,       "e887f74775c5ac3f2ed928d74dde3d8b821e9b9f251f6bb66ccc817c3c52e01b319d5cf394b0af867c871acca3375c0118ffbdafe4d7b7d23d7361cf14ed1458"},
            {1,       "9e6132d61fab636a6b95207c9d4437beabaa683d50a03a49e0fc79c1d375a067f2ced6d3be3f26e7e864a156035ad80d1a5295bf5e2be71722ed8f2708c50997"},
            {574988,  "d5f763c03e8343d4e9e55fffb45045bb25d3f9d50175453bec652e469f29419058e33bcddf8a39ba2cc84a8bf52ed58d48bcbe052b41827ed3c14b281bf088af"},
            {2778440, "d1eafd2b51d3c4485d1959ed49434c5e0684bd14e58e8b77244fca33939f64f396d205df1fb19b342f9b8605e4098ae6244d06a0a564cdd150f8e3a06681b5fb"},
            {3292172, "004aa90c2ffae1ac6c5573222a214c05a22c5d4d4db35747a945d7903aa1867ae57783cafd350fdaf23441da020403810213c3724be37e9353909d9a5283819d"},
            {4454397, "5a74ae8affebf2df925265b8467ae3f71b61814636d34bee677eeb308f28c7c18a0e6394c2c20b3026ac33386c0a2af38f92a1cc1633ea66ceefbf1e9090da57"},
            {4454398, "0504239bf95d918c1f09b9f05659376a4dd461f3b5025ab05fefd1fb6c41f08da90e38dcff6861bfe28ab3e09997b9f373f017c13f30818670c92267d5b91d55"},
        }},
    };
    // clang-format on

    for (auto& a : testCases)
    {
        const uint64_t blockNumber = a.epoch * 30000;
        const auto light = ethash_light_new(blockNumber);
        const auto items = static_cast<const h512*>(light->cache);
        const auto numItems = light->cache_size / sizeof(h512);

        for (auto& b : a.test)
        {
            BOOST_REQUIRE_LT(b.index, numItems);
            BOOST_TEST_INFO(b.index);
            BOOST_CHECK_EQUAL(toHex(items[b.index]), b.hash);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
