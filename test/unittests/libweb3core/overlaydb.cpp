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
/** @file trie.cpp
 * @author Christoph Jentzsch <cj@ethdev.com>
 * @date 2015
 * OverlayDB tests.
 */

#include <libdevcore/DBImpl.h>
#include <libdevcore/OverlayDB.h>
#include <libdevcore/TransientDirectory.h>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace dev;
using namespace dev::test;

BOOST_FIXTURE_TEST_SUITE(OverlayDBTests, TestOutputHelperFixture)

BOOST_AUTO_TEST_CASE(basicUsage)
{
    TransientDirectory td;
    std::unique_ptr<db::DBImpl> db(new db::DBImpl(td.path()));
    BOOST_REQUIRE(db);

    OverlayDB odb(std::move(db));
    BOOST_CHECK(!odb.get().size());

    // commit nothing
    odb.commit();

    string const value = "\x43";
    BOOST_CHECK(!odb.get().size());

    odb.insert(h256(42), &value);
    BOOST_CHECK(odb.get().size());
    BOOST_CHECK(odb.exists(h256(42)));
    BOOST_CHECK_EQUAL(odb.lookup(h256(42)), value);

    odb.commit();
    BOOST_CHECK(!odb.get().size());
    BOOST_CHECK(odb.exists(h256(42)));
    BOOST_CHECK_EQUAL(odb.lookup(h256(42)), value);

    odb.insert(h256(41), &value);
    odb.commit();
    BOOST_CHECK(!odb.get().size());
    BOOST_CHECK(odb.exists(h256(41)));
    BOOST_CHECK_EQUAL(odb.lookup(h256(41)), value);
}

BOOST_AUTO_TEST_CASE(auxMem)
{
    TransientDirectory td;
    std::unique_ptr<db::DBImpl> db(new db::DBImpl(td.path()));
    BOOST_REQUIRE(db);

    OverlayDB odb(std::move(db));

    string const value = "\x43";
    bytes valueAux = fromHex("44");

    odb.insert(h256(42), &value);
    odb.insert(h256(0), &value);
    odb.insert(h256(numeric_limits<u256>::max()), &value);

    odb.insertAux(h256(42), &valueAux);
    odb.insertAux(h256(0), &valueAux);
    odb.insertAux(h256(numeric_limits<u256>::max()), &valueAux);

    odb.commit();

    BOOST_CHECK(!odb.get().size());

    BOOST_CHECK(odb.exists(h256(42)));
    BOOST_CHECK_EQUAL(odb.lookup(h256(42)), value);

    BOOST_CHECK(odb.exists(h256(0)));
    BOOST_CHECK_EQUAL(odb.lookup(h256(0)), value);

    BOOST_CHECK(odb.exists(h256(std::numeric_limits<u256>::max())));
    BOOST_CHECK_EQUAL(odb.lookup(h256(std::numeric_limits<u256>::max())), value);

    BOOST_CHECK(odb.lookupAux(h256(42)) == valueAux);
    BOOST_CHECK(odb.lookupAux(h256(0)) == valueAux);
    BOOST_CHECK(odb.lookupAux(h256(std::numeric_limits<u256>::max())) == valueAux);
}

BOOST_AUTO_TEST_CASE(rollback)
{
    TransientDirectory td;
    std::unique_ptr<db::DBImpl> db(new db::DBImpl(td.path()));
    BOOST_REQUIRE(db);

    OverlayDB odb(std::move(db));
    bytes value = fromHex("42");

    odb.insert(h256(43), &value);
    BOOST_CHECK(odb.get().size());
    odb.rollback();
    BOOST_CHECK(!odb.get().size());
}

BOOST_AUTO_TEST_SUITE_END()
