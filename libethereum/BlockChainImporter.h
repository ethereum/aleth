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

#include "BlockChain.h"

#include <libdevcore/Common.h>
#include <libdevcore/RLP.h>
#include <libethcore/BlockHeader.h>

namespace dev
{

namespace eth
{

class BlockChainImporter
{
public:
	explicit BlockChainImporter(BlockChain& _blockChain): m_blockChain(_blockChain) {}

	void importBlock(BlockHeader const& _header, RLP _transactions, RLP _uncles, RLP _receipts, u256 const& _number, u256 const& _totalDifficulty)
	{
		RLPStream headerRlp;
		_header.streamRLP(headerRlp);

		RLPStream block(3);
		block.appendRaw(headerRlp.out());
		block << _transactions << _uncles;

		m_blockChain.insertWithoutParent(block.out(), _receipts.data(), _number, _totalDifficulty);
	}

	void setChainStartBlockNumber(u256 const& _number)
	{
		m_blockChain.setChainStartBlockNumber(static_cast<unsigned>(_number));
	}

private:
	BlockChain& m_blockChain;
};

}
}
