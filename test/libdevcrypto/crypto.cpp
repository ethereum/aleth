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
/** @file crypto.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 * Crypto test functions.
 */

#if ETH_HAVE_SECP256K1
#include <secp256k1/include/secp256k1.h>
#endif
#include <libdevcore/Common.h>
#include <libdevcore/RLP.h>
#include <libdevcore/Log.h>
#include <boost/test/unit_test.hpp>
#include <libdevcore/SHA3.h>
#include <libdevcrypto/ECDHE.h>
#include <libdevcrypto/CryptoPP.h>
#include <test/TestHelper.h>

using namespace std;
using namespace dev;
using namespace dev::test;
using namespace dev::crypto;
using namespace CryptoPP;

struct DevcryptoTestFixture: public TestOutputHelper {
	DevcryptoTestFixture() : s_secp256k1(Secp256k1PP::get()) {}
	~DevcryptoTestFixture() {}

	Secp256k1PP* s_secp256k1;
};
BOOST_FIXTURE_TEST_SUITE(devcrypto, DevcryptoTestFixture)
static CryptoPP::AutoSeededRandomPool s_rng;
static CryptoPP::OID s_curveOID(CryptoPP::ASN1::secp256k1());
static CryptoPP::DL_GroupParameters_EC<CryptoPP::ECP> s_params(s_curveOID);
static CryptoPP::DL_GroupParameters_EC<CryptoPP::ECP>::EllipticCurve s_curve(s_params.GetCurve());

BOOST_AUTO_TEST_CASE(sha3general)
{
	BOOST_REQUIRE_EQUAL(sha3(""), h256("c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470"));
	BOOST_REQUIRE_EQUAL(sha3("hello"), h256("1c8aff950685c2ed4bc3174f3472287b56d9517b9c948127319a09a7a36deac8"));
}

BOOST_AUTO_TEST_CASE(emptySHA3Types)
{
	h256 emptySHA3(fromHex("c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470"));
	BOOST_REQUIRE_EQUAL(emptySHA3, EmptySHA3);

	h256 emptyListSHA3(fromHex("1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347"));
	BOOST_REQUIRE_EQUAL(emptyListSHA3, EmptyListSHA3);
}

#if ETH_HAVE_SECP256K1
BOOST_AUTO_TEST_CASE(secp256k1lib)
{
	KeyPair k = KeyPair::create();
	BOOST_REQUIRE(!!k.sec());
	BOOST_REQUIRE(!!k.pub());
	Public test = toPublic(k.sec());
	BOOST_REQUIRE(k.pub() == test);
}
#endif

BOOST_AUTO_TEST_CASE(cryptopp_patch)
{
	KeyPair k = KeyPair::create();
	bytes io_text;
	s_secp256k1->decrypt(k.sec(), io_text);
	BOOST_REQUIRE_EQUAL(io_text.size(), 0);
}

BOOST_AUTO_TEST_CASE(verify_secert)
{
	Secret empty;
	KeyPair kNot(empty);
	BOOST_REQUIRE(!kNot.address());
	KeyPair k(sha3(empty));
	BOOST_REQUIRE(k.address());
}

BOOST_AUTO_TEST_CASE(common_encrypt_decrypt)
{
	string message("Now is the time for all good persons to come to the aid of humanity.");
	bytes m = asBytes(message);
	bytesConstRef bcr(&m);

	KeyPair k = KeyPair::create();
	bytes cipher;
	encrypt(k.pub(), bcr, cipher);
	BOOST_REQUIRE(cipher != asBytes(message) && cipher.size() > 0);
	
	bytes plain;
	decrypt(k.sec(), bytesConstRef(&cipher), plain);
	
	BOOST_REQUIRE(asString(plain) == message);
	BOOST_REQUIRE(plain == asBytes(message));
}

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

