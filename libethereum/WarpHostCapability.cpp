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

#include "WarpHostCapability.h"
#include "BlockChain.h"

#include <boost/fiber/all.hpp>

namespace dev
{
namespace eth
{
namespace
{
static size_t const c_freePeerBufferSize = 32;

struct SnapshotLog : public LogChannel
{
    static char const* name() { return "SNAP"; }
    static int const verbosity = 9;
    static const bool debug = false;
};

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
    WarpPeerObserver(WarpHostCapability& _host, BlockChain const& _blockChain,
        boost::filesystem::path const& _snapshotPath)
      : m_hostProtocolVersion(_host.protocolVersion()),
        m_hostNetworkId(_host.networkId()),
        m_hostGenesisHash(_blockChain.genesisHash()),
        m_freePeers(c_freePeerBufferSize),
        m_snapshotDir(_snapshotPath)
    {}
    ~WarpPeerObserver()
    {
        if (m_downloadFiber)
            m_downloadFiber->join();
    }

    // TODO somehow handle peer disconnecting

    void onPeerStatus(std::shared_ptr<WarpPeerCapability> _peer) override
    {
        if (_peer->validateStatus(m_hostGenesisHash, {m_hostProtocolVersion}, m_hostNetworkId))
            _peer->requestManifest();

        // start the downloading fiber in the thread handling network messages
        if (!m_downloadFiber)
            m_downloadFiber.reset(new boost::fibers::fiber([this]() { downloadChunks(); }));

        boost::this_fiber::yield();
    }

    void onPeerManifest(std::shared_ptr<WarpPeerCapability> _peer, RLP const& _r) override
    {
        if (validateManifest(_r))
        {
            u256 const snapshotHash = snapshotBlockHash(_r);
            if (!m_syncingSnapshotHash)
            {
                m_syncingSnapshotHash = snapshotHash;
                m_manifest.set_value(_r.data().toBytes());
            }

            if (snapshotHash == m_syncingSnapshotHash)
                m_freePeers.push(_peer);
            else
                _peer->disable("Another snapshot.");
        }
        else
            _peer->disable("Invalid snapshot manifest.");

        boost::this_fiber::yield();
    }

    void onPeerData(std::shared_ptr<WarpPeerCapability> _peer, RLP const& _r) override
    {
        if (!_r.isList() || _r.itemCount() != 1)
            return;

        RLP const data = _r[0];

        h256 const hash = sha3(data.toBytesConstRef());

        auto it = m_requestedChunks.find(_peer);
        if (it == m_requestedChunks.end())
            return;

        h256 const askedHash = it->second;
        m_requestedChunks.erase(it);

        if (hash == askedHash)
        {
            // TODO handle writeFile failure
            writeFile((boost::filesystem::path(m_snapshotDir) / toHex(hash)).string(),
                data.toBytesConstRef());

            clog(SnapshotLog) << "Saved chunk" << hash << " Chunks left: " << m_neededChunks.size()
                              << " Requested chunks: " << m_requestedChunks.size();
            if (m_neededChunks.empty() && m_requestedChunks.empty())
                clog(SnapshotLog) << "Snapshot download complete!";
        }
        else
            m_neededChunks.push_back(askedHash);

        m_freePeers.push(_peer);
        boost::this_fiber::yield();
    }

    void onPeerRequestTimeout(
        std::shared_ptr<WarpPeerCapability> _peer, Asking /*_asking*/) override
    {
        clog(SnapshotLog) << "Peer timed out";

        auto it = m_requestedChunks.find(_peer);
        if (it == m_requestedChunks.end())
            return;

        m_neededChunks.push_back(it->second);
        m_requestedChunks.erase(it);
    }

private:
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

        clog(SnapshotLog) << "MANIFEST: "
                          << "version " << version << "state root " << stateRoot << "block number "
                          << blockNumber << "block hash " << blockHash;

        // TODO handle writeFile failure
        writeFile((boost::filesystem::path(m_snapshotDir) / "MANIFEST").string(), manifest.data());

        m_neededChunks.assign(stateHashes.begin(), stateHashes.end());
        m_neededChunks.insert(m_neededChunks.end(), blockHashes.begin(), blockHashes.end());

        while (!m_neededChunks.empty())
        {
            h256 const chunkHash(m_neededChunks.front());

            std::shared_ptr<WarpPeerCapability> peer;
            while (!peer)
                peer = m_freePeers.value_pop().lock();

            clog(SnapshotLog) << "Requesting chunk " << chunkHash;
            peer->requestData(chunkHash);

            m_requestedChunks[peer] = chunkHash;
            m_neededChunks.pop_front();
        }
    }

    unsigned const m_hostProtocolVersion;
    u256 const m_hostNetworkId;
    h256 const m_hostGenesisHash;
    boost::fibers::promise<bytes> m_manifest;
    h256 m_syncingSnapshotHash;
    std::deque<h256> m_neededChunks;
    boost::fibers::buffered_channel<std::weak_ptr<WarpPeerCapability>> m_freePeers;
    boost::filesystem::path const m_snapshotDir;
    std::map<std::weak_ptr<WarpPeerCapability>, h256,
        std::owner_less<std::weak_ptr<WarpPeerCapability>>>
        m_requestedChunks;

    std::unique_ptr<boost::fibers::fiber> m_downloadFiber;
};

}  // namespace


WarpHostCapability::WarpHostCapability(BlockChain const& _blockChain, u256 const& _networkId,
    boost::filesystem::path const& _snapshotDownloadPath,
    std::shared_ptr<SnapshotStorageFace> _snapshotStorage)
  : m_blockChain(_blockChain),
    m_networkId(_networkId),
    m_snapshot(_snapshotStorage),
    m_peerObserver(std::make_shared<WarpPeerObserver>(*this, m_blockChain, _snapshotDownloadPath)),
    m_lastTick(0)
{
}

WarpHostCapability::~WarpHostCapability()
{
    terminate();
}

std::shared_ptr<p2p::Capability> WarpHostCapability::newPeerCapability(
    std::shared_ptr<p2p::SessionFace> const& _s, unsigned _idOffset, p2p::CapDesc const& _cap)
{
    auto ret = HostCapability<WarpPeerCapability>::newPeerCapability(_s, _idOffset, _cap);

    auto cap = p2p::capabilityFromSession<WarpPeerCapability>(*_s, _cap.second);
    assert(cap);
    cap->init(c_WarpProtocolVersion, m_networkId, m_blockChain.details().totalDifficulty,
        m_blockChain.currentHash(), m_blockChain.genesisHash(), m_snapshot, m_peerObserver);

    return ret;
}

void WarpHostCapability::doWork()
{
    time_t const now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    if (now - m_lastTick >= 1)
    {
        m_lastTick = now;

        auto sessions = peerSessions();
        for (auto s : sessions)
        {
            auto cap = p2p::capabilityFromSession<WarpPeerCapability>(*s.first);
            assert(cap);
            cap->tick();
        }
    }
}

}  // namespace eth
}  // namespace dev
