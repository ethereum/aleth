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
/** @file MemoryDB.cpp
 * @author Christoph Jentzsch <cj@ethdev.com>
 * @date 2015
 * memDB test functions.
 */

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <libdevcore/MemoryDB.h>
#include <test/tools/libtesteth/TestOutputHelper.h>

using namespace std;
using namespace dev;
using namespace dev::test;

namespace dev {  namespace test {


} }// Namespace Close

BOOST_FIXTURE_TEST_SUITE(memDB, TestOutputHelper)

BOOST_AUTO_TEST_CASE(kill)
{
	MemoryDB myDB;
	ETH_CHECK(myDB.get().empty());
	bytes value = fromHex("43");
	myDB.insert(h256(42), &value);
	ETH_CHECK(myDB.exists(h256(42)));
	ETH_CHECK_EQUAL(myDB.get().size(), 1);
	ETH_CHECK(!myDB.kill(h256(43)));
	ETH_CHECK(myDB.kill(h256(42)));
}

BOOST_AUTO_TEST_CASE(purgeMainMem)
{
	MemoryDB myDB;
	ETH_CHECK(myDB.get().empty());
	bytes value = fromHex("43");

	myDB.insert(h256(42), &value);
	MemoryDB copy;
	copy = myDB;
	ETH_CHECK(myDB.exists(h256(42)));
	ETH_CHECK_EQUAL(myDB.lookup(h256(42)), toString(value[0]));
	ETH_CHECK_EQUAL(myDB.get().size(), 1);
	ETH_CHECK(myDB.kill(h256(42)));

	ETH_CHECK(myDB.get() == copy.get());
	ETH_CHECK(myDB.exists(h256(42)));
	ETH_CHECK_EQUAL(myDB.lookup(h256(42)), toString(value[0]));

	ETH_CHECK_EQUAL(myDB.get().size(), 1);
	myDB.purge();
	ETH_CHECK_EQUAL(myDB.get().size(), 0);
	myDB.insert(h256(43), &value);
	ETH_CHECK_EQUAL(myDB.get().size(), 1);
	myDB.clear();
	ETH_CHECK_EQUAL(myDB.get().size(), 0);
}

BOOST_AUTO_TEST_CASE(purgeMainMem_Refs)
{
	MemoryDB myDB;
	{
		EnforceRefs enforceRefs(myDB, true);

		ETH_CHECK(myDB.get().empty());
		bytes value = fromHex("43");

		myDB.insert(h256(42), &value);
		MemoryDB copy;
		copy = myDB;
		ETH_CHECK(myDB.exists(h256(42)));
		ETH_CHECK_EQUAL(myDB.lookup(h256(42)), toString(value[0]));
		ETH_CHECK_EQUAL(myDB.get().size(), 1);
		ETH_CHECK(myDB.kill(h256(42)));

		ETH_CHECK(myDB.get() != copy.get());
		ETH_CHECK(!myDB.exists(h256(42)));
		ETH_CHECK_EQUAL(myDB.lookup(h256(42)), std::string());

		ETH_CHECK_EQUAL(myDB.get().size(), 0);
		myDB.purge();
		ETH_CHECK_EQUAL(myDB.get().size(), 0);
		myDB.insert(h256(43), &value);
		ETH_CHECK_EQUAL(myDB.get().size(), 1);
		myDB.clear();
		ETH_CHECK_EQUAL(myDB.get().size(), 0);
		// call EnforceRefs destructor
	}

	// do same tests again without EnforceRefs
	ETH_CHECK(myDB.get().empty());
	bytes value = fromHex("43");

	myDB.insert(h256(42), &value);
	MemoryDB copy;
	copy = myDB;
	ETH_CHECK(myDB.exists(h256(42)));
	ETH_CHECK_EQUAL(myDB.lookup(h256(42)), toString(value[0]));
	ETH_CHECK_EQUAL(myDB.get().size(), 1);
	ETH_CHECK(myDB.kill(h256(42)));

	ETH_CHECK(myDB.get() == copy.get());
	ETH_CHECK(myDB.exists(h256(42)));
	ETH_CHECK_EQUAL(myDB.lookup(h256(42)), toString(value[0]));

	ETH_CHECK_EQUAL(myDB.get().size(), 1);
	myDB.purge();
	ETH_CHECK_EQUAL(myDB.get().size(), 0);
	myDB.insert(h256(43), &value);
	ETH_CHECK_EQUAL(myDB.get().size(), 1);
	myDB.clear();
	ETH_CHECK_EQUAL(myDB.get().size(), 0);
}