BOOST_AUTO_TEST_CASE(cryptopp_cryptopp_secp256k1libport)
{
#if ETH_HAVE_SECP256K1
	secp256k1_context_t* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
#endif
	
	// base secret
	Secret secret(sha3("privacy"));
	
	// we get ec params from signer
	ECDSA<ECP, SHA3_256>::Signer signer;
	
	// e := sha3(msg)
	bytes e(fromHex("0x01"));
	e.resize(32);
	int tests = 2;
	while (sha3(&e, &e), secret = sha3(secret), tests--)
	{
		KeyPair key(secret);
		Public pkey = key.pub();
		signer.AccessKey().Initialize(s_params, secretToExponent(secret));
		
		h256 he(sha3(e));
		Integer heInt(he.asBytes().data(), 32);
		h256 k(crypto::kdf(secret, he));
		Integer kInt(k.asBytes().data(), 32);
		kInt %= s_params.GetSubgroupOrder()-1;

		ECP::Point rp = s_params.ExponentiateBase(kInt);
		Integer const& q = s_params.GetGroupOrder();
		Integer r = s_params.ConvertElementToInteger(rp);

		Integer kInv = kInt.InverseMod(q);
		Integer s = (kInv * (Integer(secret.data(), 32) * r + heInt)) % q;
		BOOST_REQUIRE(!!r && !!s);

		Signature sig;
		sig[64] = rp.y.IsOdd() ? 1 : 0;
		r.Encode(sig.data(), 32);
		s.Encode(sig.data() + 32, 32);

		Public p = dev::recover(sig, he);
		BOOST_REQUIRE(p == pkey);
		
		// verify w/cryptopp
		BOOST_REQUIRE(s_secp256k1->verify(pkey, sig, bytesConstRef(&e)));
		
		// verify with secp256k1lib
		byte encpub[65] = {0x04};
		memcpy(&encpub[1], pkey.data(), 64);
		byte dersig[72];
		size_t cssz = DSAConvertSignatureFormat(dersig, 72, DSA_DER, sig.data(), 64, DSA_P1363);
		BOOST_CHECK(cssz <= 72);
#if ETH_HAVE_SECP256K1
		BOOST_REQUIRE(1 == secp256k1_ecdsa_verify(ctx, he.data(), dersig, cssz, encpub, 65));
#endif
	}
}

BOOST_AUTO_TEST_CASE(cryptopp_ecdsa_sipaseckp256k1)
{
#ifdef CRYPTOPPNOTBROKEN
	secp256k1_start();
	
	// cryptopp integer encoding
	Integer nHex("f2ee15ea639b73fa3db9b34a245bdfa015c260c598b211bf05a1ecc4b3e3b4f2H");
	Integer nB(fromHex("f2ee15ea639b73fa3db9b34a245bdfa015c260c598b211bf05a1ecc4b3e3b4f2").data(), 32);
	BOOST_REQUIRE(nHex == nB);
	
	bytes sbytes(fromHex("0xFFFF"));
	Secret secret(sha3(sbytes));
	KeyPair key(secret);
	
	bytes m(1, 0xff);
	int tests = 2;
	while (m[0]++, tests--)
	{
		h256 hm(sha3(m));
		Integer hInt(hm.asBytes().data(), 32);
		h256 k(hm ^ key.sec());
		Integer kInt(k.asBytes().data(), 32);

		// raw sign w/cryptopp (doesn't pass through cryptopp hash filter)
		ECDSA<ECP, SHA3_256>::Signer signer;
		signer.AccessKey().Initialize(s_params, secretToExponent(key.sec()));
		Integer r, s;
		signer.RawSign(kInt, hInt, r, s);

		// verify cryptopp raw-signature w/cryptopp
		ECDSA<ECP, SHA3_256>::Verifier verifier;
		verifier.AccessKey().Initialize(s_params, publicToPoint(key.pub()));
		Signature sigppraw;
		r.Encode(sigppraw.data(), 32);
		s.Encode(sigppraw.data() + 32, 32);
		BOOST_REQUIRE(verifier.VerifyMessage(m.data(), m.size(), sigppraw.data(), 64));
//		BOOST_REQUIRE(crypto::verify(key.pub(), sigppraw, bytesConstRef(&m)));
		BOOST_REQUIRE(dev::verify(key.pub(), sigppraw, hm));
		
		// sign with cryptopp, verify, recover w/sec256lib
		Signature seclibsig(dev::sign(key.sec(), hm));
		BOOST_REQUIRE(verifier.VerifyMessage(m.data(), m.size(), seclibsig.data(), 64));
//		BOOST_REQUIRE(crypto::verify(key.pub(), seclibsig, bytesConstRef(&m)));
		BOOST_REQUIRE(dev::verify(key.pub(), seclibsig, hm));
		BOOST_REQUIRE(dev::recover(seclibsig, hm) == key.pub());

		// sign with cryptopp (w/hash filter?), verify with cryptopp
		bytes sigppb(signer.MaxSignatureLength());
		size_t ssz = signer.SignMessage(s_rng, m.data(), m.size(), sigppb.data());
		Signature sigpp;
		memcpy(sigpp.data(), sigppb.data(), 64);
		BOOST_REQUIRE(verifier.VerifyMessage(m.data(), m.size(), sigppb.data(), ssz));
//		BOOST_REQUIRE(crypto::verify(key.pub(), sigpp, bytesConstRef(&m)));
		BOOST_REQUIRE(dev::verify(key.pub(), sigpp, hm));

		// sign with cryptopp and stringsource hash filter
		string sigstr;
		StringSource ssrc(asString(m), true, new SignerFilter(s_rng, signer, new StringSink(sigstr)));
		FixedHash<sizeof(Signature)> retsig((byte const*)sigstr.data(), Signature::ConstructFromPointer);
		BOOST_REQUIRE(verifier.VerifyMessage(m.data(), m.size(), retsig.data(), 64));
//		BOOST_REQUIRE(crypto::verify(key.pub(), retsig, bytesConstRef(&m)));
		BOOST_REQUIRE(dev::verify(key.pub(), retsig, hm));
		
		/// verification w/sec256lib
		// requires public key and sig in standard format
		byte encpub[65] = {0x04};
		memcpy(&encpub[1], key.pub().data(), 64);
		byte dersig[72];
		
		// verify sec256lib sig w/sec256lib
		size_t cssz = DSAConvertSignatureFormat(dersig, 72, DSA_DER, seclibsig.data(), 64, DSA_P1363);
		BOOST_CHECK(cssz <= 72);
		BOOST_REQUIRE(1 == secp256k1_ecdsa_verify(hm.data(), sizeof(hm), dersig, cssz, encpub, 65));
		
		// verify cryptopp-raw sig w/sec256lib
		cssz = DSAConvertSignatureFormat(dersig, 72, DSA_DER, sigppraw.data(), 64, DSA_P1363);
		BOOST_CHECK(cssz <= 72);
		BOOST_REQUIRE(1 == secp256k1_ecdsa_verify(hm.data(), sizeof(hm), dersig, cssz, encpub, 65));

		// verify cryptopp sig w/sec256lib
		cssz = DSAConvertSignatureFormat(dersig, 72, DSA_DER, sigppb.data(), 64, DSA_P1363);
		BOOST_CHECK(cssz <= 72);
		BOOST_REQUIRE(1 == secp256k1_ecdsa_verify(hm.data(), sizeof(hm), dersig, cssz, encpub, 65));
	}
#endif
}

