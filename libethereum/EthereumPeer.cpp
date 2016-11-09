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
/** @file EthereumPeer.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "EthereumPeer.h"

#include <chrono>
#include <libdevcore/Common.h>
#include <libethcore/Exceptions.h>
#include <libp2p/Session.h>
#include <libp2p/Host.h>
#include "BlockChain.h"
#include "EthereumHost.h"
#include "TransactionQueue.h"
#include "BlockQueue.h"
#include "BlockChainSync.h"

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace p2p;

static const unsigned c_maxIncomingNewHashes = 1024;
static const unsigned c_maxHeadersToSend = 1024;

static string toString(Asking _a)
{
	switch (_a)
	{
	case Asking::BlockHeaders: return "BlockHeaders";
	case Asking::BlockBodies: return "BlockBodies";
	case Asking::NodeData: return "NodeData";
	case Asking::Receipts: return "Receipts";
	case Asking::Nothing: return "Nothing";
	case Asking::State: return "State";
	}
	return "?";
}

EthereumPeer::EthereumPeer(std::shared_ptr<Session> _s, HostCapabilityFace* _h, unsigned _i, CapDesc const& _cap, uint16_t _capID):
	Capability(_s, _h, _i, _capID),
	m_peerCapabilityVersion(_cap.second)
{
	session()->addNote("manners", isRude() ? "RUDE" : "nice");
	m_syncHashNumber = host()->chain().number() + 1;
	requestStatus();
}

EthereumPeer::~EthereumPeer()
{
	if (m_asking != Asking::Nothing)
	{
		clog(NetAllDetail) << "Peer aborting while being asked for " << ::toString(m_asking);
		setRude();
	}
	abortSync();
}

bool EthereumPeer::isRude() const
{
	auto s = session();
	if (s)
		return repMan().isRude(*s, name());
	return false;
}

unsigned EthereumPeer::askOverride() const
{
	std::string static const badGeth = "Geth/v0.9.27";
	auto s = session();
	if (!s)
		return c_maxBlocksAsk;
	if (s->info().clientVersion.substr(0, badGeth.size()) == badGeth)
		return 1;
	bytes const& d = repMan().data(*s, name());
	return d.empty() ? c_maxBlocksAsk : RLP(d).toInt<unsigned>(RLP::LaissezFaire);
}

void EthereumPeer::setRude()
{
	auto s = session();
	if (!s)
		return;
	auto old = askOverride();
	repMan().setData(*s, name(), rlp(askOverride() / 2 + 1));
	cnote << "Rude behaviour; askOverride now" << askOverride() << ", was" << old;
	repMan().noteRude(*s, name());
	session()->addNote("manners", "RUDE");
}

void EthereumPeer::abortSync()
{
	host()->onPeerAborting();
}

EthereumHost* EthereumPeer::host() const
{
	return static_cast<EthereumHost*>(Capability::hostCapability());
}

/*
 * Possible asking/syncing states for two peers:
 */

void EthereumPeer::setIdle()
{
	setAsking(Asking::Nothing);
}

void EthereumPeer::requestStatus()
{
	assert(m_asking == Asking::Nothing);
	setAsking(Asking::State);
	m_requireTransactions = true;
	RLPStream s;
	bool latest = m_peerCapabilityVersion == host()->protocolVersion();
	prep(s, StatusPacket, 5)
					<< (latest ? host()->protocolVersion() : EthereumHost::c_oldProtocolVersion)
					<< host()->networkId()
					<< host()->chain().details().totalDifficulty
					<< host()->chain().currentHash()
					<< host()->chain().genesisHash();
	sealAndSend(s);
}

void EthereumPeer::requestBlockHeaders(unsigned _startNumber, unsigned _count, unsigned _skip, bool _reverse)
{
	if (m_asking != Asking::Nothing)
	{
		clog(NetWarn) << "Asking headers while requesting " << ::toString(m_asking);
	}
	setAsking(Asking::BlockHeaders);
	RLPStream s;
	prep(s, GetBlockHeadersPacket, 4) << _startNumber << _count << _skip << (_reverse ? 1 : 0);
	clog(NetMessageDetail) << "Requesting " << _count << " block headers starting from " << _startNumber << (_reverse ? " in reverse" : "");
	m_syncHashNumber = _startNumber;
	m_lastAskedHeaders = _count;
	sealAndSend(s);
}

