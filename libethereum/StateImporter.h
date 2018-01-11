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
/** @file 
 *  Class for importing accounts into the State Trie
 */

#pragma once

#include <libdevcore/Common.h>
#include <libdevcore/Exceptions.h>
#include <libdevcore/FixedHash.h>

#include <memory>

namespace dev
{

class OverlayDB;

namespace eth
{

DEV_SIMPLE_EXCEPTION(InvalidAccountInTheDatabase);

class StateImporterFace
{
public:
	virtual ~StateImporterFace() = default;

	virtual void importAccount(h256 const& _addressHash, u256 const& _nonce, u256 const& _balance, std::map<h256, bytes> const& _storage, h256 const& _codeHash) = 0;

	virtual h256 importCode(bytesConstRef _code) = 0;

	virtual void commitStateDatabase() = 0;

	virtual bool isAccountImported(h256 const& _addressHash) const = 0;

	virtual h256 stateRoot() const = 0;

	virtual std::string lookupCode(h256 const& _hash) const = 0;
};
		
std::unique_ptr<StateImporterFace> createStateImporter(OverlayDB& _stateDb);

}
}
