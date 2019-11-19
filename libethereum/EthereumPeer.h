// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
#pragma once

#include "CommonNet.h"

namespace dev
{
namespace p2p
{
class CapabilityHostFace;
}

namespace eth
{
class EthereumPeer
{
public:
    EthereumPeer() = delete;
    EthereumPeer(std::shared_ptr<p2p::CapabilityHostFace> _host, NodeID const& _peerID,
        u256 const& /*_capabilityVersion*/)
      : m_host(std::move(_host)), m_id(_peerID)
    {
        m_logger.add_attribute(
            "Suffix", boost::log::attributes::constant<std::string>{m_id.abridged()});
    }

    bool statusReceived() const { return m_protocolVersion != 0; }

    void setStatus(unsigned _protocolVersion, u256 const& _networkId, u256 const& _totalDifficulty,
        h256 const& _latestHash, h256 const& _genesisHash);

    std::string validate(h256 const& _hostGenesisHash, unsigned _hostProtocolVersion,
        u256 const& _hostNetworkId) const;

    NodeID id() const { return m_id; }

    u256 totalDifficulty() const { return m_totalDifficulty; }

    time_t lastAsk() const { return m_lastAsk; }
    void setLastAsk(time_t _lastAsk) { m_lastAsk = _lastAsk; }

    Asking asking() const { return m_asking; }
    bool isConversing() const { return m_asking != Asking::Nothing; }
    void setAsking(Asking _asking) { m_asking = _asking; }

    h256 latestHash() const { return m_latestHash; }
    void setLatestHash(h256 const& _hash) { m_latestHash = _hash; }

    bool isWaitingForTransactions() const { return m_requireTransactions; }
    void setWaitingForTransactions(bool _value) { m_requireTransactions = _value; }

    bool isTransactionKnown(h256 const& _hash) const { return m_knownTransactions.count(_hash); }
    void markTransactionAsKnown(h256 const& _hash) { m_knownTransactions.insert(_hash); }

    bool isBlockKnown(h256 const& _hash) const { return m_knownBlocks.count(_hash); }
    void markBlockAsKnown(h256 const& _hash) { m_knownBlocks.insert(_hash); }
    void clearKnownBlocks() { m_knownBlocks.clear(); }

    unsigned unknownNewBlocks() const { return m_unknownNewBlocks; }
    void incrementUnknownNewBlocks() { ++m_unknownNewBlocks; }

    void requestStatus(u256 _hostNetworkId, u256 _chainTotalDifficulty, h256 _chainCurrentHash,
        h256 _chainGenesPeersh);

    /// Request hashes for given parent hash.
    void requestBlockHeaders(
        h256 const& _startHash, unsigned _count, unsigned _skip, bool _reverse);
    void requestBlockHeaders(unsigned _startNumber, unsigned _count, unsigned _skip, bool _reverse);

    /// Request specified blocks from peer.
    void requestBlockBodies(h256s const& _blocks);

    /// Request values for specified keys from peer.
    void requestNodeData(h256s const& _hashes);

    /// Request receipts for specified blocks from peer.
    void requestReceipts(h256s const& _blocks);

private:
    // Request of type _packetType with _hashes as input parameters
    void requestByHashes(
        h256s const& _hashes, Asking _asking, EthSubprotocolPacketType _packetType);

    std::shared_ptr<p2p::CapabilityHostFace> m_host;

    NodeID const m_id;

    /// What, if anything, we last asked the other peer for.
    Asking m_asking = Asking::Nothing;
    /// When we asked for it. Allows a time out.
    time_t m_lastAsk = 0;
    /// Peer's protocol version.
    unsigned m_protocolVersion = 0;
    /// Peer's network id.
    u256 m_networkId;
    /// These are determined through either a Status message or from NewBlock.
    /// Peer's latest block's hash that we know about or default null value if no need to sync.
    h256 m_latestHash;
    /// Peer's latest block's total difficulty.
    u256 m_totalDifficulty;
    h256 m_genesisHash;  ///< Peer's genesis hash
    /// Have we received a GetTransactions packet that we haven't yet answered?
    bool m_requireTransactions = false;

    /// Blocks that the peer already knows about (that don't need to be sent to them).
    h256Hash m_knownBlocks;
    h256Hash m_knownTransactions;     ///< Transactions that the peer already knows of.
    unsigned m_unknownNewBlocks = 0;  ///< Number of unknown NewBlocks received from this peer
    unsigned m_lastAskedHeaders = 0;  ///< Number of hashes asked

    Logger m_logger{createLogger(VerbosityDebug, "peer")};
};
}  // namespace eth
}  // namespace dev
