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

#include <libdevcore/StateCacheDB.h>

#include <gtest/gtest.h>

using namespace std;
using namespace dev;

TEST(StateCacheDB, kill)
{
    StateCacheDB myDB;
    EXPECT_TRUE(myDB.get().empty());
    bytes value = fromHex("43");
    myDB.insert(h256(42), &value);
    EXPECT_TRUE(myDB.exists(h256(42)));
    EXPECT_EQ(myDB.get().size(), 1);
    EXPECT_TRUE(!myDB.kill(h256(43)));
    EXPECT_TRUE(myDB.kill(h256(42)));
}

TEST(StateCacheDB, purgeMainMem)
{
    StateCacheDB myDB;
    EXPECT_TRUE(myDB.get().empty());
    string const value = "\x43";

    myDB.insert(h256(42), &value);
    StateCacheDB copy;
    copy = myDB;
    EXPECT_TRUE(myDB.exists(h256(42)));
    EXPECT_EQ(myDB.lookup(h256(42)), value);
    EXPECT_EQ(myDB.get().size(), 1);
    EXPECT_TRUE(myDB.kill(h256(42)));

    EXPECT_TRUE(myDB.get() == copy.get());
    EXPECT_TRUE(myDB.exists(h256(42)));
    EXPECT_EQ(myDB.lookup(h256(42)), value);

    EXPECT_EQ(myDB.get().size(), 1);
    myDB.purge();
    EXPECT_EQ(myDB.get().size(), 0);
    myDB.insert(h256(43), &value);
    EXPECT_EQ(myDB.get().size(), 1);
    myDB.clear();
    EXPECT_EQ(myDB.get().size(), 0);
}

TEST(StateCacheDB, purgeMainMem_Refs)
{
    StateCacheDB myDB;
    {
        EnforceRefs enforceRefs(myDB, true);

        EXPECT_TRUE(myDB.get().empty());
        string const value = "\x43";

        myDB.insert(h256(42), &value);
        StateCacheDB copy;
        copy = myDB;
        EXPECT_TRUE(myDB.exists(h256(42)));
        EXPECT_EQ(myDB.lookup(h256(42)), value);
        EXPECT_EQ(myDB.get().size(), 1);
        EXPECT_TRUE(myDB.kill(h256(42)));

        EXPECT_TRUE(myDB.get() != copy.get());
        EXPECT_TRUE(!myDB.exists(h256(42)));
        EXPECT_EQ(myDB.lookup(h256(42)), std::string());

        EXPECT_EQ(myDB.get().size(), 0);
        myDB.purge();
        EXPECT_EQ(myDB.get().size(), 0);
        myDB.insert(h256(43), &value);
        EXPECT_EQ(myDB.get().size(), 1);
        myDB.clear();
        EXPECT_EQ(myDB.get().size(), 0);
        // call EnforceRefs destructor
    }

    // do same tests again without EnforceRefs
    EXPECT_TRUE(myDB.get().empty());
    string const value = "\x43";

    myDB.insert(h256(42), &value);
    StateCacheDB copy;
    copy = myDB;
    EXPECT_TRUE(myDB.exists(h256(42)));
    EXPECT_EQ(myDB.lookup(h256(42)), value);
    EXPECT_EQ(myDB.get().size(), 1);
    EXPECT_TRUE(myDB.kill(h256(42)));

    EXPECT_TRUE(myDB.get() == copy.get());
    EXPECT_TRUE(myDB.exists(h256(42)));
    EXPECT_EQ(myDB.lookup(h256(42)), value);

    EXPECT_EQ(myDB.get().size(), 1);
    myDB.purge();
    EXPECT_EQ(myDB.get().size(), 0);
    myDB.insert(h256(43), &value);
    EXPECT_EQ(myDB.get().size(), 1);
    myDB.clear();
    EXPECT_EQ(myDB.get().size(), 0);
}

TEST(StateCacheDB, purgeAuxMem)
{
    class AuxStateCacheDB : public StateCacheDB
    {
    public:
        std::unordered_map<h256, std::pair<bytes, bool>> getAux() { return m_aux; }
    };

    AuxStateCacheDB myDB;
    EXPECT_TRUE(myDB.get().empty());
    bytes value = fromHex("43");

    myDB.insertAux(h256(42), &value);
    EXPECT_TRUE(myDB.lookupAux(h256(42)) == value);
    EXPECT_EQ(myDB.get().size(), 0);
    myDB.removeAux(h256(42));
    EXPECT_TRUE(myDB.lookupAux(h256(42)) == value);
    EXPECT_EQ(myDB.getAux().size(), 1);
    myDB.purge();
    EXPECT_TRUE(myDB.lookupAux(h256(42)) == bytes());
    EXPECT_EQ(myDB.getAux().size(), 0);
    myDB.insertAux(h256(43), &value);
    EXPECT_EQ(myDB.getAux().size(), 1);
    myDB.clear();
    EXPECT_EQ(myDB.getAux().size(), 0);
}

TEST(StateCacheDB, copy)
{
    StateCacheDB myDB;
    EXPECT_TRUE(myDB.get().empty());
    bytes value = fromHex("43");
    myDB.insert(h256(42), &value);
    EXPECT_TRUE(myDB.exists(h256(42)));
    EXPECT_EQ(myDB.get().size(), 1);

    StateCacheDB copyToDB;
    copyToDB = myDB;
    EXPECT_TRUE(copyToDB.exists(h256(42)));
    EXPECT_EQ(copyToDB.get().size(), 1);
    EXPECT_TRUE(myDB.keys() == copyToDB.keys());
    EXPECT_TRUE(myDB.get() == copyToDB.get());
    myDB.insert(h256(43), &value);
    EXPECT_TRUE(myDB.keys() != copyToDB.keys());
}

TEST(StateCacheDB, lookUp)
{
    StateCacheDB myDB;
    EXPECT_TRUE(myDB.get().empty());
    string const value = "\x43";

    myDB.insert(h256(42), &value);
    EXPECT_TRUE(myDB.exists(h256(42)));
    EXPECT_EQ(myDB.lookup(h256(42)), value);
    EXPECT_EQ(myDB.get().size(), 1);

    myDB.insert(h256(0), &value);
    EXPECT_TRUE(myDB.exists(h256(0)));
    EXPECT_EQ(myDB.lookup(h256(0)), value);

    myDB.insert(h256(std::numeric_limits<u256>::max()), &value);
    EXPECT_TRUE(myDB.exists(h256(std::numeric_limits<u256>::max())));
    EXPECT_EQ(myDB.lookup(h256(std::numeric_limits<u256>::max())), value);

    EXPECT_EQ(myDB.get().size(), 3);
}

TEST(StateCacheDB, stream)
{
    StateCacheDB myDB;
    EXPECT_TRUE(myDB.get().empty());
    bytes value = fromHex("43");
    myDB.insert(h256(42), &value);
    myDB.insert(h256(43), &value);
    std::ostringstream stream;
    stream << myDB;
    EXPECT_EQ(stream.str(),
        "000000000000000000000000000000000000000000000000000000000000002a: 0x43 "
        "43\n000000000000000000000000000000000000000000000000000000000000002b: 0x43 43\n");
}
