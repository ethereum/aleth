// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include <libdevcore/RangeMask.h>

#include <gtest/gtest.h>

using namespace std;
using namespace dev;

TEST(RangeMask, constructor)
{
    using RM = RangeMask;
    using Range = pair<unsigned, unsigned>;
    for (RM r : {RM(), RM(1, 10), RM(Range(2, 10))})
    {
        EXPECT_TRUE(r.empty());
        EXPECT_FALSE(r.contains(0));
        EXPECT_FALSE(r.contains(1));
        EXPECT_EQ(0, r.size());
    }
    EXPECT_TRUE(RM().full());
    EXPECT_FALSE(RM(1, 10).full());
    EXPECT_FALSE(RM(Range(2, 10)).full());
}

TEST(RangeMask, simpleUnions)
{
    using RM = RangeMask;
    using Range = pair<unsigned, unsigned>;
    RM m(Range(0, 2000));
    m.unionWith(Range(1, 2));
    EXPECT_EQ(m.size(), 1);
    m.unionWith(Range(50, 250));
    EXPECT_EQ(m.size(), 201);
    m.unionWith(Range(10, 16));
    EXPECT_EQ(m.size(), 207);
    EXPECT_TRUE(m.contains(1));
    EXPECT_TRUE(m.contains(11));
    EXPECT_TRUE(m.contains(51));
    EXPECT_TRUE(m.contains(200));
    EXPECT_FALSE(m.contains(2));
    EXPECT_FALSE(m.contains(7));
    EXPECT_FALSE(m.contains(17));
    EXPECT_FALSE(m.contains(258));
}

TEST(RangeMask, emptyUnion)
{
    using RM = RangeMask;
    using Range = pair<unsigned, unsigned>;
    RM m(Range(0, 2000));
    m.unionWith(Range(3, 6));
    EXPECT_EQ(m.size(), 3);
    m.unionWith(Range(50, 50));
    EXPECT_EQ(m.size(), 3);
    m.unionWith(Range(0, 0));
    EXPECT_EQ(m.size(), 3);
    m.unionWith(Range(1, 1));
    EXPECT_EQ(m.size(), 3);
    m.unionWith(Range(2, 2));
    EXPECT_EQ(m.size(), 3);
    m.unionWith(Range(3, 3));
    EXPECT_EQ(m.size(), 3);
}

TEST(RangeMask, overlappingUnions)
{
    using RM = RangeMask;
    using Range = pair<unsigned, unsigned>;
    RM m(Range(0, 2000));
    m.unionWith(Range(10, 20));
    EXPECT_EQ(10, m.size());
    m.unionWith(Range(30, 40));
    EXPECT_EQ(20, m.size());
    m.unionWith(Range(15, 30));
    EXPECT_EQ(40 - 10, m.size());
    m.unionWith(Range(50, 60));
    m.unionWith(Range(45, 55));
    // [40, 45) still missing here
    EXPECT_EQ(60 - 10 - 5, m.size());
    m.unionWith(Range(15, 56));
    EXPECT_EQ(60 - 10, m.size());
    m.unionWith(Range(15, 65));
    EXPECT_EQ(65 - 10, m.size());
    m.unionWith(Range(5, 70));
    EXPECT_EQ(70 - 5, m.size());
}

TEST(RangeMask, complement)
{
    using RM = RangeMask;
    using Range = pair<unsigned, unsigned>;
    RM m(Range(0, 2000));
    m.unionWith(7).unionWith(9);
    m = ~m;
    m.unionWith(7).unionWith(9);
    m = ~m;
    EXPECT_TRUE(m.empty());

    m += Range(0, 10);
    m += Range(1000, 2000);
    m.invert();
    EXPECT_EQ(m.size(), 1000 - 10);
}

TEST(RangeMask, iterator)
{
    using RM = RangeMask;
    using Range = pair<unsigned, unsigned>;
    RM m(Range(0, 2000));
    m.unionWith(Range(7, 9));
    m.unionWith(11);
    m.unionWith(Range(200, 205));

    vector<unsigned> elements;
    copy(m.begin(), m.end(), back_inserter(elements));
    EXPECT_TRUE(elements == (vector<unsigned>{7, 8, 11, 200, 201, 202, 203, 204}));
}

