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
/** @file Common.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "Common.h"
#include <random>
#include <chrono>
#include <thread>
#include <mutex>
#include <libscrypt/libscrypt.h>
#include <libdevcore/Guards.h>
#include <libdevcore/SHA3.h>
#include <libdevcore/FileSystem.h>
#include "AES.h"
#include "CryptoPP.h"
using namespace std;
using namespace dev;
using namespace dev::crypto;

static Secp256k1 s_secp256k1;

bool dev::SignatureStruct::isValid() const
{
	if (v > 1 ||
		r >= h256("0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141") ||
		s >= h256("0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141") ||
		s < h256(1) ||
		r < h256(1))
		return false;
	return true;
}

Address dev::ZeroAddress = Address();

Public dev::toPublic(Secret const& _secret)
{
	Public p;
	s_secp256k1.toPublic(_secret, p);
	return std::move(p);
}

Address dev::toAddress(Public const& _public)
{
	return s_secp256k1.toAddress(_public);
}

Address dev::toAddress(Secret const& _secret)
{
	Public p;
	s_secp256k1.toPublic(_secret, p);
	return s_secp256k1.toAddress(p);
}

void dev::encrypt(Public const& _k, bytesConstRef _plain, bytes& o_cipher)
{
	bytes io = _plain.toBytes();
	s_secp256k1.encrypt(_k, io);
	o_cipher = std::move(io);
}

bool dev::decrypt(Secret const& _k, bytesConstRef _cipher, bytes& o_plaintext)
{
	bytes io = _cipher.toBytes();
	s_secp256k1.decrypt(_k, io);
	if (io.empty())
		return false;
	o_plaintext = std::move(io);
	return true;
}

void dev::encryptECIES(Public const& _k, bytesConstRef _plain, bytes& o_cipher)
{
	bytes io = _plain.toBytes();
	s_secp256k1.encryptECIES(_k, io);
	o_cipher = std::move(io);
}

bool dev::decryptECIES(Secret const& _k, bytesConstRef _cipher, bytes& o_plaintext)
{
	bytes io = _cipher.toBytes();
	if (!s_secp256k1.decryptECIES(_k, io))
		return false;
	o_plaintext = std::move(io);
	return true;
}

void dev::encryptSym(Secret const& _k, bytesConstRef _plain, bytes& o_cipher)
{
	// TOOD: @alex @subtly do this properly.
	encrypt(KeyPair(_k).pub(), _plain, o_cipher);
}

bool dev::decryptSym(Secret const& _k, bytesConstRef _cipher, bytes& o_plain)
{
	// TODO: @alex @subtly do this properly.
	return decrypt(_k, _cipher, o_plain);
}

std::pair<bytes, h128> dev::encryptSymNoAuth(h128 const& _k, bytesConstRef _plain)
{
	h128 iv(Nonce::get());
	return make_pair(encryptSymNoAuth(_k, iv, _plain), iv);
}

bytes dev::encryptAES128CTR(bytesConstRef _k, h128 const& _iv, bytesConstRef _plain)
{
	if (_k.size() != 16 && _k.size() != 24 && _k.size() != 32)
		return bytes();
	SecByteBlock key(_k.data(), _k.size());
	try
	{
		CTR_Mode<AES>::Encryption e;
		e.SetKeyWithIV(key, key.size(), _iv.data());
		bytes ret(_plain.size());
		e.ProcessData(ret.data(), _plain.data(), _plain.size());
		return ret;
	}
	catch (CryptoPP::Exception& _e)
	{
		cerr << _e.what() << endl;
		return bytes();
	}
}

bytes dev::decryptAES128CTR(bytesConstRef _k, h128 const& _iv, bytesConstRef _cipher)
{
	if (_k.size() != 16 && _k.size() != 24 && _k.size() != 32)
		return bytes();
	SecByteBlock key(_k.data(), _k.size());
	try
	{
		CTR_Mode<AES>::Decryption d;
		d.SetKeyWithIV(key, key.size(), _iv.data());
		bytes ret(_cipher.size());
		d.ProcessData(ret.data(), _cipher.data(), _cipher.size());
		return ret;
	}
	catch (CryptoPP::Exception& _e)
	{
		cerr << _e.what() << endl;
		return bytes();
	}
}

Public dev::recover(Signature const& _sig, h256 const& _message)
{
	return s_secp256k1.recover(_sig, _message.ref());
}

Signature dev::sign(Secret const& _k, h256 const& _hash)
{
	return s_secp256k1.sign(_k, _hash);
}

bool dev::verify(Public const& _p, Signature const& _s, h256 const& _hash)
{
	return s_secp256k1.verify(_p, _s, _hash.ref(), true);
}

bytes dev::pbkdf2(string const& _pass, bytes const& _salt, unsigned _iterations, unsigned _dkLen)
{
	bytes ret(_dkLen);
	PKCS5_PBKDF2_HMAC<SHA256> pbkdf;
	pbkdf.DeriveKey(ret.data(), ret.size(), 0, (byte*)_pass.data(), _pass.size(), _salt.data(), _salt.size(), _iterations);
	return ret;
}

bytes dev::scrypt(std::string const& _pass, bytes const& _salt, uint64_t _n, uint32_t _r, uint32_t _p, unsigned _dkLen)
{
	bytes ret(_dkLen);
	libscrypt_scrypt((uint8_t const*)_pass.data(), _pass.size(), _salt.data(), _salt.size(), _n, _r, _p, ret.data(), ret.size());
	return ret;
}

KeyPair KeyPair::create()
{
	static boost::thread_specific_ptr<mt19937_64> s_eng;
	static unsigned s_id = 0;
	if (!s_eng.get())
		s_eng.reset(new mt19937_64(time(0) + chrono::high_resolution_clock::now().time_since_epoch().count() + ++s_id));

	uniform_int_distribution<uint16_t> d(0, 255);

	for (int i = 0; i < 100; ++i)
	{
		KeyPair ret(FixedHash<32>::random(*s_eng.get()));
		if (ret.address())
			return ret;
	}
	return KeyPair();
}

KeyPair::KeyPair(h256 _sec):
	m_secret(_sec)
{
	if (s_secp256k1.verifySecret(m_secret, m_public))
		m_address = s_secp256k1.toAddress(m_public);
}

KeyPair KeyPair::fromEncryptedSeed(bytesConstRef _seed, std::string const& _password)
{
	return KeyPair(sha3(aesDecrypt(_seed, _password)));
}

h256 crypto::kdf(Secret const& _priv, h256 const& _hash)
{
	// H(H(r||k)^h)
	h256 s;
	sha3mac(Nonce::get().ref(), _priv.ref(), s.ref());
	s ^= _hash;
	sha3(s.ref(), s.ref());
	
	if (!s || !_hash || !_priv)
		BOOST_THROW_EXCEPTION(InvalidState());
	return std::move(s);
}

h256 Nonce::get(bool _commit)
{
	// todo: atomic efface bit, periodic save, kdf, rr, rng
	// todo: encrypt
	static h256 s_seed;
	static string s_seedFile(getDataDir() + "/seed");
	static mutex s_x;
	Guard l(s_x);
	if (!s_seed)
	{
		static Nonce s_nonce;
		bytes b = contents(s_seedFile);
		if (b.size() == 32)
			memcpy(s_seed.data(), b.data(), 32);
		else
		{
			// todo: replace w/entropy from user and system
			std::mt19937_64 s_eng(time(0) + chrono::high_resolution_clock::now().time_since_epoch().count());
			std::uniform_int_distribution<uint16_t> d(0, 255);
			for (unsigned i = 0; i < 32; ++i)
				s_seed[i] = (byte)d(s_eng);
		}
		if (!s_seed)
			BOOST_THROW_EXCEPTION(InvalidState());
		
		// prevent seed reuse if process terminates abnormally
		writeFile(s_seedFile, bytes());
	}
	h256 prev(s_seed);
	sha3(prev.ref(), s_seed.ref());
	if (_commit)
		writeFile(s_seedFile, s_seed.asBytes());
	return std::move(s_seed);
}

Nonce::~Nonce()
{
	Nonce::get(true);
}
