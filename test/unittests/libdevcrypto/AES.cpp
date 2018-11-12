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

#include <libdevcore/SHA3.h>
#include <libdevcrypto/AES.h>

#include <gtest/gtest.h>

using namespace std;
using namespace dev;

TEST(Aes, Decrypt)
{
    bytes seed = fromHex(
        "2dbaead416c20cfd00c2fc9f1788ff9f965a2000799c96a624767cb0e1e90d2d7191efdd92349226742fdc73d1"
        "d87e2d597536c4641098b9a89836c94f58a2ab4c525c27c4cb848b3e22ea245b2bc5c8c7beaa900b0c479253fc"
        "96fce7ffc621");
    KeyPair kp(sha3Secure(aesDecrypt(&seed, "test")));
    EXPECT_EQ(Address("07746f871de684297923f933279555dda418f8a2"), kp.address());
}

TEST(Aes, DecryptWrongSeed)
{
    bytes seed = fromHex(
        "badaead416c20cfd00c2fc9f1788ff9f965a2000799c96a624767cb0e1e90d2d7191efdd92349226742fdc73d1"
        "d87e2d597536c4641098b9a89836c94f58a2ab4c525c27c4cb848b3e22ea245b2bc5c8c7beaa900b0c479253fc"
        "96fce7ffc621");
    KeyPair kp(sha3Secure(aesDecrypt(&seed, "test")));
    EXPECT_NE(Address("07746f871de684297923f933279555dda418f8a2"), kp.address());
}

TEST(Aes, DecryptWrongPassword)
{
    bytes seed = fromHex(
        "2dbaead416c20cfd00c2fc9f1788ff9f965a2000799c96a624767cb0e1e90d2d7191efdd92349226742fdc73d1"
        "d87e2d597536c4641098b9a89836c94f58a2ab4c525c27c4cb848b3e22ea245b2bc5c8c7beaa900b0c479253fc"
        "96fce7ffc621");
    KeyPair kp(sha3Secure(aesDecrypt(&seed, "badtest")));
    EXPECT_NE(Address("07746f871de684297923f933279555dda418f8a2"), kp.address());
}

TEST(Aes, DecryptFailInvalidSeed)
{
    bytes seed = fromHex(
        "xdbaead416c20cfd00c2fc9f1788ff9f965a2000799c96a624767cb0e1e90d2d7191efdd92349226742fdc73d1"
        "d87e2d597536c4641098b9a89836c94f58a2ab4c525c27c4cb848b3e22ea245b2bc5c8c7beaa900b0c479253fc"
        "96fce7ffc621");
    EXPECT_EQ(bytes(), aesDecrypt(&seed, "test"));
}

TEST(Aes, DecryptFailInvalidSeedSize)
{
    bytes seed = fromHex("000102030405060708090a0b0c0d0e0f");
    EXPECT_EQ(bytes(), aesDecrypt(&seed, "test"));
}

TEST(Aes, DecryptFailInvalidSeed2)
{
    bytes seed = fromHex("000102030405060708090a0b0c0d0e0f000102030405060708090a0b0c0d0e0f");
    EXPECT_EQ(bytes(), aesDecrypt(&seed, "test"));
}
