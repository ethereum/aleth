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
/** @file EthereumHost.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "EthereumHost.h"

#include <chrono>
#include <thread>
#include <libdevcore/Common.h>
#include <libp2p/Host.h>
#include <libp2p/Session.h>
#include <libethcore/Exceptions.h>
#include "BlockChain.h"
#include "TransactionQueue.h"
#include "BlockQueue.h"
#include "BlockChainSync.h"

using namespace std;
using namespace dev;
using namespace dev::eth;

static unsigned const c_maxSendTransactions = 256;
static const unsigned c_maxHeadersToSend = 1024;
static const unsigned c_maxIncomingNewHashes = 1024;

char const* const EthereumHost::s_stateNames[static_cast<int>(SyncState::Size)] = {"NotSynced", "Idle", "Waiting", "Blocks", "State"};

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

class EthereumPeerObserver: public EthereumPeerObserverFace
{
public:
    EthereumPeerObserver(shared_ptr<BlockChainSync> _sync, TransactionQueue& _tq): m_sync(_sync), m_tq(_tq) {}

    void onPeerStatus(NodeID const& _peerID, EthereumPeerStatus const& _status) override
    {
        try
        {
            m_sync->onPeerStatus(_peerID, _status);
        }
        catch (FailedInvariant const&)
        {
            // "fix" for https://github.com/ethereum/webthree-umbrella/issues/300
            cwarn << "Failed invariant during sync, restarting sync";
            m_sync->restartSync();
        }
    }

    void onPeerTransactions(NodeID const& _peerID, RLP const& _r) override
    {
        unsigned itemCount = _r.itemCount();
        LOG(m_logger) << "Transactions (" << dec << itemCount << " entries)";
        m_tq.enqueue(_r, _peerID);
    }

    void onPeerAborting() override
    {
        try
        {
            m_sync->onPeerAborting();
        }
        catch (Exception&)
        {
            cwarn << "Exception on peer destruciton: " << boost::current_exception_diagnostic_information();
        }
    }

    void onPeerBlockHeaders(NodeID const& _peerID, RLP const& _headers) override
    {
        try
        {
            m_sync->onPeerBlockHeaders(_peerID, _headers);
        }
        catch (FailedInvariant const&)
        {
            // "fix" for https://github.com/ethereum/webthree-umbrella/issues/300
            cwarn << "Failed invariant during sync, restarting sync";
            m_sync->restartSync();
        }
    }

    void onPeerBlockBodies(NodeID const& _peerID, RLP const& _r) override
    {
        try
        {
            m_sync->onPeerBlockBodies(_peerID, _r);
        }
        catch (FailedInvariant const&)
        {
            // "fix" for https://github.com/ethereum/webthree-umbrella/issues/300
            cwarn << "Failed invariant during sync, restarting sync";
            m_sync->restartSync();
        }
    }

    void onPeerNewHashes(
        NodeID const& _peerID, std::vector<std::pair<h256, u256>> const& _hashes) override
    {
        try
        {
            m_sync->onPeerNewHashes(_peerID, _hashes);
        }
        catch (FailedInvariant const&)
        {
            // "fix" for https://github.com/ethereum/webthree-umbrella/issues/300
            cwarn << "Failed invariant during sync, restarting sync";
            m_sync->restartSync();
        }
    }

    void onPeerNewBlock(NodeID const& _peerID, RLP const& _r) override
    {
        try
        {
            m_sync->onPeerNewBlock(_peerID, _r);
        }
        catch (FailedInvariant const&)
        {
            // "fix" for https://github.com/ethereum/webthree-umbrella/issues/300
            cwarn << "Failed invariant during sync, restarting sync";
            m_sync->restartSync();
        }
    }

    void onPeerNodeData(NodeID const& /* _peerID */, RLP const& _r) override
    {
        unsigned itemCount = _r.itemCount();
        LOG(m_logger) << "Node Data (" << dec << itemCount << " entries)";
    }

    void onPeerReceipts(NodeID const& /* _peerID */, RLP const& _r) override
    {
        unsigned itemCount = _r.itemCount();
        LOG(m_logger) << "Receipts (" << dec << itemCount << " entries)";
    }

private:
    shared_ptr<BlockChainSync> m_sync;
    TransactionQueue& m_tq;

    Logger m_logger{createLogger(VerbosityDebug, "host")};
};

