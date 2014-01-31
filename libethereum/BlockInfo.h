/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	Foobar is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file BlockInfo.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include "Common.h"
#include "Transaction.h"

namespace eth
{

struct BlockInfo
{
public:
	h256 hash;						///< SHA3 hash of the entire block!
	h256 parentHash;
	h256 sha3Uncles;
	Address coinbaseAddress;
	h256 stateRoot;
	h256 sha3Transactions;
	u256 difficulty;
	u256 timestamp;
	bytes extraData;
	u256 nonce;

	BlockInfo();
	explicit BlockInfo(bytesConstRef _block);

	explicit operator bool() const { return timestamp != Invalid256; }

	bool operator==(BlockInfo const& _cmp) const
	{
		return parentHash == _cmp.parentHash &&
				sha3Uncles == _cmp.sha3Uncles &&
				coinbaseAddress == _cmp.coinbaseAddress &&
				stateRoot == _cmp.stateRoot &&
				sha3Transactions == _cmp.sha3Transactions &&
				difficulty == _cmp.difficulty &&
				timestamp == _cmp.timestamp &&
				extraData == _cmp.extraData &&
				nonce == _cmp.nonce;
	}
	bool operator!=(BlockInfo const& _cmp) const { return !operator==(_cmp); }

	static BlockInfo const& genesis() { if (!s_genesis) (s_genesis = new BlockInfo)->populateGenesis(); return *s_genesis; }
	void populate(bytesConstRef _block);
	void verifyInternals(bytesConstRef _block) const;
	void verifyParent(BlockInfo const& _parent) const;

	u256 calculateDifficulty(BlockInfo const& _bi) const;

	/// No-nonce sha3 of the header only.
	h256 headerHashWithoutNonce() const;
	void fillStream(RLPStream& _s, bool _nonce) const;

	static bytes createGenesisBlock();

private:
	void populateGenesis();

	static BlockInfo* s_genesis;
};

inline std::ostream& operator<<(std::ostream& _out, BlockInfo const& _bi)
{
	_out << _bi.hash << " " << _bi.parentHash << " " << _bi.sha3Uncles << " " << _bi.coinbaseAddress << " " << _bi.stateRoot << " " << _bi.sha3Transactions << " " <<
			_bi.difficulty << " " << _bi.timestamp << " " << _bi.nonce;
	return _out;
}

}


