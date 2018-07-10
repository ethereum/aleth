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

static const unsigned c_maxIncomingNewHashes = 1024;
static const unsigned c_maxHeadersToSend = 1024;

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

EthereumPeer::EthereumPeer(std::shared_ptr<SessionFace> _s, HostCapabilityFace* _h, unsigned _i, CapDesc const& _cap):
    Capability(_s, _h, _i),
    m_peerCapabilityVersion(_cap.second)
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

bool EthereumPeer::interpret(unsigned _id, RLP const& _r)
{
    auto observer = m_observer.lock();
    auto hostData = m_hostData.lock();
    if (!observer || !hostData)
        return false;

    m_lastAsk = std::chrono::system_clock::to_time_t(chrono::system_clock::now());
    try
    {
    switch (_id)
    {
    case StatusPacket:
    {
        m_protocolVersion = _r[0].toInt<unsigned>();
        m_networkId = _r[1].toInt<u256>();
        m_totalDifficulty = _r[2].toInt<u256>();
        m_latestHash = _r[3].toHash<h256>();
        m_genesisHash = _r[4].toHash<h256>();
        if (m_peerCapabilityVersion == m_hostProtocolVersion)
            m_protocolVersion = m_hostProtocolVersion;

        LOG(m_logger) << "Status: " << m_protocolVersion << " / " << m_networkId << " / "
                      << m_genesisHash << ", TD: " << m_totalDifficulty << " = " << m_latestHash;
        setIdle();
        observer->onPeerStatus(dynamic_pointer_cast<EthereumPeer>(dynamic_pointer_cast<EthereumPeer>(shared_from_this())));
        break;
    }
    case TransactionsPacket:
    {
        observer->onPeerTransactions(dynamic_pointer_cast<EthereumPeer>(dynamic_pointer_cast<EthereumPeer>(shared_from_this())), _r);
        break;
    }
    case GetBlockHeadersPacket:
    {
        /// Packet layout:
        /// [ block: { P , B_32 }, maxHeaders: P, skip: P, reverse: P in { 0 , 1 } ]
        const auto blockId = _r[0];
        const auto maxHeaders = _r[1].toInt<u256>();
        const auto skip = _r[2].toInt<u256>();
        const auto reverse = _r[3].toInt<bool>();

        auto numHeadersToSend = maxHeaders <= c_maxHeadersToSend ? static_cast<unsigned>(maxHeaders) : c_maxHeadersToSend;

        if (skip > std::numeric_limits<unsigned>::max() - 1)
        {
            cnetdetails << "Requested block skip is too big: " << skip;
            break;
        }

        pair<bytes, unsigned> const rlpAndItemCount = hostData->blockHeaders(blockId, numHeadersToSend, skip, reverse);

        RLPStream s;
        prep(s, BlockHeadersPacket, rlpAndItemCount.second).appendRaw(rlpAndItemCount.first, rlpAndItemCount.second);
        sealAndSend(s);
        addRating(0);
        break;
    }
    case BlockHeadersPacket:
    {
        if (m_asking != Asking::BlockHeaders)
            LOG(m_loggerImpolite) << "Peer giving us block headers when we didn't ask for them.";
        else
        {
            setIdle();
            observer->onPeerBlockHeaders(dynamic_pointer_cast<EthereumPeer>(shared_from_this()), _r);
        }
        break;
    }
    case GetBlockBodiesPacket:
    {
        unsigned count = static_cast<unsigned>(_r.itemCount());
        cnetlog << "GetBlockBodies (" << dec << count << " entries)";

        if (!count)
        {
            LOG(m_loggerImpolite) << "Zero-entry GetBlockBodies: Not replying.";
            addRating(-10);
            break;
        }

        pair<bytes, unsigned> const rlpAndItemCount = hostData->blockBodies(_r);

        addRating(0);
        RLPStream s;
        prep(s, BlockBodiesPacket, rlpAndItemCount.second).appendRaw(rlpAndItemCount.first, rlpAndItemCount.second);
        sealAndSend(s);
        break;
    }
    case BlockBodiesPacket:
    {
        if (m_asking != Asking::BlockBodies)
            LOG(m_loggerImpolite) << "Peer giving us block bodies when we didn't ask for them.";
        else
        {
            setIdle();
            observer->onPeerBlockBodies(dynamic_pointer_cast<EthereumPeer>(shared_from_this()), _r);
        }
        break;
    }
    case NewBlockPacket:
    {
        observer->onPeerNewBlock(dynamic_pointer_cast<EthereumPeer>(shared_from_this()), _r);
        break;
    }
    case NewBlockHashesPacket:
    {
        unsigned itemCount = _r.itemCount();

        cnetlog << "BlockHashes (" << dec << itemCount << " entries) "
                << (itemCount ? "" : " : NoMoreHashes");

        if (itemCount > c_maxIncomingNewHashes)
        {
            disable("Too many new hashes");
            break;
        }

        vector<pair<h256, u256>> hashes(itemCount);
        for (unsigned i = 0; i < itemCount; ++i)
            hashes[i] = std::make_pair(_r[i][0].toHash<h256>(), _r[i][1].toInt<u256>());

        observer->onPeerNewHashes(dynamic_pointer_cast<EthereumPeer>(shared_from_this()), hashes);
        break;
    }
    case GetNodeDataPacket:
    {
        unsigned count = static_cast<unsigned>(_r.itemCount());
        if (!count)
        {
            LOG(m_loggerImpolite) << "Zero-entry GetNodeData: Not replying.";
            addRating(-10);
            break;
        }
        cnetlog << "GetNodeData (" << dec << count << " entries)";

        strings const data = hostData->nodeData(_r);

        addRating(0);
        RLPStream s;
        prep(s, NodeDataPacket, data.size());
        for (auto const& element: data)
            s.append(element);
        sealAndSend(s);
        break;
    }
    case GetReceiptsPacket:
    {
        unsigned count = static_cast<unsigned>(_r.itemCount());
        if (!count)
        {
            LOG(m_loggerImpolite) << "Zero-entry GetReceipts: Not replying.";
            addRating(-10);
            break;
        }
        cnetlog << "GetReceipts (" << dec << count << " entries)";

        pair<bytes, unsigned> const rlpAndItemCount = hostData->receipts(_r);

        addRating(0);
        RLPStream s;
        prep(s, ReceiptsPacket, rlpAndItemCount.second).appendRaw(rlpAndItemCount.first, rlpAndItemCount.second);
        sealAndSend(s);
        break;
    }
    case NodeDataPacket:
    {
        if (m_asking != Asking::NodeData)
            LOG(m_loggerImpolite) << "Peer giving us node data when we didn't ask for them.";
        else
        {
            setIdle();
            observer->onPeerNodeData(dynamic_pointer_cast<EthereumPeer>(shared_from_this()), _r);
        }
        break;
    }
    case ReceiptsPacket:
    {
        if (m_asking != Asking::Receipts)
            LOG(m_loggerImpolite) << "Peer giving us receipts when we didn't ask for them.";
        else
        {
            setIdle();
            observer->onPeerReceipts(dynamic_pointer_cast<EthereumPeer>(shared_from_this()), _r);
        }
        break;
    }
    default:
        return false;
    }
    }
    catch (Exception const&)
    {
        cnetlog << "Peer causing an Exception: "
                << boost::current_exception_diagnostic_information() << " " << _r;
    }
    catch (std::exception const& _e)
    {
        cnetlog << "Peer causing an exception: " << _e.what() << " " << _r;
    }

    return true;
}
