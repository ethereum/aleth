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
 *  Class for importing blocks into the blockchain
 */

#pragma once

#include <libdevcore/Common.h>

#include <memory>

namespace dev
{

class RLP;

namespace eth
{

class BlockChain;
class BlockHeader;

class BlockChainImporterFace
{
public:
	virtual ~BlockChainImporterFace() = default;

	virtual void importBlock(BlockHeader const& _header, RLP _transactions, RLP _uncles, RLP _receipts, u256 const& _totalDifficulty) = 0;

	virtual void setChainStartBlockNumber(u256 const& _number) = 0;
};

std::unique_ptr<BlockChainImporterFace> createBlockChainImporter(BlockChain& _blockChain);

}
}
