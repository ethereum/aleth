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
 *  Interface for getting a list of recent block hashes
 */

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