class EthereumHostData: public EthereumHostDataFace
{
public:
    EthereumHostData(BlockChain const& _chain, OverlayDB const& _db): m_chain(_chain), m_db(_db) {}

    pair<bytes, unsigned> blockHeaders(RLP const& _blockId, unsigned _maxHeaders, u256 _skip, bool _reverse) const override
    {
        auto numHeadersToSend = _maxHeaders;

        auto step = static_cast<unsigned>(_skip) + 1;
        assert(step > 0 && "step must not be 0");

        h256 blockHash;
        if (_blockId.size() == 32) // block id is a hash
        {
            blockHash = _blockId.toHash<h256>();
            cnetlog << "GetBlockHeaders (block (hash): " << blockHash
                    << ", maxHeaders: " << _maxHeaders << ", skip: " << _skip
                    << ", reverse: " << _reverse << ")";

            if (!m_chain.isKnown(blockHash))
                blockHash = {};
            else if (!_reverse)
            {
                auto n = m_chain.number(blockHash);
                if (numHeadersToSend == 0)
                    blockHash = {};
                else if (n != 0 || blockHash == m_chain.genesisHash())
                {
                    auto top = n + uint64_t(step) * numHeadersToSend - 1;
                    auto lastBlock = m_chain.number();
                    if (top > lastBlock)
                    {
                        numHeadersToSend = (lastBlock - n) / step + 1;
                        top = n + step * (numHeadersToSend - 1);
                    }
                    assert(top <= lastBlock && "invalid top block calculated");
                    blockHash = m_chain.numberHash(static_cast<unsigned>(top)); // override start block hash with the hash of the top block we have
                }
                else
                    blockHash = {};
            }
        }
        else // block id is a number
        {
            auto n = _blockId.toInt<bigint>();
            cnetlog << "GetBlockHeaders (" << n << " max: " << _maxHeaders << " skip: " << _skip
                    << (_reverse ? " reverse" : "") << ")";

            if (!_reverse)
            {
                auto lastBlock = m_chain.number();
                if (n > lastBlock || numHeadersToSend == 0)
                    blockHash = {};
                else
                {
                    bigint top = n + uint64_t(step) * (numHeadersToSend - 1);
                    if (top > lastBlock)
                    {
                        numHeadersToSend = (lastBlock - static_cast<unsigned>(n)) / step + 1;
                        top = n + step * (numHeadersToSend - 1);
                    }
                    assert(top <= lastBlock && "invalid top block calculated");
                    blockHash = m_chain.numberHash(static_cast<unsigned>(top)); // override start block hash with the hash of the top block we have
                }
            }
            else if (n <= std::numeric_limits<unsigned>::max())
                blockHash = m_chain.numberHash(static_cast<unsigned>(n));
            else
                blockHash = {};
        }

        auto nextHash = [this](h256 _h, unsigned _step)
        {
            static const unsigned c_blockNumberUsageLimit = 1000;

            const auto lastBlock = m_chain.number();
            const auto limitBlock = lastBlock > c_blockNumberUsageLimit ? lastBlock - c_blockNumberUsageLimit : 0; // find the number of the block below which we don't expect BC changes.

            while (_step) // parent hash traversal
            {
                auto details = m_chain.details(_h);
                if (details.number < limitBlock)
                    break; // stop using parent hash traversal, fallback to using block numbers
                _h = details.parent;
                --_step;
            }

            if (_step) // still need lower block
            {
                auto n = m_chain.number(_h);
                if (n >= _step)
                    _h = m_chain.numberHash(n - _step);
                else
                    _h = {};
            }


            return _h;
        };

        bytes rlp;
        unsigned itemCount = 0;
        vector<h256> hashes;
        for (unsigned i = 0; i != numHeadersToSend; ++i)
        {
            if (!blockHash || !m_chain.isKnown(blockHash))
                break;

            hashes.push_back(blockHash);
            ++itemCount;

            blockHash = nextHash(blockHash, step);
        }

        for (unsigned i = 0; i < hashes.size() && rlp.size() < c_maxPayload; ++i)
            rlp += m_chain.headerData(hashes[_reverse ? i : hashes.size() - 1 - i]);

        return make_pair(rlp, itemCount);
    }

