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

#include "SHA3.h"
#include "RLP.h"

#include <ethash/keccak.hpp>

namespace dev
{
h256 const EmptySHA3 = sha3(bytesConstRef());
h256 const EmptyListSHA3 = sha3(rlpList());

bool sha3(bytesConstRef _input, bytesRef o_output) noexcept
{
    if (o_output.size() != 32)
        return false;
    ethash::hash256 h = ethash::keccak256(_input.data(), _input.size());
    bytesConstRef{h.bytes, 32}.copyTo(o_output);
    return true;
}
}  // namespace dev
