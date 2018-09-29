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
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <boost/test/unit_test.hpp>


using namespace std;
using namespace dev::db;
using namespace dev::test;

namespace
{
array<pair<string, string>, 3> g_testData = {{{"Foo", "Bar"}, {"Baz", "Qux"}, {"Hello", "world"}}};
}

BOOST_FIXTURE_TEST_SUITE(MemoryDBTests, TestOutputHelperFixture)

BOOST_AUTO_TEST_CASE(defaultEmpty)
{
    unique_ptr<MemoryDB> db(new MemoryDB());
    BOOST_CHECK(!db->size());
}

BOOST_AUTO_TEST_CASE(insertAndKillSingle)
{
    unique_ptr<MemoryDB> db(new MemoryDB());
    string testKey("foo");
    string testVal("bar");

    size_t insertedCount = 0;
    db->insert(Slice(testKey), Slice(testVal));
    BOOST_CHECK_EQUAL(++insertedCount, db->size());
    BOOST_CHECK(db->exists(Slice(testKey)));
    BOOST_CHECK_EQUAL(testVal, db->lookup(Slice(testKey)));

    db->kill(Slice(testKey));
    BOOST_CHECK_EQUAL(--insertedCount, db->size());
    BOOST_CHECK(!db->exists(Slice(testKey)));
    BOOST_CHECK_EQUAL("", db->lookup(Slice(testKey)));
}

BOOST_AUTO_TEST_CASE(InsertAndKillMultiple)
{
    unique_ptr<MemoryDB> db(new MemoryDB());

    // Insert keys/values and verify insertion
    size_t insertedCount = 0;
    for (auto const& data : g_testData)
    {
        db->insert(Slice(data.first), Slice(data.second));
        BOOST_CHECK_EQUAL(++insertedCount, db->size());
        BOOST_CHECK(db->exists(Slice(data.first)));
        BOOST_CHECK_EQUAL(data.second, db->lookup(Slice(data.first)));
    }

    // Kill keys/values and verify deletion
    for (auto const& data : g_testData)
    {
        db->kill(Slice(data.first));
        BOOST_CHECK_EQUAL(--insertedCount, db->size());
        BOOST_CHECK(!db->exists(Slice(data.first)));
        BOOST_CHECK_EQUAL("", db->lookup(Slice(data.first)));
    }
}

