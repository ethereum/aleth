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
/** @file Common.h
 * @author Gav Wood <i@gavwood.com>
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 *
 * Ethereum-specific data structures & algorithms.
 */

#pragma once

#include <libdevcore/Common.h>
#include <libdevcore/FixedHash.h>
#include <libdevcore/Exceptions.h>

namespace dev
{

/// A secret key: 32 bytes.
/// @NOTE This is not endian-specific; it's just a bunch of bytes.
using Secret = h256;

/// A public key: 64 bytes.
/// @NOTE This is not endian-specific; it's just a bunch of bytes.
using Public = h512;

/// A signature: 65 bytes: r: [0, 32), s: [32, 64), v: 64.
/// @NOTE This is not endian-specific; it's just a bunch of bytes.
using Signature = h520;

struct SignatureStruct
{
	SignatureStruct() {}
	SignatureStruct(Signature const& _s) { *(h520*)this = _s; }
	SignatureStruct(h256 const& _r, h256 const& _s, byte _v): r(_r), s(_s), v(_v) {}
	operator Signature() const { return *(h520 const*)this; }

	/// @returns true if r,s,v values are valid, otherwise false
	bool isValid();

	h256 r;
	h256 s;
	byte v;
};

/// An Ethereum address: 20 bytes.
/// @NOTE This is not endian-specific; it's just a bunch of bytes.
using Address = h160;

/// A vector of Ethereum addresses.
using Addresses = h160s;

/// A set of Ethereum addresses.
using AddressSet = std::set<h160>;

/// A vector of secrets.
using Secrets = h256s;

/// Convert a secret key into the public key equivalent.
Public toPublic(Secret const& _secret);

/// Convert a public key to address.
Address toAddress(Public const& _public);

/// Convert a secret key into address of public key equivalent.
/// @returns 0 if it's not a valid secret key.
Address toAddress(Secret const& _secret);

/// Encrypts plain text using Public key.
void encrypt(Public const& _k, bytesConstRef _plain, bytes& o_cipher);

/// Decrypts cipher using Secret key.
bool decrypt(Secret const& _k, bytesConstRef _cipher, bytes& o_plaintext);
	
/// Recovers Public key from signed message hash.
Public recover(Signature const& _sig, h256 const& _hash);
	
/// Returns siganture of message hash.
Signature sign(Secret const& _k, h256 const& _hash);
	
/// Verify signature.
bool verify(Public const& _k, Signature const& _s, h256 const& _hash);

/// Simple class that represents a "key pair".
/// All of the data of the class can be regenerated from the secret key (m_secret) alone.
/// Actually stores a tuplet of secret, public and address (the right 160-bits of the public).
class KeyPair
{
public:
	/// Null constructor.
	KeyPair() {}

	/// Normal constructor - populates object from the given secret key.
	KeyPair(Secret _k);

	/// Create a new, randomly generated object.
	static KeyPair create();

	/// Create from an encrypted seed.
	static KeyPair fromEncryptedSeed(bytesConstRef _seed, std::string const& _password);

	/// Retrieve the secret key.
	Secret const& secret() const { return m_secret; }
	/// Retrieve the secret key.
	Secret const& sec() const { return m_secret; }

	/// Retrieve the public key.
	Public const& pub() const { return m_public; }

	/// Retrieve the associated address of the public key.
	Address const& address() const { return m_address; }

	bool operator==(KeyPair const& _c) const { return m_secret == _c.m_secret; }
	bool operator!=(KeyPair const& _c) const { return m_secret != _c.m_secret; }

private:
	Secret m_secret;
	Public m_public;
	Address m_address;
};

namespace crypto
{
struct InvalidState: public dev::Exception {};

/// Key derivation
h256 kdf(Secret const& _priv, h256 const& _hash);
	
/**
 * @brief Generator for nonce material
 */
struct Nonce
{
	static h256 get(bool _commit = false);
private:
	Nonce() {}
	~Nonce();
};
}

}
