// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "TrieHash.h"
#include "TrieCommon.h"
#include "TrieDB.h"	// @TODO replace ASAP!

namespace dev
{

void hash256aux(HexMap const& _s, HexMap::const_iterator _begin, HexMap::const_iterator _end, unsigned _preLen, RLPStream& _rlp);

void hash256rlp(HexMap const& _s, HexMap::const_iterator _begin, HexMap::const_iterator _end, unsigned _preLen, RLPStream& _rlp)
{
	if (_begin == _end)
		_rlp << "";	// NULL
	else if (std::next(_begin) == _end)
	{
		// only one left - terminate with the pair.
		_rlp.appendList(2) << hexPrefixEncode(_begin->first, true, _preLen) << _begin->second;
	}
	else
	{
		// find the number of common prefix nibbles shared
		// i.e. the minimum number of nibbles shared at the beginning between the first hex string and each successive.
		unsigned sharedPre = (unsigned)-1;
		unsigned c = 0;
		for (auto i = std::next(_begin); i != _end && sharedPre; ++i, ++c)
		{
			unsigned x = std::min(sharedPre, std::min((unsigned)_begin->first.size(), (unsigned)i->first.size()));
			unsigned shared = _preLen;
			for (; shared < x && _begin->first[shared] == i->first[shared]; ++shared) {}
			sharedPre = std::min(shared, sharedPre);
		}
		if (sharedPre > _preLen)
		{
			// if they all have the same next nibble, we also want a pair.
			_rlp.appendList(2) << hexPrefixEncode(_begin->first, false, _preLen, (int)sharedPre);
			hash256aux(_s, _begin, _end, (unsigned)sharedPre, _rlp);
		}
		else
		{
			// otherwise enumerate all 16+1 entries.
			_rlp.appendList(17);
			auto b = _begin;
			if (_preLen == b->first.size())
				++b;
			for (auto i = 0; i < 16; ++i)
			{
				auto n = b;
				for (; n != _end && n->first[_preLen] == i; ++n) {}
				if (b == n)
					_rlp << "";
				else
					hash256aux(_s, b, n, _preLen + 1, _rlp);
				b = n;
			}
			if (_preLen == _begin->first.size())
				_rlp << _begin->second;
			else
				_rlp << "";

		}
	}
}

void hash256aux(HexMap const& _s, HexMap::const_iterator _begin, HexMap::const_iterator _end, unsigned _preLen, RLPStream& _rlp)
{
	RLPStream rlp;
	hash256rlp(_s, _begin, _end, _preLen, rlp);
	if (rlp.out().size() < 32)
	{
		// RECURSIVE RLP
		_rlp.appendRaw(rlp.out());
	}
	else
		_rlp << sha3(rlp.out());
}

bytes rlp256(BytesMap const& _s)
{
	// build patricia tree.
	if (_s.empty())
		return rlp("");
	HexMap hexMap;
	for (auto i = _s.rbegin(); i != _s.rend(); ++i)
		hexMap[asNibbles(bytesConstRef(&i->first))] = i->second;
	RLPStream s;
	hash256rlp(hexMap, hexMap.cbegin(), hexMap.cend(), 0, s);
	return s.out();
}

h256 hash256(BytesMap const& _s)
{
	return sha3(rlp256(_s));
}

h256 orderedTrieRoot(std::vector<bytes> const& _data)
{
	BytesMap m;
	unsigned j = 0;
	for (auto i: _data)
		m[rlp(j++)] = i;
	return hash256(m);
}

h256 orderedTrieRoot(std::vector<bytesConstRef> const& _data)
{
	BytesMap m;
	unsigned j = 0;
	for (auto i: _data)
		m[rlp(j++)] = i.toBytes();
	return hash256(m);
}

}
