/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Foobar is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file crypto.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 * Crypto test functions.
 */

#include <random>
#include <secp256k1.h>
#include <Common.h>
#include <RLP.h>
#include <Transaction.h>
using namespace std;
using namespace eth;

int cryptoTest()
{
	// Test transaction.
	bytes tx = fromUserHex("88005401010101010101010101010101010101010101011f0de0b6b3a76400001ce8d4a5100080181c373130a009ba1f10285d4e659568bfcfec85067855c5a3c150100815dad4ef98fd37cf0593828c89db94bd6c64e210a32ef8956eaa81ea9307194996a3b879441f5d");
	cout << "TX: " << RLP(tx) << endl;

	Transaction t2(tx);
	cout << "SENDER: " << hex << t2.sender() << dec << endl;

	secp256k1_start();

	Transaction t;
	t.nonce = 0;
	t.fee = 0;
	t.value = 1;			// 1 wei.
	t.receiveAddress = toAddress(sha3("123"));

	bytes sig64 = toBigEndian(t.vrs.r) + toBigEndian(t.vrs.s);
	cout << "SIG: " << sig64.size() << " " << asHex(sig64) << " " << t.vrs.v << endl;

	auto msg = t.rlp(false);
	cout << "TX w/o SIG: " << RLP(msg) << endl;
	cout << "RLP(TX w/o SIG): " << asHex(t.rlpString(false)) << endl;
	std::string hmsg = sha3(t.rlpString(false), false);
	cout << "SHA256(RLP(TX w/o SIG)): 0x" << asHex(hmsg) << endl;

	bytes privkey = sha3Bytes("123");

	{
		bytes pubkey(65);
		int pubkeylen = 65;

		int ret = secp256k1_ecdsa_seckey_verify(privkey.data());
		cout << "SEC: " << dec << ret << " " << asHex(privkey) << endl;

		ret = secp256k1_ecdsa_pubkey_create(pubkey.data(), &pubkeylen, privkey.data(), 1);
		pubkey.resize(pubkeylen);
		int good = secp256k1_ecdsa_pubkey_verify(pubkey.data(), pubkey.size());
		cout << "PUB: " << dec << ret << " " << pubkeylen << " " << asHex(pubkey) << (good ? " GOOD" : " BAD") << endl;
	}

	// Test roundtrip...
	{
		bytes sig(64);
		u256 nonce = 0;
		int v = 0;
		cout << asHex(hmsg) << endl;
		cout << asHex(privkey) << endl;
		cout << hex << nonce << dec << endl;
		int ret = secp256k1_ecdsa_sign_compact((byte const*)hmsg.data(), hmsg.size(), sig.data(), privkey.data(), (byte const*)&nonce, &v);
		cout << "MYSIG: " << dec << ret << " " << sig.size() << " " << asHex(sig) << " " << v << endl;

		bytes pubkey(65);
		int pubkeylen = 65;
		ret = secp256k1_ecdsa_recover_compact((byte const*)hmsg.data(), hmsg.size(), (byte const*)sig.data(), pubkey.data(), &pubkeylen, 0, v);
		pubkey.resize(pubkeylen);
		cout << "MYREC: " << dec << ret << " " << pubkeylen << " " << asHex(pubkey) << endl;
	}

	{
		bytes pubkey(65);
		int pubkeylen = 65;
		int ret = secp256k1_ecdsa_recover_compact((byte const*)hmsg.data(), hmsg.size(), (byte const*)sig64.data(), pubkey.data(), &pubkeylen, 0, (int)t.vrs.v - 27);
		pubkey.resize(pubkeylen);
		cout << "RECPUB: " << dec << ret << " " << pubkeylen << " " << asHex(pubkey) << endl;
		cout << "SENDER: " << hex << low160(eth::sha3(bytesConstRef(&pubkey).cropped(1))) << dec << endl;
	}
	return 0;
}