BOOST_AUTO_TEST_CASE(sha3_norestart)
{
	CryptoPP::SHA3_256 ctx;
	bytes input(asBytes("test"));
	ctx.Update(input.data(), 4);
	CryptoPP::SHA3_256 ctxCopy(ctx);
	bytes interimDigest(32);
	ctx.Final(interimDigest.data());
	ctx.Update(input.data(), 4);
	bytes firstDigest(32);
	ctx.Final(firstDigest.data());
	BOOST_REQUIRE(interimDigest == firstDigest);
	
	ctxCopy.Update(input.data(), 4);
	bytes finalDigest(32);
	ctxCopy.Final(interimDigest.data());
	BOOST_REQUIRE(interimDigest != finalDigest);
	
	// we can do this another way -- copy the context for final
	ctxCopy.Update(input.data(), 4);
	ctxCopy.Update(input.data(), 4);
	CryptoPP::SHA3_256 finalCtx(ctxCopy);
	bytes finalDigest2(32);
	finalCtx.Final(finalDigest2.data());
	BOOST_REQUIRE(finalDigest2 == interimDigest);
	ctxCopy.Update(input.data(), 4);
	bytes finalDigest3(32);
	finalCtx.Final(finalDigest3.data());
	BOOST_REQUIRE(finalDigest2 != finalDigest3);
}

BOOST_AUTO_TEST_CASE(ecies_kdf)
{
	KeyPair local = KeyPair::create();
	KeyPair remote = KeyPair::create();
	// nonce
	Secret z1;
	ecdh::agree(local.sec(), remote.pub(), z1);
	auto key1 = s_secp256k1->eciesKDF(z1, bytes(), 64);
	bytesConstRef eKey1 = bytesConstRef(&key1).cropped(0, 32);
	bytesRef mKey1 = bytesRef(&key1).cropped(32, 32);
	sha3(mKey1, mKey1);
	
	Secret z2;
	ecdh::agree(remote.sec(), local.pub(), z2);
	auto key2 = s_secp256k1->eciesKDF(z2, bytes(), 64);
	bytesConstRef eKey2 = bytesConstRef(&key2).cropped(0, 32);
	bytesRef mKey2 = bytesRef(&key2).cropped(32, 32);
	sha3(mKey2, mKey2);
	
	BOOST_REQUIRE(eKey1.toBytes() == eKey2.toBytes());
	BOOST_REQUIRE(mKey1.toBytes() == mKey2.toBytes());
	
	BOOST_REQUIRE(!!z1);
	BOOST_REQUIRE(z1 == z2);
	
	BOOST_REQUIRE(key1.size() > 0 && ((u512)h512(key1)) > 0);
	BOOST_REQUIRE(key1 == key2);
}

