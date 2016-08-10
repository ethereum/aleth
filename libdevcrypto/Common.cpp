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
#include <cstdint>
#include <chrono>
#include <thread>
#include <mutex>
#include <libscrypt/libscrypt.h>
#include <libdevcore/Guards.h>
#include <libdevcore/SHA3.h>
#include <libdevcore/RLP.h>
#if ETH_HAVE_SECP256K1
#include <secp256k1/include/secp256k1.h>
#endif
#include "AES.h"
#include "CryptoPP.h"
#include "Exceptions.h"
using namespace std;
using namespace dev;
using namespace dev::crypto;

#ifdef ETH_HAVE_SECP256K1
class Secp256k1Context
{
public:
	static secp256k1_context_t const* get() { if (!s_this) s_this = new Secp256k1Context; return s_this->m_ctx; }

private:
	Secp256k1Context() { m_ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY); }
	~Secp256k1Context() { secp256k1_context_destroy(m_ctx); }

	secp256k1_context_t* m_ctx;

	static Secp256k1Context* s_this;
};
Secp256k1Context* Secp256k1Context::s_this = nullptr;
#endif

bool dev::SignatureStruct::isValid() const noexcept
{
	if (v > 1 ||
		r >= h256("0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141") ||
		s >= h256("0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141") ||
		s < h256(1) ||
		r < h256(1))
		return false;
	return true;
}

Public SignatureStruct::recover(h256 const& _hash) const
{
	return dev::recover((Signature)*this, _hash);
}

Address dev::ZeroAddress = Address();

Public dev::toPublic(Secret const& _secret)
{
#ifdef ETH_HAVE_SECP256K1
	bytes o(65);
	int pubkeylen;
	if (!secp256k1_ec_pubkey_create(Secp256k1Context::get(), o.data(), &pubkeylen, _secret.data(), false))
		return Public();
	return FixedHash<64>(o.data()+1, Public::ConstructFromPointer);
#else
	Public p;
	Secp256k1PP::get()->toPublic(_secret, p);
	return p;
#endif
}

Address dev::toAddress(Public const& _public)
{
	return right160(sha3(_public.ref()));
}

Address dev::toAddress(Secret const& _secret)
{
	Public p;
	Secp256k1PP::get()->toPublic(_secret, p);
	return toAddress(p);
}

Address dev::toAddress(Address const& _from, u256 const& _nonce)
{
	return right160(sha3(rlpList(_from, _nonce)));
}

void dev::encrypt(Public const& _k, bytesConstRef _plain, bytes& o_cipher)
{
	bytes io = _plain.toBytes();
	Secp256k1PP::get()->encrypt(_k, io);
	o_cipher = std::move(io);
}

bool dev::decrypt(Secret const& _k, bytesConstRef _cipher, bytes& o_plaintext)
{
	bytes io = _cipher.toBytes();
	Secp256k1PP::get()->decrypt(_k, io);
	if (io.empty())
		return false;
	o_plaintext = std::move(io);
	return true;
}

void dev::encryptECIES(Public const& _k, bytesConstRef _plain, bytes& o_cipher)
{
	encryptECIES(_k, bytesConstRef(), _plain, o_cipher);
}

void dev::encryptECIES(Public const& _k, bytesConstRef _sharedMacData, bytesConstRef _plain, bytes& o_cipher)
{
	bytes io = _plain.toBytes();
	Secp256k1PP::get()->encryptECIES(_k, _sharedMacData, io);
	o_cipher = std::move(io);
}

bool dev::decryptECIES(Secret const& _k, bytesConstRef _cipher, bytes& o_plaintext)
{
	return decryptECIES(_k, bytesConstRef(),  _cipher, o_plaintext);
}