    pair<bytes, unsigned> blockBodies(RLP const& _blockHashes) const override
    {
        unsigned const count = static_cast<unsigned>(_blockHashes.itemCount());

        bytes rlp;
        unsigned n = 0;
        auto numBodiesToSend = std::min(count, c_maxBlocks);
        for (unsigned i = 0; i < numBodiesToSend && rlp.size() < c_maxPayload; ++i)
        {
            auto h = _blockHashes[i].toHash<h256>();
            if (m_chain.isKnown(h))
            {
                bytes blockBytes = m_chain.block(h);
                RLP block{blockBytes};
                RLPStream body;
                body.appendList(2);
                body.appendRaw(block[1].data()); // transactions
                body.appendRaw(block[2].data()); // uncles
                auto bodyBytes = body.out();
                rlp.insert(rlp.end(), bodyBytes.begin(), bodyBytes.end());
                ++n;
            }
        }
        if (count > 20 && n == 0)
            cnetlog << "all " << count << " unknown blocks requested; peer on different chain?";
        else
            cnetlog << n << " blocks known and returned; " << (numBodiesToSend - n)
                    << " blocks unknown; " << (count > c_maxBlocks ? count - c_maxBlocks : 0)
                    << " blocks ignored";

        return make_pair(rlp, n);
    }

    strings nodeData(RLP const& _dataHashes) const override
    {
        unsigned const count = static_cast<unsigned>(_dataHashes.itemCount());

        strings data;
        size_t payloadSize = 0;
        auto numItemsToSend = std::min(count, c_maxNodes);
        for (unsigned i = 0; i < numItemsToSend && payloadSize < c_maxPayload; ++i)
        {
            auto h = _dataHashes[i].toHash<h256>();
            auto node = m_db.lookup(h);
            if (!node.empty())
            {
                payloadSize += node.length();
                data.push_back(move(node));
            }
        }
        cnetlog << data.size() << " nodes known and returned; " << (numItemsToSend - data.size())
                << " unknown; " << (count > c_maxNodes ? count - c_maxNodes : 0) << " ignored";

        return data;
    }

    pair<bytes, unsigned> receipts(RLP const& _blockHashes) const override
    {
        unsigned const count = static_cast<unsigned>(_blockHashes.itemCount());

        bytes rlp;
        unsigned n = 0;
        auto numItemsToSend = std::min(count, c_maxReceipts);
        for (unsigned i = 0; i < numItemsToSend && rlp.size() < c_maxPayload; ++i)
        {
            auto h = _blockHashes[i].toHash<h256>();
            if (m_chain.isKnown(h))
            {
                auto const receipts = m_chain.receipts(h);
                auto receiptsRlpList = receipts.rlp();
                rlp.insert(rlp.end(), receiptsRlpList.begin(), receiptsRlpList.end());
                ++n;
            }
        }
        cnetlog << n << " receipt lists known and returned; " << (numItemsToSend - n)
                << " unknown; " << (count > c_maxReceipts ? count - c_maxReceipts : 0)
                << " ignored";

        return make_pair(rlp, n);
    }

private:
    BlockChain const& m_chain;
    OverlayDB const& m_db;
};

}

EthereumHost::EthereumHost(shared_ptr<p2p::CapabilityHostFace> _host, BlockChain const& _ch,
    OverlayDB const& _db, TransactionQueue& _tq, BlockQueue& _bq, u256 _networkId)
  : Worker("ethsync"),
    m_host(move(_host)),
    m_chain(_ch),
    m_db(_db),
    m_tq(_tq),
    m_bq(_bq),
    m_networkId(_networkId),
    m_hostData(new EthereumHostData(m_chain, m_db))
{
    // TODO: Composition would be better. Left like that to avoid initialization
    //       issues as BlockChainSync accesses other EthereumHost members.
    m_sync.reset(new BlockChainSync(*this));
    m_peerObserver.reset(new EthereumPeerObserver(m_sync, m_tq));
    m_latestBlockSent = _ch.currentHash();
    m_tq.onImport([this](ImportResult _ir, h256 const& _h, h512 const& _nodeId) { onTransactionImported(_ir, _h, _nodeId); });
    std::random_device seed;
    m_urng = std::mt19937_64(seed());
}

EthereumHost::~EthereumHost()
{
    terminate();
}

bool EthereumHost::ensureInitialised()
{
    if (!m_latestBlockSent)
    {
        // First time - just initialise.
        m_latestBlockSent = m_chain.currentHash();
        LOG(m_logger) << "Initialising: latest=" << m_latestBlockSent;

        Guard l(x_transactions);
        m_transactionsSent = m_tq.knownTransactions();
        return true;
    }
    return false;
}

