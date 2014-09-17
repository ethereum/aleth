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
/** @file BlockChain.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <mutex>
#include <libethential/Log.h>
#include <libethcore/CommonEth.h>
#include <libethcore/BlockInfo.h>
#include "Guards.h"
#include "BlockDetails.h"
#include "AddressState.h"
#include "BlockQueue.h"
#include "BlockChainListener.h"

namespace ldb = leveldb;

namespace eth
{

static const h256s NullH256s;

class State;
class OverlayDB;

class AlreadyHaveBlock: public std::exception {};
class UnknownParent: public std::exception {};
class FutureTime: public std::exception {};

struct BlockChainChat: public LogChannel { static const char* name() { return "-B-"; } static const int verbosity = 7; };
struct BlockChainNote: public LogChannel { static const char* name() { return "=B="; } static const int verbosity = 4; };

// TODO: Move all this Genesis stuff into Genesis.h/.cpp
std::map<Address, AddressState> const& genesisState();

ldb::Slice toSlice(h256 _h, unsigned _sub = 0);

/**
 * @brief Implements the blockchain database. All data this gives is disk-backed.
 * @threadsafe
 * @todo Make not memory hog (should actually act as a cache and deallocate old entries).
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

	/// Sync the chain with any incoming blocks. All blocks should, if processed in order
	h256s sync(BlockQueue& _bq, OverlayDB const& _stateDB, unsigned _max, BlockChainListener*_listener = nullptr);

	/// Attempt to import the given block directly into the BlockChain and sync with the state DB.
	/// @returns the block hashes of any blocks that came into/went out of the canonical block chain.
	h256s attemptImport(bytes const& _block, OverlayDB const& _stateDB) noexcept;

	/// Import block into disk-backed DB
	/// @returns the block hashes of any blocks that came into/went out of the canonical block chain.
	h256s import(bytes const& _block, OverlayDB const& _stateDB, BlockChainListener* _listener = nullptr);

	/// Get the familial details concerning a block (or the most recent mined if none given). Thread-safe.
	BlockDetails details(h256 _hash) const { return queryExtras<BlockDetails, 0>(_hash, m_details, x_details, NullBlockDetails); }
	BlockDetails details() const { return details(currentHash()); }

	/// Get the transactions' bloom filters of a block (or the most recent mined if none given). Thread-safe.
	BlockBlooms blooms(h256 _hash) const { return queryExtras<BlockBlooms, 1>(_hash, m_blooms, x_blooms, NullBlockBlooms); }
	BlockBlooms blooms() const { return blooms(currentHash()); }

	/// Get the transactions' trace manifests of a block (or the most recent mined if none given). Thread-safe.
	BlockTraces traces(h256 _hash) const { return queryExtras<BlockTraces, 2>(_hash, m_traces, x_traces, NullBlockTraces); }
	BlockTraces traces() const { return traces(currentHash()); }

	/// Get a block (RLP format) for the given hash (or the most recent mined if none given). Thread-safe.
	bytes block(h256 _hash) const;
	bytes block() const { return block(currentHash()); }

	/// Get a number for the given hash (or the most recent mined if none given). Thread-safe.
	uint number(h256 _hash) const { return details(_hash).number; }
	uint number() const { return number(currentHash()); }

	/// Get a given block (RLP format). Thread-safe.
	h256 currentHash() const { ReadGuard l(x_lastBlockHash); return m_lastBlockHash; }

	/// Get the hash of the genesis block. Thread-safe.
	h256 genesisHash() const { return m_genesisHash; }

	/// Get the hash of a block of a given number. Slow; try not to use it too much.
	h256 numberHash(unsigned _n) const;

	/// @returns the genesis block header.
	static BlockInfo const& genesis() { UpgradableGuard l(x_genesis); if (!s_genesis) { auto gb = createGenesisBlock(); UpgradeGuard ul(l); (s_genesis = new BlockInfo)->populate(&gb); } return *s_genesis; }

	/// @returns the genesis block as its RLP-encoded byte array.
	/// @note This is slow as it's constructed anew each call. Consider genesis() instead.
	static bytes createGenesisBlock();

	/** @returns the hash of all blocks between @a _from and @a _to, all blocks are ordered first by a number of
	 * blocks that are parent-to-child, then two sibling blocks, then a number of blocks that are child-to-parent.
	 *
	 * If non-null, the h256 at @a o_common is set to the latest common ancestor of both blocks.
	 *
	 * e.g. if the block tree is 3a -> 2a -> 1a -> g and 2b -> 1b -> g (g is genesis, *a, *b are competing chains),
	 * then:
	 * @code
	 * treeRoute(3a, 2b) == { 3a, 2a, 1a, 1b, 2b }; // *o_common == g
	 * treeRoute(2a, 1a) == { 2a, 1a }; // *o_common == 1a
	 * treeRoute(1a, 2a) == { 1a, 2a }; // *o_common == 1a
	 * treeRoute(1b, 2a) == { 1b, 1a, 2a }; // *o_common == g
	 * @endcode
	 */
	h256s treeRoute(h256 _from, h256 _to, h256* o_common = nullptr) const;

    private:

	template<class T, unsigned N> T queryExtras(h256 _h, std::map<h256, T>& _m, boost::shared_mutex& _x, T const& _n) const
	{
		{
			ReadGuard l(_x);
			auto it = _m.find(_h);
			if (it != _m.end())
				return it->second;
		}

		std::string s;
		m_extrasDB->Get(m_readOptions, toSlice(_h, N), &s);
		if (s.empty())
		{
	//			cout << "Not found in DB: " << _h << endl;
			return _n;
		}

		WriteGuard l(_x);
		auto ret = _m.insert(std::make_pair(_h, T(RLP(s))));
		return ret.first->second;
	}

	void checkConsistency();

	/// The caches of the disk DB and their locks.
	mutable boost::shared_mutex x_details;
	mutable BlockDetailsHash m_details;
	mutable boost::shared_mutex x_blooms;
	mutable BlockBloomsHash m_blooms;
	mutable boost::shared_mutex x_traces;
	mutable BlockTracesHash m_traces;
	mutable boost::shared_mutex x_cache;
	mutable std::map<h256, bytes> m_cache;

	/// The disk DBs. Thread-safe, so no need for locks.
	ldb::DB* m_db;
	ldb::DB* m_extrasDB;

	/// Hash of the last (valid) block on the longest chain.
	mutable boost::shared_mutex x_lastBlockHash;
	h256 m_lastBlockHash;

	/// Genesis block info.
	h256 m_genesisHash;
	bytes m_genesisBlock;

	ldb::ReadOptions m_readOptions;
	ldb::WriteOptions m_writeOptions;

	friend std::ostream& operator<<(std::ostream& _out, BlockChain const& _bc);

	/// Static genesis info and its lock.
	static boost::shared_mutex x_genesis;
	static BlockInfo* s_genesis;
};

std::ostream& operator<<(std::ostream& _out, BlockChain const& _bc);

}
