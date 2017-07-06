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
/** @file SnapshotDownloader.cpp
 */

#include "BlockChain.h"
#include "SnapshotDownloader.h"
#include "WarpHostCapability.h"
#include "WarpPeerCapability.h"

#include <libdevcore/CommonIO.h>
#include <libp2p/Session.h>

#include <boost/filesystem.hpp>

using namespace std;
using namespace dev;
using namespace eth;
using namespace p2p;

struct SnapshotLog: public LogChannel
{
	static char const* name() { return "SNAP"; }
	static int const verbosity = 9;
};

SnapshotDownloader::SnapshotDownloader(WarpHostCapability& _host, BlockChain const& _blockChain, boost::filesystem::path const& _snapshotPath):
	m_host(_host), m_blockChain(_blockChain), m_snapshotPath(_snapshotPath)
{
}

void SnapshotDownloader::onPeerStatus(shared_ptr<WarpPeerCapability> _peer)
{
	if (_peer->validateStatus(m_blockChain.genesisHash(), { m_host.protocolVersion() }, m_host.networkId()))
		startDownload(_peer);
}

void SnapshotDownloader::onPeerManifest(shared_ptr<WarpPeerCapability> _peer, RLP const& _r)
{
	if (m_downloading)
		// TODO if this peer has the same snapshot, download from it, too
		return;

	if (!_r.isList() || _r.itemCount() != 1)
		return;

	RLP const manifest = _r[0];

	u256 version = manifest[0].toInt<u256>();
	if (version != 2)
		return;

	u256 const blockNumber = manifest[4].toInt<u256>();
	if (blockNumber != m_syncingSnapshotNumber)
		return;

	h256s const stateHashes = manifest[1].toVector<h256>();
	h256s const blockHashes = manifest[2].toVector<h256>();

	h256 const stateRoot = manifest[3].toHash<h256>();
	h256 const blockHash = manifest[5].toHash<h256>();

	clog(SnapshotLog) << "MANIFEST: "
		<< "version " << version
		<< "state root " << stateRoot
		<< "block number " << blockNumber
		<< "block hash " << blockHash;

	writeFile((m_snapshotPath / "MANIFEST").string(), manifest.data());

	m_stateChunksQueue.assign(stateHashes.begin(), stateHashes.end());
	m_blockChunksQueue.assign(blockHashes.begin(), blockHashes.end());
	m_downloading = true;
	downloadStateChunks(_peer);
}

void SnapshotDownloader::onPeerData(shared_ptr<WarpPeerCapability> _peer, RLP const& _r)
{
	if (!_r.isList() || _r.itemCount() != 1)
		return;

	// TODO handle timeouts

	RLP const data = _r[0];

	h256 const hash = sha3(data.toBytesConstRef());

	h256 const askedHash = m_stateChunksQueue.empty() ? m_blockChunksQueue.front() : m_stateChunksQueue.front();
	if (hash == askedHash)
	{

		// TODO handle writeFile failure
		writeFile((m_snapshotPath / toHex(hash)).string(), data.toBytesConstRef());

		clog(SnapshotLog) << "Saved chunk" << hash;

		if (m_stateChunksQueue.empty())
			m_blockChunksQueue.pop_front();
		else
			m_stateChunksQueue.pop_front();
		clog(SnapshotLog) << "State chunks left: " << m_stateChunksQueue.size() << " Block chunks left: " << m_blockChunksQueue.size();
	}

	downloadStateChunks(_peer);
}

void SnapshotDownloader::startDownload(shared_ptr<WarpPeerCapability> _peer)
{
	m_syncingSnapshotNumber = _peer->snapshotNumber();
	_peer->requestManifest();
}

void SnapshotDownloader::downloadStateChunks(shared_ptr<WarpPeerCapability> _peer)
{
	if (!m_stateChunksQueue.empty())
		_peer->requestData(m_stateChunksQueue.front());
	else
		downloadBlockChunks(_peer);
}

void SnapshotDownloader::downloadBlockChunks(shared_ptr<WarpPeerCapability> _peer)
{
	if (!m_blockChunksQueue.empty())
		_peer->requestData(m_blockChunksQueue.front());
	else
		clog(SnapshotLog) << "Snapshot download complete!";
}