BOOST_AUTO_TEST_CASE(ecies_standard)
{
	KeyPair k = KeyPair::create();
	
	string message("Now is the time for all good persons to come to the aid of humanity.");
	string original = message;
	bytes b = asBytes(message);
	
	s_secp256k1->encryptECIES(k.pub(), b);
	BOOST_REQUIRE(b != asBytes(original));
	BOOST_REQUIRE(b.size() > 0 && b[0] == 0x04);
	
	s_secp256k1->decryptECIES(k.sec(), b);
	BOOST_REQUIRE(bytesConstRef(&b).cropped(0, original.size()).toBytes() == asBytes(original));
}

BOOST_AUTO_TEST_CASE(ecies_sharedMacData)
{
	KeyPair k = KeyPair::create();

	string message("Now is the time for all good persons to come to the aid of humanity.");
	string original = message;
	bytes b = asBytes(message);

	bytesConstRef shared("shared MAC data");
	bytesConstRef wrongShared("wrong shared MAC data");

	s_secp256k1->encryptECIES(k.pub(), shared, b);
	BOOST_REQUIRE(b != asBytes(original));
	BOOST_REQUIRE(b.size() > 0 && b[0] == 0x04);

	BOOST_REQUIRE(!s_secp256k1->decryptECIES(k.sec(), wrongShared, b));

	s_secp256k1->decryptECIES(k.sec(), shared, b);

	// Temporary disable this assertion, which is failing in TravisCI only for Ubuntu Trusty.		
	// See https://travis-ci.org/bobsummerwill/cpp-ethereum/jobs/143250866.
	#if !defined(DISABLE_BROKEN_UNIT_TESTS_UNTIL_WE_FIX_THEM)
		BOOST_REQUIRE(bytesConstRef(&b).cropped(0, original.size()).toBytes() == asBytes(original));
	#endif // !defined(DISABLE_BROKEN_UNIT_TESTS_UNTIL_WE_FIX_THEM)
}

BOOST_AUTO_TEST_CASE(ecies_eckeypair)
{
	KeyPair k = KeyPair::create();

	string message("Now is the time for all good persons to come to the aid of humanity.");
	string original = message;
	
	bytes b = asBytes(message);
	s_secp256k1->encrypt(k.pub(), b);
	BOOST_REQUIRE(b != asBytes(original));

	s_secp256k1->decrypt(k.sec(), b);
	BOOST_REQUIRE(b == asBytes(original));
}

BOOST_AUTO_TEST_CASE(ecdh)
{
	cnote << "Testing ecdh...";

	ECDH<ECP>::Domain dhLocal(s_curveOID);
	SecByteBlock privLocal(dhLocal.PrivateKeyLength());
	SecByteBlock pubLocal(dhLocal.PublicKeyLength());
	dhLocal.GenerateKeyPair(s_rng, privLocal, pubLocal);
	
	ECDH<ECP>::Domain dhRemote(s_curveOID);
	SecByteBlock privRemote(dhRemote.PrivateKeyLength());
	SecByteBlock pubRemote(dhRemote.PublicKeyLength());
	dhRemote.GenerateKeyPair(s_rng, privRemote, pubRemote);
	
	assert(dhLocal.AgreedValueLength() == dhRemote.AgreedValueLength());
	
	// local: send public to remote; remote: send public to local
	
	// Local
	SecByteBlock sharedLocal(dhLocal.AgreedValueLength());
	assert(dhLocal.Agree(sharedLocal, privLocal, pubRemote));
	
	// Remote
	SecByteBlock sharedRemote(dhRemote.AgreedValueLength());
	assert(dhRemote.Agree(sharedRemote, privRemote, pubLocal));
	
	// Test
	Integer ssLocal, ssRemote;
	ssLocal.Decode(sharedLocal.BytePtr(), sharedLocal.SizeInBytes());
	ssRemote.Decode(sharedRemote.BytePtr(), sharedRemote.SizeInBytes());
	
	assert(ssLocal != 0);
	assert(ssLocal == ssRemote);
	
	
	// Now use our keys
	KeyPair a = KeyPair::create();
	byte puba[65] = {0x04};
	memcpy(&puba[1], a.pub().data(), 64);
	
	KeyPair b = KeyPair::create();
	byte pubb[65] = {0x04};
	memcpy(&pubb[1], b.pub().data(), 64);
	
	ECDH<ECP>::Domain dhA(s_curveOID);
	Secret shared;
	BOOST_REQUIRE(dhA.Agree(shared.writable().data(), a.sec().data(), pubb));
	BOOST_REQUIRE(shared);
}

