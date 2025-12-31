// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "WarpCapability.h"
#include "BlockChain.h"
#include "SnapshotStorage.h"

#include <boost/fiber/all.hpp>
#include <chrono>

namespace dev
{
namespace eth
{

std::chrono::milliseconds constexpr WarpCapability::c_backgroundWorkInterval;

char const* warpPacketTypeToString(WarpSubprotocolPacketType _packetType)
{
    switch (_packetType)
    {
    case WarpStatusPacket:
        return "WarpStatus";
    case GetSnapshotManifestPacket:
        return "GetSnapshotManifest";
    case SnapshotManifestPacket:
        return "SnapshotManifest";
    case GetSnapshotDataPacket:
        return "GetSnapshotData";
    case SnapshotDataPacket:
        return "SnapshotData";
    default:
        return "UnknownWarpPacket";
    }
}

namespace
{
constexpr size_t c_freePeerBufferSize = 32;

bool validateManifest(RLP const& _manifestRlp)
{
    if (!_manifestRlp.isList() || _manifestRlp.itemCount() != 1)
        return false;

    RLP const manifest = _manifestRlp[0];

    u256 const version = manifest[0].toInt<u256>();
    return version == 2;
}

h256 snapshotBlockHash(RLP const& _manifestRlp)
{
    RLP const manifest = _manifestRlp[0];
    return manifest[5].toHash<h256>();
}

class WarpPeerObserver : public WarpPeerObserverFace
{
public:
    WarpPeerObserver(WarpCapability& _host, BlockChain const& _blockChain,
        boost::filesystem::path const& _snapshotPath)
      : m_host(_host),
        m_hostProtocolVersion(_host.version()),
        m_hostNetworkId(_host.networkId()),
        m_hostGenesisHash(_blockChain.genesisHash()),
        m_daoForkBlock(_blockChain.sealEngine()->chainParams().daoHardforkBlock),
        m_freePeers(c_freePeerBufferSize),
        m_snapshotDir(_snapshotPath)
    {}
    ~WarpPeerObserver()
    {
        if (m_downloadFiber)
            m_downloadFiber->join();
    }

    void onPeerStatus(NodeID const& _peerID) override
    {
        boost::fibers::fiber checkPeerFiber(&WarpPeerObserver::validatePeer, this, _peerID);
        checkPeerFiber.detach();

        // start the downloading fiber in the thread handling network messages
        if (!m_downloadFiber)
            m_downloadFiber.reset(
                new boost::fibers::fiber(&WarpPeerObserver::downloadChunks, this));

        boost::this_fiber::yield();
    }

    void onPeerManifest(NodeID const& _peerID, RLP const& _r) override
    {
        m_manifests[_peerID].set_value(_r.data().toBytes());
        boost::this_fiber::yield();
    }

    void onPeerBlockHeaders(NodeID const& _peerID, RLP const& _r) override
    {
        m_daoForkHeaders[_peerID].set_value(_r.data().toBytes());
        boost::this_fiber::yield();
    }

    void onPeerData(NodeID const& _peerID, RLP const& _r) override
    {
        if (!_r.isList() || _r.itemCount() != 1)
            return;

        RLP const data = _r[0];

        h256 const hash = sha3(data.toBytesConstRef());

        auto it = m_requestedChunks.find(_peerID);
        if (it == m_requestedChunks.end())
            return;

        h256 const askedHash = it->second;
        m_requestedChunks.erase(it);

        if (hash == askedHash)
        {
            // TODO handle writeFile failure
            writeFile((boost::filesystem::path(m_snapshotDir) / toHex(hash)).string(),
                data.toBytesConstRef());

            LOG(m_logger) << "Saved chunk " << hash << " Chunks left: " << m_neededChunks.size()
                          << " (peer: " << _peerID << ")";
            if (m_neededChunks.empty() && m_requestedChunks.empty())
                LOG(m_logger) << "Snapshot download complete";
        }
        else
            m_neededChunks.push_back(askedHash);

        m_freePeers.push(_peerID);
        boost::this_fiber::yield();
    }

