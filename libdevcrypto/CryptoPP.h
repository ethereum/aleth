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

#include <mutex>

#if defined(_MSC_VER)
	#pragma warning(push)
	// Compiler Warning (level 4) C4100 - 'identifier' : unreferenced formal parameter
	#pragma warning(disable:4100)
	// Compiler Warning (levels 3 and 4) C4244 - 'conversion' conversion from 'type1' to 'type2', possible loss of data
	#pragma warning(disable:4244)
	// Compiler Warning (level 1) C4297 - 'function' : function assumed not to throw an exception but does
	#pragma warning(disable:4297)
#endif // defined(_MSC_VER)

#if defined(__GNUC__)
	#pragma GCC diagnostic push
	// Warn if a prototype causes a type conversion that is different from what would happen to the same argument in the absence of a prototype.
	#pragma GCC diagnostic ignored "-Wconversion"
	// Deleting object of polymorphic class type 'Base' which has non-virtual destructor might cause undefined behaviour
	#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
	// This enables some extra warning flags that are not enabled by -Wall
	#pragma GCC diagnostic ignored "-Wextra"
	// Warn whenever a static function is declared but not defined or a non-inline static function is unused
	#pragma GCC diagnostic ignored "-Wunused-function"
	// Warn whenever a function parameter is unused aside from its declaration.
	#pragma GCC diagnostic ignored "-Wunused-parameter"
	// Warn whenever a local or static variable is unused aside from its declaration.
	#pragma GCC diagnostic ignored "-Wunused-variable"
#endif // defined(__GNUC__)

#include <cryptopp/sha.h>
#include <cryptopp/sha3.h>
#include <cryptopp/ripemd.h>
#include <cryptopp/aes.h>
#include <cryptopp/pwdbased.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/ecp.h>
#include <cryptopp/files.h>
#include <cryptopp/osrng.h>
#include <cryptopp/oids.h>
#include <cryptopp/dsa.h>

#if defined(__GNUC__)
	#pragma GCC diagnostic pop
#endif // defined(__GNUC__)

#if defined(_MSC_VER)
	#pragma warning(pop)
#endif // defined(_MSC_VER)

#include <libdevcore/SHA3.h>
#include "Common.h"

namespace dev
{
namespace crypto
{

using namespace CryptoPP;

inline ECP::Point publicToPoint(Public const& _p) { Integer x(_p.data(), 32); Integer y(_p.data() + 32, 32); return ECP::Point(x,y); }

inline Integer secretToExponent(Secret const& _s) { return std::move(Integer(_s.data(), Secret::size)); }

/// Amount of bytes added when encrypting with encryptECIES.
static const unsigned c_eciesOverhead = 113;

/**
 * CryptoPP secp256k1 algorithms.
 * @todo Collect ECIES methods into class.
 */
class Secp256k1PP
{	
public:
	static Secp256k1PP* get() { if (!s_this) s_this = new Secp256k1PP; return s_this; }

	void toPublic(Secret const& _s, Public& o_public) { exponentToPublic(Integer(_s.data(), sizeof(_s)), o_public); }
	
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
	
protected:
	void exportPrivateKey(DL_PrivateKey_EC<ECP> const& _k, Secret& o_s) { _k.GetPrivateExponent().Encode(o_s.writable().data(), Secret::size); }
	
	void exportPublicKey(DL_PublicKey_EC<ECP> const& _k, Public& o_p);
	
	void exponentToPublic(Integer const& _e, Public& o_p);
	
	template <class T> void initializeDLScheme(Secret const& _s, T& io_operator) { std::lock_guard<std::mutex> l(x_params); io_operator.AccessKey().Initialize(m_params, secretToExponent(_s)); }
	
	template <class T> void initializeDLScheme(Public const& _p, T& io_operator) { std::lock_guard<std::mutex> l(x_params); io_operator.AccessKey().Initialize(m_params, publicToPoint(_p)); }
	
private:
	Secp256k1PP(): m_oid(ASN1::secp256k1()), m_params(m_oid), m_curve(m_params.GetCurve()), m_q(m_params.GetGroupOrder()), m_qs(m_params.GetSubgroupOrder()) {}

	OID m_oid;
	
	std::mutex x_rng;
	AutoSeededRandomPool m_rng;
	
	std::mutex x_params;
	DL_GroupParameters_EC<ECP> m_params;
	
	std::mutex x_curve;
	DL_GroupParameters_EC<ECP>::EllipticCurve m_curve;
	
	Integer m_q;
	Integer m_qs;

	static Secp256k1PP* s_this;
};

}
}

