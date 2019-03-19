// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include "CommonNet.h"
#include <libp2p/Capability.h>
#include <libp2p/CapabilityHost.h>

namespace dev
{
namespace eth
{
class SnapshotStorageFace;

unsigned const c_WarpProtocolVersion = 1;

enum WarpSubprotocolPacketType : byte
{
    WarpStatusPacket = 0x00,
    GetSnapshotManifest = 0x11,
    SnapshotManifest = 0x12,
    GetSnapshotData = 0x13,
    SnapshotData = 0x14,

    WarpSubprotocolPacketCount
};

struct WarpPeerStatus
{
    /// Peer's protocol version.
    unsigned m_protocolVersion = 0;

    /// Peer's network id.
    u256 m_networkId;

    /// What, if anything, we last asked the other peer for.
    Asking m_asking = Asking::Nothing;
    /// When we asked for it. Allows a time out.
    time_t m_lastAsk;

    /// These are determined through either a Status message.
    h256 m_latestHash;       ///< Peer's latest block's hash.
    u256 m_totalDifficulty;  ///< Peer's latest block's total difficulty.
    h256 m_genesisHash;      ///< Peer's genesis hash
    h256 m_snapshotHash;
    u256 m_snapshotNumber;
};


class WarpPeerObserverFace
{
public:
    virtual ~WarpPeerObserverFace() {}

    virtual void onPeerStatus(NodeID const& _peerID) = 0;

    virtual void onPeerManifest(NodeID const& _peerID, RLP const& _r) = 0;

    virtual void onPeerBlockHeaders(NodeID const& _peerID, RLP const& _r) = 0;

    virtual void onPeerData(NodeID const& _peerID, RLP const& _r) = 0;

    virtual void onPeerDisconnect(NodeID const& _peerID, Asking _asking) = 0;
};


class WarpCapability : public p2p::CapabilityFace
{
public:
    WarpCapability(std::shared_ptr<p2p::CapabilityHostFace> _host, BlockChain const& _blockChain,
        u256 const& _networkId, boost::filesystem::path const& _snapshotDownloadPath,
        std::shared_ptr<SnapshotStorageFace> _snapshotStorage);

    std::string name() const override { return "par"; }
    unsigned version() const override { return c_WarpProtocolVersion; }
    p2p::CapDesc descriptor() const override { return {name(), version()}; }
    unsigned messageCount() const override { return WarpSubprotocolPacketCount; }
    std::chrono::milliseconds backgroundWorkInterval() const override;

    u256 networkId() const { return m_networkId; }

    void onConnect(NodeID const& _peerID, u256 const& _peerCapabilityVersion) override;
    bool interpretCapabilityPacket(NodeID const& _peerID, unsigned _id, RLP const&) override;
    void onDisconnect(NodeID const& _peerID) override;

    void doBackgroundWork() override;

    p2p::CapabilityHostFace& capabilityHost() { return *m_host; }

    void requestStatus(NodeID const& _peerID, unsigned _hostProtocolVersion,
        u256 const& _hostNetworkId, u256 const& _chainTotalDifficulty,
        h256 const& _chainCurrentHash, h256 const& _chainGenesisHash,
        h256 const& _snapshotBlockHash, u256 const& _snapshotBlockNumber);
    void requestBlockHeaders(NodeID const& _peerID, unsigned _startNumber, unsigned _count,
        unsigned _skip, bool _reverse);
    void requestManifest(NodeID const& _peerID);
    bool requestData(NodeID const& _peerID, h256 const& _chunkHash);

    /// Validates whether peer is able to communicate with the host, disables peer if not
    bool validateStatus(NodeID const& _peerID, h256 const& _genesisHash,
        std::vector<unsigned> const& _protocolVersions, u256 const& _networkId);

    void disablePeer(NodeID const& _peerID, std::string const& _problem);

private:
    static constexpr std::chrono::milliseconds c_backgroundWorkInterval{1000};

    std::shared_ptr<WarpPeerObserverFace> createPeerObserver(
        boost::filesystem::path const& _snapshotDownloadPath);

    void setAsking(NodeID const& _peerID, Asking _a);

    void setIdle(NodeID const& _peerID) { setAsking(_peerID, Asking::Nothing); }

    std::shared_ptr<p2p::CapabilityHostFace> m_host;

    BlockChain const& m_blockChain;
    u256 const m_networkId;

    std::shared_ptr<SnapshotStorageFace> m_snapshot;
    std::shared_ptr<WarpPeerObserverFace> m_peerObserver;

    std::unordered_map<NodeID, WarpPeerStatus> m_peers;
};

}  // namespace eth
}  // namespace dev