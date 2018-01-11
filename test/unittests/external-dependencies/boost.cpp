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
/// @file
/// Unit tests for boost libraries.

#include <boost/test/unit_test.hpp>
#include <libdevcore/Common.h>
#include <test/tools/libtesteth/TestHelper.h>

using namespace dev;
using namespace dev::test;
using namespace boost;
using namespace boost::multiprecision;

BOOST_FIXTURE_TEST_SUITE(boostTests, TestOutputHelperFixture)

// test that reproduces issue https://github.com/ethereum/cpp-ethereum/issues/1977
BOOST_AUTO_TEST_CASE(u256_overflow_test)
{
	dev::u256 a = 14;
	dev::bigint b = dev::bigint("115792089237316195423570985008687907853269984665640564039457584007913129639948");
	// to fix cast `a` to dev::bigint
	BOOST_CHECK(a < b);
}

BOOST_AUTO_TEST_CASE(u256_shift_left)
{
	u256 a = 1;
	uint64_t amount = 1;
	auto b = a << amount;
	BOOST_CHECK_EQUAL(b, 2);

	auto high_bit = u256{0};
	bit_set(high_bit, 255);
	BOOST_CHECK_EQUAL(a << 255, high_bit);
	BOOST_CHECK_EQUAL(a << uint64_t(256), 0);
	BOOST_CHECK_EQUAL(a << 0, a);

	u256 c = 3;
	BOOST_CHECK_EQUAL(c, 3);
	BOOST_CHECK_EQUAL(c << uint64_t(256), 0);
	BOOST_CHECK_EQUAL(c << 0, c);

	// Bug workaround:
	BOOST_CHECK_EQUAL(static_cast<u256>(bigint(u256(3)) << 255), u256(1) << 255);
}

BOOST_AUTO_TEST_CASE(u256_shift_left_bug)
{
	// Bug reported: https://github.com/boostorg/multiprecision/issues/31
	using uint256 = number<cpp_int_backend<256, 256, unsigned_magnitude, unchecked, void>>;
	BOOST_CHECK_EQUAL(uint256(3) << 255, uint256(1) << 255);
}

BOOST_AUTO_TEST_CASE(u256_logical_shift_right)
{
	u256 a = 1;
	uint64_t amount = 1;
	auto b = a >> amount;
	BOOST_CHECK_EQUAL(b, 0);
	BOOST_CHECK_EQUAL(a >> 255, 0);
	BOOST_CHECK_EQUAL(a >> uint64_t(256), 0);
	BOOST_CHECK_EQUAL(a >> uint64_t(-1), 0);

	u256 h;
	bit_set(h, 255);
	BOOST_CHECK_EQUAL(h >> 0,   u256(1) << 255);
	BOOST_CHECK_EQUAL(h >> 1,   u256(1) << 254);
	BOOST_CHECK_EQUAL(h >> 2,   u256(1) << 253);
	BOOST_CHECK_EQUAL(h >> 254, u256(1) << 1);
	BOOST_CHECK_EQUAL(h >> 255, u256(1) << 0);
	BOOST_CHECK_EQUAL(h >> 256, 0);
	BOOST_CHECK_EQUAL(h >> uint64_t(-1), 0);

	u256 g;
	bit_set(g, 255);
	bit_set(g, 254);
	BOOST_CHECK_EQUAL(g >> 255, 1);
	BOOST_CHECK_EQUAL(g >> 254, 3);
	BOOST_CHECK_EQUAL(g >> 253, 3 << 1);
	BOOST_CHECK_EQUAL(g >> 252, 3 << 2);
	BOOST_CHECK_EQUAL(g >> 251, 3 << 3);
	BOOST_CHECK_EQUAL(g >> 0, u256(3) << 254);
	BOOST_CHECK_EQUAL(g >> 1, u256(3) << 253);
	BOOST_CHECK_EQUAL(g >> 2, u256(3) << 252);
	BOOST_CHECK_EQUAL(g >> 3, u256(3) << 251);
	BOOST_CHECK_EQUAL(g >> 100, u256(3) << 154);
	BOOST_CHECK_EQUAL(g >> 256, 0);
	BOOST_CHECK_EQUAL(g >> 257, 0);
	BOOST_CHECK_EQUAL(g >> uint32_t(-1), 0);
	BOOST_CHECK_EQUAL(g >> uint64_t(-1), 0);
	BOOST_CHECK_EQUAL(g >> uint16_t(-1), 0);
	BOOST_CHECK_EQUAL(g >> (uint16_t(-1) - 1), 0);
}

