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
/** @file CryptoPP.h
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 *
 * CryptoPP headers and primitive helper methods
 */

#pragma once

#include "Common.h"

namespace dev
{
namespace crypto
{
/// Amount of bytes added when encrypting with encryptECIES.
static const unsigned c_eciesOverhead = 113;

/**
 * CryptoPP secp256k1 algorithms.
 * @todo Collect ECIES methods into class.
 */
class Secp256k1PP
{	
public:
	static Secp256k1PP* get();

	/// Encrypts text (replace input). (ECIES w/XOR-SHA1)
	void encrypt(Public const& _k, bytes& io_cipher);
	
	/// Decrypts text (replace input). (ECIES w/XOR-SHA1)
	void decrypt(Secret const& _k, bytes& io_text);
	
	/// Encrypts text (replace input). (ECIES w/AES128-CTR-SHA256)
	void encryptECIES(Public const& _k, bytes& io_cipher);
	
	/// Encrypts text (replace input). (ECIES w/AES128-CTR-SHA256)
	void encryptECIES(Public const& _k, bytesConstRef _sharedMacData, bytes& io_cipher);
	
	/// Decrypts text (replace input). (ECIES w/AES128-CTR-SHA256)
	bool decryptECIES(Secret const& _k, bytes& io_text);
	
	/// Decrypts text (replace input). (ECIES w/AES128-CTR-SHA256)
	bool decryptECIES(Secret const& _k, bytesConstRef _sharedMacData, bytes& io_text);
	
	/// Key derivation function used by encryptECIES and decryptECIES.
	bytes eciesKDF(Secret const& _z, bytes _s1, unsigned kdBitLen = 256);
	
	/// @returns siganture of message.
	Signature sign(Secret const& _k, bytesConstRef _message);
	
	/// @returns compact siganture of provided hash.
	Signature sign(Secret const& _k, h256 const& _hash);
	
	/// Verify compact signature (public key is extracted from signature).
	bool verify(Signature const& _signature, bytesConstRef _message);
	
	/// Verify signature.
	bool verify(Public const& _p, Signature const& _sig, bytesConstRef _message, bool _hashed = false);
	
	/// Recovers public key from compact signature. Uses libsecp256k1.
	Public recover(Signature _signature, bytesConstRef _message);
	
	/// Verifies _s is a valid secret key and returns corresponding public key in o_p.
	bool verifySecret(Secret const& _s, Public& o_p);
	
	void agree(Secret const& _s, Public const& _r, Secret& o_s);

private:
	Secp256k1PP() = default;
};

}
}

