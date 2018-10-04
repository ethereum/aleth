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

#include "WarpPeerCapability.h"
#include "SnapshotStorage.h"

#include "libethcore/Exceptions.h"

#include <chrono>

namespace dev
{
namespace eth
{
WarpPeerCapability::WarpPeerCapability(std::weak_ptr<p2p::SessionFace> _s,
    std::string const& _name, unsigned _messageCount, unsigned _offset,
    p2p::CapDesc const& /* _cap */)
  : PeerCapability(std::move(_s), _name, _messageCount, _offset)
{}

void WarpPeerCapability::init(unsigned _hostProtocolVersion, u256 _hostNetworkId,
    u256 _chainTotalDifficulty, h256 _chainCurrentHash, h256 _chainGenesisHash,
    std::shared_ptr<SnapshotStorageFace const> _snapshot,
    std::weak_ptr<WarpPeerObserverFace> _observer)
{
    m_observer = std::move(_observer);

    u256 snapshotBlockNumber;
    h256 snapshotBlockHash;
    if (_snapshot)
    {
        m_snapshot = std::move(_snapshot);

        bytes const snapshotManifest(m_snapshot->readManifest());
        RLP manifest(snapshotManifest);
        if (manifest.itemCount() != 6)
            BOOST_THROW_EXCEPTION(InvalidSnapshotManifest());
        snapshotBlockNumber = manifest[4].toInt<u256>(RLP::VeryStrict);
        snapshotBlockHash = manifest[5].toHash<h256>(RLP::VeryStrict);
    }

    requestStatus(_hostProtocolVersion, _hostNetworkId, _chainTotalDifficulty, _chainCurrentHash,
        _chainGenesisHash, snapshotBlockHash, snapshotBlockNumber);
}

bool WarpPeerCapability::interpretCapabilityPacket(unsigned, RLP const&)
{
    return true;
}

void WarpPeerCapability::onDisconnect()
{
}

/// Validates whether peer is able to communicate with the host, disables peer if not
bool WarpPeerCapability::validateStatus(h256 const& _genesisHash,
    std::vector<unsigned> const& _protocolVersions, u256 const& _networkId)
{
    std::shared_ptr<p2p::SessionFace> s = session();
    if (!s)
        return false;  // Expired

    if (m_genesisHash != _genesisHash)
    {
        disable("Invalid genesis hash");
        return false;
    }
    if (find(_protocolVersions.begin(), _protocolVersions.end(), m_protocolVersion) ==
        _protocolVersions.end())
    {
        disable("Invalid protocol version.");
        return false;
    }
    if (m_networkId != _networkId)
    {
        disable("Invalid network identifier.");
        return false;
    }
    if (m_asking != Asking::State && m_asking != Asking::Nothing)
    {
        disable("Peer banned for unexpected status message.");
        return false;
    }

    return true;
}

void WarpPeerCapability::requestStatus(unsigned _hostProtocolVersion, u256 const& _hostNetworkId,
    u256 const& _chainTotalDifficulty, h256 const& _chainCurrentHash, h256 const& _chainGenesisHash,
    h256 const& _snapshotBlockHash, u256 const& _snapshotBlockNumber)
{
    RLPStream s;
    prep(s, WarpStatusPacket, 7) << _hostProtocolVersion << _hostNetworkId << _chainTotalDifficulty
                                 << _chainCurrentHash << _chainGenesisHash << _snapshotBlockHash
                                 << _snapshotBlockNumber;
    sealAndSend(s);
}


void WarpPeerCapability::requestBlockHeaders(
    unsigned _startNumber, unsigned _count, unsigned _skip, bool _reverse)
{
    assert(m_asking == Asking::Nothing);
    setAsking(Asking::BlockHeaders);
    RLPStream s;
    prep(s, GetBlockHeadersPacket, 4) << _startNumber << _count << _skip << (_reverse ? 1 : 0);
    sealAndSend(s);
}

void WarpPeerCapability::requestManifest()
{
    assert(m_asking == Asking::Nothing);
    setAsking(Asking::WarpManifest);
    RLPStream s;
    prep(s, GetSnapshotManifest);
    sealAndSend(s);
}

void WarpPeerCapability::requestData(h256 const& _chunkHash)
{
    assert(m_asking == Asking::Nothing);
    setAsking(Asking::WarpData);
    RLPStream s;
    prep(s, GetSnapshotData, 1) << _chunkHash;
    sealAndSend(s);
}

void WarpPeerCapability::setAsking(Asking _a)
{
    m_asking = _a;
    m_lastAsk = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

void WarpPeerCapability::tick()
{
    auto s = session();
    time_t const now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    if (s && (now - m_lastAsk > 10 && m_asking != Asking::Nothing))
    {
        // timeout
        s->disconnect(p2p::PingTimeout);
    }
}

}  // namespace eth
}  // namespace dev
