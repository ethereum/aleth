// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
/** 
 * Trie test functions.
 */

#include <fstream>
#include <random>

#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcore/CommonIO.h>
#include <libethereum/BlockChain.h>
#include <libethashseal/GenesisInfo.h>
#include <test/tools/libtesteth/TestHelper.h>
#include <boost/filesystem/path.hpp>
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

namespace fs = boost::filesystem;
namespace js = json_spirit;

BOOST_FIXTURE_TEST_SUITE(BasicTests, TestOutputHelperFixture)

BOOST_AUTO_TEST_CASE(emptySHA3Types)
{
	h256 emptyListSHA3(fromHex("1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347"));
	BOOST_REQUIRE_EQUAL(emptyListSHA3, EmptyListSHA3);

	h256 emptySHA3(fromHex("c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470"));
	BOOST_REQUIRE_EQUAL(emptySHA3, EmptySHA3);
}

BOOST_AUTO_TEST_CASE(genesis_tests)
{
	fs::path const testPath = test::getTestPath() / fs::path("BasicTests");

	cnote << "Testing Genesis block...";
	js::mValue v;
	string const s = contentsString(testPath / fs::path("genesishashestest.json"));
	BOOST_REQUIRE_MESSAGE(s.length() > 0, "Contents of 'genesishashestest.json' is empty. Have you cloned the 'tests' repo branch develop?");
	js::read_string(s, v);

	js::mObject o = v.get_obj();

	ChainParams p(genesisInfo(eth::Network::MainNetwork), genesisStateRoot(eth::Network::MainNetwork));
	BOOST_CHECK_EQUAL(p.calculateStateRoot(), h256(o["genesis_state_root"].get_str()));
	BOOST_CHECK_EQUAL(toHex(p.genesisBlock()), toHex(fromHex(o["genesis_rlp_hex"].get_str())));
	BOOST_CHECK_EQUAL(BlockHeader(p.genesisBlock()).hash(), h256(o["genesis_hash"].get_str()));
}

BOOST_AUTO_TEST_SUITE_END()
