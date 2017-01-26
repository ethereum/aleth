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

FastSync::FastSync()
{
}

void FastSync::onPeerStatus(shared_ptr<EthereumPeerFace> _peer)
{
	// TODO validate genesisHash, protocolVersion, networkId etc.

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
	BlockNumberRange const nextRange = m_headersToDownload.lowestRange(c_maxRequestHeaders);
	if (nextRange.first == nextRange.second)
		// nothing left to download
		// TODO add to the pool of free peers
		return;

	_peer->requestBlockHeaders(nextRange.first, nextRange.second - nextRange.first, 0, false);

	m_headersToDownload -= nextRange;
	m_peersDownloadingHeaders[_peer] = nextRange;
}

void FastSync::onPeerBlockHeaders(std::shared_ptr<EthereumPeerFace> _peer, RLP const& _r)
{
	if (_r.itemCount() == 1)
		onHighestBlockHeaderDownloaded(_r[0].data());

	BlockNumberRangeMask const dowloadedRangeMask = saveDownloadedHeaders(_r);

	updateDownloadingHeaders(_peer, dowloadedRangeMask);

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

	m_headersToDownload.extendAll(_newMaxBlockNumber);
	m_headersToDownload.unionWith(make_pair(m_headersToDownload.lastIn(), _newMaxBlockNumber));
}


FastSync::BlockNumberRangeMask FastSync::saveDownloadedHeaders(RLP const& _r)
{
	BlockNumberRangeMask downloadedRangeMask(allHeadersRange());
	try
	{
		for (size_t i = 0; i < _r.itemCount(); ++i)
		{
			BlockHeader const header(_r[i].data(), HeaderData);
			unsigned const blockNumber = static_cast<unsigned>(header.number());

			// TODO validate parent hashes

			saveDownloadedHeader(blockNumber, _r[i].data().toBytes());
			downloadedRangeMask.unionWith(blockNumber);
		}
	}
	catch (Exception const& e)
	{
		cwarn << "Exception during fast sync headers downloading: " << e.what();
	}

	return downloadedRangeMask;
}

FastSync::BlockNumberRange FastSync::allHeadersRange() const
{
	Guard guard(m_downloadingHeadersMutex);
	return m_headersToDownload.all();
}

void FastSync::saveDownloadedHeader(unsigned _blockNumber, bytes&& _headerData)
{
	Guard guard(m_downloadedHeadersMutex);
	m_downloadedHeaders.emplace(_blockNumber, _headerData);
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
