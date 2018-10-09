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

#include "CommonNet.h"
#include <libdevcore/Guards.h>
#include <libdevcore/OverlayDB.h>
#include <libdevcore/Worker.h>
#include <libethcore/BlockHeader.h>
#include <libethcore/Common.h>
#include <libethereum/BlockChainSync.h>
#include <libp2p/Common.h>
#include <libp2p/HostCapability.h>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace dev
{

class RLPStream;

namespace eth
{

class TransactionQueue;
class BlockQueue;
class BlockChainSync;

struct EthereumPeerStatus
{
    /// What, if anything, we last asked the other peer for.
    Asking m_asking = Asking::Nothing;
    // TODO why atomic
    /// When we asked for it. Allows a time out.
    std::atomic<time_t> m_lastAsk;
    /// Peer's protocol version.
    unsigned m_protocolVersion;
    /// Peer's network id.
    u256 m_networkId;
    /// These are determined through either a Status message or from NewBlock.
    h256 m_latestHash;  ///< Peer's latest block's hash that we know about or default null value if
                        ///< no need to sync.
    u256 m_totalDifficulty;        ///< Peer's latest block's total difficulty.
    h256 m_genesisHash;            ///< Peer's genesis hash
    u256 m_peerCapabilityVersion;  ///< Protocol version this peer supports received as capability
                                   /// Have we received a GetTransactions packet that we haven't yet
                                   /// answered?
    bool m_requireTransactions = false;

    mutable Mutex x_knownBlocks;
    h256Hash m_knownBlocks;  ///< Blocks that the peer already knows about (that don't need to be
                             ///< sent to them).
    mutable Mutex x_knownTransactions;
    h256Hash m_knownTransactions;     ///< Transactions that the peer already knows of.
    unsigned m_unknownNewBlocks = 0;  ///< Number of unknown NewBlocks received from this peer
    unsigned m_lastAskedHeaders = 0;  ///< Number of hashes asked
};

class EthereumPeerObserverFace
{
public:
    virtual ~EthereumPeerObserverFace() = default;

    virtual void onPeerStatus(NodeID const& _peerID, EthereumPeerStatus const& _status) = 0;

    virtual void onPeerTransactions(NodeID const& _peerID, RLP const& _r) = 0;

    virtual void onPeerBlockHeaders(NodeID const& _peerID, RLP const& _headers) = 0;

    virtual void onPeerBlockBodies(NodeID const& _peerID, RLP const& _r) = 0;

    virtual void onPeerNewHashes(
        NodeID const& _peerID, std::vector<std::pair<h256, u256>> const& _hashes) = 0;

    virtual void onPeerNewBlock(NodeID const& _peerID, RLP const& _r) = 0;

    virtual void onPeerNodeData(NodeID const& _peerID, RLP const& _r) = 0;

    virtual void onPeerReceipts(NodeID const& _peerID, RLP const& _r) = 0;

    virtual void onPeerAborting() = 0;
};

class EthereumHostDataFace
{
public:
    virtual ~EthereumHostDataFace() = default;

    virtual std::pair<bytes, unsigned> blockHeaders(
        RLP const& _blockId, unsigned _maxHeaders, u256 _skip, bool _reverse) const = 0;

    virtual std::pair<bytes, unsigned> blockBodies(RLP const& _blockHashes) const = 0;

    virtual strings nodeData(RLP const& _dataHashes) const = 0;

    virtual std::pair<bytes, unsigned> receipts(RLP const& _blockHashes) const = 0;
};


/**
 * @brief The EthereumHost class
 * @warning None of this is thread-safe. You have been warned.
 * @doWork Syncs to peers and sends new blocks and transactions.
 */
class EthereumHost: public p2p::HostCapability, Worker
{
public:
    /// Start server, but don't listen.
    EthereumHost(std::shared_ptr<p2p::CapabilityHostFace> _host, BlockChain const& _ch,
        OverlayDB const& _db, TransactionQueue& _tq, BlockQueue& _bq, u256 _networkId);

    /// Will block on network process events.
    virtual ~EthereumHost();

    unsigned protocolVersion() const { return c_protocolVersion; }
    u256 networkId() const { return m_networkId; }
    void setNetworkId(u256 _n) { m_networkId = _n; }

    void reset();
    /// Don't sync further - used only in test mode
    void completeSync();

    bool isSyncing() const;

    void noteNewTransactions() { m_newTransactions = true; }
    void noteNewBlocks() { m_newBlocks = true; }
    void onBlockImported(BlockHeader const& _info) { m_sync->onBlockImported(_info); }

    BlockChain const& chain() const { return m_chain; }
    OverlayDB const& db() const { return m_db; }
    BlockQueue& bq() { return m_bq; }
    BlockQueue const& bq() const { return m_bq; }
    SyncStatus status() const;
    h256 latestBlockSent() { return m_latestBlockSent; }
    static char const* stateName(SyncState _s) { return s_stateNames[static_cast<int>(_s)]; }

    static unsigned const c_oldProtocolVersion;
    //    void foreachPeer(std::function<bool(std::shared_ptr<EthereumPeer>)> const& _f) const;

    void onConnect(NodeID const& _nodeID, u256 const& _peerCapabilityVersion) override;
    void onDisconnect(NodeID const& _nodeID) override;
    bool interpretCapabilityPacket(NodeID const& _peerID, unsigned _id, RLP const& _r) override;

    p2p::CapabilityHostFace& capabilityHost() { return *m_host; }

    EthereumPeerStatus const& peerStatus(NodeID const& _peerID) const
    {
        // TODO can not exist
        return m_peers.find(_peerID)->second;
    }

    bool isPeerConversing(NodeID const& _peerID) const
    {
        // TODO can not exist
        return m_peers.find(_peerID)->second.m_asking != Asking::Nothing;
    }

    void markPeerAsWaitingForTransactions(NodeID const& _peerID)
    {
        m_peers[_peerID].m_requireTransactions = true;
    }

    void markBlockAsKnownToPeer(NodeID const& _peerID, h256 const& _hash)
    {
        auto& peer = m_peers[_peerID];
        DEV_GUARDED(peer.x_knownBlocks)
        peer.m_knownBlocks.insert(_hash);
    }

    void setPeerLatestHash(NodeID const& _peerID, h256 const& _hash)
    {
        m_peers[_peerID].m_latestHash = _hash;
    }

    void incrementPeerUnknownNewBlocks(NodeID const& _peerID)
    {
        ++m_peers[_peerID].m_unknownNewBlocks;
    }

    void requestStatus(NodeID const& _peerID, u256 _hostNetworkId, u256 _chainTotalDifficulty,
        h256 _chainCurrentHash, h256 _chainGenesPeersh);

    /// Request hashes for given parent hash.
    void requestBlockHeaders(NodeID const& _nodeID, h256 const& _startHash, unsigned _count,
        unsigned _skip, bool _reverse);
    void requestBlockHeaders(NodeID const& _nodeID, unsigned _startNumber, unsigned _count,
        unsigned _skip, bool _reverse);

    /// Request specified blocks from peer.
    void requestBlockBodies(NodeID const& _nodeID, h256s const& _blocks);

    /// Request values for specified keys from peer.
    void requestNodeData(NodeID const& _nodeID, h256s const& _hashes);

    /// Request receipts for specified blocks from peer.
    void requestReceipts(NodeID const& _nodeID, h256s const& _blocks);

    /// Check if this node is rude.
    bool isRude(NodeID const& _nodeID) const;

    /// Set that it's a rude node.
    void setRude(NodeID const& _nodeID);

    /// Abort the sync operation.
    void abortSync(NodeID const& _nodeID);

protected:
    /*    std::shared_ptr<p2p::PeerCapabilityFace> newPeerCapability(
            std::shared_ptr<p2p::SessionFace> const& _s, unsigned _idOffset,
            p2p::CapDesc const& _cap) override;
    */
private:
    static char const* const s_stateNames[static_cast<int>(SyncState::Size)];

    std::tuple<std::vector<NodeID>, std::vector<NodeID>> randomSelection(
        unsigned _percent = 25, std::function<bool(EthereumPeerStatus const&)> const& _allow =
                                    [](EthereumPeerStatus const&) { return true; });

    /// Sync with the BlockChain. It might contain one of our mined blocks, we might have new candidates from the network.
    virtual void doWork() override;

    void maintainTransactions();
    void maintainBlocks(h256 const& _currentBlock);
    void onTransactionImported(ImportResult _ir, h256 const& _h, h512 const& _nodeId);

    ///	Check to see if the network peer-state initialisation has happened.
    bool isInitialised() const { return (bool)m_latestBlockSent; }

    /// Initialises the network peer-state, doing the stuff that needs to be once-only. @returns true if it really was first.
    bool ensureInitialised();

    virtual void onStarting() override { startWorking(); }
    virtual void onStopping() override { stopWorking(); }

    void setIdle(NodeID const& _peerID);
    void setAsking(NodeID const& _peerID, Asking _a);

    /// Are we presently in a critical part of the syncing process with this peer?
    bool isCriticalSyncing(NodeID const& _peerID) const;

    /// Do we presently need syncing with this peer?
    bool needsSyncing(NodeID const& _peerID) const
    {
        if (m_host->isRude(_peerID, name()))
            return false;

        auto peerStatus = m_peers.find(_peerID);
        return (peerStatus != m_peers.end() && peerStatus->second.m_latestHash);
    }

    // Request of type _packetType with _hashes as input parameters
    void requestByHashes(NodeID const& _peerID, h256s const& _hashes, Asking _asking,
        SubprotocolPacketType _packetType);

    std::shared_ptr<p2p::CapabilityHostFace> m_host;

    BlockChain const& m_chain;
    OverlayDB const& m_db;					///< References to DB, needed for some of the Ethereum Protocol responses.
    TransactionQueue& m_tq;					///< Maintains a list of incoming transactions not yet in a block on the blockchain.
    BlockQueue& m_bq;						///< Maintains a list of incoming blocks not yet on the blockchain (to be imported).

    u256 m_networkId;

    h256 m_latestBlockSent;
    h256Hash m_transactionsSent;

    bool m_newTransactions = false;
    bool m_newBlocks = false;

    mutable Mutex x_transactions;
    std::shared_ptr<BlockChainSync> m_sync;
    std::atomic<time_t> m_lastTick = { 0 };

    // TODO these can be unique
    std::shared_ptr<EthereumHostDataFace> m_hostData;
    std::shared_ptr<EthereumPeerObserverFace> m_peerObserver;

    std::unordered_map<NodeID, EthereumPeerStatus> m_peers;

    std::mt19937_64 m_urng; // Mersenne Twister psuedo-random number generator


    Logger m_logger{createLogger(VerbosityDebug, "ethhost")};
    /// Logger for messages about impolite behaivour of peers.
    Logger m_loggerImpolite{createLogger(VerbosityDebug, "impolite")};
};

}
}
