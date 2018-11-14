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

#include "EthereumPeer.h"
#include <libethcore/Common.h>
#include <libp2p/CapabilityHost.h>


using namespace std;
using namespace dev;
using namespace dev::eth;

static std::string const c_ethCapability = "eth";

namespace
{
string toString(Asking _a)
{
    switch (_a)
    {
    case Asking::BlockHeaders:
        return "BlockHeaders";
    case Asking::BlockBodies:
        return "BlockBodies";
    case Asking::NodeData:
        return "NodeData";
    case Asking::Receipts:
        return "Receipts";
    case Asking::Nothing:
        return "Nothing";
    case Asking::State:
        return "State";
    case Asking::WarpManifest:
        return "WarpManifest";
    case Asking::WarpData:
        return "WarpData";
    }
    return "?";
}
}  // namespace

void EthereumPeer::setStatus(unsigned _protocolVersion, u256 const& _networkId,
    u256 const& _totalDifficulty, h256 const& _latestHash, h256 const& _genesisHash)
{
    m_protocolVersion = _protocolVersion;
    m_networkId = _networkId;
    m_totalDifficulty = _totalDifficulty;
    m_latestHash = _latestHash;
    m_genesisHash = _genesisHash;
}


std::string EthereumPeer::validate(
    h256 const& _hostGenesisHash, unsigned _hostProtocolVersion, u256 const& _hostNetworkId) const
{
    std::string error;
    if (m_genesisHash != _hostGenesisHash)
        error = "Invalid genesis hash.";
    else if (m_protocolVersion != _hostProtocolVersion)
        error = "Invalid protocol version.";
    else if (m_networkId != _hostNetworkId)
        error = "Invalid network identifier.";
    else if (m_asking != Asking::State && m_asking != Asking::Nothing)
        error = "Peer banned for unexpected status message.";

    return error;
}

void EthereumPeer::requestStatus(
    u256 _hostNetworkId, u256 _chainTotalDifficulty, h256 _chainCurrentHash, h256 _chainGenesisHash)
{
    assert(m_asking == Asking::Nothing);
    setAsking(Asking::State);
    m_requireTransactions = true;
    RLPStream s;
    m_host->prep(m_id, c_ethCapability, s, StatusPacket, 5)
        << c_protocolVersion << _hostNetworkId << _chainTotalDifficulty << _chainCurrentHash
        << _chainGenesisHash;
    m_host->sealAndSend(m_id, s);
}

void EthereumPeer::requestBlockHeaders(
    unsigned _startNumber, unsigned _count, unsigned _skip, bool _reverse)
{
    if (m_asking != Asking::Nothing)
    {
        LOG(m_logger) << "Asking headers while requesting " << ::toString(m_asking);
    }
    setAsking(Asking::BlockHeaders);
    RLPStream s;
    m_host->prep(m_id, c_ethCapability, s, GetBlockHeadersPacket, 4)
        << _startNumber << _count << _skip << (_reverse ? 1 : 0);
    LOG(m_logger) << "Requesting " << _count << " block headers starting from " << _startNumber
                  << (_reverse ? " in reverse" : "");
    m_lastAskedHeaders = _count;
    m_host->sealAndSend(m_id, s);
}

void EthereumPeer::requestBlockHeaders(
    h256 const& _startHash, unsigned _count, unsigned _skip, bool _reverse)
{
    if (m_asking != Asking::Nothing)
    {
        LOG(m_logger) << "Asking headers while requesting " << ::toString(m_asking);
    }
    setAsking(Asking::BlockHeaders);
    RLPStream s;
    m_host->prep(m_id, c_ethCapability, s, GetBlockHeadersPacket, 4)
        << _startHash << _count << _skip << (_reverse ? 1 : 0);
    LOG(m_logger) << "Requesting " << _count << " block headers starting from " << _startHash
                  << (_reverse ? " in reverse" : "");
    m_lastAskedHeaders = _count;
    m_host->sealAndSend(m_id, s);
}


void EthereumPeer::requestBlockBodies(h256s const& _blocks)
{
    requestByHashes(_blocks, Asking::BlockBodies, GetBlockBodiesPacket);
}

void EthereumPeer::requestNodeData(h256s const& _hashes)
{
    requestByHashes(_hashes, Asking::NodeData, GetNodeDataPacket);
}

void EthereumPeer::requestReceipts(h256s const& _blocks)
{
    requestByHashes(_blocks, Asking::Receipts, GetReceiptsPacket);
}

void EthereumPeer::requestByHashes(
    h256s const& _hashes, Asking _asking, SubprotocolPacketType _packetType)
{
    if (m_asking != Asking::Nothing)
    {
        LOG(m_logger) << "Asking " << ::toString(_asking) << " while requesting "
                      << ::toString(m_asking);
    }
    setAsking(_asking);
    if (_hashes.size())
    {
        RLPStream s;
        m_host->prep(m_id, c_ethCapability, s, _packetType, _hashes.size());
        for (auto const& i : _hashes)
            s << i;
        m_host->sealAndSend(m_id, s);
    }
    else
        setAsking(Asking::Nothing);
}
