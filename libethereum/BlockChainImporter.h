// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

/// @file
/// Class for importing blocks into the blockchain.
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
