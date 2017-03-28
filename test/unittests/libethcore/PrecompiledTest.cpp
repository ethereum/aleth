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
/** @file PrecompiledTest.cpp
 * Preompiled contract implemetations testing.
 */

#include <boost/test/unit_test.hpp>
#include <test/tools/libtesteth/TestHelper.h>
#include <libethcore/Precompiled.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

BOOST_FIXTURE_TEST_SUITE(PrecompiledTests, TestOutputHelper)

BOOST_AUTO_TEST_CASE(modexpFermatTheorem)
{
	PrecompiledExecutor exec = PrecompiledRegistrar::executor("modexp");

	bytes in = fromHex(
		"0000000000000000000000000000000000000000000000000000000000000001"
		"0000000000000000000000000000000000000000000000000000000000000020"
		"0000000000000000000000000000000000000000000000000000000000000020"
		"03"
		"fffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2e"
		"fffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2f");
	auto res = exec(bytesConstRef(in.data(), in.size()));

	BOOST_REQUIRE(res.first);
	bytes expected = fromHex("0000000000000000000000000000000000000000000000000000000000000001");
	BOOST_REQUIRE_EQUAL_COLLECTIONS(res.second.begin(), res.second.end(), expected.begin(), expected.end());
}
/*
BOOST_AUTO_TEST_CASE(modexpTooLarge)
{
	PrecompiledExecutor exec = PrecompiledRegistrar::executor("modexp");

	bytes in = fromHex(
		"0000000000000000000000000000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000000000000000000000000020"
		"ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
		"fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffe"
		"fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffd");
	auto res = exec(bytesConstRef(in.data(), in.size()));

	BOOST_REQUIRE_EQUAL(res.first, false);
}
*/

BOOST_AUTO_TEST_CASE(modexpExtraByteIgnored)
{
	PrecompiledExecutor exec = PrecompiledRegistrar::executor("modexp");

	bytes in = fromHex(
		"0000000000000000000000000000000000000000000000000000000000000001"
		"0000000000000000000000000000000000000000000000000000000000000002"
		"0000000000000000000000000000000000000000000000000000000000000020"
		"03"
		"ffff"
		"8000000000000000000000000000000000000000000000000000000000000000"
		"07");
	auto res = exec(bytesConstRef(in.data(), in.size()));

	BOOST_REQUIRE(res.first);
	bytes expected = fromHex("3b01b01ac41f2d6e917c6d6a221ce793802469026d9ab7578fa2e79e4da6aaab");
	BOOST_REQUIRE_EQUAL_COLLECTIONS(res.second.begin(), res.second.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(modexpRightPadding)
{
	PrecompiledExecutor exec = PrecompiledRegistrar::executor("modexp");

	bytes in = fromHex(
		"0000000000000000000000000000000000000000000000000000000000000001"
		"0000000000000000000000000000000000000000000000000000000000000002"
		"0000000000000000000000000000000000000000000000000000000000000020"
		"03"
		"ffff"
		"80");
	auto res = exec(bytesConstRef(in.data(), in.size()));

	BOOST_REQUIRE(res.first);
	bytes expected = fromHex("3b01b01ac41f2d6e917c6d6a221ce793802469026d9ab7578fa2e79e4da6aaab");
	BOOST_REQUIRE_EQUAL_COLLECTIONS(res.second.begin(), res.second.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_SUITE_END()