    void onPeerDisconnect(NodeID const& _peerID, Asking _asking) override
    {
        if (_asking == Asking::WarpManifest)
        {
            auto it = m_manifests.find(_peerID);
            if (it != m_manifests.end())
                it->second.set_exception(std::make_exception_ptr(FailedToDownloadManifest()));
        }
        else if (_asking == Asking::BlockHeaders)
        {
            auto it = m_daoForkHeaders.find(_peerID);
            if (it != m_daoForkHeaders.end())
                it->second.set_exception(
                    std::make_exception_ptr(FailedToDownloadDaoForkBlockHeader()));
        }
        else if (_asking == Asking::WarpData)
        {
            auto it = m_requestedChunks.find(_peerID);
            if (it != m_requestedChunks.end())
            {
                m_neededChunks.push_back(it->second);
                m_requestedChunks.erase(it);
            }
        }
        boost::this_fiber::yield();
    }

private:
    void validatePeer(NodeID _peerID)
    {
        if (!m_host.validateStatus(
                _peerID, m_hostGenesisHash, {m_hostProtocolVersion}, m_hostNetworkId))
            return;

        m_host.requestManifest(_peerID);

        bytes const manifestBytes = waitForManifestResponse(_peerID);
        if (manifestBytes.empty())
            return;

        RLP manifestRlp(manifestBytes);
        if (!validateManifest(manifestRlp))
        {
            // TODO try disconnecting instead of disabling; disabled peer still occupies the peer slot
            m_host.disablePeer(_peerID, "Invalid snapshot manifest.");
            return;
        }

        u256 const snapshotHash = snapshotBlockHash(manifestRlp);
        if (m_syncingSnapshotHash)
        {
            if (snapshotHash == m_syncingSnapshotHash)
                m_freePeers.push(_peerID);
            else
                m_host.disablePeer(_peerID, "Another snapshot.");
        }
        else
        {
            if (m_daoForkBlock)
            {
                m_host.requestBlockHeaders(_peerID, m_daoForkBlock, 1, 0, false);

                bytes const headerBytes = waitForDaoForkBlockResponse(_peerID);
                if (headerBytes.empty())
                    return;

                RLP headerRlp(headerBytes);
                if (!verifyDaoChallengeResponse(headerRlp))
                {
                    m_host.disablePeer(_peerID, "Peer from another fork.");
                    return;
                }
            }

            m_syncingSnapshotHash = snapshotHash;
            m_manifest.set_value(manifestBytes);
            m_freePeers.push(_peerID);
        }
    }

    bytes waitForManifestResponse(NodeID const& _peerID)
    {
        try
        {
            bytes const result = m_manifests[_peerID].get_future().get();
            m_manifests.erase(_peerID);
            return result;
        }
        catch (Exception const&)
        {
            m_manifests.erase(_peerID);
        }
        return bytes{};
    }

    bytes waitForDaoForkBlockResponse(NodeID const& _peerID)
    {
        try
        {
            bytes const result = m_daoForkHeaders[_peerID].get_future().get();
            m_daoForkHeaders.erase(_peerID);
            return result;
        }
        catch (Exception const&)
        {
            m_daoForkHeaders.erase(_peerID);
        }
        return bytes{};
    }

    bool verifyDaoChallengeResponse(RLP const& _r)
    {
        if (_r.itemCount() != 1)
            return false;

        BlockHeader info(_r[0].data(), HeaderData);
        return info.number() == m_daoForkBlock &&
               info.extraData() == fromHex("0x64616f2d686172642d666f726b");
    }