BOOST_AUTO_TEST_CASE(ecdhe)
{
	cnote << "Testing ecdhe...";
	
	ECDHE a, b;
	BOOST_CHECK_NE(a.pubkey(), b.pubkey());
	
	ECDHE local;
	ECDHE remote;
	
	// local tx pubkey -> remote
	Secret sremote;
	remote.agree(local.pubkey(), sremote);
	
	// remote tx pbukey -> local
	Secret slocal;
	local.agree(remote.pubkey(), slocal);

	BOOST_REQUIRE(sremote);
	BOOST_REQUIRE(slocal);
	BOOST_REQUIRE_EQUAL(sremote, slocal);
}

BOOST_AUTO_TEST_CASE(handshakeNew)
{
	//	authInitiator -> E(remote-pubk, S(ecdhe-random, ecdh-shared-secret^nonce) || H(ecdhe-random-pubk) || pubk || nonce || 0x0)
	//	authRecipient -> E(remote-pubk, ecdhe-random-pubk || nonce || 0x0)
	
	h256 base(sha3("privacy"));
	sha3(base.ref(), base.ref());
	Secret nodeAsecret(base);
	KeyPair nodeA(nodeAsecret);
	BOOST_REQUIRE(nodeA.pub());
	
	sha3(base.ref(), base.ref());
	Secret nodeBsecret(base);
	KeyPair nodeB(nodeBsecret);
	BOOST_REQUIRE(nodeB.pub());
	
	BOOST_REQUIRE_NE(nodeA.sec(), nodeB.sec());
	
	// Initiator is Alice (nodeA)
	ECDHE eA;
	bytes nAbytes(fromHex("0xAAAA"));
	h256 nonceA(sha3(nAbytes));
	bytes auth(Signature::size + h256::size + Public::size + h256::size + 1);
	Secret ssA;
	{
		bytesRef sig(&auth[0], Signature::size);
		bytesRef hepubk(&auth[Signature::size], h256::size);
		bytesRef pubk(&auth[Signature::size + h256::size], Public::size);
		bytesRef nonce(&auth[Signature::size + h256::size + Public::size], h256::size);
		
		crypto::ecdh::agree(nodeA.sec(), nodeB.pub(), ssA);
		sign(eA.seckey(), (ssA ^ nonceA).makeInsecure()).ref().copyTo(sig);
		sha3(eA.pubkey().ref(), hepubk);
		nodeA.pub().ref().copyTo(pubk);
		nonceA.ref().copyTo(nonce);
		auth[auth.size() - 1] = 0x0;
	}
	bytes authcipher;
	encrypt(nodeB.pub(), &auth, authcipher);
	BOOST_REQUIRE_EQUAL(authcipher.size(), 279);
	
	// Receipient is Bob (nodeB)
	ECDHE eB;
	bytes nBbytes(fromHex("0xBBBB"));
	h256 nonceB(sha3(nAbytes));
	bytes ack(Public::size + h256::size + 1);
	{
		// todo: replace nodeA.pub() in encrypt()
		// decrypt public key from auth
		bytes authdecrypted;
		decrypt(nodeB.sec(), &authcipher, authdecrypted);
		Public node;
		bytesConstRef pubk(&authdecrypted[Signature::size + h256::size], Public::size);
		pubk.copyTo(node.ref());
		
		bytesRef epubk(&ack[0], Public::size);
		bytesRef nonce(&ack[Public::size], h256::size);
		
		eB.pubkey().ref().copyTo(epubk);
		nonceB.ref().copyTo(nonce);
		auth[auth.size() - 1] = 0x0;
	}
	bytes ackcipher;
	encrypt(nodeA.pub(), &ack, ackcipher);
	BOOST_REQUIRE_EQUAL(ackcipher.size(), 182);
	
	BOOST_REQUIRE(eA.pubkey());
	BOOST_REQUIRE(eB.pubkey());
	BOOST_REQUIRE_NE(eA.seckey(), eB.seckey());
	
	/// Alice (after receiving ack)
	Secret aEncryptK;
	Secret aMacK;
	Secret aEgressMac;
	Secret aIngressMac;
	{
		bytes ackdecrypted;
		decrypt(nodeA.sec(), &ackcipher, ackdecrypted);
		BOOST_REQUIRE(ackdecrypted.size());
		bytesConstRef ackRef(&ackdecrypted);
		Public eBAck;
		h256 nonceBAck;
		ackRef.cropped(0, Public::size).copyTo(bytesRef(eBAck.data(), Public::size));
		ackRef.cropped(Public::size, h256::size).copyTo(nonceBAck.ref());
		BOOST_REQUIRE_EQUAL(eBAck, eB.pubkey());
		BOOST_REQUIRE_EQUAL(nonceBAck, nonceB);
		
		// TODO: export ess and require equal to b
		
		bytes keyMaterialBytes(512);
		bytesRef keyMaterial(&keyMaterialBytes);
		
		Secret ess;
		// todo: ecdh-agree should be able to output bytes
		eA.agree(eBAck, ess);
		ess.ref().copyTo(keyMaterial.cropped(0, h256::size));
		ssA.ref().copyTo(keyMaterial.cropped(h256::size, h256::size));
//		auto token = sha3(ssA);
		aEncryptK = sha3Secure(keyMaterial);
		aEncryptK.ref().copyTo(keyMaterial.cropped(h256::size, h256::size));
		aMacK = sha3Secure(keyMaterial);

		keyMaterialBytes.resize(h256::size + authcipher.size());
		keyMaterial.retarget(keyMaterialBytes.data(), keyMaterialBytes.size());
		(aMacK ^ nonceBAck).ref().copyTo(keyMaterial);
		bytesConstRef(&authcipher).copyTo(keyMaterial.cropped(h256::size, authcipher.size()));
		aEgressMac = sha3Secure(keyMaterial);
		
		keyMaterialBytes.resize(h256::size + ackcipher.size());
		keyMaterial.retarget(keyMaterialBytes.data(), keyMaterialBytes.size());
		(aMacK ^ nonceA).ref().copyTo(keyMaterial);
		bytesConstRef(&ackcipher).copyTo(keyMaterial.cropped(h256::size, ackcipher.size()));
		aIngressMac = sha3Secure(keyMaterial);
	}
	
	
	/// Bob (after sending ack)
	Secret ssB;
	crypto::ecdh::agree(nodeB.sec(), nodeA.pub(), ssB);
	BOOST_REQUIRE_EQUAL(ssA, ssB);
	
	Secret bEncryptK;
	Secret bMacK;
	Secret bEgressMac;
	Secret bIngressMac;
	{
		bytes authdecrypted;
		decrypt(nodeB.sec(), &authcipher, authdecrypted);
		BOOST_REQUIRE(authdecrypted.size());
		bytesConstRef ackRef(&authdecrypted);
		Signature sigAuth;
		h256 heA;
		Public eAAuth;
		Public nodeAAuth;
		h256 nonceAAuth;
		bytesConstRef sig(&authdecrypted[0], Signature::size);
		bytesConstRef hepubk(&authdecrypted[Signature::size], h256::size);
		bytesConstRef pubk(&authdecrypted[Signature::size + h256::size], Public::size);
		bytesConstRef nonce(&authdecrypted[Signature::size + h256::size + Public::size], h256::size);

		nonce.copyTo(nonceAAuth.ref());
		pubk.copyTo(nodeAAuth.ref());
		BOOST_REQUIRE(nonceAAuth);
		BOOST_REQUIRE_EQUAL(nonceA, nonceAAuth);
		BOOST_REQUIRE(nodeAAuth);
		BOOST_REQUIRE_EQUAL(nodeA.pub(), nodeAAuth); // bad test, bad!!!
		hepubk.copyTo(heA.ref());
		sig.copyTo(sigAuth.ref());
		
		Secret ss;
		s_secp256k1->agree(nodeB.sec(), nodeAAuth, ss);
		eAAuth = recover(sigAuth, (ss ^ nonceAAuth).makeInsecure());
		// todo: test when this fails; means remote is bad or packet bits were flipped
		BOOST_REQUIRE_EQUAL(heA, sha3(eAAuth));
		BOOST_REQUIRE_EQUAL(eAAuth, eA.pubkey());
		
		bytes keyMaterialBytes(512);
		bytesRef keyMaterial(&keyMaterialBytes);
		
		Secret ess;
		// todo: ecdh-agree should be able to output bytes
		eB.agree(eAAuth, ess);
//		s_secp256k1->agree(eB.seckey(), eAAuth, ess);
		ess.ref().copyTo(keyMaterial.cropped(0, h256::size));
		ssB.ref().copyTo(keyMaterial.cropped(h256::size, h256::size));
//		auto token = sha3(ssA);
		bEncryptK = sha3Secure(keyMaterial);
		bEncryptK.ref().copyTo(keyMaterial.cropped(h256::size, h256::size));
		bMacK = sha3Secure(keyMaterial);
		
		// todo: replace nonceB with decrypted nonceB
		keyMaterialBytes.resize(h256::size + ackcipher.size());
		keyMaterial.retarget(keyMaterialBytes.data(), keyMaterialBytes.size());
		(bMacK ^ nonceAAuth).ref().copyTo(keyMaterial);
		bytesConstRef(&ackcipher).copyTo(keyMaterial.cropped(h256::size, ackcipher.size()));
		bEgressMac = sha3Secure(keyMaterial);
		
		keyMaterialBytes.resize(h256::size + authcipher.size());
		keyMaterial.retarget(keyMaterialBytes.data(), keyMaterialBytes.size());
		(bMacK ^ nonceB).ref().copyTo(keyMaterial);
		bytesConstRef(&authcipher).copyTo(keyMaterial.cropped(h256::size, authcipher.size()));
		bIngressMac = sha3Secure(keyMaterial);
	}
	
	BOOST_REQUIRE_EQUAL(aEncryptK, bEncryptK);
	BOOST_REQUIRE_EQUAL(aMacK, bMacK);
	BOOST_REQUIRE_EQUAL(aEgressMac, bIngressMac);
	BOOST_REQUIRE_EQUAL(bEgressMac, aIngressMac);
	
	
	
}

