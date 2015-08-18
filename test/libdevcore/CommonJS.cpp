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
#include <test/TestHelper.h>

BOOST_AUTO_TEST_SUITE(CommonJSTests)

using namespace dev;
using namespace std;;

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
	bytes b = {0x9, 0x13, 0xff, 0xc2, 0x83};
	BOOST_CHECK(a == jsToBytes("0xffaabbcc"));
	BOOST_CHECK(b == jsToBytes("38990234243"));
	BOOST_CHECK(bytes() == jsToBytes(""));
	BOOST_CHECK(bytes() == jsToBytes("Neither decimal nor hexadecimal"));
}

BOOST_AUTO_TEST_CASE(test_padded)
{
	bytes a = {0xff, 0xaa};
	BOOST_CHECK(bytes({0x00, 0x00, 0xff, 0xaa}) == padded(a, 4));
	bytes b = {};
	BOOST_CHECK(bytes({0x00, 0x00, 0x00, 0x00}) == padded(b, 4));
	bytes c = {0xff, 0xaa, 0xbb, 0xcc};
	BOOST_CHECK(bytes({0xff, 0xaa, 0xbb, 0xcc}) == padded(c, 1));
}

BOOST_AUTO_TEST_SUITE_END()
