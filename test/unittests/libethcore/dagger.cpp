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
/** @file dagger.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 * Dashimoto test functions.
 */

#include <libethashseal/EthashAux.h>

#include <test/tools/libtesteth/TestHelper.h>

using namespace dev;
using namespace dev::eth;
using namespace dev::test;

namespace
{
struct EthashTestCase
{
    char const* nonce;
    char const* mixHash;
    char const* header;
    char const* seed;
    char const* result;
    int cacheSize;
    int fullSize;
    char const* headerHash;
    char const* cacheHash;
};

EthashTestCase ethashTestCases[] = {
    {
        "4242424242424242",
        "58f759ede17a706c93f13030328bcea40c1d1341fb26f2facd21ceb0dae57017",
        "f901f3a00000000000000000000000000000000000000000000000000000000000000000a01dcc4de8dec75d7a"
        "ab85b567b6ccd41ad312451b948a7413f0a142fd40d49347940000000000000000000000000000000000000000"
        "a09178d0f23c965d81f0834a4c72c6253ce6830f4022b1359aaebfc1ecba442d4ea056e81f171bcc55a6ff8345"
        "e692c0f86e5b48e01b996cadc001622fb5e363b421a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cad"
        "c001622fb5e363b421b90100000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000008302"
        "000080830f4240808080a058f759ede17a706c93f13030328bcea40c1d1341fb26f2facd21ceb0dae570178842"
        "42424242424242",
        "0000000000000000000000000000000000000000000000000000000000000000",
        "dd47fd2d98db51078356852d7c4014e6a5d6c387c35f40e2875b74a256ed7906",
        16776896,
        1073739904,
        "2a8de2adf89af77358250bf908bf04ba94a6e8c3ba87775564a41d269a05e4ce",
        "35ded12eecf2ce2e8da2e15c06d463aae9b84cb2530a00b932e4bbc484cde353",
    },
    {
        "307692cf71b12f6d",
        "e55d02c555a7969361cf74a9ec6211d8c14e4517930a00442f171bdb1698d175",
        "f901f7a01bef91439a3e070a6586851c11e6fd79bbbea074b2b836727b8e75c7d4a6b698a01dcc4de8dec75d7a"
        "ab85b567b6ccd41ad312451b948a7413f0a142fd40d4934794ea3cb5f94fa2ddd52ec6dd6eb75cf824f4058ca1"
        "a00c6e51346be0670ce63ac5f05324e27d20b180146269c5aab844d09a2b108c64a056e81f171bcc55a6ff8345"
        "e692c0f86e5b48e01b996cadc001622fb5e363b421a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cad"
        "c001622fb5e363b421b90100000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000008302"
        "004002832fefd880845511ed2a80a0e55d02c555a7969361cf74a9ec6211d8c14e4517930a00442f171bdb1698"
        "d17588307692cf71b12f6d",
        "0000000000000000000000000000000000000000000000000000000000000000",
        "ab9b13423cface72cbec8424221651bc2e384ef0f7a560e038fc68c8d8684829",
        16776896,
        1073739904,
        "100cbec5e5ef82991290d0d93d758f19082e71f234cf479192a8b94df6da6bfe",
        "35ded12eecf2ce2e8da2e15c06d463aae9b84cb2530a00b932e4bbc484cde353",
    },
};
}  // namespace

BOOST_FIXTURE_TEST_SUITE(DashimotoTests, TestOutputHelperFixture)

BOOST_AUTO_TEST_CASE(basic_test)
{
    for (auto& t : ethashTestCases)
    {
        BlockHeader header{fromHex(t.header), HeaderData};
        h256 headerHash{t.headerHash};
        eth::Nonce nonce{t.nonce};
        BOOST_REQUIRE_EQUAL(headerHash, header.hash(WithoutSeal));
        BOOST_REQUIRE_EQUAL(nonce, Ethash::nonce(header));

        BOOST_REQUIRE_EQUAL(EthashAux::get()->light(Ethash::seedHash(header))->size, t.cacheSize);
        BOOST_REQUIRE_EQUAL(
            sha3(EthashAux::get()->light(Ethash::seedHash(header))->data()), h256{t.cacheHash});

        EthashProofOfWork::Result r = EthashAux::eval(
            Ethash::seedHash(header), header.hash(WithoutSeal), Ethash::nonce(header));
        BOOST_REQUIRE_EQUAL(r.value, h256{t.result});
        BOOST_REQUIRE_EQUAL(r.mixHash, Ethash::mixHash(header));
    }
}

BOOST_AUTO_TEST_SUITE_END()
