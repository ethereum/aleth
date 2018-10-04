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

#pragma once

#include "CommonNet.h"

#include <libdevcore/Common.h>
#include <libp2p/PeerCapability.h>

namespace dev
{
namespace eth
{
class SnapshotStorageFace;
class WarpPeerObserverFace;

const unsigned c_WarpProtocolVersion = 1;

enum WarpSubprotocolPacketType : byte
{
    WarpStatusPacket = 0x00,
    GetSnapshotManifest = 0x11,
    SnapshotManifest = 0x12,
    GetSnapshotData = 0x13,
    SnapshotData = 0x14,

    WarpSubprotocolPacketCount
};

class WarpPeerCapability : public p2p::PeerCapability
{
public:
    WarpPeerCapability(std::weak_ptr<p2p::SessionFace> _s, std::string const& _name,
        unsigned _messageCount, unsigned _offset, p2p::CapDesc const& _cap);

    static std::string name() { return "par"; }

    static u256 version() { return c_WarpProtocolVersion; }

    static unsigned messageCount() { return WarpSubprotocolPacketCount; }

    void init(unsigned _hostProtocolVersion, u256 _hostNetworkId, u256 _chainTotalDifficulty,
        h256 _chainCurrentHash, h256 _chainGenesisHash,
        std::shared_ptr<SnapshotStorageFace const> _snapshot,
        std::weak_ptr<WarpPeerObserverFace> _observer);

    /// Validates whether peer is able to communicate with the host, disables peer if not
    bool validateStatus(h256 const& _genesisHash, std::vector<unsigned> const& _protocolVersions,
        u256 const& _networkId);

    void requestStatus(unsigned _hostProtocolVersion, u256 const& _hostNetworkId,
        u256 const& _chainTotalDifficulty, h256 const& _chainCurrentHash,
        h256 const& _chainGenesisHash, h256 const& _snapshotBlockHash,
        u256 const& _snapshotBlockNumber);
    void requestBlockHeaders(unsigned _startNumber, unsigned _count, unsigned _skip, bool _reverse);
    void requestManifest();
    void requestData(h256 const& _chunkHash);

    /// Host runs this periodically to check up on the peer.
    void tick();

    u256 snapshotNumber() const { return m_snapshotNumber; }

    using p2p::PeerCapability::disable;

private:
    using p2p::PeerCapability::sealAndSend;

    bool interpretCapabilityPacket(unsigned _id, RLP const& _r) override;

    void onDisconnect() override;

    void setAsking(Asking _a);

    void setIdle() { setAsking(Asking::Nothing); }

    /// Peer's protocol version.
    unsigned m_protocolVersion = 0;

    /// Peer's network id.
    u256 m_networkId;

    /// What, if anything, we last asked the other peer for.
    Asking m_asking = Asking::Nothing;
    /// When we asked for it. Allows a time out.
    std::atomic<time_t> m_lastAsk;

    /// These are determined through either a Status message.
    h256 m_latestHash;       ///< Peer's latest block's hash.
    u256 m_totalDifficulty;  ///< Peer's latest block's total difficulty.
    h256 m_genesisHash;      ///< Peer's genesis hash
    h256 m_snapshotHash;
    u256 m_snapshotNumber;

    std::shared_ptr<SnapshotStorageFace const> m_snapshot;
    std::weak_ptr<WarpPeerObserverFace> m_observer;
};

}  // namespace eth
}  // namespace dev