void EthereumPeer::requestBlockHeaders(h256 const& _startHash, unsigned _count, unsigned _skip, bool _reverse)
{
	if (m_asking != Asking::Nothing)
	{
		clog(NetWarn) << "Asking headers while requesting " << ::toString(m_asking);
	}
	setAsking(Asking::BlockHeaders);
	RLPStream s;
	prep(s, GetBlockHeadersPacket, 4) << _startHash << _count << _skip << (_reverse ? 1 : 0);
	clog(NetMessageDetail) << "Requesting " << _count << " block headers starting from " << _startHash << (_reverse ? " in reverse" : "");
	m_syncHash = _startHash;
	m_lastAskedHeaders = _count;
	sealAndSend(s);
}


void EthereumPeer::requestBlockBodies(h256s const& _blocks)
{
	if (m_asking != Asking::Nothing)
	{
		clog(NetWarn) << "Asking headers while requesting " << ::toString(m_asking);
	}
	setAsking(Asking::BlockBodies);
	if (_blocks.size())
	{
		RLPStream s;
		prep(s, GetBlockBodiesPacket, _blocks.size());
		for (auto const& i: _blocks)
			s << i;
		sealAndSend(s);
	}
	else
		setIdle();
}

void EthereumPeer::setAsking(Asking _a)
{
	m_asking = _a;
	m_lastAsk = std::chrono::system_clock::to_time_t(chrono::system_clock::now());

	auto s = session();
	if (s)
	{
		s->addNote("ask", ::toString(_a));
		s->addNote("sync", string(isCriticalSyncing() ? "ONGOING" : "holding") + (needsSyncing() ? " & needed" : ""));
	}
}

void EthereumPeer::tick()
{
	auto s = session();
	time_t  now = std::chrono::system_clock::to_time_t(chrono::system_clock::now());
	if (s && (now - m_lastAsk > 10 && m_asking != Asking::Nothing))
		// timeout
		s->disconnect(PingTimeout);
}

bool EthereumPeer::isConversing() const
{
	return m_asking != Asking::Nothing;
}

bool EthereumPeer::isCriticalSyncing() const
{
	return m_asking == Asking::BlockHeaders || m_asking == Asking::State || (m_asking == Asking::BlockBodies && m_protocolVersion == 62);
}

