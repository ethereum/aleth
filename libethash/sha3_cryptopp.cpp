/*
  This file is part of ethash.

  ethash is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  ethash is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with ethash.  If not, see <http://www.gnu.org/licenses/>.
*/

/** @file sha3.cpp
* @author Tim Hughes <tim@twistedfury.com>
* @date 2015
*/
#include <stdint.h>

#if defined(__GNUC__)
    // Do not warn about uses of functions (see Function Attributes), variables
    // (see Variable Attributes), and types (see Type Attributes) marked as
    // deprecated by using the deprecated attribute.
    //
    // Specifically we are suppressing the warnings from the deprecation
    // attributes added to the SHA3_256 and SHA3_512 classes in CryptoPP
    // after the 5.6.3 release.
    //
    // From that header file ...
    //
    // "The Crypto++ SHA-3 implementation dates back to January 2013 when NIST
    // selected Keccak as SHA-3. In August 2015 NIST finalized SHA-3, and it
    // was a modified version of the Keccak selection. Crypto++ 5.6.2 through
    // 5.6.4 provides the pre-FIPS 202 version of SHA-3; while Crypto++ 5.7
    // and above provides the FIPS 202 version of SHA-3.
    //
    // See also http://en.wikipedia.org/wiki/SHA-3
    //
    // This means that we will never be able to move to the CryptoPP-5.7.x
    // series of releases, because Ethereum requires Keccak, not the final
    // SHA-3 standard algorithm.   We are planning to migrate cpp-ethereum
    // off CryptoPP anyway, so this is unlikely to be a long-standing issue.
    //
    // https://github.com/ethereum/cpp-ethereum/issues/3088
    //
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif // defined(__GNUC__)

#include <cryptopp/sha3.h>

extern "C" {
struct ethash_h256;
typedef struct ethash_h256 ethash_h256_t;
void SHA3_256(ethash_h256_t const* ret, uint8_t const* data, size_t size)
{
	CryptoPP::SHA3_256().CalculateDigest((uint8_t*)ret, data, size);
}

void SHA3_512(uint8_t* const ret, uint8_t const* data, size_t size)
{
	CryptoPP::SHA3_512().CalculateDigest(ret, data, size);
}
}

#if defined(__GNUC__)
  	#pragma GCC diagnostic pop
#endif // defined(__GNUC__)