bool dev::decryptECIES(Secret const& _k, bytesConstRef _sharedMacData, bytesConstRef _cipher, bytes& o_plaintext)
{
	bytes io = _cipher.toBytes();
	if (!Secp256k1PP::get()->decryptECIES(_k, _sharedMacData, io))
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

std::pair<bytes, h128> dev::encryptSymNoAuth(SecureFixedHash<16> const& _k, bytesConstRef _plain)
{
	h128 iv(Nonce::get().makeInsecure());
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

bytesSec dev::decryptAES128CTR(bytesConstRef _k, h128 const& _iv, bytesConstRef _cipher)
{
	if (_k.size() != 16 && _k.size() != 24 && _k.size() != 32)
		return bytesSec();
	SecByteBlock key(_k.data(), _k.size());
	try
	{
		CTR_Mode<AES>::Decryption d;
		d.SetKeyWithIV(key, key.size(), _iv.data());
		bytesSec ret(_cipher.size());
		d.ProcessData(ret.writable().data(), _cipher.data(), _cipher.size());
		return ret;
	}
	catch (CryptoPP::Exception& _e)
	{
		cerr << _e.what() << endl;
		return bytesSec();
	}
}

static const Public c_zeroKey("3f17f1962b36e491b30a40b2405849e597ba5fb5");

Public dev::recover(Signature const& _sig, h256 const& _message)
{
	Public ret;
#ifdef ETH_HAVE_SECP256K1
	bytes o(65);
	int pubkeylen;
	if (_sig[64] > 3 || !secp256k1_ecdsa_recover_compact(Secp256k1Context::get(), _message.data(), _sig.data(), o.data(), &pubkeylen, false, _sig[64]))
		return Public();
	ret = FixedHash<64>(o.data() + 1, Public::ConstructFromPointer);
#else
	ret = Secp256k1PP::get()->recover(_sig, _message.ref());
#endif
	if (ret == c_zeroKey)
		return Public();
	return ret;
}

static const u256 c_secp256k1n("115792089237316195423570985008687907852837564279074904382605163141518161494337");

Signature dev::sign(Secret const& _k, h256 const& _hash)
{
	Signature s;
	SignatureStruct& ss = *reinterpret_cast<SignatureStruct*>(&s);

#ifdef ETH_HAVE_SECP256K1
	int v;
	if (!secp256k1_ecdsa_sign_compact(Secp256k1Context::get(), _hash.data(), s.data(), _k.data(), NULL, NULL, &v))
		return Signature();
	ss.v = v;
#else
	s = Secp256k1PP::get()->sign(_k, _hash);
#endif
	if (ss.s > c_secp256k1n / 2)
	{
		ss.v = ss.v ^ 1;
		ss.s = h256(c_secp256k1n - u256(ss.s));
	}
	assert(ss.s <= c_secp256k1n / 2);
	return s;
}

bool dev::verify(Public const& _p, Signature const& _s, h256 const& _hash)
{
	if (!_p)
		return false;
#ifdef ETH_HAVE_SECP256K1
	return _p == recover(_s, _hash);
#else
	return Secp256k1PP::get()->verify(_p, _s, _hash.ref(), true);
#endif
}

bytesSec dev::pbkdf2(string const& _pass, bytes const& _salt, unsigned _iterations, unsigned _dkLen)
{
	bytesSec ret(_dkLen);
	if (PKCS5_PBKDF2_HMAC<SHA256>().DeriveKey(
		ret.writable().data(),
		_dkLen,
		0,
		reinterpret_cast<byte const*>(_pass.data()),
		_pass.size(),
		_salt.data(),
		_salt.size(),
		_iterations
	) != _iterations)
		BOOST_THROW_EXCEPTION(CryptoException() << errinfo_comment("Key derivation failed."));
	return ret;
}

bytesSec dev::scrypt(std::string const& _pass, bytes const& _salt, uint64_t _n, uint32_t _r, uint32_t _p, unsigned _dkLen)
{
	bytesSec ret(_dkLen);
	if (libscrypt_scrypt(
		reinterpret_cast<uint8_t const*>(_pass.data()),
		_pass.size(),
		_salt.data(),
		_salt.size(),
		_n,
		_r,
		_p,
		ret.writable().data(),
		_dkLen
	) != 0)
		BOOST_THROW_EXCEPTION(CryptoException() << errinfo_comment("Key derivation failed."));
	return ret;
}

void KeyPair::populateFromSecret(Secret const& _sec)
{
	m_secret = _sec;
	if (Secp256k1PP::get()->verifySecret(m_secret, m_public))
		m_address = toAddress(m_public);
}

KeyPair KeyPair::create()
{
	for (int i = 0; i < 100; ++i)
	{
		KeyPair ret(Secret::random());
		if (ret.address())
			return ret;
	}
	return KeyPair();
}

KeyPair KeyPair::fromEncryptedSeed(bytesConstRef _seed, std::string const& _password)
{
	return KeyPair(Secret(sha3(aesDecrypt(_seed, _password))));
}

h256 crypto::kdf(Secret const& _priv, h256 const& _hash)
{
	// H(H(r||k)^h)
	h256 s;
	sha3mac(Secret::random().ref(), _priv.ref(), s.ref());
	s ^= _hash;
	sha3(s.ref(), s.ref());
	
	if (!s || !_hash || !_priv)
		BOOST_THROW_EXCEPTION(InvalidState());
	return s;
}

Secret Nonce::next()
{
	Guard l(x_value);
	if (!m_value)
	{
		m_value = Secret::random();
		if (!m_value)
			BOOST_THROW_EXCEPTION(InvalidState());
	}
	m_value = sha3Secure(m_value.ref());
	return sha3(~m_value);
}
