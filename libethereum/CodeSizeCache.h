/*
	This file is part of aleth.

	aleth is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	aleth is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with aleth.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file CodeSizeCache.h
 * @date 2016
 */

#pragma once

#include <map>
#include <libdevcore/FixedHash.h>
#include <libdevcore/Guards.h>

namespace dev
{
namespace eth
{

/**
 * @brief Simple thread-safe cache to store a mapping from code hash to code size.
 * If the cache is full, a random element is removed.
 */
class CodeSizeCache
{
public:
	void store(h256 const& _hash, size_t size)
	{
		UniqueGuard g(x_cache);
		if (m_cache.size() >= c_maxSize)
			removeRandomElement();
		m_cache[_hash] = size;
	}
	bool contains(h256 const& _hash) const
	{
		UniqueGuard g(x_cache);
		return m_cache.count(_hash);
	}
	size_t get(h256 const& _hash) const
	{
		UniqueGuard g(x_cache);
		return m_cache.at(_hash);
	}

	static CodeSizeCache& instance() { static CodeSizeCache cache; return cache; }

private:
	/// Removes a random element from the cache.
	void removeRandomElement()
	{
		if (!m_cache.empty())
		{
			auto it = m_cache.lower_bound(h256::random());
			if (it == m_cache.end())
				it = m_cache.begin();
			m_cache.erase(it);
		}
	}

	static const size_t c_maxSize = 50000;
	mutable Mutex x_cache;
	std::map<h256, size_t> m_cache;
};

}
}