    void downloadChunks()
    {
        bytes const manifestBytes = m_manifest.get_future().get();

        RLP manifestRlp(manifestBytes);
        RLP manifest(manifestRlp[0]);

        u256 const version = manifest[0].toInt<u256>();
        h256s const stateHashes = manifest[1].toVector<h256>();
        h256s const blockHashes = manifest[2].toVector<h256>();
        h256 const stateRoot = manifest[3].toHash<h256>();
        u256 const blockNumber = manifest[4].toInt<u256>();
        h256 const blockHash = manifest[5].toHash<h256>();

        LOG(m_logger) << "MANIFEST: "
                      << "version " << version << " state root " << stateRoot << " block number "
                      << blockNumber << " block hash " << blockHash;

        // TODO handle writeFile failure
        writeFile((boost::filesystem::path(m_snapshotDir) / "MANIFEST").string(), manifest.data());

        m_neededChunks.assign(stateHashes.begin(), stateHashes.end());
        m_neededChunks.insert(m_neededChunks.end(), blockHashes.begin(), blockHashes.end());

        while (!m_neededChunks.empty())
        {
            h256 const chunkHash(m_neededChunks.front());

            NodeID peerID;
            do
            {
                peerID = m_freePeers.value_pop();
            } while (!m_host.requestData(peerID, chunkHash));

            LOG(m_logger) << "Requested chunk " << chunkHash << " from " << peerID;

            m_requestedChunks[peerID] = chunkHash;
            m_neededChunks.pop_front();
        }
    }

    WarpCapability& m_host;
    unsigned const m_hostProtocolVersion;
    u256 const m_hostNetworkId;
    h256 const m_hostGenesisHash;
    unsigned const m_daoForkBlock;
    boost::fibers::promise<bytes> m_manifest;
    h256 m_syncingSnapshotHash;
    std::deque<h256> m_neededChunks;
    boost::fibers::buffered_channel<NodeID> m_freePeers;
    boost::filesystem::path const m_snapshotDir;
    std::map<NodeID, boost::fibers::promise<bytes>> m_manifests;
    std::map<NodeID, boost::fibers::promise<bytes>> m_daoForkHeaders;
    std::map<NodeID, h256> m_requestedChunks;

    std::unique_ptr<boost::fibers::fiber> m_downloadFiber;

