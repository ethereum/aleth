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
/** @file dagger.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 * Dashimoto test functions.
 */

#include <fstream>
#include "../JsonSpiritHeaders.h"
#include <libdevcore/CommonIO.h>
#include <libethcore/EthashAux.h>
#include <boost/test/unit_test.hpp>
#include <test/TestHelper.h>

using namespace std;
using namespace dev;
using namespace dev::eth;

namespace js = json_spirit;

using dev::operator <<;

BOOST_AUTO_TEST_SUITE(DashimotoTests)

BOOST_AUTO_TEST_CASE(basic_test)
{
	string testPath = test::getTestPath();

	testPath += "/PoWTests";

	cnote << "Testing Proof of Work...";
	js::mValue v;
	string s = contentsString(testPath + "/ethash_tests.json");
	BOOST_REQUIRE_MESSAGE(s.length() > 0, "Contents of 'ethash_tests.json' is empty. Have you cloned the 'tests' repo branch develop?");
	js::read_string(s, v);
	for (auto& i: v.get_obj())
	{
		cnote << i.first;
		js::mObject& o = i.second.get_obj();
		vector<pair<string, string>> ss;
		Ethash::BlockHeader header(fromHex(o["header"].get_str()), CheckNothing, h256{}, HeaderData);
		h256 headerHash(o["header_hash"].get_str());
		Nonce nonce(o["nonce"].get_str());
		BOOST_REQUIRE_EQUAL(headerHash, header.hashWithout());
		BOOST_REQUIRE_EQUAL(nonce, header.nonce());

		unsigned cacheSize(o["cache_size"].get_int());
		h256 cacheHash(o["cache_hash"].get_str());
		BOOST_REQUIRE_EQUAL(EthashAux::get()->light(header.seedHash())->size, cacheSize);
		BOOST_REQUIRE_EQUAL(sha3(EthashAux::get()->light(header.seedHash())->data()), cacheHash);

#if TEST_FULL
		unsigned fullSize(o["full_size"].get_int());
		h256 fullHash(o["full_hash"].get_str());
		BOOST_REQUIRE_EQUAL(EthashAux::get()->full(header.seedHash())->size(), fullSize);
		BOOST_REQUIRE_EQUAL(sha3(EthashAux::get()->full(header.seedHash())->data()), fullHash);
#endif

		h256 result(o["result"].get_str());
		EthashProofOfWork::Result r = EthashAux::eval(header.seedHash(), header.hashWithout(), header.nonce());
		BOOST_REQUIRE_EQUAL(r.value, result);
		BOOST_REQUIRE_EQUAL(r.mixHash, header.mixHash());
	}
}

BOOST_AUTO_TEST_SUITE_END()


