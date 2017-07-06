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
/** @file SnapshotDownloader.h
 */

#pragma once

#include <memory>
#include <deque>

namespace dev
{

namespace eth
{

class BlockChain;
class WarpHostCapability;
class WarpPeerCapability;

class SnapshotDownloader
{
public:
	SnapshotDownloader(WarpHostCapability& _host, BlockChain const& _blockChain, boost::filesystem::path const& _snapshotPath);

	void onPeerStatus(std::shared_ptr<WarpPeerCapability> _peer);

	void onPeerManifest(std::shared_ptr<WarpPeerCapability> _peer, RLP const& _r);

	void onPeerData(std::shared_ptr<WarpPeerCapability> _peer, RLP const& _r);

private:
	void startDownload(std::shared_ptr<WarpPeerCapability> _peer);
	void downloadStateChunks(std::shared_ptr<WarpPeerCapability> _peer);
	void downloadBlockChunks(std::shared_ptr<WarpPeerCapability> _peer);

	WarpHostCapability& m_host;
	BlockChain const& m_blockChain;

	u256 m_syncingSnapshotNumber;
	bool m_downloading = false;

	std::deque<h256> m_stateChunksQueue;
	std::deque<h256> m_blockChunksQueue;

	boost::filesystem::path const m_snapshotPath;
};

}
}