BOOST_AUTO_TEST_CASE(ecies_aes128_ctr_unaligned)
{
	SecureFixedHash<16> encryptK(sha3("..."), h128::AlignLeft);
	h256 egressMac(sha3("+++"));
	// TESTING: send encrypt magic sequence
	bytes magic {0x22,0x40,0x08,0x91};
	bytes magicCipherAndMac;
	magicCipherAndMac = encryptSymNoAuth(encryptK, h128(), &magic);
	
	magicCipherAndMac.resize(magicCipherAndMac.size() + 32);
	sha3mac(egressMac.ref(), &magic, egressMac.ref());
	egressMac.ref().copyTo(bytesRef(&magicCipherAndMac).cropped(magicCipherAndMac.size() - 32, 32));
	
	bytesConstRef cipher(&magicCipherAndMac[0], magicCipherAndMac.size() - 32);
	bytes plaintext = decryptSymNoAuth(encryptK, h128(), cipher).makeInsecure();
	
	plaintext.resize(magic.size());
	// @alex @subtly TODO: FIX: this check is pointless with the above line.
	BOOST_REQUIRE(plaintext.size() > 0);
	BOOST_REQUIRE(magic == plaintext);
}

BOOST_AUTO_TEST_CASE(ecies_aes128_ctr)
{
	SecureFixedHash<16> k(sha3("0xAAAA"), h128::AlignLeft);
	string m = "AAAAAAAAAAAAAAAA";
	bytesConstRef msg((byte*)m.data(), m.size());

	bytes ciphertext;
	h128 iv;
	tie(ciphertext, iv) = encryptSymNoAuth(k, msg);
	
	bytes plaintext = decryptSymNoAuth(k, iv, &ciphertext).makeInsecure();
	BOOST_REQUIRE_EQUAL(asString(plaintext), m);
}