void EthereumHost::reset()
{
    m_sync->abortSync();

    m_latestBlockSent = h256();
    Guard tl(x_transactions);
    m_transactionsSent.clear();
}

void EthereumHost::completeSync()
{
    m_sync->completeSync();
}

void EthereumHost::doWork()
{
    bool netChange = ensureInitialised();
    auto h = m_chain.currentHash();
    // If we've finished our initial sync (including getting all the blocks into the chain so as to reduce invalid transactions), start trading transactions & blocks
    if (!isSyncing() && m_chain.isKnown(m_latestBlockSent))
    {
        if (m_newTransactions)
        {
            m_newTransactions = false;
            maintainTransactions();
        }
        if (m_newBlocks)
        {
            m_newBlocks = false;
            maintainBlocks(h);
        }
    }

    time_t  now = std::chrono::system_clock::to_time_t(chrono::system_clock::now());
    if (now - m_lastTick >= 1)
    {
        m_lastTick = now;
        for (auto const& peer : m_peers)
        {
            time_t now = std::chrono::system_clock::to_time_t(chrono::system_clock::now());
            auto const& status = peer.second;
            if (now - status.m_lastAsk > 10 && status.m_asking != Asking::Nothing)
                // timeout
                m_host->disconnect(peer.first, p2p::PingTimeout);
        }
    }

//	return netChange;
    // TODO: Figure out what to do with netChange.
    (void)netChange;
}

void EthereumHost::maintainTransactions()
{
    // Send any new transactions.
    unordered_map<NodeID, std::vector<size_t>> peerTransactions;
    auto ts = m_tq.topTransactions(c_maxSendTransactions);
    {
        Guard l(x_transactions);
        for (size_t i = 0; i < ts.size(); ++i)
        {
            auto const& t = ts[i];
            bool unsent = !m_transactionsSent.count(t.sha3());
            auto peers = get<1>(randomSelection(0, [&](EthereumPeerStatus const& status) {
                return status.m_requireTransactions ||
                       (unsent && !status.m_knownTransactions.count(t.sha3()));
            }));
            for (auto const& p: peers)
                peerTransactions[p].push_back(i);
        }
        for (auto const& t: ts)
            m_transactionsSent.insert(t.sha3());
    }

    // TODO this is not thread-safe, call this code from net IO thread instead
    for (auto& peer : m_peers)
    {
        bytes b;
        unsigned n = 0;
        for (auto const& i : peerTransactions[peer.first])
        {
            peer.second.m_knownTransactions.insert(ts[i].sha3());
            b += ts[i].rlp();
            ++n;
        }

        if (n || peer.second.m_requireTransactions)
        {
            RLPStream ts;
            m_host->prep(peer.first, name(), ts, TransactionsPacket, n).appendRaw(b, n);
            m_host->sealAndSend(peer.first, ts);
            LOG(m_logger) << "Sent " << n << " transactions to " << peer.first;
        }
        peer.second.m_requireTransactions = false;
    }
}

tuple<vector<NodeID>, vector<NodeID>> EthereumHost::randomSelection(
    unsigned _percent, std::function<bool(EthereumPeerStatus const&)> const& _allow)
{
    vector<NodeID> chosen;
    vector<NodeID> allowed;
    double percentDecimal = _percent / 100.0;

    for (auto const& peer : m_peers)
    {
        if (_allow(peer.second))
            allowed.push_back(peer.first);
    }

    if (_percent == 0 || allowed.size() == 0)
    {
        return make_tuple(move(chosen), move(allowed));
    }

    std::shuffle(allowed.begin(), allowed.end(), m_urng);

    // Remove elements from the end of the shuffled allowed vector and move them to chosen.
    size_t chosenSize = percentDecimal * allowed.size();
    chosen.reserve(chosenSize);
    std::move(allowed.begin() + chosenSize, allowed.end(), std::back_inserter(chosen));
    allowed.erase(allowed.begin() + chosenSize, allowed.end());
    return make_tuple(move(chosen), move(allowed));
}

