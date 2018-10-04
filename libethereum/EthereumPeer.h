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
/** @file EthereumPeer.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <mutex>
#include <array>
#include <memory>
#include <utility>

#include <libdevcore/RLP.h>
#include <libdevcore/Guards.h>
#include <libethcore/Common.h>
#include <libp2p/PeerCapability.h>
#include "CommonNet.h"

namespace dev
{
namespace eth
{
class EthereumHostDataFace;
class EthereumPeerObserverFace;

/**
 * @brief The EthereumPeer class
 * @todo Document fully.
 * @todo make state transitions thread-safe.
 */
class EthereumPeer : public p2p::PeerCapability
{
    friend class EthereumHost; //TODO: remove this
    friend class BlockChainSync; //TODO: remove this

public:
    /// Basic constructor.
    EthereumPeer(std::weak_ptr<p2p::SessionFace> _s, std::string const& _name,
        unsigned _messageCount, unsigned _offset, p2p::CapDesc const& _cap);

    /// Basic destructor.
    ~EthereumPeer() override;

    /// What is our name?
    static std::string name() { return "eth"; }

    /// What is our version?
    static u256 version() { return c_protocolVersion; }

    /// How many message types do we have?
    static unsigned messageCount() { return PacketCount; }

    void init(unsigned _hostProtocolVersion, u256 _hostNetworkId, u256 _chainTotalDifficulty, h256 _chainCurrentHash, h256 _chainGenesisHash, std::shared_ptr<EthereumHostDataFace> _hostData, std::shared_ptr<EthereumPeerObserverFace> _observer);

    p2p::NodeID id() const { return session()->id(); }

    /// Abort sync and reset fetch
    void setIdle();

    /// Request hashes for given parent hash.
    void requestBlockHeaders(h256 const& _startHash, unsigned _count, unsigned _skip, bool _reverse);
    void requestBlockHeaders(unsigned _startNumber, unsigned _count, unsigned _skip, bool _reverse);

    /// Request specified blocks from peer.
    void requestBlockBodies(h256s const& _blocks);

    /// Request values for specified keys from peer.
    void requestNodeData(h256s const& _hashes);

    /// Request receipts for specified blocks from peer.
    void requestReceipts(h256s const& _blocks);

    /// Check if this node is rude.
    bool isRude() const;

    /// Set that it's a rude node.
    void setRude();

    /// Abort the sync operation.
    void abortSync();

private:
    using p2p::PeerCapability::sealAndSend;

    /// Figure out the amount of blocks we should be asking for.
    unsigned askOverride() const;

    /// Interpret an incoming message.
    bool interpretCapabilityPacket(unsigned _id, RLP const& _r) override;

    /// Request status. Called from constructor
    void requestStatus(u256 _hostNetworkId, u256 _chainTotalDifficulty, h256 _chainCurrentHash, h256 _chainGenesisHash);

    /// Clear all known transactions.
    void clearKnownTransactions() { std::lock_guard<std::mutex> l(x_knownTransactions); m_knownTransactions.clear(); }

    // Request of type _packetType with _hashes as input parameters
    void requestByHashes(h256s const& _hashes, Asking _asking, SubprotocolPacketType _packetType);

    /// Update our asking state.
    void setAsking(Asking _g);

    /// Do we presently need syncing with this peer?
    bool needsSyncing() const { return !isRude() && !!m_latestHash; }

    /// Are we presently in the process of communicating with this peer?
    bool isConversing() const;

    /// Are we presently in a critical part of the syncing process with this peer?
    bool isCriticalSyncing() const;

    /// Runs period checks to check up on the peer.
    void tick();

    unsigned m_hostProtocolVersion = 0;

    /// Peer's protocol version.
    unsigned m_protocolVersion;

    /// Peer's network id.
    u256 m_networkId;

    /// What, if anything, we last asked the other peer for.
    Asking m_asking = Asking::Nothing;
    /// When we asked for it. Allows a time out.
    std::atomic<time_t> m_lastAsk;

    /// These are determined through either a Status message or from NewBlock.
    h256 m_latestHash;						///< Peer's latest block's hash that we know about or default null value if no need to sync.
    u256 m_totalDifficulty;					///< Peer's latest block's total difficulty.
    h256 m_genesisHash;						///< Peer's genesis hash

    u256 const m_peerCapabilityVersion;			///< Protocol version this peer supports received as capability
    /// Have we received a GetTransactions packet that we haven't yet answered?
    bool m_requireTransactions = false;

    Mutex x_knownBlocks;
    h256Hash m_knownBlocks;					///< Blocks that the peer already knows about (that don't need to be sent to them).
    Mutex x_knownTransactions;
    h256Hash m_knownTransactions;			///< Transactions that the peer already knows of.
    unsigned m_unknownNewBlocks = 0;		///< Number of unknown NewBlocks received from this peer
    unsigned m_lastAskedHeaders = 0;		///< Number of hashes asked

    std::weak_ptr<EthereumPeerObserverFace> m_observer;
    std::weak_ptr<EthereumHostDataFace> m_hostData;

    Logger m_logger{createLogger(VerbosityDebug, "peer")};
    /// Logger for messages about impolite behaivour of peers.
    Logger m_loggerImpolite{createLogger(VerbosityDebug, "impolite")};
};

}
}
