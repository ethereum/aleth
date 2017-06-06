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

BOOST_AUTO_TEST_CASE(modexpZeroBase)
{
	PrecompiledExecutor exec = PrecompiledRegistrar::executor("modexp");

	bytes in = fromHex(
		"0000000000000000000000000000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000000000000000000000000020"
		"0000000000000000000000000000000000000000000000000000000000000020"
		"fffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2e"
		"fffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2f");
	auto res = exec(bytesConstRef(in.data(), in.size()));

	BOOST_REQUIRE(res.first);
	bytes expected = fromHex("0000000000000000000000000000000000000000000000000000000000000000");
	BOOST_REQUIRE_EQUAL_COLLECTIONS(res.second.begin(), res.second.end(), expected.begin(), expected.end());
}

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

BOOST_AUTO_TEST_CASE(modexpMissingValues)
{
	PrecompiledExecutor exec = PrecompiledRegistrar::executor("modexp");

	bytes in = fromHex(
		"0000000000000000000000000000000000000000000000000000000000000001"
		"0000000000000000000000000000000000000000000000000000000000000002"
		"0000000000000000000000000000000000000000000000000000000000000020"
		"03");
	auto res = exec(bytesConstRef(in.data(), in.size()));

	BOOST_REQUIRE(res.first);
	bytes expected = fromHex("0000000000000000000000000000000000000000000000000000000000000000");
	BOOST_REQUIRE_EQUAL_COLLECTIONS(res.second.begin(), res.second.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(modexpEmptyValue)
{
	PrecompiledExecutor exec = PrecompiledRegistrar::executor("modexp");

	bytes in = fromHex(
		"0000000000000000000000000000000000000000000000000000000000000001"
		"0000000000000000000000000000000000000000000000000000000000000000"
		"0000000000000000000000000000000000000000000000000000000000000020"
		"03"
		"8000000000000000000000000000000000000000000000000000000000000000");
	auto res = exec(bytesConstRef(in.data(), in.size()));

	BOOST_REQUIRE(res.first);
	bytes expected = fromHex("0000000000000000000000000000000000000000000000000000000000000001");
	BOOST_REQUIRE_EQUAL_COLLECTIONS(res.second.begin(), res.second.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(modexpZeroPowerZero)
{
	PrecompiledExecutor exec = PrecompiledRegistrar::executor("modexp");

	bytes in = fromHex(
		"0000000000000000000000000000000000000000000000000000000000000001"
		"0000000000000000000000000000000000000000000000000000000000000001"
		"0000000000000000000000000000000000000000000000000000000000000020"
		"00"
		"00"
		"80");
	auto res = exec(bytesConstRef(in.data(), in.size()));

	BOOST_REQUIRE(res.first);
	bytes expected = fromHex("0000000000000000000000000000000000000000000000000000000000000001");
	BOOST_REQUIRE_EQUAL_COLLECTIONS(res.second.begin(), res.second.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(modexpZeroPowerZeroModZero)
{
	PrecompiledExecutor exec = PrecompiledRegistrar::executor("modexp");

	bytes in = fromHex(
		"0000000000000000000000000000000000000000000000000000000000000001"
		"0000000000000000000000000000000000000000000000000000000000000001"
		"0000000000000000000000000000000000000000000000000000000000000020"
		"00"
		"00"
		"00");
	auto res = exec(bytesConstRef(in.data(), in.size()));

	BOOST_REQUIRE(res.first);
	bytes expected = fromHex("0000000000000000000000000000000000000000000000000000000000000000");
	BOOST_REQUIRE_EQUAL_COLLECTIONS(res.second.begin(), res.second.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(modexpModLengthZero)
{
	PrecompiledExecutor exec = PrecompiledRegistrar::executor("modexp");

	bytes in = fromHex(
		"0000000000000000000000000000000000000000000000000000000000000001"
		"0000000000000000000000000000000000000000000000000000000000000001"
		"0000000000000000000000000000000000000000000000000000000000000000"
		"01"
		"01");
	auto res = exec(bytesConstRef(in.data(), in.size()));

	BOOST_REQUIRE(res.first);
	BOOST_REQUIRE(res.second.empty());
}

BOOST_AUTO_TEST_CASE(modexpCostFermatTheorem)
{
	PrecompiledPricer cost = PrecompiledRegistrar::pricer("modexp");

	bytes in = fromHex(
		"0000000000000000000000000000000000000000000000000000000000000001"
		"0000000000000000000000000000000000000000000000000000000000000020"
		"0000000000000000000000000000000000000000000000000000000000000020"
		"03"
		"fffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2e"
		"fffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2f");
	auto res = cost(bytesConstRef(in.data(), in.size()));

	BOOST_REQUIRE_EQUAL(static_cast<int>(res), 2611);
}

BOOST_AUTO_TEST_CASE(modexpCostTooLarge)
{
	PrecompiledPricer cost = PrecompiledRegistrar::pricer("modexp");

	bytes in = fromHex(
		"0000000000000000000000000000000000000000000000000000000000000000"
		"ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
		"0000000000000000000000000000000000000000000000000000000000000020"
		"fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffe"
		"fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffd");
	auto res = cost(bytesConstRef(in.data(), in.size()));

	BOOST_REQUIRE(res == bigint{"9485687950320942729098935091911713411339877143809275006112365281928243580103464"});
}

BOOST_AUTO_TEST_CASE(modexpCostEmptyExponent)
{
	PrecompiledPricer cost = PrecompiledRegistrar::pricer("modexp");

	bytes in = fromHex(
		"0000000000000000000000000000000000000000000000000000000000000008" // length of B
		"0000000000000000000000000000000000000000000000000000000000000000" // length of E
		"0000000000000000000000000000000000000000000000000000000000000010" // length of M
		"998877665544332211" // B
		"" // E
		"998877665544332211998877665544332211" // M
		"9978" // Garbage that should be ignored
	);
	auto res = cost(bytesConstRef(in.data(), in.size()));

	BOOST_REQUIRE(res == bigint{"2"});
}

BOOST_AUTO_TEST_CASE(modexpCostZeroExponent)
{
	PrecompiledPricer cost = PrecompiledRegistrar::pricer("modexp");

	bytes in = fromHex(
		"0000000000000000000000000000000000000000000000000000000000000000" // length of B
		"0000000000000000000000000000000000000000000000000000000000000003" // length of E
		"000000000000000000000000000000000000000000000000000000000000000a" // length of M
		"" // B
		"000000" // E
		"112233445566778899aa" // M
	);
	auto res = cost(bytesConstRef(in.data(), in.size()));

	BOOST_REQUIRE(res == bigint{"1"});
}

BOOST_AUTO_TEST_CASE(modexpCostApproximated)
{
	PrecompiledPricer cost = PrecompiledRegistrar::pricer("modexp");

	bytes in = fromHex(
		"0000000000000000000000000000000000000000000000000000000000000003" // length of B
		"0000000000000000000000000000000000000000000000000000000000000021" // length of E
		"000000000000000000000000000000000000000000000000000000000000000a" // length of M
		"111111" // B
		"ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff" // E
		"112233445566778899aa" // M
	);
	auto res = cost(bytesConstRef(in.data(), in.size()));

	BOOST_REQUIRE(res == bigint{"263"});
}

BOOST_AUTO_TEST_CASE(modexpCostApproximatedPartialByte)
{
	PrecompiledPricer cost = PrecompiledRegistrar::pricer("modexp");

	bytes in = fromHex(
		"0000000000000000000000000000000000000000000000000000000000000003" // length of B
		"0000000000000000000000000000000000000000000000000000000000000021" // length of E
		"000000000000000000000000000000000000000000000000000000000000000a" // length of M
		"111111" // B
		"02ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff" // E
		"112233445566778899aa" // M
	);
	auto res = cost(bytesConstRef(in.data(), in.size()));

	BOOST_REQUIRE(res == bigint{"257"});
}

BOOST_AUTO_TEST_CASE(modexpCostApproximatedGhost)
{
	PrecompiledPricer cost = PrecompiledRegistrar::pricer("modexp");

	bytes in = fromHex(
		"0000000000000000000000000000000000000000000000000000000000000003" // length of B
		"0000000000000000000000000000000000000000000000000000000000000021" // length of E
		"000000000000000000000000000000000000000000000000000000000000000a" // length of M
		"111111" // B
		"000000000000000000000000000000000000000000000000000000000000000000" // E
		"112233445566778899aa" // M
	);
	auto res = cost(bytesConstRef(in.data(), in.size()));

	BOOST_REQUIRE(res == bigint{"8"});
}

BOOST_AUTO_TEST_SUITE_END()
