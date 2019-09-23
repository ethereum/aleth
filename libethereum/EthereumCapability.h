// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
#pragma once

#include "CommonNet.h"
#include "EthereumPeer.h"
#include <libdevcore/Guards.h>
#include <libdevcore/OverlayDB.h>
#include <libethcore/BlockHeader.h>
#include <libethcore/Common.h>
#include <libethereum/BlockChainSync.h>
#include <libethereum/VerifiedBlock.h>
#include <libp2p/Capability.h>
#include <libp2p/CapabilityHost.h>
#include <libp2p/Common.h>
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

class EthereumPeerObserverFace
{
public:
    virtual ~EthereumPeerObserverFace() = default;

    virtual void onPeerStatus(EthereumPeer const& _peer) = 0;

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
 * @brief The EthereumCapability class
 * @warning None of this is thread-safe. You have been warned.
 * @doWork Syncs to peers and sends new blocks and transactions.
 */
class EthereumCapability : public p2p::CapabilityFace
{
public:
    /// Start server, but don't listen.
    EthereumCapability(std::shared_ptr<p2p::CapabilityHostFace> _host, BlockChain const& _ch,
        OverlayDB const& _db, TransactionQueue& _tq, BlockQueue& _bq, u256 _networkId);

    std::string name() const override { return "eth"; }
    unsigned version() const override { return c_protocolVersion; }
    p2p::CapDesc descriptor() const override { return {name(), version()}; }
    unsigned messageCount() const override { return PacketCount; }
    char const* packetTypeToString(unsigned _packetType) const override
    {
        return ethPacketTypeToString(static_cast<EthSubprotocolPacketType>(_packetType));
    }
    std::chrono::milliseconds backgroundWorkInterval() const override;

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

    static char const* stateName(SyncState _s) { return c_stateNames[static_cast<int>(_s)]; }

    static unsigned const c_oldProtocolVersion;

    void onConnect(NodeID const& _nodeID, u256 const& _peerCapabilityVersion) override;
    void onDisconnect(NodeID const& _nodeID) override;
    bool interpretCapabilityPacket(NodeID const& _peerID, unsigned _id, RLP const& _r) override;

    /// Main work loop - sends new transactions and blocks to available peers and disconnects from
    /// timed out peers
    void doBackgroundWork() override;

    p2p::CapabilityHostFace& capabilityHost() { return *m_host; }

    EthereumPeer const& peer(NodeID const& _peerID) const;
    EthereumPeer& peer(NodeID const& _peerID);
    void disablePeer(NodeID const& _peerID, std::string const& _problem);

    /// Remove the supplied transaction hashes from the sent transactions list. Done when
    /// the transactions have been confirmed to be a part of the blockchain so we no longer
    /// need to explicitly track them to prevent sending them out to peers. Can be called safely
    /// from any thread.
    void removeSentTransactions(std::vector<h256> const& _txHashes);

    /// Send new blocks to peers. Should be done after we've verified the PoW but before we've
    /// imported the blocks into the chain (in order to reduce the uncle rate). Thread-safe (actual
    /// sending of blocks is done on the network thread).
    void propagateNewBlocks(std::shared_ptr<VerifiedBlocks const> const& _newBlocks);

private:
    static char const* const c_stateNames[static_cast<int>(SyncState::Size)];
    static constexpr std::chrono::milliseconds c_backgroundWorkInterval{1000};

    std::vector<NodeID> selectPeers(
        std::function<bool(EthereumPeer const&)> const& _predicate) const;

    std::vector<NodeID> randomPeers(std::vector<NodeID> const& _peers, size_t _count) const;

    /// Send top transactions (by nonce and gas price) to available peers
    void maintainTransactions();
    void maintainBlockHashes(h256 const& _currentBlock);
    void onTransactionImported(ImportResult _ir, h256 const& _h, h512 const& _nodeId);

    /// Initialises the network peer-state, doing the stuff that needs to be once-only. @returns true if it really was first.
    bool ensureInitialised();

    void setIdle(NodeID const& _peerID);
    void setAsking(NodeID const& _peerID, Asking _a);

    /// Are we presently in a critical part of the syncing process with this peer?
    bool isCriticalSyncing(NodeID const& _peerID) const;

    /// Do we presently need syncing with this peer?
    bool needsSyncing(NodeID const& _peerID) const;

    std::shared_ptr<p2p::CapabilityHostFace> m_host;

    BlockChain const& m_chain;
    OverlayDB const& m_db;					///< References to DB, needed for some of the Ethereum Protocol responses.
    TransactionQueue& m_tq;					///< Maintains a list of incoming transactions not yet in a block on the blockchain.
    BlockQueue& m_bq;						///< Maintains a list of incoming blocks not yet on the blockchain (to be imported).

    u256 m_networkId;

    // We need to keep track of sent blocks and block hashes separately since we propagate new
    // blocks after we've verified their PoW (and a few other things i.e. they've been imported into
    // the block queue and verified) but we propagate new block hashes after blocks have been
    // imported into the chain
    h256 m_latestBlockHashSent;
    h256 m_latestBlockSent;
    h256Hash m_transactionsSent;

    std::atomic<bool> m_newTransactions = {false};
    std::atomic<bool> m_newBlocks = {false};

    std::shared_ptr<BlockChainSync> m_sync;
    std::atomic<time_t> m_lastTick = { 0 };

    std::unique_ptr<EthereumHostDataFace> m_hostData;
    std::unique_ptr<EthereumPeerObserverFace> m_peerObserver;

    std::unordered_map<NodeID, EthereumPeer> m_peers;

    mutable std::mt19937_64 m_urng;  // Mersenne Twister psuedo-random number generator

    Logger m_logger{createLogger(VerbosityDebug, "ethcap")};
    Logger m_loggerDetail{createLogger(VerbosityTrace, "ethcap")};
    Logger m_loggerWarn{createLogger(VerbosityWarning, "ethcap")};
    /// Logger for messages about impolite behaivour of peers.
    Logger m_loggerImpolite{createLogger(VerbosityDebug, "impolite")};
};

}
}
