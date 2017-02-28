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
// libsnark has to be compiled with exactly the same switches:
// sudo PREFIX=/usr/local make NO_PROCPS=1 NO_GTEST=1 NO_DOCS=1 CURVE=ALT_BN128 FEATUREFLAGS="-DBINARY_OUTPUT=1 -DMONTGOMERY_OUTPUT=1 -DNO_PT_COMPRESSION=1" lib install
#define BINARY_OUTPUT 1
#define MONTGOMERY_OUTPUT 1
#define NO_PT_COMPRESSION 1

#include <libsnark/algebra/curves/alt_bn128/alt_bn128_g1.hpp>
#include <libsnark/algebra/curves/alt_bn128/alt_bn128_g2.hpp>
#include <libsnark/algebra/curves/alt_bn128/alt_bn128_pairing.hpp>
#include <libsnark/algebra/curves/alt_bn128/alt_bn128_pp.hpp>
#include <libsnark/zk_proof_systems/ppzksnark/r1cs_ppzksnark/r1cs_ppzksnark.hpp>

#include <libdevcrypto/Exceptions.h>

#include <libdevcore/CommonIO.h>
#include <libdevcore/Log.h>
#include <libdevcore/FixedHash.h>

#include <fstream>

using namespace std;
using namespace dev;
using namespace dev::crypto;

namespace
{

void initLibSnark()
{
	static bool initialized = 0;
	if (!initialized)
	{
		libsnark::alt_bn128_pp::init_public_params();
		// Otherwise the library would output profiling info for each run.
		libsnark::inhibit_profiling_counters = true;
		libsnark::inhibit_profiling_info = true;
		initialized = true;
	}
}

libsnark::bigint<libsnark::alt_bn128_q_limbs> toLibsnarkBigint(h256 const& _x)
{
	libsnark::bigint<libsnark::alt_bn128_q_limbs> x;
	for (unsigned i = 0; i < 4; i++)
		for (unsigned j = 0; j < 8; j++)
			x.data[3 - i] |= uint64_t(_x[i * 8 + j]) << (8 * (7 - j));
	return x;
}

h256 fromLibsnarkBigint(libsnark::bigint<libsnark::alt_bn128_q_limbs> _x)
{
	h256 x;
	for (unsigned i = 0; i < 4; i++)
		for (unsigned j = 0; j < 8; j++)
			x[i * 8 + j] = uint8_t(uint64_t(_x.data[3 - i]) >> (8 * (7 - j)));
	return x;
}

libsnark::alt_bn128_Fq decodeFqElement(dev::bytesConstRef _data)
{
	// h256::AlignLeft ensures that the h256 is zero-filled on the right if _data
	// is too short.
	h256 xbin(_data, h256::AlignLeft);
	if (u256(xbin) >= u256(fromLibsnarkBigint(libsnark::alt_bn128_Fq::mod)))
		BOOST_THROW_EXCEPTION(InvalidEncoding());
	return toLibsnarkBigint(xbin);
}

libsnark::alt_bn128_G1 decodePointG1(dev::bytesConstRef _data)
{
	libsnark::alt_bn128_Fq X = decodeFqElement(_data.cropped(0));
	libsnark::alt_bn128_Fq Y = decodeFqElement(_data.cropped(32));
	if (X == libsnark::alt_bn128_Fq::zero() && Y == libsnark::alt_bn128_Fq::zero())
		return libsnark::alt_bn128_G1::zero();
	libsnark::alt_bn128_G1 p(X, Y, libsnark::alt_bn128_Fq::one());
	if (!p.is_well_formed())
		BOOST_THROW_EXCEPTION(InvalidEncoding());
	return p;
}

bytes encodePointG1(libsnark::alt_bn128_G1 _p)
{
	if (_p.is_zero())
		return h256().asBytes() + h256().asBytes();
	_p.to_affine_coordinates();
	return
		fromLibsnarkBigint(_p.X.as_bigint()).asBytes() +
		fromLibsnarkBigint(_p.Y.as_bigint()).asBytes();
}

libsnark::alt_bn128_Fq2 decodeFq2Element(dev::bytesConstRef _data)
{
	// Encoding: c1 (256 bits) c0 (256 bits)
	// "Big endian", just like the numbers
	return libsnark::alt_bn128_Fq2(
		decodeFqElement(_data.cropped(32)),
		decodeFqElement(_data.cropped(0))
	);
}


libsnark::alt_bn128_G2 decodePointG2(dev::bytesConstRef _data)
{
	libsnark::alt_bn128_Fq2 X = decodeFq2Element(_data);
	libsnark::alt_bn128_Fq2 Y = decodeFq2Element(_data.cropped(64));
	if (X == libsnark::alt_bn128_Fq2::zero() && Y == libsnark::alt_bn128_Fq2::zero())
		return libsnark::alt_bn128_G2::zero();
	libsnark::alt_bn128_G2 p(X, Y, libsnark::alt_bn128_Fq2::one());
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
		libsnark::alt_bn128_Fq12 x = libsnark::alt_bn128_Fq12::one();
		for (size_t i = 0; i < pairs; ++i)
		{
			dev::bytesConstRef pair = _in.cropped(i * pairSize, pairSize);
			libsnark::alt_bn128_G2 p = decodePointG2(pair.cropped(2 * 32));
			if (-libsnark::alt_bn128_G2::scalar_field::one() * p + p != libsnark::alt_bn128_G2::zero())
				// p is not an element of the group (has wrong order)
				return {false, bytes()};
			x = x * libsnark::alt_bn128_miller_loop(
				libsnark::alt_bn128_precompute_G1(decodePointG1(pair)),
				libsnark::alt_bn128_precompute_G2(p)
			);
		}
		result = libsnark::alt_bn128_final_exponentiation(x) == libsnark::alt_bn128_GT::one();
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
		libsnark::alt_bn128_G1 p1 = decodePointG1(_in);
		libsnark::alt_bn128_G1 p2 = decodePointG1(_in.cropped(32 * 2));

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
		libsnark::alt_bn128_G1 p = decodePointG1(_in.cropped(0));

		libsnark::alt_bn128_G1 result = toLibsnarkBigint(h256(_in.cropped(64), h256::AlignLeft)) * p;

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
