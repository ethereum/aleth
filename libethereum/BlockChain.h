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
/** @file BlockChain.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include "Common.h"
namespace ldb = leveldb;

namespace eth
{

struct Defaults
{
	friend class BlockChain;
	friend class State;
public:
	static void setDBPath(std::string _dbPath) { s_dbPath = _dbPath; }

private:
	static std::string s_dbPath;
};

class RLP;
class RLPStream;

struct BlockDetails
{
	BlockDetails(): number(0), totalDifficulty(0) {}
	BlockDetails(uint _n, u256 _tD, h256 _p, h256s _c): number(_n), totalDifficulty(_tD), parent(_p), children(_c) {}
	BlockDetails(RLP const& _r);
	bytes rlp() const;

	bool isNull() const { return !totalDifficulty; }
	explicit operator bool() const { return !isNull(); }

	uint number;
	u256 totalDifficulty;
	h256 parent;
	h256s children;
};

static const BlockDetails NullBlockDetails;
static const h256s NullH256s;

class Overlay;

class AlreadyHaveBlock: public std::exception {};
class UnknownParent: public std::exception {};

/**
 * @brief Implements the blockchain database. All data this gives is disk-backed.
 */
class BlockChain
{
public:
	BlockChain(bool _killExisting = false): BlockChain(std::string(), _killExisting) {}
	BlockChain(std::string _path, bool _killExisting = false);
	~BlockChain();

	/// (Potentially) renders invalid existing bytesConstRef returned by lastBlock.
	/// To be called from main loop every 100ms or so.
	void process();
	
	/// Attempt to import the given block.
	bool attemptImport(bytes const& _block, Overlay const& _stateDB) { try { import(_block, _stateDB); return true; } catch (...) { return false; } }

	/// Import block into disk-backed DB
	void import(bytes const& _block, Overlay const& _stateDB);

	/// Get the number of the last block of the longest chain.
	BlockDetails const& details(h256 _hash) const;
	BlockDetails const& details() const { return details(currentHash()); }

	/// Get a given block (RLP format).
	bytesConstRef block(h256 _hash) const;
	bytesConstRef block() const { return block(currentHash()); }

	/// Get a given block (RLP format).
	h256 currentHash() const { return m_lastBlockHash; }

	/// Get the coinbase address of a given block.
	Address coinbaseAddress(h256 _hash) const;
	Address coinbaseAddress() const { return coinbaseAddress(currentHash()); }

	h256 genesisHash() const { return m_genesisHash; }

private:
	void checkConsistency();

	/// Get fully populated from disk DB.
	mutable std::map<h256, BlockDetails> m_details;
	mutable std::map<h256, std::string> m_cache;

	ldb::DB* m_db;
	ldb::DB* m_detailsDB;

	/// Hash of the last (valid) block on the longest chain.
	h256 m_lastBlockHash;
	h256 m_genesisHash;
	bytes m_genesisBlock;

	ldb::ReadOptions m_readOptions;
	ldb::WriteOptions m_writeOptions;

	friend std::ostream& operator<<(std::ostream& _out, BlockChain const& _bc);
};

std::ostream& operator<<(std::ostream& _out, BlockChain const& _bc);

}