BOOST_AUTO_TEST_CASE(cryptopp_aes128_ctr)
{
	const int aesKeyLen = 16;
	BOOST_REQUIRE(sizeof(char) == sizeof(byte));
	
	// generate test key
	AutoSeededRandomPool rng;
	SecByteBlock key(0x00, aesKeyLen);
	rng.GenerateBlock(key, key.size());
	
	// cryptopp uses IV as nonce/counter which is same as using nonce w/0 ctr
	FixedHash<AES::BLOCKSIZE> ctr;
	rng.GenerateBlock(ctr.data(), sizeof(ctr));

	// used for decrypt
	FixedHash<AES::BLOCKSIZE> ctrcopy(ctr);
	
	string text = "Now is the time for all good persons to come to the aid of humanity.";
	unsigned char const* in = (unsigned char*)&text[0];
	unsigned char* out = (unsigned char*)&text[0];
	string original = text;
	string doublespeak = text + text;
	
	string cipherCopy;
	try
	{
		CTR_Mode<AES>::Encryption e;
		e.SetKeyWithIV(key, key.size(), ctr.data());
		
		// 68 % 255 should be difference of counter
		e.ProcessData(out, in, text.size());
		ctr = h128(u128(ctr) + text.size() / 16);
		
		BOOST_REQUIRE(text != original);
		cipherCopy = text;
	}
	catch (CryptoPP::Exception& _e)
	{
		cerr << _e.what() << endl;
	}
	
	try
	{
		CTR_Mode< AES >::Decryption d;
		d.SetKeyWithIV(key, key.size(), ctrcopy.data());
		d.ProcessData(out, in, text.size());
		BOOST_REQUIRE(text == original);
	}
	catch (CryptoPP::Exception& _e)
	{
		cerr << _e.what() << endl;
	}
	
	
	// reencrypt ciphertext...
	try
	{
		BOOST_REQUIRE(cipherCopy != text);
		in = (unsigned char*)&cipherCopy[0];
		out = (unsigned char*)&cipherCopy[0];
		
		CTR_Mode<AES>::Encryption e;
		e.SetKeyWithIV(key, key.size(), ctrcopy.data());
		e.ProcessData(out, in, text.size());
		
		// yep, ctr mode.
		BOOST_REQUIRE(cipherCopy == original);
	}
	catch (CryptoPP::Exception& _e)
	{
		cerr << _e.what() << endl;
	}
	
}

