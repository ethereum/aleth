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
/** @file EthereumHost.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <mutex>
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include <memory>
#include <utility>
#include <thread>

#include <libdevcore/Guards.h>
#include <libdevcore/Worker.h>
#include <libdevcore/RangeMask.h>
#include <libethcore/Common.h>
#include <libp2p/Common.h>
#include "CommonNet.h"
#include "EthereumPeer.h"
#include "DownloadMan.h"

namespace dev
{

class RLPStream;

namespace eth
{

class TransactionQueue;
class BlockQueue;

/**
 * @brief The EthereumHost class
 * @warning None of this is thread-safe. You have been warned.
 * @doWork Syncs to peers and sends new blocks and transactions.
 */
class EthereumHost: public p2p::HostCapability<EthereumPeer>, Worker
{
public:
	/// Start server, but don't listen.
	EthereumHost(BlockChain const& _ch, TransactionQueue& _tq, BlockQueue& _bq, u256 _networkId);

	/// Will block on network process events.
	virtual ~EthereumHost();

	unsigned protocolVersion() const { return c_protocolVersion; }
	u256 networkId() const { return m_networkId; }
	void setNetworkId(u256 _n) { m_networkId = _n; }

	void reset();

	DownloadMan const& downloadMan() const { return m_man; }
	bool isSyncing() const { Guard l(x_sync); return isSyncing_UNSAFE(); }
	bool isBanned(p2p::NodeId _id) const { return !!m_banned.count(_id); }

	void noteNewTransactions() { m_newTransactions = true; }
	void noteNewBlocks() { m_newBlocks = true; }

	void onPeerStatus(EthereumPeer* _peer); ///< Called by peer to report status
	void onPeerBlocks(EthereumPeer* _peer, RLP const& _r); ///< Called by peer once it has new blocks during syn
	void onPeerNewBlock(EthereumPeer* _peer, RLP const& _r); ///< Called by peer once it has new blocks
	void onPeerNewHashes(EthereumPeer* _peer, h256s const& _hashes); ///< Called by peer once it has new hashes
	void onPeerHashes(EthereumPeer* _peer, h256s const& _hashes); ///< Called by peer once it has another sequential block of hashes during sync
	void onPeerTransactions(EthereumPeer* _peer, RLP const& _r); ///< Called by peer when it has new transactions

	DownloadMan& downloadMan() { return m_man; }
	HashDownloadMan& hashDownloadMan() { return m_hashMan; }
	BlockChain const& chain() { return m_chain; }

	static unsigned const c_oldProtocolVersion;

private:
	std::tuple<std::vector<std::shared_ptr<EthereumPeer>>, std::vector<std::shared_ptr<EthereumPeer>>, std::vector<std::shared_ptr<p2p::Session>>> randomSelection(unsigned _percent = 25, std::function<bool(EthereumPeer*)> const& _allow = [](EthereumPeer const*){ return true; });

	void foreachPeerPtr(std::function<void(std::shared_ptr<EthereumPeer>)> const& _f) const;
	void foreachPeer(std::function<void(EthereumPeer*)> const& _f) const;
	bool isSyncing_UNSAFE() const;

	/// Sync with the BlockChain. It might contain one of our mined blocks, we might have new candidates from the network.
	void doWork();

	void maintainTransactions();
	void maintainBlocks(h256 const& _currentBlock);

	/// Get a bunch of needed blocks.
	/// Removes them from our list of needed blocks.
	/// @returns empty if there's no more blocks left to fetch, otherwise the blocks to fetch.
	h256Hash neededBlocks(h256Hash const& _exclude);

	///	Check to see if the network peer-state initialisation has happened.
	bool isInitialised() const { return (bool)m_latestBlockSent; }

	/// Initialises the network peer-state, doing the stuff that needs to be once-only. @returns true if it really was first.
	bool ensureInitialised();

	virtual void onStarting() { startWorking(); }
	virtual void onStopping() { stopWorking(); }

	void continueSync(); /// Find something to do for all peers
	void continueSync(EthereumPeer* _peer); /// Find some work to do for a peer
	void onPeerDoneHashes(EthereumPeer* _peer, bool _new); /// Called when done downloading hashes from peer
	void onPeerHashes(EthereumPeer* _peer, h256s const& _hashes, bool _complete);
	bool peerShouldGrabBlocks(EthereumPeer* _peer) const;
	bool peerShouldGrabChain(EthereumPeer* _peer) const;
	void estimatePeerHashes(EthereumPeer* _peer);

	BlockChain const& m_chain;
	TransactionQueue& m_tq;					///< Maintains a list of incoming transactions not yet in a block on the blockchain.
	BlockQueue& m_bq;						///< Maintains a list of incoming blocks not yet on the blockchain (to be imported).

	u256 m_networkId;

	DownloadMan m_man;
	HashDownloadMan m_hashMan;

	h256 m_latestBlockSent;
	h256Hash m_transactionsSent;

	std::unordered_set<p2p::NodeId> m_banned;

	bool m_newTransactions = false;
	bool m_newBlocks = false;

	mutable Mutex x_sync;
	bool m_needSyncHashes = true;				///< Indicates if need to downlad hashes
	bool m_needSyncBlocks = true;				///< Indicates if we still need to download some blocks
	h256 m_syncingLatestHash;					///< Latest block's hash, as of the current sync.
	u256 m_syncingTotalDifficulty;				///< Latest block's total difficulty, as of the current sync.
	h256s m_hashes;								///< List of hashes with unknown block numbers. Used for v60 chain downloading and catching up to a particular unknown
};

}
}
