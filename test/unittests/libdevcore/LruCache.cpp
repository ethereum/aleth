// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include <libdevcore/LruCache.h>
#include <test/tools/libtestutils/Common.h>
#include <gtest/gtest.h>

using namespace std;
using namespace dev;
using namespace dev::test;

namespace
{
using LRU = LruCache<int, int>;
using PAIR = pair<int, int>;
using VEC = vector<PAIR>;

constexpr size_t c_capacity = 10;

mt19937_64 g_randomGenerator(random_device{}());

int randomNumber(int _min = INT_MIN, int _max = INT_MAX)
{
    return std::uniform_int_distribution<int>{_min, _max}(g_randomGenerator);
}

VEC Populate(LRU& _lruCache, size_t _count)
{
    VEC ret;
    for (size_t i = 0; i < _count; i++)
    {
        auto const item = PAIR{randomNumber(), randomNumber()};
        ret.push_back(item);
        _lruCache.insert(item.first, item.second);
    }
    reverse(ret.begin(), ret.end());

    return ret;
}

void VerifyEquals(LRU& _lruCache, VEC& _data)
{
    EXPECT_EQ(_lruCache.size(), _data.size());
    size_t i = 0;
    auto iter = _lruCache.begin();
    while (iter != _lruCache.cend() && i < _data.size())
    {
        EXPECT_EQ(*iter, _data[i]);
        iter++;
        i++;
    }
}
}  // namespace

TEST(LruCache, BasicOperations)
{
    LRU lruCache{c_capacity};
    EXPECT_EQ(lruCache.capacity(), c_capacity);
    EXPECT_TRUE(lruCache.empty());

    // Populate and verify
    VEC testData = Populate(lruCache, lruCache.capacity());
    VerifyEquals(lruCache, testData);

    // Reverse order and verify
    for (size_t i = 0; i < testData.size(); i++)
        lruCache.touch(testData[i].first);
    reverse(testData.begin(), testData.end());
    VerifyEquals(lruCache, testData);

    // Remove elements and verify
    auto size = lruCache.size();
    for (PAIR item : testData)
    {
        lruCache.remove(item.first);
        EXPECT_FALSE(lruCache.contains(item.first));
        EXPECT_EQ(lruCache.size(), --size);
    }
}

TEST(LruCache, AdvancedOperations)
{
    LRU lruCache{c_capacity};
    VEC testData = Populate(lruCache, lruCache.capacity());
    VerifyEquals(lruCache, testData);
    testData = Populate(lruCache, lruCache.capacity());
    VerifyEquals(lruCache, testData);
    lruCache.clear();
    EXPECT_TRUE(lruCache.empty());
    EXPECT_EQ(lruCache.capacity(), c_capacity);
}

TEST(LruCache, Constructors)
{
    LRU lruCache{c_capacity};
    VEC testData = Populate(lruCache, lruCache.capacity());
    VerifyEquals(lruCache, testData);

    LRU lruCacheCopy{lruCache};
    VerifyEquals(lruCacheCopy, testData);
    EXPECT_EQ(lruCache.capacity(), lruCacheCopy.capacity());

    LRU lruCacheMove{move(lruCache)};
    VerifyEquals(lruCacheMove, testData);
    EXPECT_TRUE(lruCache.empty());
    EXPECT_EQ(lruCache.capacity(), c_capacity);
}
