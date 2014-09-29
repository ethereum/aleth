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
/** @file PastMessage.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "PastMessage.h"
using namespace std;
using namespace dev;
using namespace dev::eth;

#pragma GCC diagnostic ignored "-Wunused-variable"
namespace { char dummy; };

PastMessage::PastMessage(bytesConstRef _r)
{
	RLP r(_r);
	to = r[0].toHash<Address>();
	from = r[1].toHash<Address>();
	value = r[2].toHash<h256>();
	input = r[3].toBytes();
	output = r[4].toBytes();
	if (r[5].itemCount() > 0)
		for (auto p: r[5])
			path.push_back(p.toInt<unsigned>());
	origin = r[6].toHash<Address>();
	coinbase = r[7].toHash<Address>();
	block = r[8].toHash<h256>();
	timestamp = r[9].toInt<u256>();
	number = r[10].toInt<unsigned>();
}

void PastMessage::streamOut(RLPStream& _s) const {
	_s.appendList(11) << to << from << value << input << output << path << origin << coinbase << block << timestamp << number;
}

