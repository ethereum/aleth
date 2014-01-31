/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	Foobar is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file Transaction.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include <secp256k1.h>
#include "vector_ref.h"
#include "Exceptions.h"
#include "Transaction.h"
using namespace std;
using namespace eth;

Transaction::Transaction(bytesConstRef _rlpData)
{
	RLP rlp(_rlpData);
	nonce = rlp[0].toInt<u256>(RLP::StrictlyInt);
	receiveAddress = rlp[1].toHash<Address>();
	value = rlp[2].toInt<u256>(RLP::StrictlyInt);
	fee = rlp[3].toInt<u256>(RLP::StrictlyInt);
	data.reserve(rlp[4].itemCountStrict());
	for (auto const& i: rlp[4])
		data.push_back(i.toInt<u256>(RLP::StrictlyInt));
	vrs = Signature{ rlp[5].toInt<byte>(RLP::StrictlyInt), rlp[6].toInt<u256>(RLP::StrictlyInt), rlp[7].toInt<u256>(RLP::StrictlyInt) };
}

Address Transaction::sender() const
{
	secp256k1_start();

	h256 sig[2] = { vrs.r, vrs.s };
	h256 msg = sha3(false);

	byte pubkey[65];
	int pubkeylen = 65;
	if (!secp256k1_ecdsa_recover_compact(msg.data(), 32, sig[0].data(), pubkey, &pubkeylen, 0, (int)vrs.v - 27))
		throw InvalidSignature();

	// TODO: check right160 is correct and shouldn't be left160.
	auto ret = right160(eth::sha3(bytesConstRef(&(pubkey[1]), 64)));

#if ETH_ADDRESS_DEBUG
	cout << "---- RECOVER -------------------------------" << endl;
	cout << "MSG: " << msg << endl;
	cout << "R S V: " << sig[0] << " " << sig[1] << " " << (int)(vrs.v - 27) << "+27" << endl;
	cout << "PUB: " << asHex(bytesConstRef(&(pubkey[1]), 64)) << endl;
	cout << "ADR: " << ret << endl;
#endif
	return ret;
}

void Transaction::sign(Secret _priv)
{
	int v = 0;

	secp256k1_start();

	h256 msg = sha3(false);
	h256 sig[2];
	h256 nonce = kFromMessage(msg, _priv);

	if (!secp256k1_ecdsa_sign_compact(msg.data(), 32, sig[0].data(), _priv.data(), nonce.data(), &v))
		throw InvalidSignature();
#if ETH_ADDRESS_DEBUG
	cout << "---- SIGN -------------------------------" << endl;
	cout << "MSG: " << msg << endl;
	cout << "SEC: " << _priv << endl;
	cout << "NON: " << nonce << endl;
	cout << "R S V: " << sig[0] << " " << sig[1] << " " << v << "+27" << endl;
#endif

	vrs.v = (byte)(v + 27);
	vrs.r = (u256)sig[0];
	vrs.s = (u256)sig[1];
}

void Transaction::fillStream(RLPStream& _s, bool _sig) const
{
	_s.appendList(_sig ? 8 : 5);
	_s << nonce << receiveAddress << value << fee << data;
	if (_sig)
		_s << vrs.v << vrs.r << vrs.s;
}

// If the h256 return is an integer, store it in bigendian (i.e. u256 ret; ... return (h256)ret; )
h256 Transaction::kFromMessage(h256 _msg, h256 _priv)
{
	// TODO!
	/*
	v = '\x01' * 32
	k = '\x00' * 32
	priv = encode_privkey(priv,'bin')
	msghash = encode(hash_to_int(msghash),256,32)
	k = hmac.new(k, v+'\x00'+priv+msghash, hashlib.sha256).digest()
	v = hmac.new(k, v, hashlib.sha256).digest()
	k = hmac.new(k, v+'\x01'+priv+msghash, hashlib.sha256).digest()
	v = hmac.new(k, v, hashlib.sha256).digest()
	return decode(hmac.new(k, v, hashlib.sha256).digest(),256)
	*/
	return _msg ^ _priv;
}

