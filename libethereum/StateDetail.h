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
/** @file StateDetail.h
 */

#pragma once

namespace dev
{

namespace  eth
{

namespace detail
{

/// An atomic state changelog entry.
struct Change
{
	enum Kind: int
	{
		/// Account balance changed. Change::value contains the amount the
		/// balance was increased by.
		Balance,

		/// Account storage was modified. Change::key contains the storage key,
		/// Change::value the storage value.
		Storage,

		/// Account nonce was increased by 1.
		Nonce,

		/// Account was created (it was not existing before).
		Create,

		/// New code was added to an account (by "create" message execution).
		NewCode,

		/// Account was touched for the first time.
		Touch
	};

	Kind kind;        ///< The kind of the change.
	Address address;  ///< Changed account address.
	u256 value;       ///< Change value, e.g. balance, storage.
	u256 key;         ///< Storage key. Last because used only in one case.

	/// Helper constructor to make change log update more readable.
	Change(Kind _kind, Address const& _addr, u256 const& _value = 0):
			kind(_kind), address(_addr), value(_value)
	{}

	/// Helper constructor especially for storage change log.
	Change(Address const& _addr, u256 const& _key, u256 const& _value):
			kind(Storage), address(_addr), value(_value), key(_key)
	{}
};

} //detail
} //eth
} //dev