BOOST_AUTO_TEST_CASE(ForEachComplete)
{
    unique_ptr<MemoryDB> db(new MemoryDB());
    array<string, 3> testData = {{"foo", "bar", "baz"}};

    // Insert keys and verify insertion
    size_t insertedCount = 0;
    for (auto const& data : testData)
    {
        db->insert(Slice(data), Slice(data));
        BOOST_CHECK_EQUAL(++insertedCount, db->size());
        BOOST_CHECK(db->exists(Slice(data)));
        BOOST_CHECK_EQUAL(data, db->lookup(Slice(data)));
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
    BOOST_CHECK_EQUAL(testData.size(), matchedCount);
}

BOOST_AUTO_TEST_CASE(ForEachTerminateEarly)
{
    unique_ptr<MemoryDB> db(new MemoryDB());

    // Insert keys and verify insertion
    size_t insertedCount = 0;
    for (auto const& data : g_testData)
    {
        db->insert(Slice(data.first), Slice(data.second));
        BOOST_CHECK_EQUAL(++insertedCount, db->size());
        BOOST_CHECK(db->exists(Slice(data.first)));
        BOOST_CHECK_EQUAL(data.second, db->lookup(Slice(data.first)));
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
    BOOST_CHECK(!matchedCount);
}

// Write batch tests

BOOST_AUTO_TEST_CASE(defaultEmptyWriteBatch)
{
    unique_ptr<MemoryDB> db(new MemoryDB());
    unique_ptr<WriteBatchFace> writeBatch = db->createWriteBatch();
    {
        MemoryDBWriteBatch* rawBatch = static_cast<MemoryDBWriteBatch*>(writeBatch.get());
        BOOST_CHECK(!rawBatch->size());
    }
}

BOOST_AUTO_TEST_CASE(insertAndKillSingleBatch)
{
    unique_ptr<MemoryDB> db(new MemoryDB());
    unique_ptr<WriteBatchFace> writeBatch = db->createWriteBatch();
    {
        MemoryDBWriteBatch* rawBatch = static_cast<MemoryDBWriteBatch*>(writeBatch.get());

        string testKey("foo");
        string testVal("bar");

        size_t insertedCount = 0;
        rawBatch->insert(Slice(testKey), Slice(testVal));
        BOOST_CHECK_EQUAL(++insertedCount, rawBatch->size());
        rawBatch->kill(Slice(testKey));
        BOOST_CHECK_EQUAL(--insertedCount, rawBatch->size());
    }
}

BOOST_AUTO_TEST_CASE(insertAndKillMultipleValuesBatch)
{
    unique_ptr<MemoryDB> db(new MemoryDB());
    unique_ptr<WriteBatchFace> writeBatch = db->createWriteBatch();
    {
        MemoryDBWriteBatch* rawBatch = static_cast<MemoryDBWriteBatch*>(writeBatch.get());

        // Insert keys/values into the batch
        size_t insertedCount = 0;
        for (auto const& data : g_testData)
        {
            rawBatch->insert(Slice(data.first), Slice(data.second));
            BOOST_CHECK_EQUAL(++insertedCount, rawBatch->size());
        }

        // Kill keys/values from batch
        for (auto const& data : g_testData)
        {
            rawBatch->kill(Slice(data.first));
            BOOST_CHECK_EQUAL(--insertedCount, rawBatch->size());
        }
    }
}

// Write batch commit tests

BOOST_AUTO_TEST_CASE(commitEmptyBatch)
{
    unique_ptr<MemoryDB> db(new MemoryDB());
    BOOST_CHECK(!db->size());
    unique_ptr<WriteBatchFace> writeBatch = db->createWriteBatch();
    {
        MemoryDBWriteBatch* rawBatch = static_cast<MemoryDBWriteBatch*>(writeBatch.get());
        BOOST_CHECK(!rawBatch->size());

        db->commit(move(writeBatch));
        BOOST_CHECK(!db->size());
    }
}

BOOST_AUTO_TEST_CASE(commitMultipleValuesBatch)
{
    unique_ptr<MemoryDB> db(new MemoryDB());
    unique_ptr<WriteBatchFace> writeBatch = db->createWriteBatch();

    // Insert keys/values
    size_t insertedCount = 0;
    for (auto const& data : g_testData)
    {
        writeBatch->insert(Slice(data.first), Slice(data.second));
        insertedCount++;
    }

    BOOST_CHECK(!db->size());
    db->commit(move(writeBatch));
    BOOST_CHECK_EQUAL(insertedCount, db->size());
    for (auto const& data : g_testData)
    {
        BOOST_CHECK_EQUAL(data.second, db->lookup(Slice(data.first)));
    }
}

BOOST_AUTO_TEST_CASE(commitMultipleBatches)
{
    unique_ptr<MemoryDB> db(new MemoryDB());
    unique_ptr<WriteBatchFace> writeBatches[g_testData.size()];

    size_t insertedCount = 0;
    for (size_t i = 0; i < g_testData.size(); i++)
    {
        writeBatches[i] = db->createWriteBatch();
        writeBatches[i]->insert(Slice(g_testData[i].first), Slice(g_testData[i].second));
        db->commit(move(writeBatches[i]));
        BOOST_CHECK_EQUAL(++insertedCount, db->size());
        BOOST_CHECK_EQUAL(g_testData[i].second, db->lookup(Slice(g_testData[i].first)));
    }
}

BOOST_AUTO_TEST_SUITE_END()