constexpr auto int64_min = std::numeric_limits<int64_t>::min();
static_assert(int64_min >> 1 == int64_min / 2, "I cannot shift!");

BOOST_AUTO_TEST_CASE(u256_arithmetic_shift_right)
{
	s256 a = 1;
	uint64_t amount = 1;
	auto b = a >> amount;
	BOOST_CHECK_EQUAL(b, 0);
	BOOST_CHECK_EQUAL(a >> 255, 0);
	BOOST_CHECK_EQUAL(a >> uint64_t(256), 0);
	BOOST_CHECK_EQUAL(a >> uint64_t(-1), 0);

	s256 n = -1;
	BOOST_CHECK_EQUAL(n >> 0, n);
	BOOST_CHECK_EQUAL(n >> 1, n);
	BOOST_CHECK_EQUAL(n >> 2, n);
	BOOST_CHECK_EQUAL(n >> 254, n);
	BOOST_CHECK_EQUAL(n >> 255, n);
	BOOST_CHECK_EQUAL(n >> 256, n);
	BOOST_CHECK_EQUAL(n >> 257, n);
	BOOST_CHECK_EQUAL(n >> ~uint64_t(0), n);

	// Test min value. This actually -(2^256-1), not -(2^255) as in C.
	s256 h = std::numeric_limits<s256>::min();
	BOOST_CHECK_LT(h, 0);
	BOOST_CHECK_EQUAL(h >> 0, h);
	BOOST_CHECK_EQUAL(h >> 256, -1);

	// Test EVM min value.
	s256 g = s256(-1) << 255;
	BOOST_CHECK_LT(g, 0);
	BOOST_CHECK_EQUAL(static_cast<u256>(g), u256(1) << 255);
	BOOST_CHECK_EQUAL(g >> 0, g);
	BOOST_CHECK_EQUAL(static_cast<u256>(g >> 1), u256(0b11) << 254);
	BOOST_CHECK_EQUAL(static_cast<u256>(g >> 2), u256(0b111) << 253);
	BOOST_CHECK_EQUAL(static_cast<u256>(g >> 3), u256(0b1111) << 252);

	BOOST_CHECK_EQUAL(static_cast<u256>(g >> 255), ~u256(0));
	BOOST_CHECK_EQUAL(static_cast<u256>(g >> 254), ~u256(0b1));
	BOOST_CHECK_EQUAL(static_cast<u256>(g >> 253), ~u256(0b11));

	// Test shifting more that one bit.
	s256 k = s256(0b111) << 252;
	BOOST_CHECK_EQUAL(k, u256(0b111) << 252);
	BOOST_CHECK_EQUAL(k >> 1, u256(0b111) << 251);
	BOOST_CHECK_EQUAL(k >> 2, u256(0b111) << 250);
	BOOST_CHECK_EQUAL(k >> 252, 0b111);
	BOOST_CHECK_EQUAL(k >> 253, 0b11);
	BOOST_CHECK_EQUAL(k >> 254, 0b1);
	BOOST_CHECK_EQUAL(k >> 255, 0);
	BOOST_CHECK_EQUAL(k >> 256, 0);
	BOOST_CHECK_EQUAL(k >> ~uint32_t(0), 0);

	// Division equivalence.

	// Built-in type:
	int64_t d = int64_min;
	BOOST_CHECK_EQUAL(d >> 1, d / 2);
	int64_t e = d + 1;
	BOOST_CHECK_EQUAL(e >> 1, e / 2 - 1);

	// Boost type:
	BOOST_CHECK_EQUAL(h >> 1, h / 2 - 1);
}

BOOST_AUTO_TEST_SUITE_END()
