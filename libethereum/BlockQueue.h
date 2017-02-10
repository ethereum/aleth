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
/** @file BlockQueue.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <condition_variable>
#include <thread>
#include <deque>
#include <boost/thread.hpp>
#include <libdevcore/Common.h>
#include <libdevcore/Log.h>
#include <libethcore/Common.h>
#include <libdevcore/Guards.h>
#include <libethcore/Common.h>
#include <libethcore/BlockHeader.h>
#include "VerifiedBlock.h"

namespace dev
{

namespace eth
{

class BlockChain;

struct BlockQueueChannel: public LogChannel { static const char* name(); static const int verbosity = 4; };
struct BlockQueueTraceChannel: public LogChannel { static const char* name(); static const int verbosity = 7; };
#define cblockq dev::LogOutputStream<dev::eth::BlockQueueChannel, true>()

struct BlockQueueStatus
{
	size_t importing;
	size_t verified;
	size_t verifying;
	size_t unverified;
	size_t future;
	size_t unknown;
	size_t bad;
};

enum class QueueStatus
{
	Ready,
	Importing,
	UnknownParent,
	Bad,
	Unknown
};

std::ostream& operator<< (std::ostream& os, QueueStatus const& obj);

template<class T>
class SizedBlockQueue
{
public:
	std::size_t count() const { return m_queue.size(); }

	std::size_t size() const { return m_size; }
	
	bool isEmpty() const { return m_queue.empty(); }

	h256 nextHash() const { return m_queue.front().verified.info.sha3Uncles(); }

	T const& next() const { return m_queue.front(); }

	void clear()
	{
		m_queue.clear();
		m_size = 0;
	}

	void enqueue(T&& _t)
	{
		m_size += _t.blockData.size();
		m_queue.emplace_back(std::move(_t));
	}

	T dequeue()
	{
		T t;
		std::swap(t, m_queue.front());
		m_queue.pop_front();
		m_size -= t.blockData.size();

		return t;
	}

	std::vector<T> dequeueMultiple(std::size_t _n)
	{
		return removeRange(m_queue.begin(), m_queue.begin() + _n);
	}

	bool remove(h256 const& _hash)
	{
		std::vector<T> removed = removeIf(sha3UnclesEquals(_hash));
		return !removed.empty();
	}

	template<class Pred>
	std::vector<T> removeIf(Pred _pred)
	{
		auto const removedBegin = std::remove_if(m_queue.begin(), m_queue.end(), _pred);

		return removeRange(removedBegin, m_queue.end());
	}

	bool replace(h256 const& _hash, T&& _t)
	{
		auto const it = std::find_if(m_queue.begin(), m_queue.end(), sha3UnclesEquals(_hash));

		if (it == m_queue.end())
			return false;

		m_size -= it->blockData.size();
		m_size += _t.blockData.size();
		*it = std::move(_t);

		return true;
	}

private:
	static std::function<bool(T const&)> sha3UnclesEquals(h256 const& _hash)
	{
		return [&_hash](T const& _t) { return _t.verified.info.sha3Uncles() == _hash; };
	}

	std::vector<T> removeRange(typename std::deque<T>::iterator _begin, typename std::deque<T>::iterator _end)
	{
		std::vector<T> ret(std::make_move_iterator(_begin), std::make_move_iterator(_end));

		for (auto it = ret.begin(); it != ret.end(); ++it)
			m_size -= it->blockData.size();

		m_queue.erase(_begin, _end);
		return ret;
	}

	std::deque<T> m_queue;
	std::atomic<size_t> m_size;	///< Tracks total size in bytes
};

/**
 * @brief A queue of blocks. Sits between network or other I/O and the BlockChain.
 * Sorts them ready for blockchain insertion (with the BlockChain::sync() method).
 * @threadsafe
 */
class BlockQueue: HasInvariants
{
public:
	BlockQueue();
	~BlockQueue();

	void setChain(BlockChain const& _bc) { m_bc = &_bc; }

	/// Import a block into the queue.
	ImportResult import(bytesConstRef _block, bool _isOurs = false);

	/// Notes that time has moved on and some blocks that used to be "in the future" may no be valid.
	void tick();

	/// Grabs at most @a _max of the blocks that are ready, giving them in the correct order for insertion into the chain.
	/// Don't forget to call doneDrain() once you're done importing.
	void drain(std::vector<VerifiedBlock>& o_out, unsigned _max);

	/// Must be called after a drain() call. Notes that the drained blocks have been imported into the blockchain, so we can forget about them.
	/// @returns true iff there are additional blocks ready to be processed.
	bool doneDrain(h256s const& _knownBad = h256s());

