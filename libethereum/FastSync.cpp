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

#include "FastSync.h"
#include "BlockChain.h"
#include "EthereumHost.h"

namespace dev
{
namespace eth
{
namespace
{
size_t const c_freePeerBufferSize = 64;
}

FastSync::FastSync(EthereumHost const& _host, BlockChain const& _blockChain)
    : m_hostProtocolVersion(_host.protocolVersion()),
    m_hostNetworkId(_host.networkId()),
    m_hostGenesisHash(_blockChain.genesisHash()),
    m_daoForkBlock(_blockChain.sealEngine()->chainParams().daoHardforkBlock),
    m_freePeers(c_freePeerBufferSize)
{}

FastSync::~FastSync()
{
    if (m_trieDownloadFiber)
        m_trieDownloadFiber->join();
}

void FastSync::onPeerStatus(std::shared_ptr<EthereumPeer> _peer)
{
    boost::fibers::fiber checkPeerFiber(&FastSync::validatePeer, this, std::move(_peer));
    checkPeerFiber.detach();
    // TODO fiber for selecting pivot point - waiting for several peers' head block, then selecting
    // one after time passed and pushing into promise for the next fiber

    boost::this_fiber::yield();
}

void FastSync::onPeerBlockHeaders(std::shared_ptr<EthereumPeer> _peer, RLP const& _r)
{
    m_headers[_peer].set_value(_r.data().toBytes());
    boost::this_fiber::yield();
}

void FastSync::onPeerDisconnect(std::shared_ptr<EthereumPeer> _peer, Asking _asking)
{
    if (_asking == Asking::BlockHeaders)
    {
        auto it = m_headers.find(_peer);
        if (it != m_headers.end())
            it->second.set_exception(
                std::make_exception_ptr(FailedToDownloadBlockHeaders()));
    }

    boost::this_fiber::yield();
}

void FastSync::validatePeer(std::shared_ptr<EthereumPeer> _peer)
{
    if (!_peer->validateStatus(m_hostGenesisHash, {m_hostProtocolVersion}, m_hostNetworkId))
        return;

    if (m_daoForkBlock)
    {
        _peer->requestBlockHeaders(m_daoForkBlock, 1, 0, false);

        bytes const headerBytes = waitHeaderResponse(_peer);
        if (headerBytes.empty())
            return;

        RLP headerRlp(headerBytes);
        if (!verifyDaoChallengeResponse(headerRlp))
        {
            _peer->disable("Peer from another fork.");
            return;
        }
    }

    m_freePeers.push(_peer);

    if (!m_trieDownloadFiber)
        m_trieDownloadFiber.reset(
            new boost::fibers::fiber(&FastSync::downloadStateTrie, this, _peer->latestHash()));
}

bytes FastSync::waitHeaderResponse(std::weak_ptr<EthereumPeer> _peer)
{
    try
    {
        bytes const result = m_headers[_peer].get_future().get();
        m_headers.erase(_peer);
        return result;
    }
    catch (std::exception const& ex)
    {
        LOG(m_loggerDebug) << "Failed to get headers: " << ex.what();
        m_headers.erase(_peer);
    }
    return bytes{};
}

bool FastSync::verifyDaoChallengeResponse(RLP const& _r)
{
    if (_r.itemCount() != 1)
        return false;

    BlockHeader info(_r[0].data(), HeaderData);
    return info.number() == m_daoForkBlock &&
           info.extraData() == fromHex("0x64616f2d686172642d666f726b");
}

void FastSync::downloadStateTrie(h256 const& _blockHash)
{
    h256 const stateRoot = findStateRoot(_blockHash);

    LOG(m_loggerDebug) << "State Trie root: " << stateRoot;

    /*    bytes const manifestBytes = m_manifest.get_future().get();

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

        std::shared_ptr<WarpPeerCapability> peer;
        while (!peer)
            peer = m_freePeers.value_pop().lock();

        LOG(m_logger) << "Requesting chunk " << chunkHash;
        peer->requestData(chunkHash);

        m_requestedChunks[peer] = chunkHash;
        m_neededChunks.pop_front();
    }
*/
}

h256 FastSync::findStateRoot(h256 const& _blockHash)
{
    h256 stateRoot;
    while (!stateRoot)
    {
        std::shared_ptr<EthereumPeer> peer;
        while (!peer)
            peer = m_freePeers.value_pop().lock();

        LOG(m_loggerDebug) << "Requesting pivot block header " << _blockHash;
        peer->requestBlockHeaders(_blockHash, 1, 0, false);

        bytes const headerBytes = waitHeaderResponse(peer);
        if (headerBytes.empty())
            continue;

        try
        {
            RLP headersRlp(headerBytes);
            if (headersRlp.itemCount() != 1)
                continue;

            BlockHeader header(headersRlp[0].data(), HeaderData);
            stateRoot = header.stateRoot();
        }
        catch (Exception const& e)
        {
            LOG(m_loggerDebug) << "Couldn't get pivot state root: " << e.what();
        }
    }
    return stateRoot;
}

}  // namespace eth
}  // namespace dev
