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
/** @file StateCacheDB.cpp
 * @author Christoph Jentzsch <cj@ethdev.com>
 * @date 2015
 * stateCacheDB test functions.
 */

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <libdevcore/StateCacheDB.h>
#include <test/tools/libtesteth/TestOutputHelper.h>

using namespace std;
using namespace dev;
using namespace dev::test;

namespace dev {  namespace test {


} }// Namespace Close

BOOST_FIXTURE_TEST_SUITE(stateCacheDB, TestOutputHelperFixture)

BOOST_AUTO_TEST_CASE(kill)
{
    StateCacheDB myDB;
    BOOST_CHECK(myDB.get().empty());
    bytes value = fromHex("43");
    myDB.insert(h256(42), &value);
    BOOST_CHECK(myDB.exists(h256(42)));
    BOOST_CHECK_EQUAL(myDB.get().size(), 1);
    BOOST_CHECK(!myDB.kill(h256(43)));
    BOOST_CHECK(myDB.kill(h256(42)));
}

BOOST_AUTO_TEST_CASE(purgeMainMem)
{
    StateCacheDB myDB;
    BOOST_CHECK(myDB.get().empty());
    string const value = "\x43";

    myDB.insert(h256(42), &value);
    StateCacheDB copy;
    copy = myDB;
    BOOST_CHECK(myDB.exists(h256(42)));
    BOOST_CHECK_EQUAL(myDB.lookup(h256(42)), value);
    BOOST_CHECK_EQUAL(myDB.get().size(), 1);
    BOOST_CHECK(myDB.kill(h256(42)));

    BOOST_CHECK(myDB.get() == copy.get());
    BOOST_CHECK(myDB.exists(h256(42)));
    BOOST_CHECK_EQUAL(myDB.lookup(h256(42)), value);

    BOOST_CHECK_EQUAL(myDB.get().size(), 1);
    myDB.purge();
    BOOST_CHECK_EQUAL(myDB.get().size(), 0);
    myDB.insert(h256(43), &value);
    BOOST_CHECK_EQUAL(myDB.get().size(), 1);
    myDB.clear();
    BOOST_CHECK_EQUAL(myDB.get().size(), 0);
}

BOOST_AUTO_TEST_CASE(purgeMainMem_Refs)
{
    StateCacheDB myDB;
    {
        EnforceRefs enforceRefs(myDB, true);

        BOOST_CHECK(myDB.get().empty());
        string const value = "\x43";

        myDB.insert(h256(42), &value);
        StateCacheDB copy;
        copy = myDB;
        BOOST_CHECK(myDB.exists(h256(42)));
        BOOST_CHECK_EQUAL(myDB.lookup(h256(42)), value);
        BOOST_CHECK_EQUAL(myDB.get().size(), 1);
        BOOST_CHECK(myDB.kill(h256(42)));

        BOOST_CHECK(myDB.get() != copy.get());
        BOOST_CHECK(!myDB.exists(h256(42)));
        BOOST_CHECK_EQUAL(myDB.lookup(h256(42)), std::string());

        BOOST_CHECK_EQUAL(myDB.get().size(), 0);
        myDB.purge();
        BOOST_CHECK_EQUAL(myDB.get().size(), 0);
        myDB.insert(h256(43), &value);
        BOOST_CHECK_EQUAL(myDB.get().size(), 1);
        myDB.clear();
        BOOST_CHECK_EQUAL(myDB.get().size(), 0);
        // call EnforceRefs destructor
    }

    // do same tests again without EnforceRefs
    BOOST_CHECK(myDB.get().empty());
    string const value = "\x43";

    myDB.insert(h256(42), &value);
    StateCacheDB copy;
    copy = myDB;
    BOOST_CHECK(myDB.exists(h256(42)));
    BOOST_CHECK_EQUAL(myDB.lookup(h256(42)), value);
    BOOST_CHECK_EQUAL(myDB.get().size(), 1);
    BOOST_CHECK(myDB.kill(h256(42)));

    BOOST_CHECK(myDB.get() == copy.get());
    BOOST_CHECK(myDB.exists(h256(42)));
    BOOST_CHECK_EQUAL(myDB.lookup(h256(42)), value);

    BOOST_CHECK_EQUAL(myDB.get().size(), 1);
    myDB.purge();
    BOOST_CHECK_EQUAL(myDB.get().size(), 0);
    myDB.insert(h256(43), &value);
    BOOST_CHECK_EQUAL(myDB.get().size(), 1);
    myDB.clear();
    BOOST_CHECK_EQUAL(myDB.get().size(), 0);
}

