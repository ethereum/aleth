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
/** @file BlockChainSync.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "BlockChainSync.h"

#include <chrono>
#include <libdevcore/Common.h>
#include <libdevcore/TrieHash.h>
#include <libp2p/Host.h>
#include <libp2p/Session.h>
#include <libethcore/Exceptions.h>
#include <libethcore/Params.h>
#include "BlockChain.h"
#include "BlockQueue.h"
#include "EthereumPeer.h"
#include "EthereumHost.h"
#include "DownloadMan.h"

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace p2p;

unsigned const c_chainReorgSize = 30000; /// Added to estimated hashes to account for potential chain reorganiation
unsigned const c_maxPeerUknownNewBlocks = 1024; /// Max number of unknown new blocks peer can give us
unsigned const c_subChainSize = 8192; /// PV62 subchain size
unsigned const c_subChainHeaders = 20; /// Number of subchain headers to request at once

std::ostream& dev::eth::operator<<(std::ostream& _out, SyncStatus const& _sync)
{
	_out << "protocol: " << _sync.protocolVersion << endl;
	_out << "state: " << EthereumHost::stateName(_sync.state) << " ";
	if (_sync.state == SyncState::Blocks || _sync.state == SyncState::NewBlocks)
		_out << _sync.currentBlockNumber << "/" << _sync.highestBlockNumber;
	return _out;
}

BlockChainSync::BlockChainSync(EthereumHost& _host):
	m_host(_host),
	m_startingBlock(_host.chain().number())
{
	m_bqRoomAvailable = host().bq().onRoomAvailable([this]()
	{
		RecursiveGuard l(x_sync);
		continueSync();
	});
}

BlockChainSync::~BlockChainSync()
{
	RecursiveGuard l(x_sync);
	abortSync();
}

void BlockChainSync::abortSync()
{
	resetSync();
	host().foreachPeer([&](std::shared_ptr<EthereumPeer> _p)
	{
		_p->abortSync();
		return true;
	});

}

void BlockChainSync::onPeerStatus(std::shared_ptr<EthereumPeer> _peer)
{
	RecursiveGuard l(x_sync);
	DEV_INVARIANT_CHECK;
	std::shared_ptr<Session> session = _peer->session();
	if (!session)
		return; // Expired
	if (_peer->m_genesisHash != host().chain().genesisHash())
		_peer->disable("Invalid genesis hash");
	else if (_peer->m_protocolVersion != host().protocolVersion() && _peer->m_protocolVersion != EthereumHost::c_oldProtocolVersion)
		_peer->disable("Invalid protocol version.");
	else if (_peer->m_networkId != host().networkId())
		_peer->disable("Invalid network identifier.");
	else if (session->info().clientVersion.find("/v0.7.0/") != string::npos)
		_peer->disable("Blacklisted client version.");
	else if (host().isBanned(session->id()))
		_peer->disable("Peer banned for previous bad behaviour.");
	else if (_peer->m_asking != Asking::State && _peer->m_asking != Asking::Nothing)
		_peer->disable("Peer banned for unexpected status message.");
	else
		syncPeer(_peer);
}



void BlockChainSync::syncPeer(std::shared_ptr<EthereumPeer> _peer)
{
	if (_peer->m_asking != Asking::Nothing)
	{
		clog(NetAllDetail) << "Can't sync with this peer - outstanding asks.";
		return;
	}

	u256 td = host().chain().details().totalDifficulty;
	if (host().bq().isActive())
		td += host().bq().difficulty();

	u256 syncingDifficulty = std::max(m_syncingTotalDifficulty, td);


	if (_peer->m_totalDifficulty > syncingDifficulty && _peer->m_height == 0)
	{
		// start sync
		m_syncingTotalDifficulty = _peer->m_totalDifficulty;
		if (m_state == SyncState::Idle)
			m_state = SyncState::Blocks;
		_peer->requestBlockHeaders(_peer->m_latestHash, 1, 0, false);
		_peer->m_requireTransactions = true;
		return;
	}

	if (m_state == SyncState::Blocks)
	{
		requestBlocks(_peer);
		return;
	}
}

void BlockChainSync::continueSync()
{
	host().foreachPeer([this](std::shared_ptr<EthereumPeer> _p)
	{
		syncPeer(_p);
		return true;
	});
}

void BlockChainSync::requestBlocks(std::shared_ptr<EthereumPeer> _peer)
{
	if (host().bq().knownFull())
	{
		clog(NetAllDetail) << "Waiting for block queue before downloading blocks";
		pauseSync();
		return;
	}
	// check to see if we need to download any block bodies first
	auto header = m_headers.begin();
	h256s neededBodies;
	vector<unsigned> neededNumbers;
	unsigned index = 0;
	while (header != m_headers.end() && neededBodies.size() < 1024 && index < header->second.size())
	{
		unsigned block = header->first + index;
		if (m_downloadingBodies.count(block) == 0)
		{
			neededBodies.push_back(header->second[index].hash);
			neededNumbers.push_back(block);
			m_downloadingBodies.insert(block);
		}

		++index;
		if (index >= header->second.size())
		{
			index = 0;
			++header;
		}
	}
	if (neededBodies.size() > 0)
	{
		m_bodySyncPeers[_peer] = neededNumbers;
		_peer->requestBlockBodies(neededBodies);
	}
	else
	{
		// check if need to download headers
		if (!m_haveCommonHeader)
		{
			// download backwards until common block is found 1 header at a time
			unsigned block = m_host.chain().number();
			if (!m_headers.empty() != 0)
				block = std::min(block, m_headers.begin()->first - 1);

			if (block <= 1)
				m_haveCommonHeader = true; //reached genesis
		}
		if (m_haveCommonHeader)
		{
			unsigned start = m_lastImportedBlock + 1;
			auto next = m_headers.begin();
			unsigned count = 0;
			while (count == 0)
			{
				count = 1024;
				if (next != m_headers.end())
				{
					if (next->first > start)
						count = std::min(count, next->first - start);
					else
						count = 0;
				}
				++next;
				if (count == 0)
					start = next->first + next->second.size();
				else
				{
					while(count > 0 && m_downloadingHeaders.count(start) != 0)
					{
						start++;
						count--;
					}
					std::vector<unsigned> headers;
					for (unsigned block = start; block < start + count; block++)
						if (m_downloadingHeaders.count(block) == 0)
							headers.push_back(block);
					count = headers.size();
					if (count > 0)
					{
						m_headerSyncPeers[_peer] = headers;
						_peer->requestBlockHeaders(start, count, 0, false);
					}
				}
			}
		}
	}
}


void BlockChainSync::clearPeerDownload(std::shared_ptr<EthereumPeer> _peer)
{
	auto syncPeer = m_headerSyncPeers.find(_peer);
	if (syncPeer != m_headerSyncPeers.end())
	{
		for (unsigned block : syncPeer->second)
			m_downloadingHeaders.erase(block);
		m_headerSyncPeers.erase(syncPeer);
	}
	syncPeer = m_bodySyncPeers.find(_peer);
	if (syncPeer != m_bodySyncPeers.end())
	{
		for (unsigned block : syncPeer->second)
			m_downloadingBodies.erase(block);
		m_bodySyncPeers.erase(syncPeer);
	}
}

void BlockChainSync::clearPeerDownload()
{
	for (auto s = m_headerSyncPeers.begin(); s != m_headerSyncPeers.end();)
	{
		if (s->first.expired())
		{
			for (unsigned block : s->second)
				m_downloadingHeaders.erase(block);
			m_headerSyncPeers.erase(s++);
		}
		else
			++s;
	}
	for (auto s = m_bodySyncPeers.begin(); s != m_bodySyncPeers.end();)
	{
		if (s->first.expired())
		{
			for (unsigned block : s->second)
				m_downloadingBodies.erase(block);
			m_bodySyncPeers.erase(s++);
		}
		else
			++s;
	}
}

void BlockChainSync::logNewBlock(h256 const& _h)
{
	if (m_state == SyncState::NewBlocks)
		clog(NetNote) << "NewBlock: " << _h;
	m_knownNewHashes.erase(_h);
}

template<typename T> void mergeInto(std::map<unsigned, std::vector<T>>& _container, unsigned _number, T&& _data)
{
		auto lower = _container.lower_bound(_number);
		if (lower != _container.end() && (lower->first + lower->second.size() <= _number))
		{
			// extend existing chunk
			assert(lower->first + lower->second.size() == _number);
			lower->second.emplace_back(_data);

			auto next = lower++;
			if (next != _container.end() && (lower->first + lower->second.size() == next->first))
			{
				// merge with the next chunk
				std::move(lower->second.begin(), lower->second.end(), std::back_inserter(next->second));
				_container.erase(next);
			}

		}
		else
			// insert a new chunk
			_container.insert(lower, std::make_pair(_number, std::vector<T> { _data }));
}

void BlockChainSync::onPeerBlockHeaders(std::shared_ptr<EthereumPeer> _peer, RLP const& _r)
{
	RecursiveGuard l(x_sync);
	size_t itemCount = _r.itemCount();
	clog(NetMessageSummary) << "BlocksHeaders (" << dec << itemCount << "entries)" << (itemCount ? "" : ": NoMoreHeaders");
	clearPeerDownload(_peer);
	if (m_state != SyncState::Blocks && m_state != SyncState::NewBlocks && m_state != SyncState::Waiting) {
		clog(NetMessageSummary) << "Ignoring unexpected blocks";
		return;
	}
	if (m_state == SyncState::Waiting)
	{
		clog(NetAllDetail) << "Ignored blocks while waiting";
		return;
	}
	for (unsigned i = 0; i < itemCount; i++)
	{
		BlockInfo info(_r[i].data(), CheckEverything, h256(), HeaderData);
		unsigned blockNumber = static_cast<unsigned>(info.number());
		if (blockNumber > m_highestBlock)
			m_highestBlock = blockNumber;

		if (host().chain().isKnown(info.hash()))
		{
			m_haveCommonHeader = true;
			m_lastImportedBlock = (unsigned)info.number();
		}
		else
		{
			mergeInto(m_headers, blockNumber, Header { _r[i].toBytes(), info.hash() });
			HeaderId id { info.transactionsRoot(), info.sha3Uncles() };
			if (id.transactionsRoot == EmptyTrie && id.uncles ==  EmptyListSHA3)
			{
				//empty body, just mark as downloaded
				RLPStream r(2);
				bytes body;
				r.swapOut(body);
				mergeInto(m_bodies, blockNumber, std::move(body));
			}
			else
			{
				m_headerIdToNumber[id] = blockNumber;
			}
		}
	}
	collectBlocks();
	continueSync();
}

void BlockChainSync::onPeerBlockBodies(std::shared_ptr<EthereumPeer> _peer, RLP const& _r)
{
	RecursiveGuard l(x_sync);
	size_t itemCount = _r.itemCount();
	clog(NetMessageSummary) << "BlocksBodies (" << dec << itemCount << "entries)" << (itemCount ? "" : ": NoMoreHeaders");
	clearPeerDownload(_peer);
	if (m_state != SyncState::Blocks && m_state != SyncState::NewBlocks && m_state != SyncState::Waiting) {
		clog(NetMessageSummary) << "Ignoring unexpected blocks";
		return;
	}
	if (m_state == SyncState::Waiting)
	{
		clog(NetAllDetail) << "Ignored blocks while waiting";
		return;
	}
	for (unsigned i = 0; i < itemCount; i++)
	{
		RLP body(_r[i]);

		auto txList = body[0];
		h256 transactionRoot = trieRootOver(txList.itemCount(), [&](unsigned i){ return rlp(i); }, [&](unsigned i){ return txList[i].data().toBytes(); });
		h256 uncles = sha3(body[1].data());
		HeaderId id { transactionRoot, uncles };
		auto iter = m_headerIdToNumber.find(id);
		if (iter == m_headerIdToNumber.end())
		{
			clog(NetAllDetail) << "Ignored unknown block body";
			continue;
		}
		m_headerIdToNumber.erase(id);
		unsigned blockNumber = iter->second;
		mergeInto(m_bodies, blockNumber, body.toBytes());
	}
	collectBlocks();
	continueSync();
}

void BlockChainSync::collectBlocks()
{
	if (m_headers.empty() || m_bodies.empty())
		return;

	// merge headers and bodies
	auto& headers = *m_headers.begin();
	auto& bodies = *m_bodies.begin();
	if (headers.first != bodies.first)
		return;

	unsigned success = 0;
	unsigned future = 0;
	unsigned got = 0;
	unsigned unknown = 0;
	size_t i = 0;
	for (; i < headers.second.size() && i < bodies.second.size(); i++)
	{
		RLPStream blockStream(3);
		blockStream.appendRaw(headers.second[i].data);
		blockStream.appendRaw(bodies.second[i]);
		bytes block;
		blockStream.swapOut(block);
		switch (host().bq().import(&block))
		{
		case ImportResult::Success:
			success++;
			//logNewBlock(h);
			break;

		case ImportResult::Malformed:
		case ImportResult::BadChain:
			//logNewBlock(h);
			//_peer->disable("Malformed block received."); TODO: track block origin
			restartSync();
			return;

		case ImportResult::FutureTimeKnown:
			//logNewBlock(h);
			future++;
			break;
		case ImportResult::AlreadyInChain:
		case ImportResult::AlreadyKnown:
			got++;
			break;

		case ImportResult::FutureTimeUnknown:
			future++; //Fall through

		case ImportResult::UnknownParent:
		{
			unknown++;
			//logNewBlock(h);
			break;
		}

		default:;
		}
	}

	clog(NetMessageSummary) << dec << success << "imported OK," << unknown << "with unknown parents," << future << "with future timestamps," << got << " already known received.";

	if (host().bq().unknownFull())
	{
		clog(NetWarn) << "Too many unknown blocks, restarting sync";
		restartSync();
		return;
	}

	headers.second.erase(headers.second.begin(), headers.second.begin() + i);
	bodies.second.erase(bodies.second.begin(), bodies.second.begin() + i);
	if (headers.second.empty())
		m_headers.erase(m_headers.begin());
	if (bodies.second.empty())
		m_bodies.erase(m_bodies.begin());

	if (m_headers.empty())
	{
		assert(m_bodies.empty());
		completeSync();
	}
	DEV_INVARIANT_CHECK_HERE;
}

void BlockChainSync::onPeerNewBlock(std::shared_ptr<EthereumPeer> _peer, RLP const& _r)
{
	RecursiveGuard l(x_sync);
	DEV_INVARIANT_CHECK;
	auto h = BlockInfo::headerHashFromBlock(_r[0].data());

	if (_r.itemCount() != 2)
		_peer->disable("NewBlock without 2 data fields.");
	else
	{
		switch (host().bq().import(_r[0].data()))
		{
		case ImportResult::Success:
			_peer->addRating(100);
			logNewBlock(h);
			break;
		case ImportResult::FutureTimeKnown:
			//TODO: Rating dependent on how far in future it is.
			break;

		case ImportResult::Malformed:
		case ImportResult::BadChain:
			logNewBlock(h);
			_peer->disable("Malformed block received.");
			return;

		case ImportResult::AlreadyInChain:
		case ImportResult::AlreadyKnown:
			break;

		case ImportResult::FutureTimeUnknown:
		case ImportResult::UnknownParent:
		{
			_peer->m_unknownNewBlocks++;
			if (_peer->m_unknownNewBlocks > c_maxPeerUknownNewBlocks)
			{
				_peer->disable("Too many uknown new blocks");
				if (m_state == SyncState::Idle)
					host().bq().clear();
			}
			logNewBlock(h);
			u256 totalDifficulty = _r[1].toInt<u256>();
			if (totalDifficulty > _peer->m_totalDifficulty)
			{
				clog(NetMessageDetail) << "Received block with no known parent. Peer needs syncing...";
				syncPeer(_peer);
			}
			break;
		}
		default:;
		}

		DEV_GUARDED(_peer->x_knownBlocks)
			_peer->m_knownBlocks.insert(h);
	}
}

SyncStatus BlockChainSync::status() const
{
	RecursiveGuard l(x_sync);
	SyncStatus res;
	res.state = m_state;
	res.protocolVersion = 62;
	res.startBlockNumber = m_startingBlock;
	res.currentBlockNumber = host().chain().number();
	res.highestBlockNumber = m_highestBlock;
	return res;
}

void BlockChainSync::resetSync()
{
	m_haveCommonHeader = false;
	m_lastImportedBlock = 0;
	m_startingBlock = 0;
	m_highestBlock = 0;
	m_chainPeer.reset();
	m_downloadingHeaders.clear();
	m_downloadingBodies.clear();
	m_headers.clear();
	m_bodies.clear();
	m_headerSyncPeers.clear();
	m_bodySyncPeers.clear();
	m_headerIdToNumber.clear();
	m_syncingTotalDifficulty = 0;
	m_state = SyncState::Idle;
}

void BlockChainSync::restartSync()
{
	resetSync();
	host().bq().clear();
	m_startingBlock = host().chain().number();
	m_state = SyncState::NotSynced;
}

void BlockChainSync::completeSync()
{
	resetSync();
	m_state = SyncState::Idle;
}

void BlockChainSync::pauseSync()
{
	m_state = SyncState::Waiting;
}

bool BlockChainSync::isSyncing() const
{
	return m_state != SyncState::Idle;
}

void BlockChainSync::onPeerNewHashes(std::shared_ptr<EthereumPeer> _peer, std::vector<std::pair<h256, u256>> const& _hashes)
{
	RecursiveGuard l(x_sync);
	DEV_INVARIANT_CHECK;
	if (isSyncing() && (m_state != SyncState::NewBlocks || _peer->isConversing()))
	{
		clog(NetMessageDetail) << "Ignoring new hashes since we're already downloading.";
		return;
	}
	clog(NetMessageDetail) << "Not syncing and new block hash discovered: syncing.";
	unsigned knowns = 0;
	unsigned unknowns = 0;
	unsigned maxHeight = 0;
	for (auto const& p: _hashes)
	{
		h256 const& h = p.first;
		_peer->addRating(1);
		DEV_GUARDED(_peer->x_knownBlocks)
			_peer->m_knownBlocks.insert(h);
		auto status = host().bq().blockStatus(h);
		if (status == QueueStatus::Importing || status == QueueStatus::Ready || host().chain().isKnown(h))
			knowns++;
		else if (status == QueueStatus::Bad)
		{
			cwarn << "block hash bad!" << h << ". Bailing...";
			return;
		}
		else if (status == QueueStatus::Unknown)
		{
			unknowns++;
			if (p.second > maxHeight)
			{
				maxHeight = (unsigned)p.second;
				_peer->m_latestHash = h;
			}
		}
		else
			knowns++;
	}
	clog(NetMessageSummary) << knowns << "knowns," << unknowns << "unknowns";
	if (unknowns > 0)
	{
		if (m_state == SyncState::NewBlocks)
		{
			clog(NetMessageDetail) << "Downloading new blocks and seeing new hashes. Trying grabbing blocks";
			syncPeer(_peer);
		}
		else
		{
			clog(NetMessageDetail) << "Not syncing and new block hash discovered: syncing without help.";
			syncPeer(_peer);
		}
		resetSync();
	}
}

void BlockChainSync::onPeerAborting()
{
	RecursiveGuard l(x_sync);
	// Can't check invariants here since the peers is already removed from the list and the state is not updated yet.
	clearPeerDownload();
	continueSync();
	DEV_INVARIANT_CHECK_HERE;
}

bool BlockChainSync::invariants() const
{
	return true;
}
