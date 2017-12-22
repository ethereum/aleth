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

void WarpPeerCapability::init(unsigned _hostProtocolVersion, u256 _hostNetworkId,
    u256 _chainTotalDifficulty, h256 _chainCurrentHash, h256 _chainGenesisHash,
    std::shared_ptr<SnapshotStorageFace const> _snapshot)
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
    
    try
    {
        switch (_id)
        {
        case WarpStatusPacket:
        {
            if (_r.itemCount() < 7)
                BOOST_THROW_EXCEPTION(InvalidWarpStatusPacket());

            auto const protocolVersion = _r[0].toInt<unsigned>(RLP::VeryStrict);
            auto const networkId = _r[1].toInt<u256>(RLP::VeryStrict);
            auto const totalDifficulty = _r[2].toInt<u256>(RLP::VeryStrict);
            auto const latestHash = _r[3].toHash<h256>(RLP::VeryStrict);
            auto const genesisHash = _r[4].toHash<h256>(RLP::VeryStrict);
            auto const snapshotHash = _r[5].toHash<h256>(RLP::VeryStrict);
            auto const snapshotNumber = _r[6].toInt<u256>(RLP::VeryStrict);

            clog(p2p::NetMessageSummary)
                << "Status: "
                << "protocol version " << protocolVersion << "networkId " << networkId
                << "genesis hash " << genesisHash << "total difficulty " << totalDifficulty
                << "latest hash" << latestHash << "snapshot hash" << snapshotHash
                << "snapshot number" << snapshotNumber;
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
        }
    }
    catch (Exception const&)
    {
        clog(p2p::NetWarn) << "Warp Peer causing an Exception:"
                           << boost::current_exception_diagnostic_information() << _r;
    }
    catch (std::exception const& _e)
    {
        clog(p2p::NetWarn) << "Warp Peer causing an exception:" << _e.what() << _r;
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

}  // namespace eth
}  // namespace dev
