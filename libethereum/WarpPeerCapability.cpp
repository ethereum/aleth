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

namespace dev
{
namespace eth
{

WarpPeerCapability::WarpPeerCapability(std::shared_ptr<p2p::SessionFace> _s,
    p2p::HostCapabilityFace* _h, unsigned _i, p2p::CapDesc const& /* _cap */)
  : Capability(_s, _h, _i)
{}

void WarpPeerCapability::init(unsigned _hostProtocolVersion, u256 _hostNetworkId, u256 _chainTotalDifficulty, h256 _chainCurrentHash, h256 _chainGenesisHash, std::shared_ptr<SnapshotStorageFace const> _snapshot,
    std::weak_ptr<WarpPeerObserverFace> _observer)
{
    assert(_snapshot);
    m_snapshot = std::move(_snapshot);

    bytes const snapshotManifest(m_snapshot->readManifest());
    RLP manifest(snapshotManifest);
    if (manifest.itemCount() != 6)
        BOOST_THROW_EXCEPTION(InvalidSnapshotManifest());
    u256 const snapshotBlockNumber = manifest[4].toInt<u256>(RLP::VeryStrict);
    h256 const snapshotBlockHash = manifest[5].toHash<h256>(RLP::VeryStrict);

    requestStatus(_hostProtocolVersion, _hostNetworkId, _chainTotalDifficulty, _chainCurrentHash,
        _chainGenesisHash, snapshotBlockHash, snapshotBlockNumber);
}

bool WarpPeerCapability::interpret(unsigned _id, RLP const& _r)
{
    assert(m_snapshot);
    std::shared_ptr<WarpPeerObserverFace> observer(m_observer.lock());
    if (!observer)
        return false;

    m_lastAsk = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    try
    {
        switch (_id)
        {
        case WarpStatusPacket:
        {
            if (_r.itemCount() < 7)
                BOOST_THROW_EXCEPTION(InvalidWarpStatusPacket());

            // Packet layout:
            // [ version:P, state_hashes : [hash_1:B_32, hash_2 : B_32, ...],  block_hashes : [hash_1:B_32, hash_2 : B_32, ...],
            //      state_root : B_32, block_number : P, block_hash : B_32 ]
            m_protocolVersion = _r[0].toInt<unsigned>();
            m_networkId = _r[1].toInt<u256>();
            m_totalDifficulty = _r[2].toInt<u256>();
            m_latestHash = _r[3].toHash<h256>();
            m_genesisHash = _r[4].toHash<h256>();
            m_snapshotHash = _r[5].toHash<h256>();
            m_snapshotNumber = _r[6].toInt<u256>();

            clog(p2p::NetMessageSummary) << "Status: "
                << "protocol version " << m_protocolVersion
                << "networkId " << m_networkId
                << "genesis hash " << m_genesisHash
                << "total difficulty " << m_totalDifficulty
                << "latest hash" << m_latestHash
                << "snapshot hash" << m_snapshotHash
                << "snapshot number" << m_snapshotNumber;
            setIdle();
            observer->onPeerStatus(std::dynamic_pointer_cast<WarpPeerCapability>(shared_from_this()));
            break;
        }
        case GetSnapshotManifest:
        {
            RLPStream s;
            prep(s, SnapshotManifest, 1).appendRaw(m_snapshot->readManifest());
            sealAndSend(s);
            break;
        }
        case GetSnapshotData:
        {
            const h256 chunkHash = _r[0].toHash<h256>(RLP::VeryStrict);

            RLPStream s;
            prep(s, SnapshotData, 1).append(m_snapshot->readCompressedChunk(chunkHash));
            sealAndSend(s);
            break;
        }
        case GetBlockHeadersPacket:
        {
            // TODO We are being asked DAO fork block sometimes, need to be able to answer this
            RLPStream s;
            prep(s, BlockHeadersPacket);
            sealAndSend(s);
            break;
        }
        case SnapshotManifest:
        {
            setIdle();
            observer->onPeerManifest((std::dynamic_pointer_cast<WarpPeerCapability>(shared_from_this())), _r);
            break;
        }
        case SnapshotData:
        {
            setIdle();
            observer->onPeerData((std::dynamic_pointer_cast<WarpPeerCapability>(shared_from_this())), _r);
            break;
        }
        default:
            return false;
        }
    }
    catch (Exception const&)
    {
        clog(p2p::NetWarn) << "Warp Peer causing an Exception:" << boost::current_exception_diagnostic_information() << _r;
    }
    catch (std::exception const& _e)
    {
        clog(p2p::NetWarn) << "Warp Peer causing an exception:" << _e.what() << _r;
    }

    return true;
}

/// Validates whether peer is able to communicate with the host, disables peer if not
bool WarpPeerCapability::validateStatus(h256 const& _genesisHash, std::vector<unsigned> const& _protocolVersions, u256 const& _networkId)
{
    std::shared_ptr<p2p::SessionFace> s = session();
    if (!s)
        return false; // Expired

    if (m_genesisHash != _genesisHash)
    {
        disable("Invalid genesis hash");
        return false;
    }
    if (find(_protocolVersions.begin(), _protocolVersions.end(), m_protocolVersion) == _protocolVersions.end())
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

}  // namespace eth
}  // namespace dev
