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
#include "EthereumPeer.h"

#include <libethcore/BlockHeader.h>

using namespace std;
using namespace dev;
using namespace dev::eth;

namespace
{
unsigned const c_maxRequestHeaders = 1024;
}

FastSync::FastSync(h256 const& _genesisHash, vector<unsigned> const& _protocolVersions, u256 const& _networkId):
	m_genesisHash(_genesisHash),
	m_protocolVersions(_protocolVersions),
	m_networkId(_networkId)
{
}

void FastSync::onPeerStatus(shared_ptr<EthereumPeerFace> _peer)
{
	if (!_peer->validateStatus(m_genesisHash, m_protocolVersions, m_networkId))
		return;

	// TODO don't request header if it's already in block queue
	if (_peer->totalDifficulty() > m_maxDifficulty)
	{
		m_maxDifficulty = _peer->totalDifficulty();
		_peer->requestBlockHeaders(_peer->latestHash(), 1, 0, false);
	}
	else
		syncPeer(_peer);
}

void FastSync::syncPeer(std::shared_ptr<EthereumPeerFace> _peer)
{
	Guard guard(m_downloadingHeadersMutex);

	if (m_headersToDownload.empty())
		// nothing left to download
		// TODO add to the pool of free peers
		return;

	BlockNumberRange const nextRange = m_headersToDownload.lowestRange(c_maxRequestHeaders);
	assert(nextRange.second > nextRange.first);

	_peer->requestBlockHeaders(nextRange.first, rangeLength(nextRange), 0, false);

	m_headersToDownload -= nextRange;
	m_peersDownloadingHeaders[_peer] = nextRange;
}

void FastSync::onPeerBlockHeaders(std::shared_ptr<EthereumPeerFace> _peer, RLP const& _r)
{
	// TODO get highest block headers from several peers, average block height and select pivot point
	if (_r.itemCount() == 1)
		onHighestBlockHeaderDownloaded(_r[0].data());

	BlockNumberRange const expectedRange = downloadingRange(_peer);
	if (isEmptyRange(expectedRange))
	{
		// peer returned headers when not asked, EthereumPeer checks that, so we shoudln't get here
		clog(p2p::NetImpolite) << "Received not expected headers";
		_peer->addRating(-1);
	}

	pair<BlockNumberRangeMask, bool> const saveResult = saveDownloadedHeaders(_r, expectedRange);

	updateDownloadingHeaders(_peer, saveResult.first);

	if (!saveResult.second)
		_peer->addRating(-1);

	syncPeer(_peer);
}

void FastSync::onHighestBlockHeaderDownloaded(bytesConstRef _data)
{
	try
	{
		// expand chain to download when new highest block found
		BlockHeader const header(_data, HeaderData);

		extendHeadersToDownload(static_cast<unsigned>(header.number()));
	}
	catch (Exception const& e)
	{
		cwarn << "Exception during fast sync highest headers downloading: " << e.what();
	}
}

void FastSync::extendHeadersToDownload(unsigned _newMaxBlockNumber)
{
	Guard guard(m_downloadingHeadersMutex);

	unsigned const allEnd = m_headersToDownload.all().second;
	if (_newMaxBlockNumber >= allEnd)
	{
		m_headersToDownload.extendAll(_newMaxBlockNumber);
		m_headersToDownload.unionWith(make_pair(allEnd, _newMaxBlockNumber + 1));
	}
}


FastSync::BlockNumberRange FastSync::downloadingRange(std::shared_ptr<EthereumPeerFace> _peer) const
{
	Guard guard(m_downloadingHeadersMutex);

	auto itExpectedRange = m_peersDownloadingHeaders.find(_peer);
	if (itExpectedRange == m_peersDownloadingHeaders.end())
		// peer returned headers when not asked, EthereumPeer checks that, so we shoudln't get here
		return BlockNumberRange();

	return itExpectedRange->second;
}

