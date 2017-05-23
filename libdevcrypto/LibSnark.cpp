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
/**
 * @file LibSnark.cpp
 */

#include <libdevcrypto/LibSnark.h>

#include <algebra/curves/alt_bn128/alt_bn128_g1.hpp>
#include <algebra/curves/alt_bn128/alt_bn128_g2.hpp>
#include <algebra/curves/alt_bn128/alt_bn128_pairing.hpp>
#include <algebra/curves/alt_bn128/alt_bn128_pp.hpp>

#include <libdevcrypto/Exceptions.h>

#include <libdevcore/CommonIO.h>
#include <libdevcore/Log.h>

using namespace std;
using namespace dev;
using namespace dev::crypto;

namespace
{

void initLibSnark()
{
	// This is hackish, but we really want to use `static` variable for lock
	// free thread-safe initialization.
	static bool initialized = (libff::alt_bn128_pp::init_public_params(), true);
	(void)initialized;
}

libff::bigint<libff::alt_bn128_q_limbs> toLibsnarkBigint(h256 const& _x)
{
	libff::bigint<libff::alt_bn128_q_limbs> x;
	constexpr auto N = x.N;
	constexpr auto L = sizeof(x.data[0]);
	for (unsigned i = 0; i < N; i++)
		for (unsigned j = 0; j < L; j++)
			x.data[N - 1 - i] |= uint64_t(_x[i * L + j]) << (8 * (L - 1 - j));
	return x;
}

h256 fromLibsnarkBigint(libff::bigint<libff::alt_bn128_q_limbs> _x)
{
	constexpr auto N = _x.N;
	constexpr auto L = sizeof(_x.data[0]);
	h256 x;
	for (unsigned i = 0; i < N; i++)
		for (unsigned j = 0; j < L; j++)
			x[i * L + j] = uint8_t(uint64_t(_x.data[N - 1 - i]) >> (8 * (L - 1 - j)));
	return x;
}

libff::alt_bn128_Fq decodeFqElement(dev::bytesConstRef _data)
{
	// h256::AlignLeft ensures that the h256 is zero-filled on the right if _data
	// is too short.
	h256 xbin(_data, h256::AlignLeft);
	if (u256(xbin) >= u256(fromLibsnarkBigint(libff::alt_bn128_Fq::mod)))
		BOOST_THROW_EXCEPTION(InvalidEncoding());
	return toLibsnarkBigint(xbin);
}

libff::alt_bn128_G1 decodePointG1(dev::bytesConstRef _data)
{
	libff::alt_bn128_Fq X = decodeFqElement(_data.cropped(0));
	libff::alt_bn128_Fq Y = decodeFqElement(_data.cropped(32));
	if (X == libff::alt_bn128_Fq::zero() && Y == libff::alt_bn128_Fq::zero())
		return libff::alt_bn128_G1::zero();
	libff::alt_bn128_G1 p(X, Y, libff::alt_bn128_Fq::one());
	if (!p.is_well_formed())
		BOOST_THROW_EXCEPTION(InvalidEncoding());
	return p;
}

bytes encodePointG1(libff::alt_bn128_G1 _p)
{
	if (_p.is_zero())
		return h256().asBytes() + h256().asBytes();
	_p.to_affine_coordinates();
	return
		fromLibsnarkBigint(_p.X.as_bigint()).asBytes() +
		fromLibsnarkBigint(_p.Y.as_bigint()).asBytes();
}

libff::alt_bn128_Fq2 decodeFq2Element(dev::bytesConstRef _data)
{
	// Encoding: c1 (256 bits) c0 (256 bits)
	// "Big endian", just like the numbers
	return libff::alt_bn128_Fq2(
		decodeFqElement(_data.cropped(32)),
		decodeFqElement(_data.cropped(0))
	);
}


libff::alt_bn128_G2 decodePointG2(dev::bytesConstRef _data)
{
	libff::alt_bn128_Fq2 X = decodeFq2Element(_data);
	libff::alt_bn128_Fq2 Y = decodeFq2Element(_data.cropped(64));
	if (X == libff::alt_bn128_Fq2::zero() && Y == libff::alt_bn128_Fq2::zero())
		return libff::alt_bn128_G2::zero();
	libff::alt_bn128_G2 p(X, Y, libff::alt_bn128_Fq2::one());
	if (!p.is_well_formed())
		BOOST_THROW_EXCEPTION(InvalidEncoding());
	return p;
}

}

pair<bool, bytes> dev::crypto::alt_bn128_pairing_product(dev::bytesConstRef _in)
{
	// Input: list of pairs of G1 and G2 points
	// Output: 1 if pairing evaluates to 1, 0 otherwise (left-padded to 32 bytes)

	bool result = true;
	size_t const pairSize = 2 * 32 + 2 * 64;
	size_t const pairs = _in.size() / pairSize;
	if (pairs * pairSize != _in.size())
	{
		// Invalid length.
		return make_pair(false, bytes());
	}
	try
	{
		initLibSnark();
		libff::alt_bn128_Fq12 x = libff::alt_bn128_Fq12::one();
		for (size_t i = 0; i < pairs; ++i)
		{
			dev::bytesConstRef pair = _in.cropped(i * pairSize, pairSize);
			libff::alt_bn128_G2 p = decodePointG2(pair.cropped(2 * 32));
			if (-libff::alt_bn128_G2::scalar_field::one() * p + p != libff::alt_bn128_G2::zero())
				// p is not an element of the group (has wrong order)
				return {false, bytes()};
			x = x * libff::alt_bn128_miller_loop(
				libff::alt_bn128_precompute_G1(decodePointG1(pair)),
				libff::alt_bn128_precompute_G2(p)
			);
		}
		result = libff::alt_bn128_final_exponentiation(x) == libff::alt_bn128_GT::one();
	}
	catch (InvalidEncoding const&)
	{
		return make_pair(false, bytes());
	}
	catch (...)
	{
		cwarn << "Internal exception from libsnark. Forwarding as precompiled contract failure.";
		return make_pair(false, bytes());
	}

	bytes res(32, 0);
	res[31] = unsigned(result);
	return {true, res};
}

pair<bool, bytes> dev::crypto::alt_bn128_G1_add(dev::bytesConstRef _in)
{
	try
	{
		initLibSnark();
		libff::alt_bn128_G1 p1 = decodePointG1(_in);
		libff::alt_bn128_G1 p2 = decodePointG1(_in.cropped(32 * 2));

		return {true, encodePointG1(p1 + p2)};
	}
	catch (InvalidEncoding const&)
	{
	}
	catch (...)
	{
		cwarn << "Internal exception from libsnark. Forwarding as precompiled contract failure.";
	}
	return make_pair(false, bytes());
}

pair<bool, bytes> dev::crypto::alt_bn128_G1_mul(dev::bytesConstRef _in)
{
	try
	{
		initLibSnark();
		libff::alt_bn128_G1 p = decodePointG1(_in.cropped(0));

		libff::alt_bn128_G1 result = toLibsnarkBigint(h256(_in.cropped(64), h256::AlignLeft)) * p;

		return {true, encodePointG1(result)};
	}
	catch (InvalidEncoding const&)
	{
	}
	catch (...)
	{
		cwarn << "Internal exception from libsnark. Forwarding as precompiled contract failure.";
	}
	return make_pair(false, bytes());
}
