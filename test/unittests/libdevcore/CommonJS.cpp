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

#include <libdevcore/CommonJS.h>

#include <gtest/gtest.h>

using namespace dev;
using namespace std;

TEST(common_js, toJS)
{
    h64 a("0xbaadf00ddeadbeef");
    u64 b("0xffff0000bbbaaaa");
    uint64_t c = 38990234243;
    bytes d = {0xff, 0x0, 0xef, 0xbc};

    EXPECT_EQ(toJS(a), "0xbaadf00ddeadbeef");
    EXPECT_EQ(toJS(b), "0xffff0000bbbaaaa");
    EXPECT_EQ(toJS(c), "0x913ffc283");
    EXPECT_EQ(toJS(d), "0xff00efbc");
}

TEST(common_js, jsToBytes)
{
    bytes a = {0xff, 0xaa, 0xbb, 0xcc};
    bytes b = {0x03, 0x89, 0x90, 0x23, 0x42, 0x43};
    EXPECT_EQ(a, jsToBytes("0xffaabbcc"));
    EXPECT_EQ(b, jsToBytes("38990234243"));
    EXPECT_EQ(bytes(), jsToBytes(""));
    EXPECT_EQ(bytes(), jsToBytes("Invalid hex chars"));
}

TEST(common_js, padded)
{
    bytes a = {0xff, 0xaa};
    EXPECT_EQ(bytes({0x00, 0x00, 0xff, 0xaa}), padded(a, 4));
    bytes b = {};
    EXPECT_EQ(bytes({0x00, 0x00, 0x00, 0x00}), padded(b, 4));
    bytes c = {0xff, 0xaa, 0xbb, 0xcc};
    EXPECT_EQ(bytes{0xcc}, padded(c, 1));
}

TEST(common_js, paddedRight)
{
    bytes a = {0xff, 0xaa};
    EXPECT_EQ(bytes({0xff, 0xaa, 0x00, 0x00}), paddedRight(a, 4));
    bytes b = {};
    EXPECT_EQ(bytes({0x00, 0x00, 0x00, 0x00}), paddedRight(b, 4));
    bytes c = {0xff, 0xaa, 0xbb, 0xcc};
    EXPECT_EQ(bytes{0xff}, paddedRight(c, 1));
}

TEST(common_js, unpadded)
{
    bytes a = {0xff, 0xaa, 0x00, 0x00, 0x00};
    EXPECT_EQ(bytes({0xff, 0xaa}), unpadded(a));
    bytes b = {0x00, 0x00};
    EXPECT_EQ(bytes(), unpadded(b));
    bytes c = {};
    EXPECT_EQ(bytes(), unpadded(c));
}

TEST(common_js, unpaddedLeft)
{
    bytes a = {0x00, 0x00, 0x00, 0xff, 0xaa};
    EXPECT_EQ(bytes({0xff, 0xaa}), unpadLeft(a));
    bytes b = {0x00, 0x00};
    EXPECT_EQ(bytes(), unpadLeft(b));
    bytes c = {};
    EXPECT_EQ(bytes(), unpadLeft(c));
}

TEST(common_js, fromRaw)
{
    // non ascii characters means empty string
    h256 a("0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    EXPECT_EQ("", fromRaw(a));
    h256 b("");
    EXPECT_EQ("", fromRaw(b));
    h256 c("0x4173636969436861726163746572730000000000000000000000000000000000");
    EXPECT_EQ("AsciiCharacters", fromRaw(c));
}

TEST(common_js, jsToFixed)
{
    h256 a("0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    EXPECT_EQ(
        a, jsToFixed<32>("0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    h256 b("0x000000000000000000000000000000000000000000000000000000740c54b42f");
    EXPECT_EQ(b, jsToFixed<32>("498423084079"));
    EXPECT_EQ(h256(), jsToFixed<32>("NotAHexadecimalOrDecimal"));
}

TEST(common_js, jsToInt)
{
    EXPECT_EQ(43832124, jsToInt("43832124"));
    EXPECT_EQ(1342356623, jsToInt("0x5002bc8f"));
    EXPECT_EQ(3483942, jsToInt("015224446"));
    EXPECT_EQ(0, jsToInt("NotAHexadecimalOrDecimal"));

    EXPECT_EQ(u256("983298932490823474234"), jsToInt<32>("983298932490823474234"));
    EXPECT_EQ(u256("983298932490823474234"), jsToInt<32>("0x354e03915c00571c3a"));
    EXPECT_EQ(u256(), jsToInt<32>("NotAHexadecimalOrDecimal"));
    EXPECT_EQ(u128("228273101986715476958866839113050921216"),
        jsToInt<16>("0xabbbccddeeff11223344556677889900"));
    EXPECT_EQ(u128(), jsToInt<16>("NotAHexadecimalOrDecimal"));
}

TEST(common_js, jsToU256)
{
    EXPECT_EQ(u256("983298932490823474234"), jsToU256("983298932490823474234"));
    EXPECT_EQ(u256(), jsToU256("NotAHexadecimalOrDecimal"));
}
