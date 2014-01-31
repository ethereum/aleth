/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Foobar is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file BlockInfo.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "Common.h"
#include "Dagger.h"
#include "Exceptions.h"
#include "RLP.h"
#include "BlockInfo.h"
using namespace std;
using namespace eth;

BlockInfo* BlockInfo::s_genesis = nullptr;

BlockInfo::BlockInfo(): timestamp(Invalid256)
{
}

BlockInfo::BlockInfo(bytesConstRef _block)
{
	populate(_block);
}

bytes BlockInfo::createGenesisBlock()
{
	RLPStream block(3);
	auto sha3EmptyList = sha3(RLPEmptyList);
	block.appendList(9) << h256() << sha3EmptyList << h160() << sha3(RLPNull) << sha3EmptyList << c_genesisDifficulty << (uint)0 << string() << (uint)0;
	block.appendRaw(RLPEmptyList);
	block.appendRaw(RLPEmptyList);
	return block.out();
}

h256 BlockInfo::headerHashWithoutNonce() const
{
	RLPStream s;
	fillStream(s, false);
	return sha3(s.out());
}

void BlockInfo::fillStream(RLPStream& _s, bool _nonce) const
{
	_s.appendList(_nonce ? 9 : 8) << parentHash << sha3Uncles << coinbaseAddress << stateRoot << sha3Transactions << difficulty << timestamp << extraData;
	if (_nonce)
		_s << nonce;
}

void BlockInfo::populateGenesis()
{
	bytes genesisBlock = createGenesisBlock();
	populate(&genesisBlock);
}

void BlockInfo::populate(bytesConstRef _block)
{
	RLP root(_block);
	try
	{
		RLP header = root[0];
		hash = eth::sha3(_block);
		parentHash = header[0].toHash<h256>();
		sha3Uncles = header[1].toHash<h256>();
		coinbaseAddress = header[2].toHash<Address>();
		stateRoot = header[3].toHash<h256>();
		sha3Transactions = header[4].toHash<h256>();
		difficulty = header[5].toInt<u256>();
		timestamp = header[6].toInt<u256>();
		extraData = header[7].toBytes();
		nonce = header[8].toInt<u256>();
	}
	catch (RLP::BadCast)
	{
		throw InvalidBlockFormat();
	}

	// check it hashes according to proof of work or that it's the genesis block.
	if (parentHash && !Dagger::verify(headerHashWithoutNonce(), nonce, difficulty))
		throw InvalidNonce();
}

void BlockInfo::verifyInternals(bytesConstRef _block) const
{
	RLP root(_block);

	if (sha3Transactions != sha3(root[1].data()))
		throw InvalidTransactionsHash();

	if (sha3Uncles != sha3(root[2].data()))
		throw InvalidUnclesHash();
}

u256 BlockInfo::calculateDifficulty(BlockInfo const& _parent) const
{
	if (!parentHash)
		return c_genesisDifficulty;
	else
		return timestamp >= _parent.timestamp + 42 ? _parent.difficulty - (_parent.difficulty >> 10) : (_parent.difficulty + (_parent.difficulty >> 10));
}

void BlockInfo::verifyParent(BlockInfo const& _parent) const
{
	// Check difficulty is correct given the two timestamps.
	if (difficulty != calculateDifficulty(_parent))
		throw InvalidDifficulty();

	// Check timestamp is after previous timestamp.
	if (parentHash && _parent.timestamp >= timestamp)
		throw InvalidTimestamp();
}
