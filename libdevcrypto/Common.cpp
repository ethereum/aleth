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
/** @file Common.cpp
 * @author Gav Wood <i@gavwood.com>
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 */

#include <random>
#include <chrono>
#include <mutex>
#include "EC.h"
#include "SHA3.h"
#include "FileSystem.h"
#include "Common.h"
using namespace std;
using namespace dev;
using namespace crypto;

//#define ETH_ADDRESS_DEBUG 1

Address dev::toAddress(Secret _secret)
{
	return KeyPair(_secret).address();
}

KeyPair KeyPair::create()
{
	static mt19937_64 s_eng(time(0) + chrono::high_resolution_clock::now().time_since_epoch().count());
	uniform_int_distribution<uint16_t> d(0, 255);

	for (int i = 0; i < 100; ++i)
	{
		h256 sec;
		for (unsigned i = 0; i < 32; ++i)
			sec[i] = (byte)d(s_eng);

		KeyPair ret(sec);
		if (ret.address())
			return ret;
	}
	return KeyPair();
}

KeyPair::KeyPair(h256 _sec):
	m_secret(_sec)
{
	toPublic(m_secret, m_public);
	if (verifySecret(m_secret, m_public))
		m_address = right160(dev::sha3(m_public.ref()));
	
#if ETH_ADDRESS_DEBUG
	cout << "---- ADDRESS -------------------------------" << endl;
	cout << "SEC: " << m_secret << endl;
	cout << "PUB: " << m_public << endl;
	cout << "ADR: " << m_address << endl;
#endif
}

KeyPair KeyPair::fromEncryptedSeed(bytesConstRef _seed, std::string const& _password)
{
	return KeyPair(sha3(aesDecrypt(_seed, _password)));
}

void dev::encrypt(Public _k, bytesConstRef _plain, bytes& o_cipher)
{
	bytes io = _plain.toBytes();
	crypto::encrypt(_k, io);
	o_cipher = std::move(io);
}

bool dev::decrypt(Secret _k, bytesConstRef _cipher, bytes& o_plaintext)
{
	bytes io = _cipher.toBytes();
	crypto::decrypt(_k, io);
	if (io.empty())
		return false;
	o_plaintext = std::move(io);
	return true;
}

Public dev::recover(Signature _sig, h256 _message)
{
	return crypto::recover(_sig, _message.ref());
}

Signature dev::sign(Secret _k, h256 _hash)
{
	return crypto::sign(_k, _hash);
}

bool dev::verify(Public _p, Signature _s, h256 _hash)
{
	return crypto::verify(_p, _s, bytesConstRef(_hash.data(), 32), true);
}

h256 Nonce::get(bool _commit)
{
	// todo: atomic efface bit, periodic save, kdf, rr, rng
	// todo: encrypt
	static h256 s_seed;
	static string s_seedFile(getDataDir() + "/seed");
	static mutex s_x;
	lock_guard<mutex> l(s_x);
	if (!s_seed)
	{
		static Nonce s_nonce;
		bytes b = contents(s_seedFile);
		if (b.size() == 32)
			memcpy(s_seed.data(), b.data(), 32);
		else
		{
			// todo: replace w/entropy from user and system
			std::mt19937_64 s_eng(time(0));
			std::uniform_int_distribution<uint16_t> d(0, 255);
			for (unsigned i = 0; i < 32; ++i)
				s_seed[i] = (byte)d(s_eng);
		}
		if (!s_seed)
			BOOST_THROW_EXCEPTION(InvalidState());
		
		// prevent seed reuse if process terminates abnormally
		writeFile(s_seedFile, bytes());
	}
	h256 prev(s_seed);
	sha3(prev.ref(), s_seed.ref());
	if (_commit)
		writeFile(s_seedFile, s_seed.asBytes());
	return std::move(s_seed);
}

Nonce::~Nonce()
{
	Nonce::get(true);
}
