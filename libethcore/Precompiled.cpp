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
/** @file Precompiled.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "Precompiled.h"
#include <libdevcore/Log.h>
#include <libdevcore/SHA3.h>
#include <libdevcrypto/Hash.h>
#include <libdevcrypto/Common.h>
#include <libethcore/Common.h>
using namespace std;
using namespace dev;
using namespace dev::eth;

PrecompiledRegistrar* PrecompiledRegistrar::s_this = nullptr;

PrecompiledExecutor const& PrecompiledRegistrar::executor(std::string const& _name)
{
	if (!get()->m_execs.count(_name))
		BOOST_THROW_EXCEPTION(ExecutorNotFound());
	return get()->m_execs[_name];
}

PrecompiledPricer const& PrecompiledRegistrar::pricer(std::string const& _name)
{
	if (!get()->m_pricers.count(_name))
		BOOST_THROW_EXCEPTION(PricerNotFound());
	return get()->m_pricers[_name];
}

namespace
{

ETH_REGISTER_PRECOMPILED(ecrecover)(bytesConstRef _in)
{
	struct
	{
		h256 hash;
		h256 v;
		h256 r;
		h256 s;
	} in;

	memcpy(&in, _in.data(), min(_in.size(), sizeof(in)));

	h256 ret;
	u256 v = (u256)in.v;
	if (v >= 27 && v <= 28)
	{
		SignatureStruct sig(in.r, in.s, (byte)((int)v - 27));
		if (sig.isValid())
		{
			try
			{
				if (Public rec = recover(sig, in.hash))
				{
					ret = dev::sha3(rec);
					memset(ret.data(), 0, 12);
					return {true, ret.asBytes()};
				}
			}
			catch (...) {}
		}
	}
	return {true, {}};
}

ETH_REGISTER_PRECOMPILED(sha256)(bytesConstRef _in)
{
	return {true, dev::sha256(_in).asBytes()};
}

ETH_REGISTER_PRECOMPILED(ripemd160)(bytesConstRef _in)
{
	return {true, h256(dev::ripemd160(_in), h256::AlignRight).asBytes()};
}

ETH_REGISTER_PRECOMPILED(identity)(bytesConstRef _in)
{
	return {true, _in.toBytes()};
}

template<class T>
T parseBigEndianRightPadded(bytesConstRef _in, size_t _begin, size_t _count)
{
	if (_begin > _in.count())
		return 0;

	bytesConstRef cropped = _in.cropped(_begin, min(_count, _in.count() - _begin));

	T ret = fromBigEndian<T>(cropped);
	ret <<= 8 * (_count - cropped.count());
	
	return ret;
}

ETH_REGISTER_PRECOMPILED(modexp)(bytesConstRef _in)
{
	size_t const baseLength(parseBigEndianRightPadded<u256>(_in, 0, 32));
	size_t const expLength(parseBigEndianRightPadded<u256>(_in, 32, 32));
	size_t const modLength(parseBigEndianRightPadded<u256>(_in, 64, 32));

	bigint const base(parseBigEndianRightPadded<bigint>(_in, 96, baseLength));
	bigint const exp(parseBigEndianRightPadded<bigint>(_in, 96 + baseLength, expLength));
	bigint const mod(parseBigEndianRightPadded<bigint>(_in, 96 + baseLength + expLength, modLength));

	bigint const result = boost::multiprecision::powm(base, exp, mod);
	
	bytes ret(modLength); 
	toBigEndian(result, ret);

	return {true, ret};
}

ETH_REGISTER_PRECOMPILED_PRICER(modexp)(bytesConstRef _in)
{
	u256 const baseLength(parseBigEndianRightPadded<u256>(_in, 0, 32));
	u256 const expLength(parseBigEndianRightPadded<u256>(_in, 32, 32));
	u256 const modLength(parseBigEndianRightPadded<u256>(_in, 64, 32));

	bigint const maxLength = max(modLength, baseLength);

	return maxLength * maxLength * max<bigint>(expLength, 1) / 20;
}

}
