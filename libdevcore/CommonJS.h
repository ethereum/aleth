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
/** @file CommonJS.h
 * @authors:
 *   Gav Wood <i@gavwood.com>
 *   Marek Kotewicz <marek@ethdev.com>
 * @date 2014
 */

#pragma once

#include <string>
#include <libethereum/Interface.h>
#include "Common.h"
#include "CommonData.h"

namespace dev
{

template <unsigned S> std::string toJS(FixedHash<S> const& _h)
{
	return "0x" + toHex(_h.ref());
}
	
template <unsigned N> std::string toJS(boost::multiprecision::number<boost::multiprecision::cpp_int_backend<N, N, boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>> const& _n)
{
	return "0x" + toHex(toCompactBigEndian(_n));
}
	
inline std::string toJS(dev::bytes const& _n)
{
	return "0x" + dev::toHex(_n);
}

bytes jsToBytes(std::string const& _s);
std::string jsPadded(std::string const& _s, unsigned _l, unsigned _r);
std::string jsPadded(std::string const& _s, unsigned _l);
std::string jsUnpadded(std::string _s);

template <unsigned N> FixedHash<N> jsToFixed(std::string const& _s)
{
	if (_s.substr(0, 2) == "0x")
		// Hex
		return FixedHash<N>(_s.substr(2));
	else if (_s.find_first_not_of("0123456789") == std::string::npos)
		// Decimal
		return (typename FixedHash<N>::Arith)(_s);
	else
		// Binary
		return FixedHash<N>(asBytes(jsPadded(_s, N)));
}

inline std::string jsToFixed(double _s)
{
	return toJS(dev::u256(_s * (double)(dev::u256(1) << 128)));
}

template <unsigned N> boost::multiprecision::number<boost::multiprecision::cpp_int_backend<N * 8, N * 8, boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>> jsToInt(std::string const& _s)
{
	if (_s.substr(0, 2) == "0x")
		// Hex
		return fromBigEndian<boost::multiprecision::number<boost::multiprecision::cpp_int_backend<N * 8, N * 8, boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>>>(fromHex(_s.substr(2)));
	else if (_s.find_first_not_of("0123456789") == std::string::npos)
		// Decimal
		return boost::multiprecision::number<boost::multiprecision::cpp_int_backend<N * 8, N * 8, boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>>(_s);
	else
		// Binary
		return fromBigEndian<boost::multiprecision::number<boost::multiprecision::cpp_int_backend<N * 8, N * 8, boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>>>(asBytes(jsPadded(_s, N)));
}

inline Address jsToAddress(std::string const& _s) { return jsToFixed<sizeof(dev::Address)>(_s); }
inline Public jsToPublic(std::string const& _s) { return jsToFixed<sizeof(dev::Public)>(_s); }
inline Secret jsToSecret(std::string const& _s) { return jsToFixed<sizeof(dev::Secret)>(_s); }
inline u256 jsToU256(std::string const& _s) { return jsToInt<32>(_s); }

inline std::string jsToBinary(std::string const& _s)
{
	return jsUnpadded(dev::toString(jsToBytes(_s)));
}

inline std::string jsToDecimal(std::string const& _s)
{
	return dev::toString(jsToU256(_s));
}

inline std::string jsFromBinary(dev::bytes _s, unsigned _padding = 32)
{
	_s.resize(std::max<unsigned>(_s.size(), _padding));
	return "0x" + dev::toHex(_s);
}

inline std::string jsFromBinary(std::string const& _s, unsigned _padding = 32)
{
	return jsFromBinary(asBytes(_s), _padding);
}

inline double jsFromFixed(std::string const& _s)
{
	return (double)jsToU256(_s) / (double)(dev::u256(1) << 128);
}

struct TransactionSkeleton
{
	Address from;
	Address to;
	u256 value;
	bytes data;
	u256 gas;
	u256 gasPrice;
};

}
