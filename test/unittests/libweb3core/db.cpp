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

#include <libdevcore/DBFactory.h>
#include <libdevcore/db.h>
#include <test/tools/libtesteth/BlockChainHelper.h>
#include <test/tools/libtesteth/TestHelper.h>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <chrono>

using namespace std;
using namespace std::chrono;
using namespace dev::db;
using namespace dev::test;

BOOST_FIXTURE_TEST_SUITE(DBPerfTests, TestOutputHelperFixture)

BOOST_AUTO_TEST_CASE(insertAndReadBlocks)
{
    const size_t iterationCount = 11;
    const size_t blockCount = 1000;

    vector<double> insertResults;
    vector<double> readResults;

    cout << "Generating " << blockCount << " test blocks..." << endl;
    vector<TestBlock> blocks;
    for (size_t j = 0; j < blockCount; j++)
    {
        TestBlock block;
        block.addTransaction(TestTransaction::defaultTransaction(j));
        blocks.push_back(block);
    }
    cout << "Block generation complete!" << endl;

    cout << "Running " << iterationCount << " iterations..." << endl << endl;
    for (size_t i = 0; i < iterationCount; i++)
    {
        cout << "Running iteration " << i + 1 << "/" << iterationCount << endl;

        unique_ptr<DatabaseFace> db = nullptr;
        if (isDiskDatabase())
        {
            TransientDirectory dbDir;
            db = DBFactory::create(dbDir.path());
        }
        else
            db = DBFactory::create();

        // Insertion test
        cout << "Inserting " << blockCount << " blocks..." << endl;
        auto start = steady_clock::now();
        for (auto const& block : blocks)
        {
            unique_ptr<WriteBatchFace> writeBatch = db->createWriteBatch();
            writeBatch->insert(
                toSlice(block.blockHeader().hash()), Slice(bytesConstRef(&block.bytes())));
            db->commit(move(writeBatch));
        }
        auto end = steady_clock::now();
        duration<double, milli> duration = end - start;
        cout << "Insertion time: " << duration.count() << endl;
        insertResults.push_back(duration.count());

        // Read test
        cout << "Reading " << blockCount << " blocks..." << endl;
        start = steady_clock::now();
        for (auto const& block : blocks)
        {
            db->lookup(toSlice(block.blockHeader().hash()));
        }
        end = steady_clock::now();
        duration = end - start;
        cout << "Read time: " << duration.count() << endl << endl;
        readResults.push_back(duration.count());
    }

    sort(insertResults.begin(), insertResults.end());
    sort(readResults.begin(), readResults.end());
    cout << "Insertion median: " << insertResults[insertResults.size() / 2] << " ms" << endl;
    cout << "Read median: " << readResults[readResults.size() / 2] << " ms" << endl;
}

BOOST_AUTO_TEST_SUITE_END()