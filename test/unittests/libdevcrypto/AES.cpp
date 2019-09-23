// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include <libdevcore/SHA3.h>
#include <libdevcrypto/AES.h>

#include <gtest/gtest.h>

using namespace std;
using namespace dev;

TEST(AES, decrypt)
{
    bytes seed = fromHex(
        "2dbaead416c20cfd00c2fc9f1788ff9f965a2000799c96a624767cb0e1e90d2d7191efdd92349226742fdc73d1"
        "d87e2d597536c4641098b9a89836c94f58a2ab4c525c27c4cb848b3e22ea245b2bc5c8c7beaa900b0c479253fc"
        "96fce7ffc621");
    KeyPair kp(sha3Secure(aesDecrypt(&seed, "test")));
    EXPECT_EQ(Address("07746f871de684297923f933279555dda418f8a2"), kp.address());
}

TEST(AES, decryptWrongSeed)
{
    bytes seed = fromHex(
        "badaead416c20cfd00c2fc9f1788ff9f965a2000799c96a624767cb0e1e90d2d7191efdd92349226742fdc73d1"
        "d87e2d597536c4641098b9a89836c94f58a2ab4c525c27c4cb848b3e22ea245b2bc5c8c7beaa900b0c479253fc"
        "96fce7ffc621");
    KeyPair kp(sha3Secure(aesDecrypt(&seed, "test")));
    EXPECT_NE(Address("07746f871de684297923f933279555dda418f8a2"), kp.address());
}

TEST(AES, decryptWrongPassword)
{
    bytes seed = fromHex(
        "2dbaead416c20cfd00c2fc9f1788ff9f965a2000799c96a624767cb0e1e90d2d7191efdd92349226742fdc73d1"
        "d87e2d597536c4641098b9a89836c94f58a2ab4c525c27c4cb848b3e22ea245b2bc5c8c7beaa900b0c479253fc"
        "96fce7ffc621");
    KeyPair kp(sha3Secure(aesDecrypt(&seed, "badtest")));
    EXPECT_NE(Address("07746f871de684297923f933279555dda418f8a2"), kp.address());
}

TEST(AES, decryptFailInvalidSeed)
{
    bytes seed = fromHex(
        "xdbaead416c20cfd00c2fc9f1788ff9f965a2000799c96a624767cb0e1e90d2d7191efdd92349226742fdc73d1"
        "d87e2d597536c4641098b9a89836c94f58a2ab4c525c27c4cb848b3e22ea245b2bc5c8c7beaa900b0c479253fc"
        "96fce7ffc621");
    EXPECT_EQ(bytes(), aesDecrypt(&seed, "test"));
}

TEST(AES, decryptFailInvalidSeedSize)
{
    bytes seed = fromHex("000102030405060708090a0b0c0d0e0f");
    EXPECT_EQ(bytes(), aesDecrypt(&seed, "test"));
}

TEST(AES, decryptFailInvalidSeed2)
{
    bytes seed = fromHex("000102030405060708090a0b0c0d0e0f000102030405060708090a0b0c0d0e0f");
    EXPECT_EQ(bytes(), aesDecrypt(&seed, "test"));
}
