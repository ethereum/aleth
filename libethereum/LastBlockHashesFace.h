// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

/// @file
/// Interface for getting a list of recent block hashes
#pragma once

#include <libdevcore/FixedHash.h>


namespace dev
{

namespace eth
{

/**
* @brief Interface for getting a list of recent block hashes
* @threadsafe
*/
class LastBlockHashesFace
{
public:
	virtual ~LastBlockHashesFace() {}

	/// Get hashes of 256 consecutive blocks preceding and including @a _mostRecentHash
	/// Hashes are returned in the order of descending height,
	/// i.e. result[0] is @a _mostRecentHash, result[1] is its parent, result[2] is grandparent etc.
	virtual h256s precedingHashes(h256 const& _mostRecentHash) const = 0;

	/// Clear any cached result
	virtual void clear() = 0;
};

}
}
