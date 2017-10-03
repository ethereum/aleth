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
/** @file RangeMask.cpp
 * @author Christian <c@ethdev.com>
 * @date 2015
 */

#include <libdevcore/RangeMask.h>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace dev;
using namespace boost::unit_test;

namespace dev
{
namespace test
{

BOOST_FIXTURE_TEST_SUITE(RangeMaskTest, TestOutputHelper)

BOOST_AUTO_TEST_CASE(constructor)
{
	using RM = RangeMask;
	using Range = pair<unsigned, unsigned>;
	for (RM r: {RM(), RM(1, 10), RM(Range(2, 10))})
	{
		ETH_CHECK(r.empty());
		ETH_CHECK(!r.contains(0));
		ETH_CHECK(!r.contains(1));
		ETH_CHECK_EQUAL(0, r.size());
	}
	ETH_CHECK(RM().full());
	ETH_CHECK(!RM(1, 10).full());
	ETH_CHECK(!RM(Range(2, 10)).full());
}

BOOST_AUTO_TEST_CASE(simple_unions)
{
	using RM = RangeMask;
	using Range = pair<unsigned, unsigned>;
	RM m(Range(0, 2000));
	m.unionWith(Range(1, 2));
	ETH_CHECK_EQUAL(m.size(), 1);
	m.unionWith(Range(50, 250));
	ETH_CHECK_EQUAL(m.size(), 201);
	m.unionWith(Range(10, 16));
	ETH_CHECK_EQUAL(m.size(), 207);
	ETH_CHECK(m.contains(1));
	ETH_CHECK(m.contains(11));
	ETH_CHECK(m.contains(51));
	ETH_CHECK(m.contains(200));
	ETH_CHECK(!m.contains(2));
	ETH_CHECK(!m.contains(7));
	ETH_CHECK(!m.contains(17));
	ETH_CHECK(!m.contains(258));
}

BOOST_AUTO_TEST_CASE(empty_union)
{
	using RM = RangeMask;
	using Range = pair<unsigned, unsigned>;
	RM m(Range(0, 2000));
	m.unionWith(Range(3, 6));
	ETH_CHECK_EQUAL(m.size(), 3);
	m.unionWith(Range(50, 50));
	ETH_CHECK_EQUAL(m.size(), 3);
	m.unionWith(Range(0, 0));
	ETH_CHECK_EQUAL(m.size(), 3);
	m.unionWith(Range(1, 1));
	ETH_CHECK_EQUAL(m.size(), 3);
	m.unionWith(Range(2, 2));
	ETH_CHECK_EQUAL(m.size(), 3);
	m.unionWith(Range(3, 3));
	ETH_CHECK_EQUAL(m.size(), 3);
}

BOOST_AUTO_TEST_CASE(overlapping_unions)
{
	using RM = RangeMask;
	using Range = pair<unsigned, unsigned>;
	RM m(Range(0, 2000));
	m.unionWith(Range(10, 20));
	ETH_CHECK_EQUAL(10, m.size());
	m.unionWith(Range(30, 40));
	ETH_CHECK_EQUAL(20, m.size());
	m.unionWith(Range(15, 30));
	ETH_CHECK_EQUAL(40 - 10, m.size());
	m.unionWith(Range(50, 60));
	m.unionWith(Range(45, 55));
	// [40, 45) still missing here
	ETH_CHECK_EQUAL(60 - 10 - 5, m.size());
	m.unionWith(Range(15, 56));
	ETH_CHECK_EQUAL(60 - 10, m.size());
	m.unionWith(Range(15, 65));
	ETH_CHECK_EQUAL(65 - 10, m.size());
	m.unionWith(Range(5, 70));
	ETH_CHECK_EQUAL(70 - 5, m.size());
}

BOOST_AUTO_TEST_CASE(complement)
{
	using RM = RangeMask;
	using Range = pair<unsigned, unsigned>;
	RM m(Range(0, 2000));
	m.unionWith(7).unionWith(9);
	m = ~m;
	m.unionWith(7).unionWith(9);
	m = ~m;
	ETH_CHECK(m.empty());

	m += Range(0, 10);
	m += Range(1000, 2000);
	m.invert();
	ETH_CHECK_EQUAL(m.size(), 1000 - 10);
}

BOOST_AUTO_TEST_CASE(iterator)
{
	using RM = RangeMask;
	using Range = pair<unsigned, unsigned>;
	RM m(Range(0, 2000));
	m.unionWith(Range(7, 9));
	m.unionWith(11);
	m.unionWith(Range(200, 205));

	vector<unsigned> elements;
	copy(m.begin(), m.end(), back_inserter(elements));
	ETH_CHECK(elements == (vector<unsigned>{7, 8, 11, 200, 201, 202, 203, 204}));
}

BOOST_AUTO_TEST_SUITE_END()

}
}
