/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Foobar is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file Common.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */
#if WIN32
#pragma warning(push)
#pragma warning(disable:4244)
#else
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include <secp256k1.h>
#include <sha3.h>
#if WIN32
#pragma warning(pop)
#else
#endif
#include <random>
#include "Common.h"
#include "Exceptions.h"
using namespace std;
using namespace eth;

u256 const eth::c_genesisDifficulty = (u256)1 << 20;

std::string eth::escaped(std::string const& _s, bool _all)
{
	std::string ret;
	ret.reserve(_s.size());
	ret.push_back('"');
	for (auto i: _s)
		if (i == '"' && !_all)
			ret += "\\\"";
		else if (i == '\\' && !_all)
			ret += "\\\\";
		else if (i < ' ' || i > 127 || _all)
		{
			ret += "\\x";
			ret.push_back("0123456789abcdef"[(uint8_t)i / 16]);
			ret.push_back("0123456789abcdef"[(uint8_t)i % 16]);
		}
		else
			ret.push_back(i);
	ret.push_back('"');
	return ret;
}

std::string eth::randomWord()
{
	static std::mt19937_64 s_eng(0);
	std::string ret(std::uniform_int_distribution<int>(4, 10)(s_eng), ' ');
	char const n[] = "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890";
	std::uniform_int_distribution<int> d(0, sizeof(n) - 2);
	for (char& c: ret)
		c = n[d(s_eng)];
	return ret;
}

int eth::fromHex(char _i)
{
	if (_i >= '0' && _i <= '9')
		return _i - '0';
	if (_i >= 'a' && _i <= 'f')
		return _i - 'a' + 10;
	if (_i >= 'A' && _i <= 'F')
		return _i - 'A' + 10;
	throw BadHexCharacter();
}

bytes eth::fromUserHex(std::string const& _s)
{
	assert(_s.size() % 2 == 0);
	if (_s.size() < 2)
		return bytes();
	uint s = (_s[0] == '0' && _s[1] == 'x') ? 2 : 0;
	std::vector<uint8_t> ret;
	ret.reserve((_s.size() - s) / 2);
	for (uint i = s; i < _s.size(); i += 2)
		ret.push_back((byte)(fromHex(_s[i]) * 16 + fromHex(_s[i + 1])));
	return ret;
}

bytes eth::toHex(std::string const& _s)
{
	std::vector<uint8_t> ret;
	ret.reserve(_s.size() * 2);
	for (auto i: _s)
	{
		ret.push_back(i / 16);
		ret.push_back(i % 16);
	}
	return ret;
}

std::string eth::sha3(std::string const& _input, bool _hex)
{
	if (!_hex)
	{
		string ret(32, '\0');
		sha3(bytesConstRef((byte const*)_input.data(), _input.size()), bytesRef((byte*)ret.data(), 32));
		return ret;
	}

	uint8_t buf[32];
	sha3(bytesConstRef((byte const*)_input.data(), _input.size()), bytesRef((byte*)&(buf[0]), 32));
	std::string ret(64, '\0');
	for (unsigned int i = 0; i < 32; i++)
		sprintf((char*)(ret.data())+i*2, "%02x", buf[i]);
	return ret;
}

void eth::sha3(bytesConstRef _input, bytesRef _output)
{
	CryptoPP::SHA3_256 ctx;
	ctx.Update((byte*)_input.data(), _input.size());
	assert(_output.size() >= 32);
	ctx.Final(_output.data());
}

bytes eth::sha3Bytes(bytesConstRef _input)
{
	bytes ret(32);
	sha3(_input, &ret);
	return ret;
}

h256 eth::sha3(bytesConstRef _input)
{
	h256 ret;
	sha3(_input, bytesRef(&ret[0], 32));
	return ret;
}

Address eth::toAddress(Secret _private)
{
	secp256k1_start();

	byte pubkey[65];
	int pubkeylen = 65;
	int ok = secp256k1_ecdsa_seckey_verify(_private.data());
	if (!ok)
		return Address();
	ok = secp256k1_ecdsa_pubkey_create(pubkey, &pubkeylen, _private.data(), 0);
	assert(pubkeylen == 65);
	if (!ok)
		return Address();
	ok = secp256k1_ecdsa_pubkey_verify(pubkey, 65);
	if (!ok)
		return Address();
	auto ret = right160(eth::sha3(bytesConstRef(&(pubkey[1]), 64)));
#if ETH_ADDRESS_DEBUG
	cout << "---- ADDRESS -------------------------------" << endl;
	cout << "SEC: " << _private << endl;
	cout << "PUB: " << asHex(bytesConstRef(&(pubkey[1]), 64)) << endl;
	cout << "ADR: " << ret << endl;
#endif
	return ret;
}

KeyPair KeyPair::create()
{
	static std::mt19937_64 s_eng(time(0));
	std::uniform_int_distribution<byte> d(0, 255);
	KeyPair ret;
	for (uint i = 0; i < 32; ++i)
		ret.m_secret[i] = d(s_eng);
	ret.m_address = toAddress(ret.m_secret);
	return ret;
}