BOOST_AUTO_TEST_CASE(cryptopp_aes128_cbc)
{
	const int aesKeyLen = 16;
	BOOST_REQUIRE(sizeof(char) == sizeof(byte));
	
	AutoSeededRandomPool rng;
	SecByteBlock key(0x00, aesKeyLen);
	rng.GenerateBlock(key, key.size());
	
	// Generate random IV
	byte iv[AES::BLOCKSIZE];
	rng.GenerateBlock(iv, AES::BLOCKSIZE);
	
	string string128("AAAAAAAAAAAAAAAA");
	string plainOriginal = string128;
	
	CryptoPP::CBC_Mode<Rijndael>::Encryption cbcEncryption(key, key.size(), iv);
	cbcEncryption.ProcessData((byte*)&string128[0], (byte*)&string128[0], string128.size());
	BOOST_REQUIRE(string128 != plainOriginal);
	
	CBC_Mode<Rijndael>::Decryption cbcDecryption(key, key.size(), iv);
	cbcDecryption.ProcessData((byte*)&string128[0], (byte*)&string128[0], string128.size());
	BOOST_REQUIRE(plainOriginal == string128);
	
	
	// plaintext whose size isn't divisible by block size must use stream filter for padding
	string string192("AAAAAAAAAAAAAAAABBBBBBBB");
	plainOriginal = string192;

	string cipher;
	StreamTransformationFilter* aesStream = new StreamTransformationFilter(cbcEncryption, new StringSink(cipher));
	StringSource source(string192, true, aesStream);
	BOOST_REQUIRE(cipher.size() == 32);

	cbcDecryption.ProcessData((byte*)&cipher[0], (byte*)&string192[0], cipher.size());
	BOOST_REQUIRE(string192 == plainOriginal);
}

BOOST_AUTO_TEST_CASE(recoverVgt3)
{
	// base secret
	Secret secret(sha3("privacy"));

	// we get ec params from signer
	ECDSA<ECP, SHA3_256>::Signer signer;

	// e := sha3(msg)
	bytes e(fromHex("0x01"));
	e.resize(32);
	int tests = 2;
	while (sha3(&e, &e), secret = sha3(secret), tests--)
	{
		KeyPair key(secret);
		Public pkey = key.pub();
		signer.AccessKey().Initialize(s_params, secretToExponent(secret));

		h256 he(sha3(e));
		Integer heInt(he.asBytes().data(), 32);
		h256 k(crypto::kdf(secret, he));
		Integer kInt(k.asBytes().data(), 32);
		kInt %= s_params.GetSubgroupOrder()-1;

		ECP::Point rp = s_params.ExponentiateBase(kInt);
		Integer const& q = s_params.GetGroupOrder();
		Integer r = s_params.ConvertElementToInteger(rp);

		Integer kInv = kInt.InverseMod(q);
		Integer s = (kInv * (Integer(secret.data(), 32) * r + heInt)) % q;
		BOOST_REQUIRE(!!r && !!s);

		//try recover function on diffrent v values (should be invalid)
		for (size_t i = 0; i < 10; i++)
		{
			Signature sig;
			sig[64] = i;
			r.Encode(sig.data(), 32);
			s.Encode(sig.data() + 32, 32);

			Public p = dev::recover(sig, he);
			size_t expectI = rp.y.IsOdd() ? 1 : 0;
			if (i == expectI)
				BOOST_REQUIRE(p == pkey);
			else
				BOOST_REQUIRE(p != pkey);
		}
	}
}

#if defined(__GNUC__)
  	#pragma GCC diagnostic pop
#endif // defined(__GNUC__)

BOOST_AUTO_TEST_SUITE_END()