BOOST_AUTO_TEST_CASE(purgeAuxMem)
{
	class AuxMemDB : public MemoryDB
	{
	public:
		std::unordered_map<h256, std::pair<bytes, bool>> getAux() { return m_aux;}
	};

	AuxMemDB myDB;
	ETH_CHECK(myDB.get().empty());
	bytes value = fromHex("43");

	myDB.insertAux(h256(42), &value);
	ETH_CHECK(myDB.lookupAux(h256(42)) == value);
	ETH_CHECK_EQUAL(myDB.get().size(), 0);
	myDB.removeAux(h256(42));
	ETH_CHECK(myDB.lookupAux(h256(42)) == value);
	ETH_CHECK_EQUAL(myDB.getAux().size(), 1);
	myDB.purge();
	ETH_CHECK(myDB.lookupAux(h256(42)) == bytes());
	ETH_CHECK_EQUAL(myDB.getAux().size(), 0);
	myDB.insertAux(h256(43), &value);
	ETH_CHECK_EQUAL(myDB.getAux().size(), 1);
	myDB.clear();
	ETH_CHECK_EQUAL(myDB.getAux().size(), 0);
}

BOOST_AUTO_TEST_CASE(copy)
{
	MemoryDB myDB;
	ETH_CHECK(myDB.get().empty());
	bytes value = fromHex("43");
	myDB.insert(h256(42), &value);
	ETH_CHECK(myDB.exists(h256(42)));
	ETH_CHECK_EQUAL(myDB.get().size(), 1);

	MemoryDB copyToDB;
	copyToDB = myDB;
	ETH_CHECK(copyToDB.exists(h256(42)));
	ETH_CHECK_EQUAL(copyToDB.get().size(), 1);
	ETH_CHECK(myDB.keys() == copyToDB.keys());
	ETH_CHECK(myDB.get() == copyToDB.get());
	myDB.insert(h256(43), &value);
	ETH_CHECK(myDB.keys() != copyToDB.keys());
}

BOOST_AUTO_TEST_CASE(lookUp)
{
	MemoryDB myDB;
	ETH_CHECK(myDB.get().empty());
	bytes value = fromHex("43");

	myDB.insert(h256(42), &value);
	ETH_CHECK(myDB.exists(h256(42)));
	ETH_CHECK_EQUAL(myDB.lookup(h256(42)), toString(value[0]));
	ETH_CHECK_EQUAL(myDB.get().size(), 1);

	myDB.insert(h256(0), &value);
	ETH_CHECK(myDB.exists(h256(0)));
	ETH_CHECK_EQUAL(myDB.lookup(h256(0)), toString(value[0]));

	myDB.insert(h256(std::numeric_limits<u256>::max()), &value);
	ETH_CHECK(myDB.exists(h256(std::numeric_limits<u256>::max())));
	ETH_CHECK_EQUAL(myDB.lookup(h256(std::numeric_limits<u256>::max())), toString(value[0]));

	ETH_CHECK_EQUAL(myDB.get().size(), 3);
}

BOOST_AUTO_TEST_CASE(stream)
{
	MemoryDB myDB;
	ETH_CHECK(myDB.get().empty());
	bytes value = fromHex("43");
	myDB.insert(h256(42), &value);
	myDB.insert(h256(43), &value);
	std::ostringstream stream;
	stream << myDB;
	ETH_CHECK_EQUAL(stream.str(), "000000000000000000000000000000000000000000000000000000000000002a: 0x43 43\n000000000000000000000000000000000000000000000000000000000000002b: 0x43 43\n");
}

BOOST_AUTO_TEST_SUITE_END()
