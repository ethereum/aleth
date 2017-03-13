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
/** @file CryptoPP.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 */

#include <libdevcore/Guards.h>  // <boost/thread> conflicts with <thread>
#include "CryptoPP.h"
#include <libdevcore/Assertions.h>
#include <libdevcore/SHA3.h>
#include <secp256k1_sha256.h>
#include "ECDHE.h"

using namespace std;
using namespace dev;
using namespace dev::crypto;

static_assert(dev::Secret::size == 32, "Secret key must be 32 bytes.");
static_assert(dev::Public::size == 64, "Public key must be 64 bytes.");
static_assert(dev::Signature::size == 65, "Signature must be 65 bytes.");

Secp256k1PP* Secp256k1PP::get()
{
	static Secp256k1PP s_this;
	return &s_this;
}

void Secp256k1PP::encryptECIES(Public const& _k, bytes& io_cipher)
{
	encryptECIES(_k, bytesConstRef(), io_cipher);
}

void Secp256k1PP::encryptECIES(Public const& _k, bytesConstRef _sharedMacData, bytes& io_cipher)
{
	// interop w/go ecies implementation
	auto r = KeyPair::create();
	Secret z;
	ecdh::agree(r.secret(), _k, z);
	auto key = ecies::kdf(z, bytes(), 32);
	bytesConstRef eKey = bytesConstRef(&key).cropped(0, 16);
	bytesRef mKeyMaterial = bytesRef(&key).cropped(16, 16);
	secp256k1_sha256_t ctx;
	secp256k1_sha256_initialize(&ctx);
	secp256k1_sha256_write(&ctx, mKeyMaterial.data(), mKeyMaterial.size());
	bytes mKey(32);
	secp256k1_sha256_finalize(&ctx, mKey.data());

	auto iv = h128::random();
	bytes cipherText = encryptSymNoAuth(SecureFixedHash<16>(eKey), iv, bytesConstRef(&io_cipher));
	if (cipherText.empty())
		return;

	bytes msg(1 + Public::size + h128::size + cipherText.size() + 32);
	msg[0] = 0x04;
	r.pub().ref().copyTo(bytesRef(&msg).cropped(1, Public::size));
	iv.ref().copyTo(bytesRef(&msg).cropped(1 + Public::size, h128::size));
	bytesRef msgCipherRef = bytesRef(&msg).cropped(1 + Public::size + h128::size, cipherText.size());
	bytesConstRef(&cipherText).copyTo(msgCipherRef);

	// tag message
	secp256k1_hmac_sha256_t hmacCtx;
	secp256k1_hmac_sha256_initialize(&hmacCtx, mKey.data(), mKey.size());
	bytesConstRef cipherWithIV = bytesRef(&msg).cropped(1 + Public::size, h128::size + cipherText.size());
	secp256k1_hmac_sha256_write(&hmacCtx, cipherWithIV.data(), cipherWithIV.size());
	secp256k1_hmac_sha256_write(&hmacCtx, _sharedMacData.data(), _sharedMacData.size());
	secp256k1_hmac_sha256_finalize(&hmacCtx, msg.data() + 1 + Public::size + cipherWithIV.size());

	io_cipher.resize(msg.size());
	io_cipher.swap(msg);
}

bool Secp256k1PP::decryptECIES(Secret const& _k, bytes& io_text)
{
	return decryptECIES(_k, bytesConstRef(), io_text);
}

bool Secp256k1PP::decryptECIES(Secret const& _k, bytesConstRef _sharedMacData, bytes& io_text)
{
	// interop w/go ecies implementation
	
	// io_cipher[0] must be 2, 3, or 4, else invalidpublickey
	if (io_text.empty() || io_text[0] < 2 || io_text[0] > 4)
		// invalid message: publickey
		return false;
	
	if (io_text.size() < (1 + Public::size + h128::size + 1 + h256::size))
		// invalid message: length
		return false;

	Secret z;
	ecdh::agree(_k, *(Public*)(io_text.data() + 1), z);
	auto key = ecies::kdf(z, bytes(), 64);
	bytesConstRef eKey = bytesConstRef(&key).cropped(0, 16);
	bytesRef mKeyMaterial = bytesRef(&key).cropped(16, 16);
	bytes mKey(32);
	// FIXME: Use crypto::sha256()
	secp256k1_sha256_t ctx;
	secp256k1_sha256_initialize(&ctx);
	secp256k1_sha256_write(&ctx, mKeyMaterial.data(), mKeyMaterial.size());
	secp256k1_sha256_finalize(&ctx, mKey.data());
	
	bytes plain;
	size_t cipherLen = io_text.size() - 1 - Public::size - h128::size - h256::size;
	bytesConstRef cipherWithIV(io_text.data() + 1 + Public::size, h128::size + cipherLen);
	bytesConstRef cipherIV = cipherWithIV.cropped(0, h128::size);
	bytesConstRef cipherNoIV = cipherWithIV.cropped(h128::size, cipherLen);
	bytesConstRef msgMac(cipherNoIV.data() + cipherLen, h256::size);
	h128 iv(cipherIV.toBytes());
	
	// verify tag

	secp256k1_hmac_sha256_t hmacCtx;
	secp256k1_hmac_sha256_initialize(&hmacCtx, mKey.data(), mKey.size());
	secp256k1_hmac_sha256_write(&hmacCtx, cipherWithIV.data(), cipherWithIV.size());
	secp256k1_hmac_sha256_write(&hmacCtx, _sharedMacData.data(), _sharedMacData.size());
	h256 mac;
	secp256k1_hmac_sha256_finalize(&hmacCtx, mac.data());
	for (unsigned i = 0; i < h256::size; i++)
		if (mac[i] != msgMac[i])
			return false;
	
	plain = decryptSymNoAuth(SecureFixedHash<16>(eKey), iv, cipherNoIV).makeInsecure();
	io_text.resize(plain.size());
	io_text.swap(plain);
	
	return true;
}