    Logger m_logger{createLogger(VerbosityInfo, "snap")};
};

}  // namespace


WarpCapability::WarpCapability(std::shared_ptr<p2p::CapabilityHostFace> _host,
    BlockChain const& _blockChain, u256 const& _networkId,
    boost::filesystem::path const& _snapshotDownloadPath,
    std::shared_ptr<SnapshotStorageFace> _snapshotStorage)
  : m_host(std::move(_host)),
    m_blockChain(_blockChain),
    m_networkId(_networkId),
    m_snapshot(_snapshotStorage),
    // observer needed only in case we download snapshot
    m_peerObserver(
        _snapshotDownloadPath.empty() ? nullptr : createPeerObserver(_snapshotDownloadPath))
{
}

std::chrono::milliseconds WarpCapability::backgroundWorkInterval() const
{
    return c_backgroundWorkInterval;
}

std::shared_ptr<WarpPeerObserverFace> WarpCapability::createPeerObserver(
    boost::filesystem::path const& _snapshotDownloadPath)
{
    return std::make_shared<WarpPeerObserver>(*this, m_blockChain, _snapshotDownloadPath);
}

void WarpCapability::onConnect(NodeID const& _peerID, u256 const& /* _peerCapabilityVersion */)
{
    m_peers.emplace(_peerID, WarpPeerStatus{});

    u256 snapshotBlockNumber;
    h256 snapshotBlockHash;
    if (m_snapshot)
    {
        bytes const snapshotManifest(m_snapshot->readManifest());
        RLP manifest(snapshotManifest);
        if (manifest.itemCount() != 6)
            BOOST_THROW_EXCEPTION(InvalidSnapshotManifest());
        snapshotBlockNumber = manifest[4].toInt<u256>(RLP::VeryStrict);
        snapshotBlockHash = manifest[5].toHash<h256>(RLP::VeryStrict);
    }

    requestStatus(_peerID, c_WarpProtocolVersion, m_networkId,
        m_blockChain.details().totalDifficulty, m_blockChain.currentHash(),
        m_blockChain.genesisHash(), snapshotBlockHash, snapshotBlockNumber);
}

bool WarpCapability::interpretCapabilityPacket(NodeID const& _peerID, unsigned _id, RLP const& _r)
{
    auto& peerStatus = m_peers[_peerID];
    peerStatus.m_lastAsk = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    try
    {
        switch (_id)
        {
        case WarpStatusPacket:
        {
            if (_r.itemCount() < 7)
                BOOST_THROW_EXCEPTION(InvalidWarpStatusPacket());

            // Packet layout:
            // [ version:P, state_hashes : [hash_1:B_32, hash_2 : B_32, ...],  block_hashes :
            // [hash_1:B_32, hash_2 : B_32, ...],
            //      state_root : B_32, block_number : P, block_hash : B_32 ]
            peerStatus.m_protocolVersion = _r[0].toInt<unsigned>();
            peerStatus.m_networkId = _r[1].toInt<u256>();
            peerStatus.m_totalDifficulty = _r[2].toInt<u256>();
            peerStatus.m_latestHash = _r[3].toHash<h256>();
            peerStatus.m_genesisHash = _r[4].toHash<h256>();
            peerStatus.m_snapshotHash = _r[5].toHash<h256>();
            peerStatus.m_snapshotNumber = _r[6].toInt<u256>();

            LOG(m_logger) << "Status (from " << _peerID << "): "
                          << " protocol version " << peerStatus.m_protocolVersion << " networkId "
                          << peerStatus.m_networkId << " genesis hash " << peerStatus.m_genesisHash
                          << " total difficulty " << peerStatus.m_totalDifficulty << " latest hash "
                          << peerStatus.m_latestHash << " snapshot hash "
                          << peerStatus.m_snapshotHash << " snapshot number "
                          << peerStatus.m_snapshotNumber;
            setIdle(_peerID);
            m_peerObserver->onPeerStatus(_peerID);
            break;
        }
        case GetSnapshotManifestPacket:
        {
            if (!m_snapshot)
                return false;

            RLPStream s;
            m_host->prep(_peerID, name(), s, SnapshotManifestPacket, 1)
                .appendRaw(m_snapshot->readManifest());
            m_host->sealAndSend(_peerID, s);
            break;
        }
        case GetSnapshotDataPacket:
        {
            if (!m_snapshot)
                return false;

            const h256 chunkHash = _r[0].toHash<h256>(RLP::VeryStrict);

            RLPStream s;
            m_host->prep(_peerID, name(), s, SnapshotDataPacket, 1)
                .append(m_snapshot->readCompressedChunk(chunkHash));
            m_host->sealAndSend(_peerID, s);
            break;
        }
        case GetBlockHeadersPacket:
        {
            // TODO We are being asked DAO fork block sometimes, need to be able to answer this
            RLPStream s;
            m_host->prep(_peerID, name(), s, BlockHeadersPacket);
            m_host->sealAndSend(_peerID, s);
            break;
        }
        case BlockHeadersPacket:
        {
            setIdle(_peerID);
            m_peerObserver->onPeerBlockHeaders(_peerID, _r);
            break;
        }
        case SnapshotManifestPacket:
        {
            setIdle(_peerID);
            m_peerObserver->onPeerManifest(_peerID, _r);
            break;
        }
        case SnapshotDataPacket:
        {
            setIdle(_peerID);
            m_peerObserver->onPeerData(_peerID, _r);
            break;
        }
        default:
            return false;
        }
    }
    catch (Exception const&)
    {
        LOG(m_loggerWarn) << "Warp Peer " << _peerID << " causing an exception: "
                          << boost::current_exception_diagnostic_information() << " " << _r;
    }
    catch (std::exception const& _e)
    {
        LOG(m_loggerWarn) << "Warp Peer " << _peerID << " causing an exception: " << _e.what()
                          << " " << _r;
    }

    return true;
}

void WarpCapability::onDisconnect(NodeID const& _peerID)
{
    m_peerObserver->onPeerDisconnect(_peerID, m_peers[_peerID].m_asking);
    m_peers.erase(_peerID);
}

void WarpCapability::doBackgroundWork()
{
    for (auto const& peer : m_peers)
    {
        time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        auto const& status = peer.second;
        if (now - status.m_lastAsk > 10 && status.m_asking != Asking::Nothing)
        {
            // timeout
            m_host->disconnect(peer.first, p2p::PingTimeout);
        }
    }
}

void WarpCapability::requestStatus(NodeID const& _peerID, unsigned _hostProtocolVersion,
    u256 const& _hostNetworkId, u256 const& _chainTotalDifficulty, h256 const& _chainCurrentHash,
    h256 const& _chainGenesisHash, h256 const& _snapshotBlockHash, u256 const& _snapshotBlockNumber)
{
    RLPStream s;
    m_host->prep(_peerID, name(), s, WarpStatusPacket, 7)
        << _hostProtocolVersion << _hostNetworkId << _chainTotalDifficulty << _chainCurrentHash
        << _chainGenesisHash << _snapshotBlockHash << _snapshotBlockNumber;
    m_host->sealAndSend(_peerID, s);
}


void WarpCapability::requestBlockHeaders(
    NodeID const& _peerID, unsigned _startNumber, unsigned _count, unsigned _skip, bool _reverse)
{
    auto itPeerStatus = m_peers.find(_peerID);
    if (itPeerStatus == m_peers.end())
        return;

    assert(itPeerStatus->second.m_asking == Asking::Nothing);
    setAsking(_peerID, Asking::BlockHeaders);
    RLPStream s;
    m_host->prep(_peerID, name(), s, GetBlockHeadersPacket, 4)
        << _startNumber << _count << _skip << (_reverse ? 1 : 0);
    m_host->sealAndSend(_peerID, s);
}

void WarpCapability::requestManifest(NodeID const& _peerID)
{
    auto itPeerStatus = m_peers.find(_peerID);
    if (itPeerStatus == m_peers.end())
        return;

    assert(itPeerStatus->second.m_asking == Asking::Nothing);
    setAsking(_peerID, Asking::WarpManifest);
    RLPStream s;
    m_host->prep(_peerID, name(), s, GetSnapshotManifestPacket);
    m_host->sealAndSend(_peerID, s);
}

bool WarpCapability::requestData(NodeID const& _peerID, h256 const& _chunkHash)
{
    auto itPeerStatus = m_peers.find(_peerID);
    if (itPeerStatus == m_peers.end())
        return false;

    assert(itPeerStatus->second.m_asking == Asking::Nothing);
    setAsking(_peerID, Asking::WarpData);
    RLPStream s;

    m_host->prep(_peerID, name(), s, GetSnapshotDataPacket, 1) << _chunkHash;
    m_host->sealAndSend(_peerID, s);
    return true;
}

void WarpCapability::setAsking(NodeID const& _peerID, Asking _a)
{
    auto itPeerStatus = m_peers.find(_peerID);
    if (itPeerStatus == m_peers.end())
        return;

    auto& peerStatus = itPeerStatus->second;

    peerStatus.m_asking = _a;
    peerStatus.m_lastAsk = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

/// Validates whether peer is able to communicate with the host, disables peer if not
bool WarpCapability::validateStatus(NodeID const& _peerID, h256 const& _genesisHash,
    std::vector<unsigned> const& _protocolVersions, u256 const& _networkId)
{
    auto itPeerStatus = m_peers.find(_peerID);
    if (itPeerStatus == m_peers.end())
        return false;  // Expired

    auto const& peerStatus = itPeerStatus->second;

    if (peerStatus.m_genesisHash != _genesisHash)
    {
        disablePeer(_peerID, "Invalid genesis hash");
        return false;
    }
    if (find(_protocolVersions.begin(), _protocolVersions.end(), peerStatus.m_protocolVersion) ==
        _protocolVersions.end())
    {
        disablePeer(_peerID, "Invalid protocol version.");
        return false;
    }
    if (peerStatus.m_networkId != _networkId)
    {
        disablePeer(_peerID, "Invalid network identifier.");
        return false;
    }
    if (peerStatus.m_asking != Asking::State && peerStatus.m_asking != Asking::Nothing)
    {
        disablePeer(_peerID, "Peer banned for unexpected status message.");
        return false;
    }

    return true;
}

void WarpCapability::disablePeer(NodeID const& _peerID, std::string const& _problem)
{
    m_host->disableCapability(_peerID, name(), _problem);
}

}  // namespace eth
}  // namespace dev
