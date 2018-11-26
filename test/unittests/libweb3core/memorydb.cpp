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

#include <libdevcore/MemoryDB.h>

#include <gtest/gtest.h>

using namespace std;
using namespace dev::db;

namespace
{
array<pair<string, string>, 3> g_testData = {{{"Foo", "Bar"}, {"Baz", "Qux"}, {"Hello", "world"}}};
}

TEST(MemoryDB, defaultEmpty)
{
    unique_ptr<MemoryDB> db(new MemoryDB());
    ASSERT_TRUE(db);
    EXPECT_TRUE(!db->size());
}

TEST(MemoryDB, insertAndKillSingle)
{
    unique_ptr<MemoryDB> db(new MemoryDB());
    ASSERT_TRUE(db);
    string testKey("foo");
    string testVal("bar");

    size_t insertedCount = 0;
    db->insert(Slice(testKey), Slice(testVal));
    EXPECT_EQ(++insertedCount, db->size());
    EXPECT_TRUE(db->exists(Slice(testKey)));
    EXPECT_EQ(testVal, db->lookup(Slice(testKey)));

    db->kill(Slice(testKey));
    EXPECT_EQ(--insertedCount, db->size());
    EXPECT_TRUE(!db->exists(Slice(testKey)));
    EXPECT_EQ("", db->lookup(Slice(testKey)));
}

TEST(MemoryDB, InsertAndKillMultiple)
{
    unique_ptr<MemoryDB> db(new MemoryDB());
    ASSERT_TRUE(db);

    // Insert keys/values and verify insertion
    size_t insertedCount = 0;
    for (auto const& data : g_testData)
    {
        db->insert(Slice(data.first), Slice(data.second));
        EXPECT_EQ(++insertedCount, db->size());
        EXPECT_TRUE(db->exists(Slice(data.first)));
        EXPECT_EQ(data.second, db->lookup(Slice(data.first)));
    }

    // Kill keys/values and verify deletion
    for (auto const& data : g_testData)
    {
        db->kill(Slice(data.first));
        EXPECT_EQ(--insertedCount, db->size());
        EXPECT_TRUE(!db->exists(Slice(data.first)));
        EXPECT_EQ("", db->lookup(Slice(data.first)));
    }
}

TEST(MemoryDB, ForEachComplete)
{
    unique_ptr<MemoryDB> db(new MemoryDB());
    ASSERT_TRUE(db);
    array<string, 3> testData = {{"foo", "bar", "baz"}};

    // Insert keys and verify insertion
    size_t insertedCount = 0;
    for (auto const& data : testData)
    {
        db->insert(Slice(data), Slice(data));
        EXPECT_EQ(++insertedCount, db->size());
        EXPECT_TRUE(db->exists(Slice(data)));
        EXPECT_EQ(data, db->lookup(Slice(data)));
    }

    size_t matchedCount = 0;
    db->forEach([&matchedCount](Slice const& key, Slice const& value) {
        if (key.toString() == value.toString())
        {
            matchedCount++;
            return true;
        }
        return false;
    });
    EXPECT_EQ(testData.size(), matchedCount);
}

TEST(MemoryDB, ForEachTerminateEarly)
{
    unique_ptr<MemoryDB> db(new MemoryDB());
    ASSERT_TRUE(db);

    // Insert keys and verify insertion
    size_t insertedCount = 0;
    for (auto const& data : g_testData)
    {
        db->insert(Slice(data.first), Slice(data.second));
        EXPECT_EQ(++insertedCount, db->size());
        EXPECT_TRUE(db->exists(Slice(data.first)));
        EXPECT_EQ(data.second, db->lookup(Slice(data.first)));
    }

    size_t matchedCount = 0;
    db->forEach([&matchedCount](Slice const& key, Slice const& value) {
        if (key.toString() == value.toString())
        {
            matchedCount++;
            return true;
        }
        return false;
    });
    EXPECT_TRUE(!matchedCount);
}

// Write batch tests

