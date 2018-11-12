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

#include <libdevcore/FixedHash.h>
#include <libdevcore/SHA3.h>

#include <gtest/gtest.h>

using namespace std;
using namespace dev;

TEST(FixedHash, Comparisons)
{
    FixedHash<4> h1(sha3("abcd"));
    FixedHash<4> h2(sha3("abcd"));
    FixedHash<4> h3(sha3("aadd"));
    FixedHash<4> h4(0xBAADF00D);
    FixedHash<4> h5(0xAAAAAAAA);
    FixedHash<4> h6(0xBAADF00D);

    EXPECT_EQ(h1, h2);
    EXPECT_NE(h2, h3);

    EXPECT_GT(h4, h5);
    EXPECT_LT(h5, h4);
    EXPECT_LE(h6, h4);
    EXPECT_GE(h6, h4);
}

TEST(FixedHash, XOR)
{
    FixedHash<2> h1("0xAAAA");
    FixedHash<2> h2("0xBBBB");

    EXPECT_EQ((h1 ^ h2), FixedHash<2>("0x1111"));
    h1 ^= h2;
    EXPECT_EQ(h1, FixedHash<2>("0x1111"));
}

TEST(FixedHash, OR)
{
    FixedHash<4> h1("0xD3ADB33F");
    FixedHash<4> h2("0xBAADF00D");
    FixedHash<4> res("0xFBADF33F");

    EXPECT_EQ((h1 | h2), res);
    h1 |= h2;
    EXPECT_EQ(h1, res);
}

TEST(FixedHash, AND)
{
    FixedHash<4> h1("0xD3ADB33F");
    FixedHash<4> h2("0xBAADF00D");
    FixedHash<4> h3("0x92aDB00D");

    EXPECT_EQ((h1 & h2), h3);
    h1 &= h2;
    EXPECT_EQ(h1, h3);
}

TEST(FixedHash, Invert)
{
    FixedHash<4> h1("0xD3ADB33F");
    FixedHash<4> h2("0x2C524CC0");

    EXPECT_EQ(~h1, h2);
}

TEST(FixedHash, Contains)
{
    FixedHash<4> h1("0xD3ADB331");
    FixedHash<4> h2("0x0000B331");
    FixedHash<4> h3("0x0000000C");

    EXPECT_TRUE(h1.contains(h2));
    EXPECT_FALSE(h1.contains(h3));
}

void incrementSingleIteration(unsigned seed)
{
    unsigned next = seed + 1;

    FixedHash<4> h1(seed);
    FixedHash<4> h2 = h1;
    FixedHash<4> h3(next);

    FixedHash<32> hh1(seed);
    FixedHash<32> hh2 = hh1;
    FixedHash<32> hh3(next);

    EXPECT_EQ(++h2, h3);
    EXPECT_EQ(++hh2, hh3);

    EXPECT_GT(h2, h1);
    EXPECT_GT(hh2, hh1);

    unsigned reverse1 = ((FixedHash<4>::Arith)h2).convert_to<unsigned>();
    unsigned reverse2 = ((FixedHash<32>::Arith)hh2).convert_to<unsigned>();

    EXPECT_EQ(next, reverse1);
    EXPECT_EQ(next, reverse2);
}

TEST(FixedHash, Increment)
{
    incrementSingleIteration(0);
    incrementSingleIteration(1);
    incrementSingleIteration(0xBAD);
    incrementSingleIteration(0xBEEF);
    incrementSingleIteration(0xFFFF);
    incrementSingleIteration(0xFEDCBA);
    incrementSingleIteration(0x7FFFFFFF);

    FixedHash<4> h(0xFFFFFFFF);
    FixedHash<4> zero;
    EXPECT_EQ(++h, zero);
}
