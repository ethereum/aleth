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
/** @file CommonJS.cpp
 * @author Lefteris Karapetsas <lefteris@refu.co>
 * @date 2015
 * Tests for functions in CommonJS.h
 */

#include <boost/test/unit_test.hpp>
#include <libdevcore/CommonJS.h>
#include <test/libtesteth/TestHelper.h>

using namespace dev;
using namespace std;
using namespace test;

BOOST_FIXTURE_TEST_SUITE(CommonJSTests, TestOutputHelper)

BOOST_AUTO_TEST_CASE(test_toJS)
{
	h64 a("0xbaadf00ddeadbeef");
	u64 b("0xffff0000bbbaaaa");
	uint64_t c = 38990234243;
	bytes d = {0xff, 0x0, 0xef, 0xbc};

	BOOST_CHECK(toJS(a) == "0xbaadf00ddeadbeef");
	BOOST_CHECK(toJS(b) == "0xffff0000bbbaaaa");
	BOOST_CHECK(toJS(c) == "0x913ffc283");
	BOOST_CHECK(toJS(d) == "0xff00efbc");
}

BOOST_AUTO_TEST_CASE(test_jsToBytes)
{
	bytes a = {0xff, 0xaa, 0xbb, 0xcc};
	bytes b = {0x03, 0x89, 0x90, 0x23, 0x42, 0x43};
	BOOST_CHECK(a == jsToBytes("0xffaabbcc"));
	BOOST_CHECK(b == jsToBytes("38990234243"));
	BOOST_CHECK(bytes() == jsToBytes(""));
	BOOST_CHECK(bytes() == jsToBytes("Invalid hex chars"));
}

BOOST_AUTO_TEST_CASE(test_padded)
{
	bytes a = {0xff, 0xaa};
	BOOST_CHECK(bytes({0x00, 0x00, 0xff, 0xaa}) == padded(a, 4));
	bytes b = {};
	BOOST_CHECK(bytes({0x00, 0x00, 0x00, 0x00}) == padded(b, 4));
	bytes c = {0xff, 0xaa, 0xbb, 0xcc};
	BOOST_CHECK(bytes{0xcc} == padded(c, 1));
}

BOOST_AUTO_TEST_CASE(test_paddedRight)
{
	bytes a = {0xff, 0xaa};
	BOOST_CHECK(bytes({0xff, 0xaa, 0x00, 0x00}) == paddedRight(a, 4));
	bytes b = {};
	BOOST_CHECK(bytes({0x00, 0x00, 0x00, 0x00}) == paddedRight(b, 4));
	bytes c = {0xff, 0xaa, 0xbb, 0xcc};
	BOOST_CHECK(bytes{0xff} == paddedRight(c, 1));
}

BOOST_AUTO_TEST_CASE(test_unpadded)
{
	bytes a = {0xff, 0xaa, 0x00, 0x00, 0x00};
	BOOST_CHECK(bytes({0xff, 0xaa}) == unpadded(a));
	bytes b = {0x00, 0x00};
	BOOST_CHECK(bytes() == unpadded(b));
	bytes c = {};
	BOOST_CHECK(bytes() == unpadded(c));
}

BOOST_AUTO_TEST_CASE(test_unpaddedLeft)
{
	bytes a = {0x00, 0x00, 0x00, 0xff, 0xaa};
	BOOST_CHECK(bytes({0xff, 0xaa}) == unpadLeft(a));
	bytes b = {0x00, 0x00};
	BOOST_CHECK(bytes() == unpadLeft(b));
	bytes c = {};
	BOOST_CHECK(bytes() == unpadLeft(c));
}

BOOST_AUTO_TEST_CASE(test_fromRaw)
{
	//non ascii characters means empty string
	h256 a("0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
	BOOST_CHECK("" == fromRaw(a));
	h256 b("");
	BOOST_CHECK("" == fromRaw(b));
	h256 c("0x4173636969436861726163746572730000000000000000000000000000000000");
	BOOST_CHECK("AsciiCharacters" == fromRaw(c));
}

BOOST_AUTO_TEST_CASE(test_jsToFixed)
{
	h256 a("0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
	BOOST_CHECK(a == jsToFixed<32>("0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
	h256 b("0x000000000000000000000000000000000000000000000000000000740c54b42f");
	BOOST_CHECK(b == jsToFixed<32>("498423084079"));
	BOOST_CHECK(h256() == jsToFixed<32>("NotAHexadecimalOrDecimal"));
}

BOOST_AUTO_TEST_CASE(test_jsToInt)
{
	BOOST_CHECK(43832124 == jsToInt("43832124"));
	BOOST_CHECK(1342356623 == jsToInt("0x5002bc8f"));
	BOOST_CHECK(3483942 == jsToInt("015224446"));
	BOOST_CHECK(0 == jsToInt("NotAHexadecimalOrDecimal"));

	BOOST_CHECK(u256("983298932490823474234") == jsToInt<32>("983298932490823474234"));
	BOOST_CHECK(u256("983298932490823474234") == jsToInt<32>("0x354e03915c00571c3a"));
	BOOST_CHECK(u256() == jsToInt<32>("NotAHexadecimalOrDecimal"));
	BOOST_CHECK(u128("228273101986715476958866839113050921216") == jsToInt<16>("0xabbbccddeeff11223344556677889900"));
	BOOST_CHECK(u128() == jsToInt<16>("NotAHexadecimalOrDecimal"));
}

BOOST_AUTO_TEST_CASE(test_jsToU256)
{
	BOOST_CHECK(u256("983298932490823474234") == jsToU256("983298932490823474234"));
	BOOST_CHECK(u256() == jsToU256("NotAHexadecimalOrDecimal"));
}

BOOST_AUTO_TEST_SUITE_END()