bool EthereumPeer::interpret(unsigned _id, RLP const& _r)
{
	m_lastAsk = std::chrono::system_clock::to_time_t(chrono::system_clock::now());
	try
	{
	switch (_id)
	{
	case StatusPacket:
	{
		m_protocolVersion = _r[0].toInt<unsigned>();
		m_networkId = _r[1].toInt<u256>();
		m_totalDifficulty = _r[2].toInt<u256>();
		m_latestHash = _r[3].toHash<h256>();
		m_genesisHash = _r[4].toHash<h256>();
		if (m_peerCapabilityVersion == host()->protocolVersion())
			m_protocolVersion = host()->protocolVersion();

		clog(NetMessageSummary) << "Status:" << m_protocolVersion << "/" << m_networkId << "/" << m_genesisHash << ", TD:" << m_totalDifficulty << "=" << m_latestHash;
		setIdle();
		host()->onPeerStatus(dynamic_pointer_cast<EthereumPeer>(dynamic_pointer_cast<EthereumPeer>(shared_from_this())));
		break;
	}
	case TransactionsPacket:
	{
		host()->onPeerTransactions(dynamic_pointer_cast<EthereumPeer>(dynamic_pointer_cast<EthereumPeer>(shared_from_this())), _r);
		break;
	}
	case GetBlockHeadersPacket:
	{
		/// Packet layout:
		/// [ block: { P , B_32 }, maxHeaders: P, skip: P, reverse: P in { 0 , 1 } ]
		const auto blockId = _r[0];
		const auto maxHeaders = _r[1].toInt<u256>();
		const auto skip = _r[2].toInt<u256>();
		const auto reverse = _r[3].toInt<bool>();

		auto& bc = host()->chain();

		auto numHeadersToSend = maxHeaders <= c_maxHeadersToSend ? static_cast<unsigned>(maxHeaders) : c_maxHeadersToSend;

		if (skip > std::numeric_limits<unsigned>::max() - 1)
		{
			clog(NetAllDetail) << "Requested block skip is too big: " << skip;
			break;
		}

		auto step = static_cast<unsigned>(skip) + 1;
		assert(step > 0 && "step must not be 0");

		h256 blockHash;
		if (blockId.size() == 32) // block id is a hash
		{
			blockHash = blockId.toHash<h256>();
			//blockNumber = host()->chain().number(blockHash);
			clog(NetMessageSummary) << "GetBlockHeaders (block (hash): " << blockHash
			<< ", maxHeaders: " << maxHeaders
			<< ", skip: " << skip << ", reverse: " << reverse << ")";

			if (!reverse)
			{
				auto n = bc.number(blockHash);
				if (numHeadersToSend == 0)
					blockHash = {};
				else if (n != 0 || blockHash == bc.genesisHash())
				{
					auto top = n + uint64_t(step) * numHeadersToSend - 1;
					auto lastBlock = bc.number();
					if (top > lastBlock)
					{
						numHeadersToSend = (lastBlock - n) / step + 1;
						top = n + step * (numHeadersToSend - 1);
					}
					assert(top <= lastBlock && "invalid top block calculated");
					blockHash = bc.numberHash(static_cast<unsigned>(top)); // override start block hash with the hash of the top block we have
				}
				else
					blockHash = {};
			}
			else if (!bc.isKnown(blockHash))
				blockHash = {};
		}
		else // block id is a number
		{
			auto n = blockId.toInt<bigint>();
			clog(NetMessageSummary) << "GetBlockHeaders (" << n
			<< "max: " << maxHeaders
			<< "skip: " << skip << (reverse ? "reverse" : "") << ")";

			if (!reverse)
			{
				auto lastBlock = bc.number();
				if (n > lastBlock || numHeadersToSend == 0)
					blockHash = {};
				else
				{
					bigint top = n + uint64_t(step) * (numHeadersToSend - 1);
					if (top > lastBlock)
					{
						numHeadersToSend = (lastBlock - static_cast<unsigned>(n)) / step + 1;
						top = n + step * (numHeadersToSend - 1);
					}
					assert(top <= lastBlock && "invalid top block calculated");
					blockHash = bc.numberHash(static_cast<unsigned>(top)); // override start block hash with the hash of the top block we have
				}
			}
			else if (n <= std::numeric_limits<unsigned>::max())
				blockHash = bc.numberHash(static_cast<unsigned>(n));
			else
				blockHash = {};
		}

		auto nextHash = [&bc](h256 _h, unsigned _step)
		{
			static const unsigned c_blockNumberUsageLimit = 1000;

			const auto lastBlock = bc.number();
			const auto limitBlock = lastBlock > c_blockNumberUsageLimit ? lastBlock - c_blockNumberUsageLimit : 0; // find the number of the block below which we don't expect BC changes.

			while (_step) // parent hash traversal
			{
				auto details = bc.details(_h);
				if (details.number < limitBlock)
					break; // stop using parent hash traversal, fallback to using block numbers
				_h = details.parent;
				--_step;
			}

			if (_step) // still need lower block
			{
				auto n = bc.number(_h);
				if (n >= _step)
					_h = bc.numberHash(n - _step);
				else
					_h = {};
			}


			return _h;
		};

		bytes rlp;
		unsigned itemCount = 0;
		vector<h256> hashes;
		for (unsigned i = 0; i != numHeadersToSend; ++i)
		{
			if (!blockHash || !bc.isKnown(blockHash))
				break;

			hashes.push_back(blockHash);
			++itemCount;

			blockHash = nextHash(blockHash, step);
		}

		for (unsigned i = 0; i < hashes.size() && rlp.size() < c_maxPayload; ++i)
			rlp += bc.headerData(hashes[reverse ? i : hashes.size() - 1 - i]);

		RLPStream s;
		prep(s, BlockHeadersPacket, itemCount).appendRaw(rlp, itemCount);
		sealAndSend(s);
		addRating(0);
		break;
	}
	case BlockHeadersPacket:
	{
		if (m_asking != Asking::BlockHeaders)
			clog(NetImpolite) << "Peer giving us blocks when we didn't ask for them.";
		else
		{
			setIdle();
			host()->onPeerBlockHeaders(dynamic_pointer_cast<EthereumPeer>(shared_from_this()), _r);
		}
		break;
	}
	case GetBlockBodiesPacket:
	{
		unsigned count = static_cast<unsigned>(_r.itemCount());
		clog(NetMessageSummary) << "GetBlockBodies (" << dec << count << "entries)";

		if (!count)
		{
			clog(NetImpolite) << "Zero-entry GetBlockBodies: Not replying.";
			addRating(-10);
			break;
		}
		// return the requested blocks.
		bytes rlp;
		unsigned n = 0;
		auto numBodiesToSend = std::min(count, c_maxBlocks);
		for (unsigned i = 0; i < numBodiesToSend && rlp.size() < c_maxPayload; ++i)
		{
			auto h = _r[i].toHash<h256>();
			if (host()->chain().isKnown(h))
			{
				bytes blockBytes = host()->chain().block(h);
				RLP block{blockBytes};
				RLPStream body;
				body.appendList(2);
				body.appendRaw(block[1].data()); // transactions
				body.appendRaw(block[2].data()); // uncles
				auto bodyBytes = body.out();
				rlp.insert(rlp.end(), bodyBytes.begin(), bodyBytes.end());
				++n;
			}
		}
		if (count > 20 && n == 0)
			clog(NetWarn) << "all" << count << "unknown blocks requested; peer on different chain?";
		else
			clog(NetMessageSummary) << n << "blocks known and returned;" << (numBodiesToSend - n) << "blocks unknown;" << (count > c_maxBlocks ? count - c_maxBlocks : 0) << "blocks ignored";

		addRating(0);
		RLPStream s;
		prep(s, BlockBodiesPacket, n).appendRaw(rlp, n);
		sealAndSend(s);
		break;
	}
	case BlockBodiesPacket:
	{
		if (m_asking != Asking::BlockBodies)
			clog(NetImpolite) << "Peer giving us block bodies when we didn't ask for them.";
		else
		{
			setIdle();
			host()->onPeerBlockBodies(dynamic_pointer_cast<EthereumPeer>(shared_from_this()), _r);
		}
		break;
	}
	case NewBlockPacket:
	{
		host()->onPeerNewBlock(dynamic_pointer_cast<EthereumPeer>(shared_from_this()), _r);
		break;
	}
	case NewBlockHashesPacket:
	{
		unsigned itemCount = _r.itemCount();

		clog(NetMessageSummary) << "BlockHashes (" << dec << itemCount << "entries)" << (itemCount ? "" : ": NoMoreHashes");

		if (itemCount > c_maxIncomingNewHashes)
		{
			disable("Too many new hashes");
			break;
		}

		vector<pair<h256, u256>> hashes(itemCount);
		for (unsigned i = 0; i < itemCount; ++i)
			hashes[i] = std::make_pair(_r[i][0].toHash<h256>(), _r[i][1].toInt<u256>());

		host()->onPeerNewHashes(dynamic_pointer_cast<EthereumPeer>(shared_from_this()), hashes);
		break;
	}
	case GetNodeDataPacket:
	{
		unsigned count = static_cast<unsigned>(_r.itemCount());
		if (!count)
		{
			clog(NetImpolite) << "Zero-entry GetNodeData: Not replying.";
			addRating(-10);
			break;
		}
		clog(NetMessageSummary) << "GetNodeData (" << dec << count << " entries)";

		// return the requested nodes.
		strings data;
		unsigned n = 0;
		size_t payloadSize = 0;
		auto numItemsToSend = std::min(count, c_maxNodes);
		for (unsigned i = 0; i < numItemsToSend && payloadSize < c_maxPayload; ++i)
		{
			auto h = _r[i].toHash<h256>();
			auto node = host()->db().lookup(h);
			if (!node.empty())
			{
				payloadSize += node.length();
				data.push_back(move(node));
				++n;
			}
		}
		clog(NetMessageSummary) << n << " nodes known and returned;" << (numItemsToSend - n) << " unknown;" << (count > c_maxNodes ? count - c_maxNodes : 0) << " ignored";

		addRating(0);
		RLPStream s;
		prep(s, NodeDataPacket, n);
		for (auto const& element: data)
			s.append(element);
		sealAndSend(s);
		break;
	}
	case GetReceiptsPacket:
	{
		unsigned count = static_cast<unsigned>(_r.itemCount());
		if (!count)
		{
			clog(NetImpolite) << "Zero-entry GetReceipts: Not replying.";
			addRating(-10);
			break;
		}
		clog(NetMessageSummary) << "GetReceipts (" << dec << count << " entries)";

		// return the requested receipts.
		bytes rlp;
		unsigned n = 0;
		auto numItemsToSend = std::min(count, c_maxReceipts);
		for (unsigned i = 0; i < numItemsToSend && rlp.size() < c_maxPayload; ++i)
		{
			auto h = _r[i].toHash<h256>();
			if (host()->chain().isKnown(h))
			{
				auto const receipts = host()->chain().receipts(h);
				auto receiptsRlpList = receipts.rlp();
				rlp.insert(rlp.end(), receiptsRlpList.begin(), receiptsRlpList.end());
				++n;
			}
		}
		clog(NetMessageSummary) << n << " receipt lists known and returned;" << (numItemsToSend - n) << " unknown;" << (count > c_maxReceipts ? count - c_maxReceipts : 0) << " ignored";

		addRating(0);
		RLPStream s;
		prep(s, ReceiptsPacket, n).appendRaw(rlp, n);
		sealAndSend(s);
		break;
	}
	default:
		return false;
	}
	}
	catch (Exception const&)
	{
		clog(NetWarn) << "Peer causing an Exception:" << boost::current_exception_diagnostic_information() << _r;
	}
	catch (std::exception const& _e)
	{
		clog(NetWarn) << "Peer causing an exception:" << _e.what() << _r;
	}

	return true;
}
