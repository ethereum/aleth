// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

/// @file
/// Class for importing accounts into the State Trie
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
