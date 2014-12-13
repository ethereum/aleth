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
/** @file ProofOfWork.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#if !ETH_LANGUAGES

#include <boost/detail/endian.hpp>
#include <chrono>
#include <array>
#include <random>
#include <thread>
#include <libdevcrypto/CryptoPP.h>
#include <libdevcore/Common.h>
#include "ProofOfWork.h"
using namespace std;
using namespace std::chrono;

namespace dev
{
namespace eth
{

template <class _T>
static inline void update(_T& _sha, u256 const& _value)
{
	int i = 0;
	for (u256 v = _value; v; ++i, v >>= 8) {}
	byte buf[32];
	bytesRef bufRef(buf, i);
	toBigEndian(_value, bufRef);
	_sha.Update(buf, i);
}

template <class _T>
static inline void update(_T& _sha, h256 const& _value)
{
	int i = 0;
	byte const* data = _value.data();
	for (; i != 32 && data[i] == 0; ++i);
	_sha.Update(data + i, 32 - i);
}

template <class _T>
static inline h256 get(_T& _sha)
{
	h256 ret;
	_sha.TruncatedFinal(&ret[0], 32);
	return ret;
}

h256 DaggerEvaluator::node(h256 const& _root, h256 const& _xn, uint_fast32_t _L, uint_fast32_t _i)
{
	if (_L == _i)
		return _root;
	u256 m = (_L == 9) ? 16 : 3;
	CryptoPP::SHA3_256 bsha;
	for (uint_fast32_t k = 0; k < m; ++k)
	{
		CryptoPP::SHA3_256 sha;
		update(sha, _root);
		update(sha, _xn);
		update(sha, (u256)_L);
		update(sha, (u256)_i);
		update(sha, (u256)k);
		uint_fast32_t pk = (uint_fast32_t)(u256)get(sha) & ((1 << ((_L - 1) * 3)) - 1);
		auto u = node(_root, _xn, _L - 1, pk);
		update(bsha, u);
	}
	return get(bsha);
}

h256 DaggerEvaluator::eval(h256 const& _root, h256 const& _nonce)
{
	h256 extranonce = (u256)_nonce >> 26;				// with xn = floor(n / 2^26) -> assuming this is with xn = floor(N / 2^26)
	CryptoPP::SHA3_256 bsha;
	for (uint_fast32_t k = 0; k < 4; ++k)
	{
		//sha256(D || xn || i || k)		-> sha256(D || xn || k)	- there's no 'i' here!
		CryptoPP::SHA3_256 sha;
		update(sha, _root);
		update(sha, extranonce);
		update(sha, _nonce);
		update(sha, (u256)k);
		uint_fast32_t pk = (uint_fast32_t)(u256)get(sha) & 0x1ffffff;	// mod 8^8 * 2  [ == mod 2^25 ?! ] [ == & ((1 << 25) - 1) ] [ == & 0x1ffffff ]
		auto u = node(_root, extranonce, 9, pk);
		update(bsha, u);
	}
	return get(bsha);
}

}
}
#endif