	/// Notify the queue that the chain has changed and a new block has attained 'ready' status (i.e. is in the chain).
	void noteReady(h256 const& _b) { WriteGuard l(m_lock); noteReady_WITH_LOCK(_b); }

	/// Force a retry of all the blocks with unknown parents.
	void retryAllUnknown();

	/// Get information on the items queued.
	std::pair<unsigned, unsigned> items() const { ReadGuard l(m_lock); return std::make_pair(m_readySet.size(), m_unknownSet.size()); }

	/// Clear everything.
	void clear();

	/// Stop all activity, leaves the class in limbo, waiting for destruction
	void stop();

	/// Return first block with an unknown parent.
	h256 firstUnknown() const { ReadGuard l(m_lock); return m_unknownSet.size() ? *m_unknownSet.begin() : h256(); }

	/// Get some infomration on the current status.
	BlockQueueStatus status() const { ReadGuard l(m_lock); Guard l2(m_verification); return BlockQueueStatus{m_drainingSet.size(), m_verified.count(), m_verifying.count(), m_unverified.count(), m_future.size(), m_unknown.size(), m_knownBad.size()}; }

	/// Get some infomration on the given block's status regarding us.
	QueueStatus blockStatus(h256 const& _h) const;

	template <class T> Handler<> onReady(T const& _t) { return m_onReady.add(_t); }
	template <class T> Handler<> onRoomAvailable(T const& _t) { return m_onRoomAvailable.add(_t); }

	template <class T> void setOnBad(T const& _t) { m_onBad = _t; }

	bool knownFull() const;
	bool unknownFull() const;
	u256 difficulty() const;	// Total difficulty of queueud blocks
	bool isActive() const;

private:
	struct UnverifiedBlock
	{
		h256 hash;
		h256 parentHash;
		bytes blockData;
	};

	void noteReady_WITH_LOCK(h256 const& _b);

	bool invariants() const override;

	void verifierBody();
	void collectUnknownBad_WITH_BOTH_LOCKS(h256 const& _bad);
	void updateBad_WITH_LOCK(h256 const& _bad);
	void drainVerified_WITH_BOTH_LOCKS();

	std::size_t knownSize() const;
	std::size_t knownCount() const;

	BlockChain const* m_bc;												///< The blockchain into which our imports go.

	mutable boost::shared_mutex m_lock;									///< General lock for the sets, m_future and m_unknown.
	h256Hash m_drainingSet;												///< All blocks being imported.
	h256Hash m_readySet;												///< All blocks ready for chain import.
	h256Hash m_unknownSet;												///< Set of all blocks whose parents are not ready/in-chain.
	std::unordered_multimap<h256, std::pair<h256, bytes>> m_unknown;	///< For blocks that have an unknown parent; we map their parent hash to the block stuff, and insert once the block appears.
	h256Hash m_knownBad;												///< Set of blocks that we know will never be valid.
	std::multimap<unsigned, std::pair<h256, bytes>> m_future;			///< Set of blocks that are not yet valid. Ordered by timestamp
	Signal<> m_onReady;													///< Called when a subsequent call to import blocks will return a non-empty container. Be nice and exit fast.
	Signal<> m_onRoomAvailable;											///< Called when space for new blocks becomes availabe after a drain. Be nice and exit fast.

	mutable Mutex m_verification;										///< Mutex that allows writing to m_verified, m_verifying and m_unverified.
	std::condition_variable m_moreToVerify;								///< Signaled when m_unverified has a new entry.
	SizedBlockQueue<VerifiedBlock> m_verified;								///< List of blocks, in correct order, verified and ready for chain-import.
	SizedBlockQueue<VerifiedBlock> m_verifying;								///< List of blocks being verified; as long as the block component (bytes) is empty, it's not finished.
	SizedBlockQueue<UnverifiedBlock> m_unverified;							///< List of <block hash, parent hash, block data> in correct order, ready for verification.

	std::vector<std::thread> m_verifiers;								///< Threads who only verify.
	std::atomic<bool> m_deleting = {false};								///< Exit condition for verifiers.

	std::function<void(Exception&)> m_onBad;							///< Called if we have a block that doesn't verify.
	std::atomic<size_t> m_unknownSize;									///< Tracks total size in bytes of all unknown blocks
	std::atomic<size_t> m_unknownCount;									///< Tracks total count of unknown blocks. Used to avoid additional syncing
	u256 m_difficulty;													///< Total difficulty of blocks in the queue
	u256 m_drainingDifficulty;											///< Total difficulty of blocks in draining
};

std::ostream& operator<<(std::ostream& _out, BlockQueueStatus const& _s);

}
}
