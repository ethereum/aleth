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
/** @file AddressState.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "AddressState.h"
#include <libethcore/CommonEth.h>
using namespace std;
using namespace dev;
using namespace dev::eth;

#pragma GCC diagnostic ignored "-Wunused-variable"
namespace { char dummy; };

bytes const& AddressState::code() const
{
	if (m_codeHash != EmptySHA3 && m_codeHash && !m_codeCache.size())
		BOOST_THROW_EXCEPTION(Exception() << errinfo_comment("Can't get the code"));
	return m_codeCache;
}

void AddressState::noteCode(bytesConstRef _code)
{
	if (sha3(_code) != m_codeHash)
		BOOST_THROW_EXCEPTION(BadHash() << errinfo_comment("m_code_hash does not match hash of code ")); m_codeCache = _code.toBytes();
}