TEST(MemoryDB, defaultEmptyWriteBatch)
{
    unique_ptr<MemoryDB> db(new MemoryDB());
    ASSERT_TRUE(db);
    unique_ptr<WriteBatchFace> writeBatch = db->createWriteBatch();
    ASSERT_TRUE(writeBatch);
    {
        MemoryDBWriteBatch* rawBatch = static_cast<MemoryDBWriteBatch*>(writeBatch.get());
        EXPECT_TRUE(!rawBatch->size());
    }
}

TEST(MemoryDB, insertAndKillSingleBatch)
{
    unique_ptr<MemoryDB> db(new MemoryDB());
    ASSERT_TRUE(db);
    unique_ptr<WriteBatchFace> writeBatch = db->createWriteBatch();
    ASSERT_TRUE(writeBatch);
    {
        MemoryDBWriteBatch* rawBatch = static_cast<MemoryDBWriteBatch*>(writeBatch.get());

        string testKey("foo");
        string testVal("bar");

        size_t insertedCount = 0;
        rawBatch->insert(Slice(testKey), Slice(testVal));
        EXPECT_EQ(++insertedCount, rawBatch->size());
        rawBatch->kill(Slice(testKey));
        EXPECT_EQ(--insertedCount, rawBatch->size());
    }
}

TEST(MemoryDB, insertAndKillMultipleValuesBatch)
{
    unique_ptr<MemoryDB> db(new MemoryDB());
    ASSERT_TRUE(db);
    unique_ptr<WriteBatchFace> writeBatch = db->createWriteBatch();
    ASSERT_TRUE(writeBatch);
    {
        MemoryDBWriteBatch* rawBatch = static_cast<MemoryDBWriteBatch*>(writeBatch.get());

        // Insert keys/values into the batch
        size_t insertedCount = 0;
        for (auto const& data : g_testData)
        {
            rawBatch->insert(Slice(data.first), Slice(data.second));
            EXPECT_EQ(++insertedCount, rawBatch->size());
        }

        // Kill keys/values from batch
        for (auto const& data : g_testData)
        {
            rawBatch->kill(Slice(data.first));
            EXPECT_EQ(--insertedCount, rawBatch->size());
        }
    }
}

// Write batch commit tests

TEST(MemoryDB, commitEmptyBatch)
{
    unique_ptr<MemoryDB> db(new MemoryDB());
    ASSERT_TRUE(db);
    EXPECT_TRUE(!db->size());
    unique_ptr<WriteBatchFace> writeBatch = db->createWriteBatch();
    ASSERT_TRUE(writeBatch);
    {
        MemoryDBWriteBatch* rawBatch = static_cast<MemoryDBWriteBatch*>(writeBatch.get());
        EXPECT_TRUE(!rawBatch->size());

        db->commit(move(writeBatch));
        EXPECT_TRUE(!db->size());
    }
}

TEST(MemoryDB, commitMultipleValuesBatch)
{
    unique_ptr<MemoryDB> db(new MemoryDB());
    ASSERT_TRUE(db);
    unique_ptr<WriteBatchFace> writeBatch = db->createWriteBatch();
    ASSERT_TRUE(writeBatch);

    // Insert keys/values
    size_t insertedCount = 0;
    for (auto const& data : g_testData)
    {
        writeBatch->insert(Slice(data.first), Slice(data.second));
        insertedCount++;
    }

    EXPECT_TRUE(!db->size());
    db->commit(move(writeBatch));
    EXPECT_EQ(insertedCount, db->size());
    for (auto const& data : g_testData)
    {
        EXPECT_EQ(data.second, db->lookup(Slice(data.first)));
    }
}

TEST(MemoryDB, commitMultipleBatches)
{
    unique_ptr<MemoryDB> db(new MemoryDB());
    ASSERT_TRUE(db);
    unique_ptr<WriteBatchFace> writeBatches[g_testData.size()];

    size_t insertedCount = 0;
    for (size_t i = 0; i < g_testData.size(); i++)
    {
        writeBatches[i] = db->createWriteBatch();
        ASSERT_TRUE(writeBatches[i]);
        writeBatches[i]->insert(Slice(g_testData[i].first), Slice(g_testData[i].second));
        db->commit(move(writeBatches[i]));
        EXPECT_EQ(++insertedCount, db->size());
        EXPECT_EQ(g_testData[i].second, db->lookup(Slice(g_testData[i].first)));
    }
}
