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
/** @file crypto.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 * Crypto test functions.
 */

#include <boost/test/unit_test.hpp>
#include <libdevcore/Log.h>
#include <libdevcore/SHA3.h>
#include <libdevcrypto/ECDHE.h>
#include <libdevcrypto/CryptoPP.h>
#include <libdevcore/Common.h>
#include <libethereum/Transaction.h>
#include <test/TestHelper.h>

using namespace std;
using namespace dev;
using namespace dev::crypto;
using namespace CryptoPP;
using namespace dev::test;

BOOST_FIXTURE_TEST_SUITE(devcrypto, TestOutputHelper)

BOOST_AUTO_TEST_CASE(eth_keypairs)
{
	KeyPair p(Secret(fromHex("3ecb44df2159c26e0f995712d4f39b6f6e499b40749b1cf1246c37f9516cb6a4")));
	BOOST_REQUIRE(p.pub() == Public(fromHex("97466f2b32bc3bb76d4741ae51cd1d8578b48d3f1e68da206d47321aec267ce78549b514e4453d74ef11b0cd5e4e4c364effddac8b51bcfc8de80682f952896f")));
	BOOST_REQUIRE(p.address() == Address(fromHex("8a40bfaa73256b60764c1bf40675a99083efb075")));
	{
		eth::Transaction t(1000, 0, 0, h160(fromHex("944400f4b88ac9589a0f17ed4671da26bddb668b")), bytes(), 0, p.secret());
		auto rlp = t.rlp(eth::WithoutSignature);
		cnote << RLP(rlp);
		cnote << toHex(rlp);
		cnote << t.sha3(eth::WithoutSignature);
		rlp = t.rlp(eth::WithSignature);
		cnote << RLP(rlp);
		cnote << toHex(rlp);
		cnote << t.sha3(eth::WithSignature);
		BOOST_REQUIRE(t.sender() == p.address());
	}
}

int cryptoTest()
{
	KeyPair p(Secret(fromHex("3ecb44df2159c26e0f995712d4f39b6f6e499b40749b1cf1246c37f9516cb6a4")));
	BOOST_REQUIRE(p.pub() == Public(fromHex("97466f2b32bc3bb76d4741ae51cd1d8578b48d3f1e68da206d47321aec267ce78549b514e4453d74ef11b0cd5e4e4c364effddac8b51bcfc8de80682f952896f")));
	BOOST_REQUIRE(p.address() == Address(fromHex("8a40bfaa73256b60764c1bf40675a99083efb075")));
	{
		eth::Transaction t(1000, 0, 0, h160(fromHex("944400f4b88ac9589a0f17ed4671da26bddb668b")), bytes(), 0, p.secret());
		auto rlp = t.rlp(eth::WithoutSignature);
		cnote << RLP(rlp);
		cnote << toHex(rlp);
		cnote << t.sha3(eth::WithoutSignature);
		rlp = t.rlp(eth::WithSignature);
		cnote << RLP(rlp);
		cnote << toHex(rlp);
		cnote << t.sha3(eth::WithSignature);
		assert(t.sender() == p.address());
	}

	return 0;
}

BOOST_AUTO_TEST_SUITE_END()
