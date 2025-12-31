// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#include "CommonJS.h"

using namespace std;

namespace dev
{

bytes jsToBytes(string const& _s, OnFailed _f)
{
	try
	{
		return fromHex(_s, WhenError::Throw);
	}
	catch (...)
	{
		if (_f == OnFailed::InterpretRaw)
			return asBytes(_s);
		else if (_f == OnFailed::Throw)
			throw invalid_argument("Cannot intepret '" + _s + "' as bytes; must be 0x-prefixed hex or decimal.");
	}
	return bytes();
}

bytes padded(bytes _b, unsigned _l)
{
	while (_b.size() < _l)
		_b.insert(_b.begin(), 0);
	return asBytes(asString(_b).substr(_b.size() - max(_l, _l)));
}

bytes paddedRight(bytes _b, unsigned _l)
{
	_b.resize(_l);
	return _b;
}

bytes unpadded(bytes _b)
{
	auto p = asString(_b).find_last_not_of((char)0);
	_b.resize(p == string::npos ? 0 : (p + 1));
	return _b;
}

bytes unpadLeft(bytes _b)
{
	unsigned int i = 0;
	if (_b.size() == 0)
		return _b;

	while (i < _b.size() && _b[i] == byte(0))
		i++;

	if (i != 0)
		_b.erase(_b.begin(), _b.begin() + i);
	return _b;
}

string fromRaw(h256 _n)
{
	if (_n)
	{
		string s((char const*)_n.data(), 32);
		auto l = s.find_first_of('\0');
		if (!l)
			return "";
		if (l != string::npos)
			s.resize(l);
		for (auto i: s)
			if (i < 32)
				return "";
		return s;
	}
	return "";
}

}