pair<FastSync::BlockNumberRangeMask, bool> FastSync::saveDownloadedHeaders(RLP const& _r, BlockNumberRange const& expectedRange)
{
	bool politePeer = true;

	BlockNumberRangeMask downloadedRangeMask(allHeadersRange());
	try
	{
		for (RLP rlpHeader : _r)
		{
			BlockHeader const header(rlpHeader.data(), HeaderData);
			unsigned const blockNumber = static_cast<unsigned>(header.number());

			if (!isNumberInRange(expectedRange, blockNumber))
			{
				// peer giving us not expected headers
				clog(p2p::NetImpolite) << "Unexpected block header " << blockNumber;
				politePeer = false;
				break;
			}

			// TODO check also against the last block imported to database
			if (!checkDownloadedHash(blockNumber - 1, header.parentHash()))
			{
				// mismatching parent hash, don't add it and all the following
				clog(p2p::NetImpolite) << "Unknown parent hash for block header " << blockNumber << " " << header.hash();
				politePeer = false;
				break;
			}

			if (!checkDownloadedParentHash(blockNumber + 1, header.parentHash()))
				removeConsecutiveHeadersFromDownloaded(blockNumber + 1);

			saveDownloadedHeader(blockNumber, header.hash(), header.parentHash(), rlpHeader.data().toBytes());

			downloadedRangeMask.unionWith(blockNumber);
		}
	}
	catch (Exception const& e)
	{
		cwarn << "Exception during fast sync headers downloading: " << e.what();
	}

	return make_pair(downloadedRangeMask, politePeer);
}

FastSync::BlockNumberRange FastSync::allHeadersRange() const
{
	Guard guard(m_downloadingHeadersMutex);
	return m_headersToDownload.all();
}

bool FastSync::checkDownloadedHash(unsigned _blockNumber, h256 const& _hash) const
{
	auto const block = m_downloadedHeaders.find(_blockNumber);
	return block == m_downloadedHeaders.end() || _hash == block->second.hash;
}

bool FastSync::checkDownloadedParentHash(unsigned _blockNumber, h256 const& _hash) const
{
	auto const block = m_downloadedHeaders.find(_blockNumber);
	return block == m_downloadedHeaders.end() || _hash == block->second.parentHash;
}

void FastSync::removeConsecutiveHeadersFromDownloaded(unsigned _startNumber)
{
	unsigned prevNumber = 0;
	auto block = m_downloadedHeaders.find(_startNumber);

	while (block != m_downloadedHeaders.end() && (prevNumber == 0 || block->first == prevNumber + 1))
	{
		prevNumber = block->first;
		block = m_downloadedHeaders.erase(block);
	}
}

void FastSync::saveDownloadedHeader(unsigned _blockNumber, h256 const& _hash, h256 const& _parentHash, bytes&& _headerData)
{
	Header headerData{ _headerData, _hash, _parentHash };
	m_downloadedHeaders.emplace(_blockNumber, headerData);
}

void FastSync::updateDownloadingHeaders(std::shared_ptr<EthereumPeerFace> _peer, const BlockNumberRangeMask& _downloaded)
{
	Guard guard(m_downloadingHeadersMutex);

	auto itPeer = m_peersDownloadingHeaders.find(_peer);
	if (itPeer != m_peersDownloadingHeaders.end())
	{
		BlockNumberRange const rangeRequested = itPeer->second;
		m_peersDownloadingHeaders.erase(itPeer);

		BlockNumberRangeMask const requested = BlockNumberRangeMask(m_headersToDownload.all()).unionWith(rangeRequested);
		BlockNumberRangeMask const notReturned = requested - _downloaded;
		m_headersToDownload.unionWith(notReturned);
	}
}

void FastSync::onPeerRequestTimeout(std::shared_ptr<EthereumPeerFace> _peer, Asking _asking)
{
	assert(_asking == Asking::BlockHeaders);

	// mark all requested data as pending again
	BlockNumberRangeMask const emptyDownloaded(allHeadersRange());
	updateDownloadingHeaders(_peer, emptyDownloaded);
}
