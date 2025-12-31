// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include <fstream>
#include <boost/test/unit_test.hpp>
#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcore/Log.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Base64.h>
#include <libdevcore/TrieCommon.h>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <boost/filesystem/path.hpp>

using namespace std;
using namespace dev;
using namespace dev::test;
namespace fs = boost::filesystem;
namespace js = json_spirit;

BOOST_AUTO_TEST_SUITE(Crypto)

BOOST_FIXTURE_TEST_SUITE(Basic, TestOutputHelperFixture)

BOOST_AUTO_TEST_CASE(hexPrefix_test)
{

	fs::path testPath = test::getTestPath();
	testPath /= fs::path("BasicTests");

	cnote << "Testing Hex-Prefix-Encode...";
	js::mValue v;
	string const s = contentsString(testPath / fs::path("hexencodetest.json"));
	BOOST_REQUIRE_MESSAGE(s.length() > 0, "Content from 'hexencodetest.json' is empty. Have you cloned the 'tests' repo branch develop?");
	js::read_string(s, v);
	for (auto& i: v.get_obj())
	{
		js::mObject& o = i.second.get_obj();
		cnote << i.first;
		bytes v;
		for (auto& i: o["seq"].get_array())
			v.push_back((byte)i.get_int());
		auto e = hexPrefixEncode(v, o["term"].get_bool());
		BOOST_REQUIRE( ! o["out"].is_null() );
		BOOST_CHECK( o["out"].get_str() == toHex(e) );
	}
}

BOOST_AUTO_TEST_CASE(base64)
{
	static char const* const s_tests[][2] =
	{
		{"", ""},
		{"f", "Zg=="},
		{"fo", "Zm8="},
		{"foo", "Zm9v"},
		{"foob", "Zm9vYg=="},
		{"fooba", "Zm9vYmE="},
		{"foobar", "Zm9vYmFy"},
		{
			"So?<p>"
			"This 4, 5, 6, 7, 8, 9, z, {, |, } tests Base64 encoder. "
			"Show me: @, A, B, C, D, E, F, G, H, I, J, K, L, M, "
			"N, O, P, Q, R, S, T, U, V, W, X, Y, Z, [, \\, ], ^, _, `, "
			"a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s.",
			"U28/PHA+VGhpcyA0LCA1LCA2LCA3LCA4LCA5LCB6LCB7LCB8LCB9IHRlc3RzIEJhc2U2NCBlbmNv"
			"ZGVyLiBTaG93IG1lOiBALCBBLCBCLCBDLCBELCBFLCBGLCBHLCBILCBJLCBKLCBLLCBMLCBNLCBO"
			"LCBPLCBQLCBRLCBSLCBTLCBULCBVLCBWLCBXLCBYLCBZLCBaLCBbLCBcLCBdLCBeLCBfLCBgLCBh"
			"LCBiLCBjLCBkLCBlLCBmLCBnLCBoLCBpLCBqLCBrLCBsLCBtLCBuLCBvLCBwLCBxLCByLCBzLg=="
		}
	};
	static const auto c_numTests = sizeof(s_tests) / sizeof(s_tests[0]);

	for (size_t i = 0; i < c_numTests; ++i)
	{
		auto expectedDecoded = std::string{s_tests[i][0]};
		auto expectedEncoded = std::string{s_tests[i][1]};

		auto encoded = toBase64(expectedDecoded);
		BOOST_CHECK_EQUAL(expectedEncoded, encoded);
		auto decodedBytes = fromBase64(expectedEncoded);
		auto decoded = bytesConstRef{decodedBytes.data(), decodedBytes.size()}.toString();
		BOOST_CHECK_EQUAL(decoded, expectedDecoded);
	}
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
