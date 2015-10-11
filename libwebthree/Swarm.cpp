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
/** @file Swarm.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#include "Swarm.h"

#include <libdevcore/Log.h>
#include <libdevcore/SHA3.h>
using namespace std;
using namespace dev;
using namespace dev::bzz;

Interface::~Interface()
{
}

h256 Client::put(bytes const& _data)
{
	h256 ret = sha3(_data);
	m_cache[ret] = _data;
	return ret;
}

bytes Client::get(h256 const& _hash)
{
	auto it = m_cache.find(_hash);
	if (it != m_cache.end())
		return it->second;

	return bytes();
}
