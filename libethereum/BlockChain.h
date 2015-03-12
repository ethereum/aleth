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

#pragma warning(push)
#pragma warning(disable: 4100 4267)
#include <leveldb/db.h>
#pragma warning(pop)

#include <deque>
#include <chrono>
#include <libdevcore/Log.h>
#include <libdevcore/Exceptions.h>
#include <libethcore/Common.h>
#include <libethcore/BlockInfo.h>
#include <libdevcore/Guards.h>
#include "BlockDetails.h"
#include "Account.h"
#include "Transaction.h"
#include "BlockQueue.h"
namespace ldb = leveldb;

namespace dev
{

class OverlayDB;

namespace eth
{

static const h256s NullH256s;

class State;

struct AlreadyHaveBlock: virtual Exception {};
struct UnknownParent: virtual Exception {};
struct FutureTime: virtual Exception {};

struct BlockChainChat: public LogChannel { static const char* name() { return "-B-"; } static const int verbosity = 7; };
struct BlockChainNote: public LogChannel { static const char* name() { return "=B="; } static const int verbosity = 4; };

// TODO: Move all this Genesis stuff into Genesis.h/.cpp
std::map<Address, Account> const& genesisState();

ldb::Slice toSlice(h256 _h, unsigned _sub = 0);

using BlocksHash = std::map<h256, bytes>;
using TransactionHashes = h256s;

enum {
	ExtraDetails = 0,
	ExtraBlockHash,
	ExtraTransactionAddress,
	ExtraLogBlooms,
	ExtraReceipts,
	ExtraBlocksBlooms
};

/**
 * @brief Implements the blockchain database. All data this gives is disk-backed.
 * @threadsafe
 * @todo Make not memory hog (should actually act as a cache and deallocate old entries).
 */
class BlockChain
{
public:
	BlockChain(bytes const& _genesisBlock, std::string _path, bool _killExisting);
	~BlockChain();

	void reopen(std::string _path, bool _killExisting = false) { close(); open(_path, _killExisting); }

	/// (Potentially) renders invalid existing bytesConstRef returned by lastBlock.
	/// To be called from main loop every 100ms or so.
	void process();

	/// Sync the chain with any incoming blocks. All blocks should, if processed in order
	h256s sync(BlockQueue& _bq, OverlayDB const& _stateDB, unsigned _max);

	/// Attempt to import the given block directly into the CanonBlockChain and sync with the state DB.
	/// @returns the block hashes of any blocks that came into/went out of the canonical block chain.
	h256s attemptImport(bytes const& _block, OverlayDB const& _stateDB) noexcept;

	/// Import block into disk-backed DB
	/// @returns the block hashes of any blocks that came into/went out of the canonical block chain.
	h256s import(bytes const& _block, OverlayDB const& _stateDB);

	/// Returns true if the given block is known (though not necessarily a part of the canon chain).
	bool isKnown(h256 _hash) const;

	/// Get the familial details concerning a block (or the most recent mined if none given). Thread-safe.
	BlockInfo info(h256 _hash) const { return BlockInfo(block(_hash)); }
	BlockInfo info() const { return BlockInfo(block()); }

	/// Get a block (RLP format) for the given hash (or the most recent mined if none given). Thread-safe.
	bytes block(h256 _hash) const;
	bytes block() const { return block(currentHash()); }

	/// Get the familial details concerning a block (or the most recent mined if none given). Thread-safe.
	BlockDetails details(h256 _hash) const { return queryExtras<BlockDetails, ExtraDetails>(_hash, m_details, x_details, NullBlockDetails); }
	BlockDetails details() const { return details(currentHash()); }

	/// Get the transactions' log blooms of a block (or the most recent mined if none given). Thread-safe.
	BlockLogBlooms logBlooms(h256 _hash) const { return queryExtras<BlockLogBlooms, ExtraLogBlooms>(_hash, m_logBlooms, x_logBlooms, NullBlockLogBlooms); }
	BlockLogBlooms logBlooms() const { return logBlooms(currentHash()); }

	/// Get the transactions' receipts of a block (or the most recent mined if none given). Thread-safe.
	BlockReceipts receipts(h256 _hash) const { return queryExtras<BlockReceipts, ExtraReceipts>(_hash, m_receipts, x_receipts, NullBlockReceipts); }
	BlockReceipts receipts() const { return receipts(currentHash()); }

	/// Get a list of transaction hashes for a given block. Thread-safe.
	TransactionHashes transactionHashes(h256 _hash) const { auto b = block(_hash); RLP rlp(b); h256s ret; for (auto t: rlp[1]) ret.push_back(sha3(t.data())); return ret; }
	TransactionHashes transactionHashes() const { return transactionHashes(currentHash()); }

	/// Get a list of transaction hashes for a given block. Thread-safe.
	h256 numberHash(u256 _index) const { if (!_index) return genesisHash(); return queryExtras<BlockHash, ExtraBlockHash>(h256(_index), m_blockHashes, x_blockHashes, NullBlockHash).value; }

