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
/** @file EthereumPeer.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "EthereumPeer.h"

#include <chrono>
#include <libdevcore/Common.h>
#include <libethcore/Exceptions.h>
#include <libp2p/Session.h>
#include <libp2p/Host.h>
#include "EthereumHost.h"

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace p2p;

static string toString(Asking _a)
{
    switch (_a)
    {
    case Asking::BlockHeaders: return "BlockHeaders";
    case Asking::BlockBodies: return "BlockBodies";
    case Asking::NodeData: return "NodeData";
    case Asking::Receipts: return "Receipts";
    case Asking::Nothing: return "Nothing";
    case Asking::State: return "State";
    case Asking::WarpManifest:
        return "WarpManifest";
    case Asking::WarpData:
        return "WarpData";
    }
    return "?";
}

EthereumPeer::EthereumPeer(std::weak_ptr<SessionFace> _s, std::string const& _name,
    unsigned _messageCount, unsigned _offset, CapDesc const& _cap)
  : PeerCapability(move(_s), _name, _messageCount, _offset), m_peerCapabilityVersion(_cap.second)
{
    session()->addNote("manners", isRude() ? "RUDE" : "nice");
}

EthereumPeer::~EthereumPeer()
{
    if (m_asking != Asking::Nothing)
    {
        cnetdetails << "Peer aborting while being asked for " << ::toString(m_asking);
        setRude();
    }
    abortSync();
}

void EthereumPeer::init(unsigned _hostProtocolVersion, u256 _hostNetworkId, u256 _chainTotalDifficulty, h256 _chainCurrentHash, h256 _chainGenesisHash, shared_ptr<EthereumHostDataFace> _hostData, shared_ptr<EthereumPeerObserverFace> _observer)
{
    m_hostData = _hostData;
    m_observer = _observer;
    m_hostProtocolVersion = _hostProtocolVersion;
    requestStatus(_hostNetworkId, _chainTotalDifficulty, _chainCurrentHash, _chainGenesisHash);
}

bool EthereumPeer::isRude() const
{
    auto s = session();
    if (s)
        return s->repMan().isRude(*s, name());
    return false;
}

unsigned EthereumPeer::askOverride() const
{
    std::string static const badGeth = "Geth/v0.9.27";
    auto s = session();
    if (!s)
        return c_maxBlocksAsk;
    if (s->info().clientVersion.substr(0, badGeth.size()) == badGeth)
        return 1;
    bytes const& d = s->repMan().data(*s, name());
    return d.empty() ? c_maxBlocksAsk : RLP(d).toInt<unsigned>(RLP::LaissezFaire);
}

void EthereumPeer::setRude()
{
    auto s = session();
    if (!s)
        return;
    auto old = askOverride();
    s->repMan().setData(*s, name(), rlp(askOverride() / 2 + 1));
    cnote << "Rude behaviour; askOverride now" << askOverride() << ", was" << old;
    s->repMan().noteRude(*s, name());
    session()->addNote("manners", "RUDE");
}

void EthereumPeer::abortSync()
{
    if (auto observer = m_observer.lock())
        observer->onPeerAborting();
}


/*
 * Possible asking/syncing states for two peers:
 */

void EthereumPeer::setIdle()
{
    setAsking(Asking::Nothing);
}

void EthereumPeer::requestStatus(u256 _hostNetworkId, u256 _chainTotalDifficulty, h256 _chainCurrentHash, h256 _chainGenesisHash)
{
    assert(m_asking == Asking::Nothing);
    setAsking(Asking::State);
    m_requireTransactions = true;
    RLPStream s;
    prep(s, StatusPacket, 5) << m_hostProtocolVersion << _hostNetworkId << _chainTotalDifficulty
                             << _chainCurrentHash << _chainGenesisHash;
    sealAndSend(s);
}

void EthereumPeer::requestBlockHeaders(unsigned _startNumber, unsigned _count, unsigned _skip, bool _reverse)
{
    if (m_asking != Asking::Nothing)
    {
        LOG(m_logger) << "Asking headers while requesting " << ::toString(m_asking);
    }
    setAsking(Asking::BlockHeaders);
    RLPStream s;
    prep(s, GetBlockHeadersPacket, 4) << _startNumber << _count << _skip << (_reverse ? 1 : 0);
    LOG(m_logger) << "Requesting " << _count << " block headers starting from " << _startNumber
                  << (_reverse ? " in reverse" : "");
    m_lastAskedHeaders = _count;
    sealAndSend(s);
}

void EthereumPeer::requestBlockHeaders(h256 const& _startHash, unsigned _count, unsigned _skip, bool _reverse)
{
    if (m_asking != Asking::Nothing)
    {
        LOG(m_logger) << "Asking headers while requesting " << ::toString(m_asking);
    }
    setAsking(Asking::BlockHeaders);
    RLPStream s;
    prep(s, GetBlockHeadersPacket, 4) << _startHash << _count << _skip << (_reverse ? 1 : 0);
    LOG(m_logger) << "Requesting " << _count << " block headers starting from " << _startHash
                  << (_reverse ? " in reverse" : "");
    m_lastAskedHeaders = _count;
    sealAndSend(s);
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

void EthereumPeer::requestByHashes(h256s const& _hashes, Asking _asking, SubprotocolPacketType _packetType)
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
        prep(s, _packetType, _hashes.size());
        for (auto const& i: _hashes)
            s << i;
        sealAndSend(s);
    }
    else
        setIdle();
}

void EthereumPeer::setAsking(Asking _a)
{
    m_asking = _a;
    m_lastAsk = std::chrono::system_clock::to_time_t(chrono::system_clock::now());

    auto s = session();
    if (s)
    {
        s->addNote("ask", ::toString(_a));
        s->addNote("sync", string(isCriticalSyncing() ? "ONGOING" : "holding") + (needsSyncing() ? " & needed" : ""));
    }
}

void EthereumPeer::tick()
{
    auto s = session();
    time_t  now = std::chrono::system_clock::to_time_t(chrono::system_clock::now());
    if (s && (now - m_lastAsk > 10 && m_asking != Asking::Nothing))
        // timeout
        s->disconnect(PingTimeout);
}

bool EthereumPeer::isConversing() const
{
    return m_asking != Asking::Nothing;
}

bool EthereumPeer::isCriticalSyncing() const
{
    return m_asking == Asking::BlockHeaders || m_asking == Asking::State || (m_asking == Asking::BlockBodies && m_protocolVersion == 62);
}

bool EthereumPeer::interpretCapabilityPacket(unsigned, RLP const&)
{
    return true;
}
