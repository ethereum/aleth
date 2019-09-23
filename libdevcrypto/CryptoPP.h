// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
/** 
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

private:
	Secp256k1PP() = default;
};

}
}

