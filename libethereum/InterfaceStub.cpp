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
/** @file InterfaceStub.h
 * @author Marek Kotewicz <marek@ethdev.com>
 * @date 2015
 */

#include "InterfaceStub.h"
#include "BlockChain.h"

using namespace std;
using namespace dev;
using namespace dev::eth;

u256 InterfaceStub::balanceAt(Address _a, int _block) const
{
	return asOf(_block).balance(_a);
}

h256 InterfaceStub::hashFromNumber(unsigned _number) const
{
	return bc().numberHash(_number);
}

unsigned InterfaceStub::transactionCount(h256 _blockHash) const
{
	auto bl = bc().block(_blockHash);
	RLP b(bl);
	return b[1].itemCount();
}

unsigned InterfaceStub::uncleCount(h256 _blockHash) const
{
	auto bl = bc().block(_blockHash);
	RLP b(bl);
	return b[2].itemCount();
}

unsigned InterfaceStub::number() const
{
	return bc().number();
}