void EthereumHost::maintainBlocks(h256 const& _currentHash)
{
    // Send any new blocks.
    auto detailsFrom = m_chain.details(m_latestBlockSent);
    auto detailsTo = m_chain.details(_currentHash);
    if (detailsFrom.totalDifficulty < detailsTo.totalDifficulty)
    {
        if (diff(detailsFrom.number, detailsTo.number) < 20)
        {
            // don't be sending more than 20 "new" blocks. if there are any more we were probably waaaay behind.
            LOG(m_logger) << "Sending a new block (current is " << _currentHash << ", was "
                          << m_latestBlockSent << ")";

            h256s blocks = get<0>(m_chain.treeRoute(m_latestBlockSent, _currentHash, false, false, true));

            auto s = randomSelection(25, [&](EthereumPeerStatus const& _status) {
                Guard guard(x_transactions);
                return !_status.m_knownBlocks.count(_currentHash);
            });
            for (NodeID const& peerID : get<0>(s))
                for (auto const& b: blocks)
                {
                    RLPStream ts;
                    m_host->prep(peerID, name(), ts, NewBlockPacket, 2)
                        .appendRaw(m_chain.block(b), 1)
                        .append(m_chain.details(b).totalDifficulty);

                    auto itPeer = m_peers.find(peerID);
                    if (itPeer != m_peers.end())
                    {
                        Guard l(itPeer->second.x_knownBlocks);
                        m_host->sealAndSend(peerID, ts);
                        itPeer->second.m_knownBlocks.clear();
                    }
                }
            for (NodeID const& peerID : get<1>(s))
            {
                RLPStream ts;
                m_host->prep(peerID, name(), ts, NewBlockHashesPacket, blocks.size());
                for (auto const& b: blocks)
                {
                    ts.appendList(2);
                    ts.append(b);
                    ts.append(m_chain.number(b));
                }

                auto itPeer = m_peers.find(peerID);
                if (itPeer != m_peers.end())
                {
                    Guard l(itPeer->second.x_knownBlocks);
                    m_host->sealAndSend(peerID, ts);
                    itPeer->second.m_knownBlocks.clear();
                }
            }
        }
        m_latestBlockSent = _currentHash;
    }
}

bool EthereumHost::isSyncing() const
{
    return m_sync->isSyncing();
}

SyncStatus EthereumHost::status() const
{
    return m_sync->status();
}

void EthereumHost::onTransactionImported(ImportResult _ir, h256 const& _h, h512 const& _nodeId)
{
    auto itPeerStatus = m_peers.find(_nodeId);
    if (itPeerStatus == m_peers.end())
        return;

    auto& peerStatus = itPeerStatus->second;

    Guard l(peerStatus.x_knownTransactions);
    peerStatus.m_knownTransactions.insert(_h);
    switch (_ir)
    {
    case ImportResult::Malformed:
        m_host->addRating(_nodeId, -100);
        break;
    case ImportResult::AlreadyKnown:
        // if we already had the transaction, then don't bother sending it on.
        DEV_GUARDED(x_transactions) { m_transactionsSent.insert(_h); }
        m_host->addRating(_nodeId, 0);
        break;
    case ImportResult::Success:
        m_host->addRating(_nodeId, 100);
        break;
    default:;
    }
}

void EthereumHost::onConnect(NodeID const& _peerID, u256 const& _peerCapabilityVersion)
{
    m_peers[_peerID].m_peerCapabilityVersion = _peerCapabilityVersion;
    requestStatus(_peerID, m_networkId, m_chain.details().totalDifficulty, m_chain.currentHash(),
        m_chain.genesisHash());
}

void EthereumHost::onDisconnect(NodeID const& _nodeID)
{
    m_peers.erase(_nodeID);
}

