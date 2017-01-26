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

#pragma once

#include <libdevcore/Common.h>
#include <libdevcore/Guards.h>
#include <libdevcore/RangeMask.h>
#include <libdevcore/RLP.h>

#include <memory>

namespace dev
{

namespace eth
{
class EthereumPeerFace;

class FastSync
{
public:
	FastSync();

	void onPeerStatus(std::shared_ptr<EthereumPeerFace> _peer);

	void onPeerBlockHeaders(std::shared_ptr<EthereumPeerFace> _peer, RLP const& _r);

private:
	using BlockNumberRangeMask = RangeMask<unsigned>;
	using BlockNumberRange = BlockNumberRangeMask::Range;

	void syncPeer(std::shared_ptr<EthereumPeerFace> _peer);
	void onHighestBlockHeaderDownloaded(bytesConstRef _data);
	void extendHeadersToDownload(unsigned _newMaxBlockNumber);
	BlockNumberRangeMask saveDownloadedHeaders(RLP const& _r);
	BlockNumberRange allHeadersRange() const;
	void saveDownloadedHeader(unsigned _blockNumber, bytes&& _headerData);
	void updateDownloadingHeaders(std::shared_ptr<EthereumPeerFace> _peer, const BlockNumberRangeMask& _downloaded);

private:
	u256 m_maxDifficulty;				///< Highest peer difficulty

	mutable Mutex m_downloadingHeadersMutex;
	BlockNumberRangeMask m_headersToDownload; ///< Set of pending block number ranges
	std::map<std::weak_ptr<EthereumPeerFace>, BlockNumberRange, std::owner_less<std::weak_ptr<EthereumPeerFace>>> m_peersDownloadingHeaders; ///< Peers to assigned block number range

	Mutex m_downloadedHeadersMutex;
	std::map<unsigned, bytes> m_downloadedHeaders; ///< Block numbers to downloaded header data
};

}

}