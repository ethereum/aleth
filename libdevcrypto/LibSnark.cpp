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
// These also need to be defined when compiling libsnark
#define BINARY_OUTPUT 1
#define MONTGOMERY_OUTPUT 1
#define NO_PT_COMPRESSION 1

#include <libsnark/algebra/curves/alt_bn128/alt_bn128_g1.hpp>
#include <libsnark/algebra/curves/alt_bn128/alt_bn128_g2.hpp>
#include <libsnark/algebra/curves/alt_bn128/alt_bn128_pairing.hpp>
#include <libsnark/algebra/curves/alt_bn128/alt_bn128_pp.hpp>
#include <libsnark/zk_proof_systems/ppzksnark/r1cs_ppzksnark/r1cs_ppzksnark.hpp>

#include <libdevcore/CommonIO.h>
#include <libdevcore/FixedHash.h>

#include <fstream>

using namespace std;
using namespace dev;
using namespace dev::snark;

namespace
{

void initLibSnark()
{
	static bool initialized = 0;
	if (!initialized)
	{
		libsnark::alt_bn128_pp::init_public_params();
		initialized = true;
	}
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
	return toLibsnarkBigint(h256(_data.cropped(0, 32)));
}

libsnark::alt_bn128_G1 decodePointG1(dev::bytesConstRef _data)
{
	return libsnark::alt_bn128_G1(
		decodeFqElement(_data.cropped(0, 32)),
		decodeFqElement(_data.cropped(32, 32)),
		decodeFqElement(_data.cropped(64, 32))
	);
}

void encodePointG1(libsnark::alt_bn128_G1 _p, dev::bytesRef _out)
{
	libsnark::alt_bn128_G1 p_norm = _p;
	p_norm.to_affine_coordinates();
	fromLibsnarkBigint(p_norm.X.as_bigint()).ref().copyTo(_out);
	fromLibsnarkBigint(p_norm.Y.as_bigint()).ref().copyTo(_out.cropped(32));
	fromLibsnarkBigint(p_norm.Z.as_bigint()).ref().copyTo(_out.cropped(64));
}

libsnark::alt_bn128_Fq2 decodeFq2Element(dev::bytesConstRef _data)
{
	// Encoding: c1 (256 bits) c0 (256 bits)
	// "Big endian", just like the numbers
	return libsnark::alt_bn128_Fq2(
		decodeFqElement(_data.cropped(32, 32)),
		decodeFqElement(_data.cropped(0, 32))
	);
}


libsnark::alt_bn128_G2 decodePointG2(dev::bytesConstRef _data)
{
	return libsnark::alt_bn128_G2(
		decodeFq2Element(_data.cropped(0, 64)),
		decodeFq2Element(_data.cropped(64, 64)),
		decodeFq2Element(_data.cropped(128, 64))
	);
}


void dev::snark::alt_bn128_pairing_product(dev::bytesConstRef _in, dev::bytesRef _out)
{
	initLibSnark();
	// Input: list of pairs of G1 and G2 points
	// Output: 1 if pairing evaluates to 1, 0 otherwise (left-padded to 32 bytes)

	// TODO catch exceptions from assertions
	// TODO check that the second points are part of the correct subgroup

	size_t const pairSize = 3 * 32 + 3 * 64;
	// TODO this does not round correctly
	size_t const pairs = _in.size() / pairSize;
	libsnark::alt_bn128_Fq12 x = libsnark::alt_bn128_Fq12::one();
	cout << "P2: ";
	libsnark::alt_bn128_G2::one().print();
	for (size_t i = 0; i < pairs; ++i)
	{
		dev::bytesConstRef pair = _in.cropped(i * pairSize, pairSize);
		x = x * libsnark::alt_bn128_miller_loop(
			libsnark::alt_bn128_precompute_G1(decodePointG1(pair)),
			libsnark::alt_bn128_precompute_G2(decodePointG2(pair.cropped(3 * 32)))
		);
	}
	bool result = libsnark::alt_bn128_final_exponentiation(x) == libsnark::alt_bn128_GT::one();

	bytes res(32, 0);
	res[31] = unsigned(result);
	ref(res).copyTo(_out);
}

void dev::snark::alt_bn128_G1_add(dev::bytesConstRef _in, dev::bytesRef _out)
{
	initLibSnark();
	// Elliptic curve point addition in Jacobian, big endian encoding:
	// (P1.X: 256 bits, P1.Y: 256 bits, P1.Z: 256 bits,
	//  P2.X: 256 bits, P2.Y: 256 bits, P2.Z: 256 bits)

	// TODO: This cannot be final code because it behaves incorrectly if
	// the input is too short (decoder returns zero instead of filling with zeros).
	libsnark::alt_bn128_G1 p1 = decodePointG1(_in);
	libsnark::alt_bn128_G1 p2 = decodePointG1(_in.cropped(32 * 3));

	encodePointG1(p1 + p2, _out);
}

void dev::snark::alt_bn128_G1_mul(dev::bytesConstRef _in, dev::bytesRef _out)
{
	initLibSnark();
	// Scalar multiplication with a curve point in Jacobian encoding, big endian:
	// (s: u256, X: 256 bits, Y: 256 bits, Z: 256 bits)

	// TODO: This cannot be final code because it behaves incorrectly if
	// the input is too short (decoder returns zero instead of filling with zeros).
	u256 s = h256(_in.cropped(0, 32));
	libsnark::alt_bn128_G1 p = decodePointG1(_in.cropped(32));

	libsnark::alt_bn128_G1 result = libsnark::alt_bn128_G1::zero();

	for (int i = 255; i >= 0; --i)
	{
		result = result.dbl();
		if (boost::multiprecision::bit_test(s, i))
			result = result + p;
	}

	encodePointG1(result, _out);
}

std::string outputPointG1Affine(libsnark::alt_bn128_G1 _p)
{
	libsnark::alt_bn128_G1 aff = _p;
	aff.to_affine_coordinates();
	return
		"Pairing.g1FromAffine(0x" +
		fromLibsnarkBigint(aff.X.as_bigint()).hex() +
		", 0x" +
		fromLibsnarkBigint(aff.Y.as_bigint()).hex() +
		")";
}

std::string outputPointG2Affine(libsnark::alt_bn128_G2 _p)
{
	libsnark::alt_bn128_G2 aff = _p;
	aff.to_affine_coordinates();
	return
		"Pairing.g2FromAffine([0x" +
		fromLibsnarkBigint(aff.X.c1.as_bigint()).hex() + ", 0x" +
		fromLibsnarkBigint(aff.X.c0.as_bigint()).hex() + "], [0x" +
		fromLibsnarkBigint(aff.Y.c1.as_bigint()).hex() + ", 0x" +
		fromLibsnarkBigint(aff.Y.c0.as_bigint()).hex() + "])";
}

void dev::snark::exportVK(string const& _VKFilename)
{
	initLibSnark();

	std::stringstream ss;
	std::ifstream fh(_VKFilename, std::ios::binary);

	if (!fh.is_open())
		throw std::runtime_error((boost::format("could not load param file at %s") % _VKFilename).str());

	ss << fh.rdbuf();
	fh.close();

	ss.rdbuf()->pubseekpos(0, std::ios_base::in);

	libsnark::r1cs_ppzksnark_verification_key<libsnark::alt_bn128_pp> verificationKey;
	ss >> verificationKey;

	unsigned icLength = verificationKey.encoded_IC_query.rest.indices.size() + 1;


	cout << "\tfunction verifyingKey() internal returns (VerifyingKey vk) {" << endl;
	cout << "\t\tvk.A = " << outputPointG2Affine(verificationKey.alphaA_g2) << ";" << endl;
	cout << "\t\tvk.B = " << outputPointG1Affine(verificationKey.alphaB_g1) << ";" << endl;
	cout << "\t\tvk.C = " << outputPointG2Affine(verificationKey.alphaC_g2) << ";" << endl;
	cout << "\t\tvk.gamma = " << outputPointG2Affine(verificationKey.gamma_g2) << ";" << endl;
	cout << "\t\tvk.gammaBeta1 = " << outputPointG1Affine(verificationKey.gamma_beta_g1) << ";" << endl;
	cout << "\t\tvk.gammaBeta2 = " << outputPointG2Affine(verificationKey.gamma_beta_g2) << ";" << endl;
	cout << "\t\tvk.Z = " << outputPointG2Affine(verificationKey.rC_Z_g2) << ";" << endl;
	cout << "\t\tvk.IC = new Pairing.G1Point[](" << icLength << ");" << endl;
	cout << "\t\tvk.IC[0] = " << outputPointG1Affine(verificationKey.encoded_IC_query.first) << ";" << endl;
	for (size_t i = 1; i < icLength; ++i)
	{
		auto vkICi = outputPointG1Affine(verificationKey.encoded_IC_query.rest.values[i - 1]);
		cout << "\t\tvk.IC[" << i << "] = " << vkICi << ";" << endl;
	}
	cout << "\t\t}" << endl;
	cout << "\tfunction verify(uint[] input, Proof proof) internal returns (bool) {" << endl;
	cout << "\t\tVerifyingKey memory vk = verifyingKey();" << endl;
	cout << "\t\tif (input.length + 1 != vk.IC.length) throw;" << endl;
	cout << "\t\t// Compute the linear combination vk_x" << endl;
	cout << "\t\tPairing.G1Point memory vk_x = vk.IC[0];" << endl;
	for (size_t i = 0; i < verificationKey.encoded_IC_query.rest.indices.size(); ++i)
		cout << "\t\tvk_x = Pairing.add(vk_x, Pairing.mul(vk.IC[" << (i + 1) << "], input[" << i << "]));" << endl;
	cout << "\t\tif (!Pairing.pairingProd2(proof.A, vk.A, Pairing.negate(proof.A_p), Pairing.P2())) return false;" << endl;
	cout << "\t\tif (!Pairing.pairingProd2(vk.B, proof.B, Pairing.negate(proof.B_p), Pairing.P2())) return false;" << endl;
	cout << "\t\tif (!Pairing.pairingProd2(proof.C, vk.C, Pairing.negate(proof.C_p), Pairing.P2())) return false;" << endl;
	cout << "\t\tif (!Pairing.pairingProd3(proof.K, vk.gamma, Pairing.negate(Pairing.add(vk_x, Pairing.add(proof.A, proof.C))), vk.gammaBeta2, Pairing.negate(vk.gammaBeta1), proof.B)) return false;" << endl;
	cout << "\t\tif (!Pairing.pairingProd3(" << endl;
	cout << "\t\t\t\tPairing.add(vk_x, proof.A), proof.B," << endl;
	cout << "\t\t\t\tPairing.negate(proof.H), vk.Z," << endl;
	cout << "\t\t\t\tPairing.negate(proof.C), Pairing.P2()" << endl;
	cout << "\t\t)) return false;" << endl;
	cout << "\t\treturn true;" << endl;
	cout << "\t}" << endl;
}
