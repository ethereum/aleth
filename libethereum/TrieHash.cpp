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
/** @file TrieHash.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "Common.h"
#include "TrieCommon.h"
#include "TrieHash.h"
using namespace std;
using namespace eth;

namespace eth
{

/*/
#define APPEND_CHILD appendString
/*/
#define APPEND_CHILD appendRaw
/**/

#define ENABLE_DEBUG_PRINT 0

#if ENABLE_DEBUG_PRINT
bool g_hashDebug = false;
#endif

void hash256aux(HexMap const& _s, HexMap::const_iterator _begin, HexMap::const_iterator _end, unsigned _preLen, RLPStream& _rlp);

void hash256rlp(HexMap const& _s, HexMap::const_iterator _begin, HexMap::const_iterator _end, unsigned _preLen, RLPStream& _rlp)
{
#if ENABLE_DEBUG_PRINT
	static std::string s_indent;
	if (_preLen)
		s_indent += "  ";
#endif

	if (_begin == _end)
		_rlp << "";	// NULL
	else if (std::next(_begin) == _end)
	{
		// only one left - terminate with the pair.
		_rlp.appendList(2) << hexPrefixEncode(_begin->first, true, _preLen) << _begin->second;
#if ENABLE_DEBUG_PRINT
		if (g_hashDebug)
			std::cerr << s_indent << asHex(bytesConstRef(_begin->first.data() + _preLen, _begin->first.size() - _preLen), 1) << ": " << _begin->second << " = " << sha3(_rlp.out()) << std::endl;
#endif
	}
	else
	{
		// find the number of common prefix nibbles shared
		// i.e. the minimum number of nibbles shared at the beginning between the first hex string and each successive.
		uint sharedPre = (uint)-1;
		uint c = 0;
		for (auto i = std::next(_begin); i != _end && sharedPre; ++i, ++c)
		{
			uint x = std::min(sharedPre, std::min((uint)_begin->first.size(), (uint)i->first.size()));
			uint shared = _preLen;
			for (; shared < x && _begin->first[shared] == i->first[shared]; ++shared) {}
			sharedPre = std::min(shared, sharedPre);
		}
		if (sharedPre > _preLen)
		{
			// if they all have the same next nibble, we also want a pair.
#if ENABLE_DEBUG_PRINT
			if (g_hashDebug)
				std::cerr << s_indent << asHex(bytesConstRef(_begin->first.data() + _preLen, sharedPre), 1) << ": " << std::endl;
#endif
			_rlp.appendList(2) << hexPrefixEncode(_begin->first, false, _preLen, (int)sharedPre);
			hash256aux(_s, _begin, _end, (unsigned)sharedPre, _rlp);
#if ENABLE_DEBUG_PRINT
			if (g_hashDebug)
				std::cerr << s_indent << "= " << hex << sha3(_rlp.out()) << dec << std::endl;
#endif
		}
		else
		{
			// otherwise enumerate all 16+1 entries.
			_rlp.appendList(17);
			auto b = _begin;
			if (_preLen == b->first.size())
			{
#if ENABLE_DEBUG_PRINT
				if (g_hashDebug)
					std::cerr << s_indent << "@: " << b->second << std::endl;
#endif
				++b;
			}
			for (auto i = 0; i < 16; ++i)
			{
				auto n = b;
				for (; n != _end && n->first[_preLen] == i; ++n) {}
				if (b == n)
					_rlp << "";
				else
				{
#if ENABLE_DEBUG_PRINT
					if (g_hashDebug)
						std::cerr << s_indent << std::hex << i << ": " << std::dec << std::endl;
#endif
					hash256aux(_s, b, n, _preLen + 1, _rlp);
				}
				b = n;
			}
			if (_preLen == _begin->first.size())
				_rlp << _begin->second;
			else
				_rlp << "";

#if ENABLE_DEBUG_PRINT
			if (g_hashDebug)
				std::cerr << s_indent << "= " << hex << sha3(_rlp.out()) << dec << std::endl;
#endif
		}
	}
#if ENABLE_DEBUG_PRINT
	if (_preLen)
		s_indent.resize(s_indent.size() - 2);
#endif
}

void hash256aux(HexMap const& _s, HexMap::const_iterator _begin, HexMap::const_iterator _end, unsigned _preLen, RLPStream& _rlp)
{
	RLPStream rlp;
	hash256rlp(_s, _begin, _end, _preLen, rlp);
	if (rlp.out().size() < 32)
	{
		// RECURSIVE RLP
#if ENABLE_DEBUG_PRINT
		cerr << "[INLINE: " << dec << rlp.out().size() << " < 32]" << endl;
#endif
		_rlp.APPEND_CHILD(rlp.out());
	}
	else
	{
#if ENABLE_DEBUG_PRINT
		cerr << "[HASH: " << dec << rlp.out().size() << " >= 32]" << endl;
#endif
		_rlp << sha3(rlp.out());
	}
}

h256 hash256(StringMap const& _s)
{
	// build patricia tree.
	if (_s.empty())
		return sha3(RLPNull);
	HexMap hexMap;
	for (auto i = _s.rbegin(); i != _s.rend(); ++i)
		hexMap[toHex(i->first)] = i->second;
	RLPStream s;
	hash256rlp(hexMap, hexMap.cbegin(), hexMap.cend(), 0, s);
	return sha3(s.out());
}

bytes rlp256(StringMap const& _s)
{
	// build patricia tree.
	if (_s.empty())
		return RLPNull;
	HexMap hexMap;
	for (auto i = _s.rbegin(); i != _s.rend(); ++i)
		hexMap[toHex(i->first)] = i->second;
	RLPStream s;
	hash256aux(hexMap, hexMap.cbegin(), hexMap.cend(), 0, s);
	return s.out();
}

h256 hash256(u256Map const& _s)
{
	// build patricia tree.
	if (_s.empty())
		return sha3(RLPNull);
	HexMap hexMap;
	for (auto i = _s.rbegin(); i != _s.rend(); ++i)
		hexMap[toHex(toBigEndianString(i->first))] = asString(rlp(i->second));
	RLPStream s;
	hash256rlp(hexMap, hexMap.cbegin(), hexMap.cend(), 0, s);
	return sha3(s.out());
}

}
