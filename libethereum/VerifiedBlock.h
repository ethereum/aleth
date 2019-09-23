// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.



#include <libdevcore/Common.h>
#include <libethcore/BlockHeader.h>

#pragma once

namespace dev
{
namespace eth
{

class Transaction;

/// @brief Verified block info, does not hold block data, but a reference instead
struct VerifiedBlockRef
{
	bytesConstRef block; 					///<  Block data reference
	BlockHeader info;							///< Prepopulated block info
	std::vector<Transaction> transactions;	///< Verified list of block transactions
};

/// @brief Verified block info, combines block data and verified info/transactions
struct VerifiedBlock
{
	VerifiedBlock() {}

	VerifiedBlock(BlockHeader&& _bi)
	{
		verified.info = std::move(_bi);
	}

	VerifiedBlock(VerifiedBlock&& _other):
		verified(std::move(_other.verified)),
		blockData(std::move(_other.blockData))
	{
	}

	VerifiedBlock& operator=(VerifiedBlock&& _other)
	{
		assert(&_other != this);

		verified = std::move(_other.verified);
		blockData = std::move(_other.blockData);
		return *this;
	}

	VerifiedBlockRef verified;				///< Verified block structures
	bytes blockData;						///< Block data

private:
	VerifiedBlock(VerifiedBlock const&) = delete;
	VerifiedBlock operator=(VerifiedBlock const&) = delete;
};

using VerifiedBlocks = std::vector<VerifiedBlock>;

}
}