	/** Get the block blooms for a number of blocks. Thread-safe.
	 * @returns the object pertaining to the blocks:
	 * level 0:
	 * 0x, 0x + 1, .. (1x - 1)
	 * 1x, 1x + 1, .. (2x - 1)
	 * ...
	 * (255x .. (256x - 1))
	 * level 1:
	 * 0x .. (1x - 1), 1x .. (2x - 1), ..., (255x .. (256x - 1))
	 * 256x .. (257x - 1), 257x .. (258x - 1), ..., (511x .. (512x - 1))
	 * ...
	 * level n, index i, offset o:
	 * i * (x ^ n) + o * x ^ (n - 1)
	 */
	BlocksBlooms blocksBlooms(unsigned _level, unsigned _index) const { return blocksBlooms(chunkId(_level, _index)); }
	BlocksBlooms blocksBlooms(h256 const& _chunkId) const { return queryExtras<BlocksBlooms, ExtraBlocksBlooms>(_chunkId, m_blocksBlooms, x_blocksBlooms, NullBlocksBlooms); }
	LogBloom blockBloom(unsigned _number) const { return blocksBlooms(chunkId(0, _number / c_bloomIndexSize)).blooms[_number % c_bloomIndexSize]; }
	std::vector<unsigned> withBlockBloom(LogBloom const& _b, unsigned _earliest, unsigned _latest) const;
	std::vector<unsigned> withBlockBloom(LogBloom const& _b, unsigned _earliest, unsigned _latest, unsigned _topLevel, unsigned _index) const;

	/// Get a transaction from its hash. Thread-safe.
	bytes transaction(h256 _transactionHash) const { TransactionAddress ta = queryExtras<TransactionAddress, ExtraTransactionAddress>(_transactionHash, m_transactionAddresses, x_transactionAddresses, NullTransactionAddress); if (!ta) return bytes(); return transaction(ta.blockHash, ta.index); }

	/// Get a block's transaction (RLP format) for the given block hash (or the most recent mined if none given) & index. Thread-safe.
	bytes transaction(h256 _blockHash, unsigned _i) const { bytes b = block(_blockHash); return RLP(b)[1][_i].data().toBytes(); }
	bytes transaction(unsigned _i) const { return transaction(currentHash(), _i); }

	/// Get a number for the given hash (or the most recent mined if none given). Thread-safe.
	unsigned number(h256 _hash) const { return details(_hash).number; }
	unsigned number() const { return number(currentHash()); }

	/// Get a given block (RLP format). Thread-safe.
	h256 currentHash() const { ReadGuard l(x_lastBlockHash); return m_lastBlockHash; }

	/// Get the hash of the genesis block. Thread-safe.
	h256 genesisHash() const { return m_genesisHash; }

	/// Get all blocks not allowed as uncles given a parent (i.e. featured as uncles/main in parent, parent + 1, ... parent + 5).
	/// @returns set including the header-hash of every parent (including @a _parent) up to and including generation +5
	/// togther with all their quoted uncles.
	h256Set allUnclesFrom(h256 _parent) const;

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
	h256s treeRoute(h256 _from, h256 _to, h256* o_common = nullptr, bool _pre = true, bool _post = true) const;

	struct Statistics
	{
		unsigned memBlocks;
		unsigned memDetails;
		unsigned memLogBlooms;
		unsigned memReceipts;
		unsigned memTransactionAddresses;
		unsigned memBlockHashes;
		unsigned memTotal() const { return memBlocks + memDetails + memLogBlooms + memReceipts + memTransactionAddresses + memBlockHashes; }
	};

	/// @returns statistics about memory usage.
	Statistics usage(bool _freshen = false) const { if (_freshen) updateStats(); return m_lastStats; }

	/// Deallocate unused data.
	void garbageCollect(bool _force = false);

private:
	static h256 chunkId(unsigned _level, unsigned _index) { return h256(_index * 0xff + _level); }

	void open(std::string _path, bool _killExisting = false);
	void close();

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

		noteUsed(_h, N);

		WriteGuard l(_x);
		auto ret = _m.insert(std::make_pair(_h, T(RLP(s))));
		return ret.first->second;
	}

	void checkConsistency();

	/// The caches of the disk DB and their locks.
	mutable SharedMutex x_blocks;
	mutable BlocksHash m_blocks;
	mutable SharedMutex x_details;
	mutable BlockDetailsHash m_details;
	mutable SharedMutex x_logBlooms;
	mutable BlockLogBloomsHash m_logBlooms;
	mutable SharedMutex x_receipts;
	mutable BlockReceiptsHash m_receipts;
	mutable SharedMutex x_transactionAddresses;
	mutable TransactionAddressHash m_transactionAddresses;
	mutable SharedMutex x_blockHashes;
	mutable BlockHashHash m_blockHashes;
	mutable SharedMutex x_blocksBlooms;
	mutable BlocksBloomsHash m_blocksBlooms;

	using CacheID = std::pair<h256, unsigned>;
	mutable Mutex x_cacheUsage;
	mutable std::deque<std::set<CacheID>> m_cacheUsage;
	mutable std::set<CacheID> m_inUse;
	void noteUsed(h256 const& _h, unsigned _extra = (unsigned)-1) const;
	std::chrono::system_clock::time_point m_lastCollection;

	void updateStats() const;
	mutable Statistics m_lastStats;

	/// The disk DBs. Thread-safe, so no need for locks.
	ldb::DB* m_blocksDB;
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
};

std::ostream& operator<<(std::ostream& _out, BlockChain const& _bc);

}
}
