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
/** @file IPFS.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */
#pragma once

#include <libdevcore/Exceptions.h>
#include <libdevcore/Common.h>
#include <libdevcore/FixedHash.h>


namespace dev
{

DEV_SIMPLE_EXCEPTION(IPFSExecutableNotFound);

class IPFS
{
public:
	/// Places data into IPFS and @returns SHA2-256 hash of the data.
	h256 putBlockForSHA256(bytesConstRef _data);
	/// Places data into IPFS and @returns multihash of the data.
	bytes putBlock(bytesConstRef _data);

	/// Retrieves data from IPFS SHA2-256 hash is given.
	bytes getBlockForSHA256(h256 const& _sha256);
	/// Retrieves data from IPFS whose multihash is given.
	bytes getBlock(bytesConstRef _multihash);

	static h256 multihashAsSHA256(bytesConstRef _b)
	{
		if (_b.size() == 34 && _b[0] == 0x12 && _b[1] == 0x20)
			return h256(_b.cropped(2));
		throw std::invalid_argument("Given data is not a SHA2-256 multihash");
	}

	static bytes sha256AsMultihash(h256 const& _h)
	{
		return bytes{0x12, 0x20} + _h.asBytes();
	}
};

}
