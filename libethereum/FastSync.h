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
#include <libethereum/CommonNet.h>

#include <memory>
#include <vector>

namespace dev
{

namespace eth
{
class EthereumPeerFace;

class FastSync
{
public:
	FastSync(h256 const& _genesisHash, std::vector<unsigned> const& _protocolVersions, u256 const& _networkId);

	void onPeerStatus(std::shared_ptr<EthereumPeerFace> _peer);

	void onPeerBlockHeaders(std::shared_ptr<EthereumPeerFace> _peer, RLP const& _r);
	
	void onPeerRequestTimeout(std::shared_ptr<EthereumPeerFace> _peer, Asking _asking);
	
	// for tests only
	std::map<unsigned, bytes> downloadedHeaders() const
	{ 
		std::map<unsigned, bytes> result;
		std::transform(m_downloadedHeaders.begin(), m_downloadedHeaders.end(), std::inserter(result, result.end()),
			[](std::pair<unsigned const, Header> const& _p) { return std::make_pair(_p.first, _p.second.data); });

		return result;
	}

private:
	using BlockNumberRangeMask = RangeMask<unsigned>;
	using BlockNumberRange = BlockNumberRangeMask::Range;

	static unsigned rangeLength(BlockNumberRange const& _range) { return _range.second - _range.first; }
	static bool isEmptyRange(BlockNumberRange const& _range) { return _range.first == _range.second; }
	static bool isNumberInRange(BlockNumberRange const& _range, unsigned _number) { return _number >= _range.first && _number < _range.second; }

	void syncPeer(std::shared_ptr<EthereumPeerFace> _peer);
	void onHighestBlockHeaderDownloaded(bytesConstRef _data);
	void extendHeadersToDownload(unsigned _newMaxBlockNumber);
	/// @returns sucessfully saved header ranges and true if the response was correct
	BlockNumberRange downloadingRange(std::shared_ptr<EthereumPeerFace> _peer) const;
	std::pair<BlockNumberRangeMask, bool> saveDownloadedHeaders(RLP const& _r, BlockNumberRange const& expectedRange);
	BlockNumberRange allHeadersRange() const;
	bool checkDownloadedHash(unsigned _blockNumber, h256 const& _hash) const;
	bool checkDownloadedParentHash(unsigned _blockNumber, h256 const& _hash) const;
	void removeConsecutiveHeadersFromDownloaded(unsigned _startNumber);
	void saveDownloadedHeader(unsigned _blockNumber, h256 const& _hash, h256 const& _parentHash, bytes&& _headerData);
	void updateDownloadingHeaders(std::shared_ptr<EthereumPeerFace> _peer, const BlockNumberRangeMask& _downloaded);

private:
	struct Header
	{
		bytes data;
		h256 hash;
		h256 parentHash;
	};

	u256 const m_genesisHash;
	std::vector<unsigned> const m_protocolVersions;
	u256 const m_networkId;

	u256 m_maxDifficulty;				///< Highest peer difficulty

	mutable Mutex m_downloadingHeadersMutex;
	BlockNumberRangeMask m_headersToDownload; ///< Set of pending block number ranges
	std::map<std::weak_ptr<EthereumPeerFace>, BlockNumberRange, std::owner_less<std::weak_ptr<EthereumPeerFace>>> m_peersDownloadingHeaders; ///< Peers to assigned block number range

	Mutex m_downloadedHeadersMutex;
	std::map<unsigned, Header> m_downloadedHeaders; ///< Block numbers to downloaded header data
};

}

}