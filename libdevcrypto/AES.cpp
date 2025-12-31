// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "AES.h"
#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/pwdbased.h>
#include <cryptopp/modes.h>
#include <cryptopp/sha.h>

using namespace dev;
using namespace dev::crypto;

bytes dev::aesDecrypt(bytesConstRef _ivCipher, std::string const& _password, unsigned _rounds, bytesConstRef _salt)
{
	bytes pw = asBytes(_password);

	if (!_salt.size())
		_salt = &pw;

	bytes target(64);
	CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::SHA256>().DeriveKey(target.data(), target.size(), 0, pw.data(), pw.size(), _salt.data(), _salt.size(), _rounds);

	try
	{
		CryptoPP::AES::Decryption aesDecryption(target.data(), 16);
		auto cipher = _ivCipher.cropped(16);
		auto iv = _ivCipher.cropped(0, 16);
		CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv.data());
		std::string decrypted;
		CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decrypted));
		stfDecryptor.Put(cipher.data(), cipher.size());
		stfDecryptor.MessageEnd();
		return asBytes(decrypted);
	}
	catch (std::exception const& e)
	{
		// FIXME: Handle this error better.
		std::cerr << e.what() << '\n';
		return bytes();
	}
}
