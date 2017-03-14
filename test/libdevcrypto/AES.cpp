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

#include <boost/test/unit_test.hpp>
#include <random>
#include <libdevcore/Common.h>
#include <libdevcrypto/Common.h>
#include <libdevcore/SHA3.h>
#include <libdevcore/Log.h>
#include <libdevcrypto/AES.h>
#include <test/libtesteth/TestHelper.h>

using namespace std;
using namespace dev;
using namespace dev::test;

BOOST_AUTO_TEST_SUITE(Crypto)

BOOST_FIXTURE_TEST_SUITE(AES, TestOutputHelper)

BOOST_AUTO_TEST_CASE(AesEncryptConst)
{
	auto key = asBytes("0123456789abcdef");
	h128 iv;
	iv[5] = 1;
	auto msg = asBytes("Hello!.........................................");
	auto e = encryptAES128CTR(&key, iv, &msg);
	auto expected = "0378b494602bb95ef9e4832f9ba6cba50ea3423b5b2904a5ea66cfef31019ca39135f7e551829e85f64a099edc223d";
	BOOST_CHECK_GT(msg.size(), 32);
	BOOST_CHECK_EQUAL(msg.size(), e.size());
	BOOST_CHECK_EQUAL(toHex(e), expected);
}

BOOST_AUTO_TEST_CASE(AesDecryptConst)
{
	auto key = asBytes("0123456789abcdef");
	h128 iv;
	iv[5] = 1;
	auto msg = asBytes("Hello!.........................................");
	auto d = decryptAES128CTR(&key, iv, &msg);
	auto expected = "0378b494602bb95ef9e4832f9ba6cba50ea3423b5b2904a5ea66cfef31019ca39135f7e551829e85f64a099edc223d";
	BOOST_CHECK_GT(msg.size(), 32);
	BOOST_CHECK_EQUAL(msg.size(), d.size());
	BOOST_CHECK_EQUAL(toHex(d.makeInsecure()), expected);
}

BOOST_AUTO_TEST_CASE(AES128CTR_random)
{
	std::mt19937 gen(static_cast<unsigned long>(std::rand()));
	std::uniform_int_distribution<> dis(1, 6);
	constexpr auto numBytes = 234567;
	bytes b;
	std::generate_n(std::back_inserter(b), numBytes, [&]{ return static_cast<byte>(dis(gen)); });
	BOOST_CHECK_EQUAL(b.size(), numBytes);

	auto key = asBytes("ABCDEF0123456789");
	auto iv = h128::random();

	auto e = encryptAES128CTR(&key, iv, &b);
	auto d = decryptAES128CTR(&key, iv, &e);
	BOOST_CHECK(d.makeInsecure() == b);
}

BOOST_AUTO_TEST_CASE(AES128CTREncryptNISTTestVector)
{
	// Test vectors from http://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38a.pdf
	auto key = fromHex("2b7e151628aed2a6abf7158809cf4f3c");
	auto initCounter = h128("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff");
	auto plainText = fromHex("6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710");
	auto cipherText = "874d6191b620e3261bef6864990db6ce9806f66b7970fdff8617187bb9fffdff5ae4df3edbd5d35e5b4f09020db03eab1e031dda2fbe03d1792170a0f3009cee";

	auto e = encryptAES128CTR(&key, initCounter, &plainText);
	BOOST_CHECK_EQUAL(toHex(e), cipherText);
}

BOOST_AUTO_TEST_CASE(AES128CTRDecryptNISTTestVector)
{
	// Test vectors from http://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38a.pdf
	auto key = fromHex("2b7e151628aed2a6abf7158809cf4f3c");
	auto initCounter = h128("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff");
	auto cipherText = fromHex("874d6191b620e3261bef6864990db6ce9806f66b7970fdff8617187bb9fffdff5ae4df3edbd5d35e5b4f09020db03eab1e031dda2fbe03d1792170a0f3009cee");
	auto plainText = "6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710";

	auto d = decryptAES128CTR(&key, initCounter, &cipherText);
	BOOST_CHECK_EQUAL(toHex(d.makeInsecure()), plainText);
}

BOOST_AUTO_TEST_CASE(AesDecrypt)
{
	bytes seed = fromHex("2dbaead416c20cfd00c2fc9f1788ff9f965a2000799c96a624767cb0e1e90d2d7191efdd92349226742fdc73d1d87e2d597536c4641098b9a89836c94f58a2ab4c525c27c4cb848b3e22ea245b2bc5c8c7beaa900b0c479253fc96fce7ffc621");
	KeyPair kp(sha3Secure(aesDecrypt(&seed, "test")));
	BOOST_CHECK(Address("07746f871de684297923f933279555dda418f8a2") == kp.address());
}

BOOST_AUTO_TEST_CASE(AesDecryptWrongSeed)
{
	bytes seed = fromHex("badaead416c20cfd00c2fc9f1788ff9f965a2000799c96a624767cb0e1e90d2d7191efdd92349226742fdc73d1d87e2d597536c4641098b9a89836c94f58a2ab4c525c27c4cb848b3e22ea245b2bc5c8c7beaa900b0c479253fc96fce7ffc621");
	KeyPair kp(sha3Secure(aesDecrypt(&seed, "test")));
	BOOST_CHECK(Address("07746f871de684297923f933279555dda418f8a2") != kp.address());
}

BOOST_AUTO_TEST_CASE(AesDecryptWrongPassword)
{
	bytes seed = fromHex("2dbaead416c20cfd00c2fc9f1788ff9f965a2000799c96a624767cb0e1e90d2d7191efdd92349226742fdc73d1d87e2d597536c4641098b9a89836c94f58a2ab4c525c27c4cb848b3e22ea245b2bc5c8c7beaa900b0c479253fc96fce7ffc621");
	KeyPair kp(sha3Secure(aesDecrypt(&seed, "badtest")));
	BOOST_CHECK(Address("07746f871de684297923f933279555dda418f8a2") != kp.address());
}

BOOST_AUTO_TEST_CASE(AesDecryptFailInvalidSeed)
{
	bytes seed = fromHex("xdbaead416c20cfd00c2fc9f1788ff9f965a2000799c96a624767cb0e1e90d2d7191efdd92349226742fdc73d1d87e2d597536c4641098b9a89836c94f58a2ab4c525c27c4cb848b3e22ea245b2bc5c8c7beaa900b0c479253fc96fce7ffc621");
	BOOST_CHECK(bytes() == aesDecrypt(&seed, "test"));
}

BOOST_AUTO_TEST_CASE(AesDecryptFailInvalidSeedSize)
{
	bytes seed = fromHex("000102030405060708090a0b0c0d0e0f");
	BOOST_CHECK(bytes() == aesDecrypt(&seed, "test"));
}

BOOST_AUTO_TEST_CASE(AesDecryptFailInvalidSeed2)
{
	bytes seed = fromHex("000102030405060708090a0b0c0d0e0f000102030405060708090a0b0c0d0e0f");
	BOOST_CHECK(bytes() == aesDecrypt(&seed, "test"));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
