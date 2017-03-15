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
#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcore/CommonIO.h>
#include <libethashseal/Ethash.h>
#include <libethashseal/EthashAux.h>
#include <boost/test/unit_test.hpp>
#include <test/libtesteth/TestHelper.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

namespace js = json_spirit;

using dev::operator <<;

BOOST_FIXTURE_TEST_SUITE(DashimotoTests, TestOutputHelper)

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
		BlockHeader header(fromHex(o["header"].get_str()), HeaderData);
		h256 headerHash(o["header_hash"].get_str());
		Nonce nonce(o["nonce"].get_str());
		BOOST_REQUIRE_EQUAL(headerHash, header.hash(WithoutSeal));
		BOOST_REQUIRE_EQUAL(nonce, Ethash::nonce(header));

		unsigned cacheSize(o["cache_size"].get_int());
		h256 cacheHash(o["cache_hash"].get_str());
		BOOST_REQUIRE_EQUAL(EthashAux::get()->light(Ethash::seedHash(header))->size, cacheSize);
		BOOST_REQUIRE_EQUAL(sha3(EthashAux::get()->light(Ethash::seedHash(header))->data()), cacheHash);

#if TEST_FULL
		unsigned fullSize(o["full_size"].get_int());
		h256 fullHash(o["full_hash"].get_str());
		BOOST_REQUIRE_EQUAL(EthashAux::get()->full(Ethash::seedHash(header))->size(), fullSize);
		BOOST_REQUIRE_EQUAL(sha3(EthashAux::get()->full(Ethash::seedHash(header))->data()), fullHash);
#endif

		h256 result(o["result"].get_str());
		EthashProofOfWork::Result r = EthashAux::eval(Ethash::seedHash(header), header.hash(WithoutSeal), Ethash::nonce(header));
		BOOST_REQUIRE_EQUAL(r.value, result);
		BOOST_REQUIRE_EQUAL(r.mixHash, Ethash::mixHash(header));
	}
}

BOOST_AUTO_TEST_SUITE_END()