bool EthereumHost::interpretCapabilityPacket(NodeID const& _peerID, unsigned _id, RLP const& _r)
{
    auto& peerStatus = m_peers[_peerID];
    peerStatus.m_lastAsk = std::chrono::system_clock::to_time_t(chrono::system_clock::now());

    try
    {
        switch (_id)
        {
        case StatusPacket:
        {
            peerStatus.m_protocolVersion = _r[0].toInt<unsigned>();
            peerStatus.m_networkId = _r[1].toInt<u256>();
            peerStatus.m_totalDifficulty = _r[2].toInt<u256>();
            peerStatus.m_latestHash = _r[3].toHash<h256>();
            peerStatus.m_genesisHash = _r[4].toHash<h256>();
            if (peerStatus.m_peerCapabilityVersion == protocolVersion())
                peerStatus.m_protocolVersion = protocolVersion();

            LOG(m_logger) << "Status: " << peerStatus.m_protocolVersion << " / "
                          << peerStatus.m_networkId << " / " << peerStatus.m_genesisHash
                          << ", TD: " << peerStatus.m_totalDifficulty << " = "
                          << peerStatus.m_latestHash;
            setIdle(_peerID);
            m_peerObserver->onPeerStatus(_peerID, peerStatus);
            break;
        }
        case TransactionsPacket:
        {
            m_peerObserver->onPeerTransactions(_peerID, _r);
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

            auto numHeadersToSend = maxHeaders <= c_maxHeadersToSend ?
                                        static_cast<unsigned>(maxHeaders) :
                                        c_maxHeadersToSend;

            if (skip > std::numeric_limits<unsigned>::max() - 1)
            {
                cnetdetails << "Requested block skip is too big: " << skip;
                break;
            }

            pair<bytes, unsigned> const rlpAndItemCount =
                m_hostData->blockHeaders(blockId, numHeadersToSend, skip, reverse);

            RLPStream s;
            m_host->prep(_peerID, name(), s, BlockHeadersPacket, rlpAndItemCount.second)
                .appendRaw(rlpAndItemCount.first, rlpAndItemCount.second);
            m_host->sealAndSend(_peerID, s);
            m_host->addRating(_peerID, 0);
            break;
        }
        case BlockHeadersPacket:
        {
            if (peerStatus.m_asking != Asking::BlockHeaders)
                LOG(m_loggerImpolite)
                    << "Peer giving us block headers when we didn't ask for them.";
            else
            {
                setIdle(_peerID);
                m_peerObserver->onPeerBlockHeaders(_peerID, _r);
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
                m_host->addRating(_peerID, -10);
                break;
            }

            pair<bytes, unsigned> const rlpAndItemCount = m_hostData->blockBodies(_r);

            m_host->addRating(_peerID, 0);
            RLPStream s;
            m_host->prep(_peerID, name(), s, BlockBodiesPacket, rlpAndItemCount.second)
                .appendRaw(rlpAndItemCount.first, rlpAndItemCount.second);
            m_host->sealAndSend(_peerID, s);
            break;
        }
        case BlockBodiesPacket:
        {
            if (peerStatus.m_asking != Asking::BlockBodies)
                LOG(m_loggerImpolite) << "Peer giving us block bodies when we didn't ask for them.";
            else
            {
                setIdle(_peerID);
                m_peerObserver->onPeerBlockBodies(_peerID, _r);
            }
            break;
        }
        case NewBlockPacket:
        {
            m_peerObserver->onPeerNewBlock(_peerID, _r);
            break;
        }
        case NewBlockHashesPacket:
        {
            unsigned itemCount = _r.itemCount();

            cnetlog << "BlockHashes (" << dec << itemCount << " entries) "
                    << (itemCount ? "" : " : NoMoreHashes");

            if (itemCount > c_maxIncomingNewHashes)
            {
                disablePeer(_peerID, "Too many new hashes");
                break;
            }

            vector<pair<h256, u256>> hashes(itemCount);
            for (unsigned i = 0; i < itemCount; ++i)
                hashes[i] = std::make_pair(_r[i][0].toHash<h256>(), _r[i][1].toInt<u256>());

            m_peerObserver->onPeerNewHashes(_peerID, hashes);
            break;
        }
        case GetNodeDataPacket:
        {
            unsigned count = static_cast<unsigned>(_r.itemCount());
            if (!count)
            {
                LOG(m_loggerImpolite) << "Zero-entry GetNodeData: Not replying.";
                m_host->addRating(_peerID, -10);
                break;
            }
            cnetlog << "GetNodeData (" << dec << count << " entries)";

            strings const data = m_hostData->nodeData(_r);

            m_host->addRating(_peerID, 0);
            RLPStream s;
            m_host->prep(_peerID, name(), s, NodeDataPacket, data.size());
            for (auto const& element : data)
                s.append(element);
            m_host->sealAndSend(_peerID, s);
            break;
        }
        case GetReceiptsPacket:
        {
            unsigned count = static_cast<unsigned>(_r.itemCount());
            if (!count)
            {
                LOG(m_loggerImpolite) << "Zero-entry GetReceipts: Not replying.";
                m_host->addRating(_peerID, -10);
                break;
            }
            cnetlog << "GetReceipts (" << dec << count << " entries)";

            pair<bytes, unsigned> const rlpAndItemCount = m_hostData->receipts(_r);

            m_host->addRating(_peerID, 0);
            RLPStream s;
            m_host->prep(_peerID, name(), s, ReceiptsPacket, rlpAndItemCount.second)
                .appendRaw(rlpAndItemCount.first, rlpAndItemCount.second);
            m_host->sealAndSend(_peerID, s);
            break;
        }
        case NodeDataPacket:
        {
            if (peerStatus.m_asking != Asking::NodeData)
                LOG(m_loggerImpolite) << "Peer giving us node data when we didn't ask for them.";
            else
            {
                setIdle(_peerID);
                m_peerObserver->onPeerNodeData(_peerID, _r);
            }
            break;
        }
        case ReceiptsPacket:
        {
            if (peerStatus.m_asking != Asking::Receipts)
                LOG(m_loggerImpolite) << "Peer giving us receipts when we didn't ask for them.";
            else
            {
                setIdle(_peerID);
                m_peerObserver->onPeerReceipts(_peerID, _r);
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

void EthereumHost::setIdle(NodeID const& _peerID)
{
    setAsking(_peerID, Asking::Nothing);
}

void EthereumHost::setAsking(NodeID const& _peerID, Asking _a)
{
    auto itPeerStatus = m_peers.find(_peerID);
    if (itPeerStatus == m_peers.end())
        return;

    auto& peerStatus = itPeerStatus->second;

    peerStatus.m_asking = _a;
    peerStatus.m_lastAsk = std::chrono::system_clock::to_time_t(chrono::system_clock::now());

    m_host->addNote(_peerID, "ask", ::toString(_a));
    m_host->addNote(_peerID, "sync",
        string(isCriticalSyncing(_peerID) ? "ONGOING" : "holding") +
            (needsSyncing(_peerID) ? " & needed" : ""));
}

bool EthereumHost::isCriticalSyncing(NodeID const& _peerID) const
{
    auto itPeerStatus = m_peers.find(_peerID);
    if (itPeerStatus == m_peers.end())
        return false;

    auto const& peerStatus = itPeerStatus->second;

    return peerStatus.m_asking == Asking::BlockHeaders || peerStatus.m_asking == Asking::State ||
           (peerStatus.m_asking == Asking::BlockBodies && peerStatus.m_protocolVersion == 62);
}

bool EthereumHost::needsSyncing(NodeID const& _peerID) const
{
    if (m_host->isRude(_peerID, name()))
        return false;

    auto peerStatus = m_peers.find(_peerID);
    return (peerStatus != m_peers.end() && peerStatus->second.m_latestHash);
}

EthereumPeerStatus const& EthereumHost::peerStatus(NodeID const& _peerID) const
{
    auto const peer = m_peers.find(_peerID);
    if (peer == m_peers.end())
        BOOST_THROW_EXCEPTION(PeerDisconnected() << errinfo_nodeID(_peerID));

    return peer->second;
}

bool EthereumHost::isPeerConversing(NodeID const& _peerID) const
{
    auto const peer = m_peers.find(_peerID);
    if (peer == m_peers.end())
        BOOST_THROW_EXCEPTION(PeerDisconnected() << errinfo_nodeID(_peerID));

    return peer->second.m_asking != Asking::Nothing;
}

void EthereumHost::markPeerAsWaitingForTransactions(NodeID const& _peerID)
{
    m_peers[_peerID].m_requireTransactions = true;
}

void EthereumHost::markBlockAsKnownToPeer(NodeID const& _peerID, h256 const& _hash)
{
    auto& peer = m_peers[_peerID];
    DEV_GUARDED(peer.x_knownBlocks)
    peer.m_knownBlocks.insert(_hash);
}

void EthereumHost::setPeerLatestHash(NodeID const& _peerID, h256 const& _hash)
{
    m_peers[_peerID].m_latestHash = _hash;
}

void EthereumHost::incrementPeerUnknownNewBlocks(NodeID const& _peerID)
{
    ++m_peers[_peerID].m_unknownNewBlocks;
}

void EthereumHost::disablePeer(NodeID const& _peerID, std::string const& _problem)
{
    m_host->disableCapability(_peerID, name(), _problem);
}

void EthereumHost::requestStatus(NodeID const& _peerID, u256 _hostNetworkId,
    u256 _chainTotalDifficulty, h256 _chainCurrentHash, h256 _chainGenesisHash)
{
    auto itPeerStatus = m_peers.find(_peerID);
    if (itPeerStatus == m_peers.end())
        return;

    auto& peerStatus = itPeerStatus->second;

    assert(peerStatus.m_asking == Asking::Nothing);
    setAsking(_peerID, Asking::State);
    peerStatus.m_requireTransactions = true;
    RLPStream s;
    m_host->prep(_peerID, name(), s, StatusPacket, 5)
        << protocolVersion() << _hostNetworkId << _chainTotalDifficulty << _chainCurrentHash
        << _chainGenesisHash;
    m_host->sealAndSend(_peerID, s);
}

void EthereumHost::requestBlockHeaders(
    NodeID const& _peerID, unsigned _startNumber, unsigned _count, unsigned _skip, bool _reverse)
{
    auto itPeerStatus = m_peers.find(_peerID);
    if (itPeerStatus == m_peers.end())
        return;

    auto& peerStatus = itPeerStatus->second;

    if (peerStatus.m_asking != Asking::Nothing)
    {
        LOG(m_logger) << "Asking headers while requesting " << ::toString(peerStatus.m_asking);
    }
    setAsking(_peerID, Asking::BlockHeaders);
    RLPStream s;
    m_host->prep(_peerID, name(), s, GetBlockHeadersPacket, 4)
        << _startNumber << _count << _skip << (_reverse ? 1 : 0);
    LOG(m_logger) << "Requesting " << _count << " block headers starting from " << _startNumber
                  << (_reverse ? " in reverse" : "");
    peerStatus.m_lastAskedHeaders = _count;
    m_host->sealAndSend(_peerID, s);
}

void EthereumHost::requestBlockHeaders(
    NodeID const& _peerID, h256 const& _startHash, unsigned _count, unsigned _skip, bool _reverse)
{
    auto itPeerStatus = m_peers.find(_peerID);
    if (itPeerStatus == m_peers.end())
        return;

    auto& peerStatus = itPeerStatus->second;

    if (peerStatus.m_asking != Asking::Nothing)
    {
        LOG(m_logger) << "Asking headers while requesting " << ::toString(peerStatus.m_asking);
    }
    setAsking(_peerID, Asking::BlockHeaders);
    RLPStream s;
    m_host->prep(_peerID, name(), s, GetBlockHeadersPacket, 4)
        << _startHash << _count << _skip << (_reverse ? 1 : 0);
    LOG(m_logger) << "Requesting " << _count << " block headers starting from " << _startHash
                  << (_reverse ? " in reverse" : "");
    peerStatus.m_lastAskedHeaders = _count;
    m_host->sealAndSend(_peerID, s);
}


void EthereumHost::requestBlockBodies(NodeID const& _peerID, h256s const& _blocks)
{
    requestByHashes(_peerID, _blocks, Asking::BlockBodies, GetBlockBodiesPacket);
}

void EthereumHost::requestNodeData(NodeID const& _peerID, h256s const& _hashes)
{
    requestByHashes(_peerID, _hashes, Asking::NodeData, GetNodeDataPacket);
}

void EthereumHost::requestReceipts(NodeID const& _peerID, h256s const& _blocks)
{
    requestByHashes(_peerID, _blocks, Asking::Receipts, GetReceiptsPacket);
}

void EthereumHost::requestByHashes(
    NodeID const& _peerID, h256s const& _hashes, Asking _asking, SubprotocolPacketType _packetType)
{
    auto itPeerStatus = m_peers.find(_peerID);
    if (itPeerStatus == m_peers.end())
        return;

    auto& peerStatus = itPeerStatus->second;

    if (peerStatus.m_asking != Asking::Nothing)
    {
        LOG(m_logger) << "Asking " << ::toString(_asking) << " while requesting "
                      << ::toString(peerStatus.m_asking);
    }
    setAsking(_peerID, _asking);
    if (_hashes.size())
    {
        RLPStream s;
        m_host->prep(_peerID, name(), s, _packetType, _hashes.size());
        for (auto const& i : _hashes)
            s << i;
        m_host->sealAndSend(_peerID, s);
    }
    else
        setIdle(_peerID);
}
