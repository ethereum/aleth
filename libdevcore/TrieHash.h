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
/** @file TrieHash.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <libdevcore/FixedHash.h>

#include <vector>

namespace dev
{

bytes rlp256(BytesMap const& _s);
h256 hash256(BytesMap const& _s);

h256 orderedTrieRoot(std::vector<bytes> const& _data);

template <class T, class U> inline h256 trieRootOver(unsigned _itemCount, T const& _getKey, U const& _getValue)
{
	BytesMap m;
	for (unsigned i = 0; i < _itemCount; ++i)
		m[_getKey(i)] = _getValue(i);
	return hash256(m);
}

h256 orderedTrieRoot(std::vector<bytesConstRef> const& _data);
h256 orderedTrieRoot(std::vector<bytes> const& _data);

}
