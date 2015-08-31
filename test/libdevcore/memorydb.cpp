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
#include <libdevcore/MemoryDB.h>

using namespace std;
using namespace dev;

namespace dev {  namespace test {


} }// Namespace Close

BOOST_AUTO_TEST_SUITE(memDB)

BOOST_AUTO_TEST_CASE(kill)
{
	MemoryDB myDB;
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
	MemoryDB myDB;
	BOOST_CHECK(myDB.get().empty());
	bytes value = fromHex("43");

	myDB.insert(h256(42), &value);
	BOOST_CHECK(myDB.exists(h256(42)));
	BOOST_CHECK_EQUAL(myDB.lookup(h256(42)), toString(value[0]));
	BOOST_CHECK_EQUAL(myDB.get().size(), 1);
	BOOST_CHECK(myDB.kill(h256(42)));
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
	class AuxMemDB : public MemoryDB
	{
	public:
		std::unordered_map<h256, std::pair<bytes, bool>> getAux() { return m_aux;}
	};

	AuxMemDB myDB;
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

BOOST_AUTO_TEST_SUITE_END()