BOOST_AUTO_TEST_CASE(purgeAuxMem)
{
    class AuxStateCacheDB : public StateCacheDB
    {
    public:
        std::unordered_map<h256, std::pair<bytes, bool>> getAux() { return m_aux;}
    };

    AuxStateCacheDB myDB;
    BOOST_CHECK(myDB.get().empty());
    bytes value = fromHex("43");

    myDB.insertAux(h256(42), &value);
    BOOST_CHECK(myDB.lookupAux(h256(42)) == value);
    BOOST_CHECK_EQUAL(myDB.get().size(), 0);
    myDB.removeAux(h256(42));
    BOOST_CHECK(myDB.lookupAux(h256(42)) == value);
    BOOST_CHECK_EQUAL(myDB.getAux().size(), 1);
    myDB.purge();
    BOOST_CHECK(myDB.lookupAux(h256(42)) == bytes());
    BOOST_CHECK_EQUAL(myDB.getAux().size(), 0);
    myDB.insertAux(h256(43), &value);
    BOOST_CHECK_EQUAL(myDB.getAux().size(), 1);
    myDB.clear();
    BOOST_CHECK_EQUAL(myDB.getAux().size(), 0);
}

BOOST_AUTO_TEST_CASE(copy)
{
    StateCacheDB myDB;
    BOOST_CHECK(myDB.get().empty());
    bytes value = fromHex("43");
    myDB.insert(h256(42), &value);
    BOOST_CHECK(myDB.exists(h256(42)));
    BOOST_CHECK_EQUAL(myDB.get().size(), 1);

    StateCacheDB copyToDB;
    copyToDB = myDB;
    BOOST_CHECK(copyToDB.exists(h256(42)));
    BOOST_CHECK_EQUAL(copyToDB.get().size(), 1);
    BOOST_CHECK(myDB.keys() == copyToDB.keys());
    BOOST_CHECK(myDB.get() == copyToDB.get());
    myDB.insert(h256(43), &value);
    BOOST_CHECK(myDB.keys() != copyToDB.keys());
}

BOOST_AUTO_TEST_CASE(lookUp)
{
    StateCacheDB myDB;
    BOOST_CHECK(myDB.get().empty());
    string const value = "\x43";

    myDB.insert(h256(42), &value);
    BOOST_CHECK(myDB.exists(h256(42)));
    BOOST_CHECK_EQUAL(myDB.lookup(h256(42)), value);
    BOOST_CHECK_EQUAL(myDB.get().size(), 1);

    myDB.insert(h256(0), &value);
    BOOST_CHECK(myDB.exists(h256(0)));
    BOOST_CHECK_EQUAL(myDB.lookup(h256(0)), value);

    myDB.insert(h256(std::numeric_limits<u256>::max()), &value);
    BOOST_CHECK(myDB.exists(h256(std::numeric_limits<u256>::max())));
    BOOST_CHECK_EQUAL(myDB.lookup(h256(std::numeric_limits<u256>::max())), value);

    BOOST_CHECK_EQUAL(myDB.get().size(), 3);
}

BOOST_AUTO_TEST_CASE(stream)
{
    StateCacheDB myDB;
    BOOST_CHECK(myDB.get().empty());
    bytes value = fromHex("43");
    myDB.insert(h256(42), &value);
    myDB.insert(h256(43), &value);
    std::ostringstream stream;
    stream << myDB;
    BOOST_CHECK_EQUAL(stream.str(), "000000000000000000000000000000000000000000000000000000000000002a: 0x43 43\n000000000000000000000000000000000000000000000000000000000000002b: 0x43 43\n");
}

BOOST_AUTO_TEST_SUITE_END()
