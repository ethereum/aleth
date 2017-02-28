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
/** @file LibSnark.cpp
 * @date 2017
 * Tests for libsnark integration.
 */

#include <libdevcrypto/LibSnark.h>

#include <libdevcore/CommonIO.h>

#include <boost/test/unit_test.hpp>

using namespace std;
using namespace dev;
using namespace dev::crypto;

BOOST_AUTO_TEST_SUITE(LibSnark)

namespace dev
{
namespace test
{

// ecadd, ecmul, encoding decoding

BOOST_AUTO_TEST_CASE(ecadd)
{
	// "0 + 0 == 0"
	bytes input(0x20 * 4, 0);
	auto result = alt_bn128_G1_add(ref(input));
	BOOST_CHECK(result.first);
	BOOST_CHECK_EQUAL(result.second, input);
	// The same, truncated.
	bytes empty;
	result = alt_bn128_G1_add(ref(empty));
	BOOST_CHECK(result.first);
	BOOST_CHECK(result.second == input);
}

}
}

BOOST_AUTO_TEST_SUITE_END()

#if 0

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


void testProof(libsnark::r1cs_ppzksnark_verification_key<libsnark::alt_bn128_pp> const& _vk)
{
	using ppT = libsnark::alt_bn128_pp;
	using Fq = libsnark::alt_bn128_Fq;
	using Fq2 = libsnark::alt_bn128_Fq2;
	using G1 = libsnark::alt_bn128_G1;
	using G2 = libsnark::alt_bn128_G2;
	libsnark::r1cs_ppzksnark_primary_input<ppT> primary_input;
	libsnark::r1cs_ppzksnark_proof<ppT> proof;

	primary_input.emplace_back("13986731495506593864492662381614386532349950841221768152838255933892789078521");
	primary_input.emplace_back("622860516154313070522697309645122400675542217310916019527100517240519630053");
	primary_input.emplace_back("11094488463398718754251685950409355128550342438297986977413505294941943071569");
	primary_input.emplace_back("6627643779954497813586310325594578844876646808666478625705401786271515864467");
	primary_input.emplace_back("2957286918163151606545409668133310005545945782087581890025685458369200827463");
	primary_input.emplace_back("1384290496819542862903939282897996566903332587607290986044945365745128311081");
	primary_input.emplace_back("5613571677741714971687805233468747950848449704454346829971683826953541367271");
	primary_input.emplace_back("9643208548031422463313148630985736896287522941726746581856185889848792022807");
	primary_input.emplace_back("18066496933330839731877828156604");

	proof.g_A.g = G1(
		Fq("12873740738727497448187997291915224677121726020054032516825496230827252793177"),
		Fq("21804419174137094775122804775419507726154084057848719988004616848382402162497"),
		Fq::one()
	);
	proof.g_A.h = G1(
		Fq("7742452358972543465462254569134860944739929848367563713587808717088650354556"),
		Fq("7324522103398787664095385319014038380128814213034709026832529060148225837366"),
		Fq::one()
	);
	proof.g_B.g = G2(
		Fq2(
			Fq("15588556568726919713003060429893850972163943674590384915350025440408631945055"),
			Fq("8176651290984905087450403379100573157708110416512446269839297438960217797614")
		),
		Fq2(
			Fq("4265071979090628150845437155927259896060451682253086069461962693761322642015"),
			Fq("15347511022514187557142999444367533883366476794364262773195059233657571533367")
		),
		Fq2::one()
	);
	proof.g_B.h = G1(
		Fq("2979746655438963305714517285593753729335852012083057917022078236006592638393"),
		Fq("6470627481646078059765266161088786576504622012540639992486470834383274712950"),
		Fq::one()
	);
	proof.g_C.g = G1(
		Fq("6851077925310461602867742977619883934042581405263014789956638244065803308498"),
		Fq("10336382210592135525880811046708757754106524561907815205241508542912494488506"),
		Fq::one()
	);
	proof.g_C.h = G1(
		Fq("12491625890066296859584468664467427202390981822868257437245835716136010795448"),
		Fq("13818492518017455361318553880921248537817650587494176379915981090396574171686"),
		Fq::one()
	);
	proof.g_H = G1(
		Fq("12091046215835229523641173286701717671667447745509192321596954139357866668225"),
		Fq("14446807589950902476683545679847436767890904443411534435294953056557941441758"),
		Fq::one()
	);
	proof.g_K = G1(
		Fq("21341087976609916409401737322664290631992568431163400450267978471171152600502"),
		Fq("2942165230690572858696920423896381470344658299915828986338281196715687693170"),
		Fq::one()
	);


	cout << "Verifying..." << endl;
	bool verified = libsnark::r1cs_ppzksnark_verifier_strong_IC(_vk, primary_input, proof);
	cout << "Verified: " << verified << endl;
}

void dev::crypto::exportVK(string const& _VKFilename)
{
	initLibSnark();

	std::stringstream ss;
	std::ifstream fh(_VKFilename, std::ios::binary);

	if (!fh.is_open())
		throw std::runtime_error("could not load param file at " +  _VKFilename);

	ss << fh.rdbuf();
	fh.close();

	ss.rdbuf()->pubseekpos(0, std::ios_base::in);

	libsnark::r1cs_ppzksnark_verification_key<libsnark::alt_bn128_pp> verificationKey;
	ss >> verificationKey;

//	testProof(verificationKey);
//	return;

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
#endif
