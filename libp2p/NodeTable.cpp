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
/** @file NodeTable.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 */

#include "NodeTable.h"
using namespace std;
using namespace dev;
using namespace dev::p2p;

NodeEntry::NodeEntry(Node _src, Public _pubk, NodeIPEndpoint _gw): Node(_pubk, _gw), distance(NodeTable::distance(_src.id,_pubk)) {}
NodeEntry::NodeEntry(Node _src, Public _pubk, bi::udp::endpoint _udp): Node(_pubk, NodeIPEndpoint(_udp)), distance(NodeTable::distance(_src.id,_pubk)) {}

NodeTable::NodeTable(ba::io_service& _io, KeyPair _alias, bi::address const& _udpAddress, uint16_t _udp):
	m_node(Node(_alias.pub(), bi::udp::endpoint(_udpAddress, _udp))),
	m_secret(_alias.sec()),
	m_io(_io),
	m_socket(new NodeSocket(m_io, *this, m_node.endpoint.udp)),
	m_socketPointer(m_socket.get()),
	m_bucketRefreshTimer(m_io),
	m_evictionCheckTimer(m_io)
{
	for (unsigned i = 0; i < s_bins; i++)
	{
		m_state[i].distance = i;
		m_state[i].modified = chrono::steady_clock::now() - chrono::seconds(1);
	}
	
	m_socketPointer->connect();
	doRefreshBuckets(boost::system::error_code());
}
	
NodeTable::~NodeTable()
{
	// Cancel scheduled tasks to ensure.
	m_evictionCheckTimer.cancel();
	m_bucketRefreshTimer.cancel();
	
	// Disconnect socket so that deallocation is safe.
	m_socketPointer->disconnect();
}

void NodeTable::processEvents()
{
	if (m_nodeEventHandler)
		m_nodeEventHandler->processEvents();
}

shared_ptr<NodeEntry> NodeTable::addNode(Public const& _pubk, bi::udp::endpoint const& _udp, bi::tcp::endpoint const& _tcp)
{
	auto node = Node(_pubk, NodeIPEndpoint(_udp, _tcp));
	return addNode(node);
}

shared_ptr<NodeEntry> NodeTable::addNode(Node const& _node)
{
	// re-enable tcp checks when NAT hosts are handled by discover
	// we handle when tcp endpoint is 0 below
	if (_node.endpoint.udp.address().to_string() == "0.0.0.0")
	{
		clog(NodeTableWarn) << "addNode Failed. Invalid UDP address 0.0.0.0 for" << _node.id.abridged();
		return move(shared_ptr<NodeEntry>());
	}
	
	// ping address to recover nodeid if nodeid is empty
	if (!_node.id)
	{
		clog(NodeTableConnect) << "Sending public key discovery Ping to" << _node.endpoint.udp << "(Advertising:" << m_node.endpoint.udp << ")";
		{
			Guard l(x_pubkDiscoverPings);
			m_pubkDiscoverPings[_node.endpoint.udp.address()] = std::chrono::steady_clock::now();
		}
		PingNode p(_node.endpoint.udp, m_node.endpoint.udp.address().to_string(), m_node.endpoint.udp.port());
		p.sign(m_secret);
		m_socketPointer->send(p);
		return move(shared_ptr<NodeEntry>());
	}
	
	{
		Guard ln(x_nodes);
		if (m_nodes.count(_node.id))
			return m_nodes[_node.id];
	}
	
	shared_ptr<NodeEntry> ret(new NodeEntry(m_node, _node.id, NodeIPEndpoint(_node.endpoint.udp, _node.endpoint.tcp)));
	m_nodes[_node.id] = ret;
	ret->cullEndpoint();
	clog(NodeTableConnect) << "addNode pending for" << m_node.endpoint.udp << m_node.endpoint.tcp;
	PingNode p(_node.endpoint.udp, m_node.endpoint.udp.address().to_string(), m_node.endpoint.udp.port());
	p.sign(m_secret);
	m_socketPointer->send(p);
	return ret;
}

void NodeTable::discover()
{
	static chrono::steady_clock::time_point s_lastDiscover = chrono::steady_clock::now() - std::chrono::seconds(30);
	if (chrono::steady_clock::now() > s_lastDiscover + std::chrono::seconds(30))
	{
		s_lastDiscover = chrono::steady_clock::now();
		discover(m_node.id);
	}
}

list<NodeId> NodeTable::nodes() const
{
	list<NodeId> nodes;
	Guard l(x_nodes);
	for (auto& i: m_nodes)
		nodes.push_back(i.second->id);
	return move(nodes);
}

list<NodeEntry> NodeTable::snapshot() const
{
	list<NodeEntry> ret;
	Guard l(x_state);
	for (auto s: m_state)
		for (auto n: s.nodes)
			ret.push_back(*n.lock());
	return move(ret);
}

Node NodeTable::node(NodeId const& _id)
{
	Guard l(x_nodes);
	if (m_nodes.count(_id))
	{
		auto entry = m_nodes[_id];
		Node n(_id, NodeIPEndpoint(entry->endpoint.udp, entry->endpoint.tcp), entry->required);
		return move(n);
	}
	return move(Node());
}

shared_ptr<NodeEntry> NodeTable::nodeEntry(NodeId _id)
{
	Guard l(x_nodes);
	return m_nodes.count(_id) ? m_nodes[_id] : shared_ptr<NodeEntry>();
}

void NodeTable::discover(NodeId _node, unsigned _round, shared_ptr<set<shared_ptr<NodeEntry>>> _tried)
{
	if (!m_socketPointer->isOpen() || _round == s_maxSteps)
		return;
	
	if (_round == s_maxSteps)
	{
		clog(NodeTableEvent) << "Terminating discover after " << _round << " rounds.";
		return;
	}
	else if(!_round && !_tried)
		// initialized _tried on first round
		_tried.reset(new set<shared_ptr<NodeEntry>>());
	
	auto nearest = nearestNodeEntries(_node);
	list<shared_ptr<NodeEntry>> tried;
	for (unsigned i = 0; i < nearest.size() && tried.size() < s_alpha; i++)
		if (!_tried->count(nearest[i]))
		{
			auto r = nearest[i];
			tried.push_back(r);
			FindNode p(r->endpoint.udp, _node);
			p.sign(m_secret);
			m_socketPointer->send(p);
		}
	
	if (tried.empty())
	{
		clog(NodeTableEvent) << "Terminating discover after " << _round << " rounds.";
		return;
	}
		
	while (!tried.empty())
	{
		_tried->insert(tried.front());
		tried.pop_front();
	}
	
	auto self(shared_from_this());
	m_evictionCheckTimer.expires_from_now(boost::posix_time::milliseconds(c_reqTimeout.count() * 2));
	m_evictionCheckTimer.async_wait([this, self, _node, _round, _tried](boost::system::error_code const& _ec)
	{
		if (_ec)
			return;
		discover(_node, _round + 1, _tried);
	});
}

vector<shared_ptr<NodeEntry>> NodeTable::nearestNodeEntries(NodeId _target)
{
	// send s_alpha FindNode packets to nodes we know, closest to target
	static unsigned lastBin = s_bins - 1;
	unsigned head = distance(m_node.id, _target);
	unsigned tail = head == 0 ? lastBin : (head - 1) % s_bins;
	
	map<unsigned, list<shared_ptr<NodeEntry>>> found;
	unsigned count = 0;
	
	// if d is 0, then we roll look forward, if last, we reverse, else, spread from d
	if (head > 1 && tail != lastBin)
		while (head != tail && head < s_bins && count < s_bucketSize)
		{
			Guard l(x_state);
			for (auto n: m_state[head].nodes)
				if (auto p = n.lock())
				{
					if (count < s_bucketSize)
						found[distance(_target, p->id)].push_back(p);
					else
						break;
				}
			
			if (count < s_bucketSize && tail)
				for (auto n: m_state[tail].nodes)
					if (auto p = n.lock())
					{
						if (count < s_bucketSize)
							found[distance(_target, p->id)].push_back(p);
						else
							break;
					}

			head++;
			if (tail)
				tail--;
		}
	else if (head < 2)
		while (head < s_bins && count < s_bucketSize)
		{
			Guard l(x_state);
			for (auto n: m_state[head].nodes)
				if (auto p = n.lock())
				{
					if (count < s_bucketSize)
						found[distance(_target, p->id)].push_back(p);
					else
						break;
				}
			head++;
		}
	else
		while (tail > 0 && count < s_bucketSize)
		{
			Guard l(x_state);
			for (auto n: m_state[tail].nodes)
				if (auto p = n.lock())
				{
					if (count < s_bucketSize)
						found[distance(_target, p->id)].push_back(p);
					else
						break;
				}
			tail--;
		}
	
	vector<shared_ptr<NodeEntry>> ret;
	for (auto& nodes: found)
		for (auto n: nodes.second)
			ret.push_back(n);
	return move(ret);
}

void NodeTable::ping(bi::udp::endpoint _to) const
{
	PingNode p(_to, m_node.endpoint.udp.address().to_string(), m_node.endpoint.udp.port());
	p.sign(m_secret);
	m_socketPointer->send(p);
}

void NodeTable::ping(NodeEntry* _n) const
{
	if (_n)
		ping(_n->endpoint.udp);
}

void NodeTable::evict(shared_ptr<NodeEntry> _leastSeen, shared_ptr<NodeEntry> _new)
{
	if (!m_socketPointer->isOpen())
		return;
	
	{
		Guard l(x_evictions);
		m_evictions.push_back(EvictionTimeout(make_pair(_leastSeen->id,chrono::steady_clock::now()), _new->id));
		if (m_evictions.size() == 1)
			doCheckEvictions(boost::system::error_code());
	}
	ping(_leastSeen.get());
}

void NodeTable::noteActiveNode(Public const& _pubk, bi::udp::endpoint const& _endpoint)
{
	if (_pubk == m_node.address())
		return;

	shared_ptr<NodeEntry> node = nodeEntry(_pubk);
	if (!!node && !node->pending)
	{
		clog(NodeTableConnect) << "Noting active node:" << _pubk.abridged() << _endpoint.address().to_string() << ":" << _endpoint.port();
		node->endpoint.udp.address(_endpoint.address());
		node->endpoint.udp.port(_endpoint.port());
		node->cullEndpoint();
		
		shared_ptr<NodeEntry> contested;
		{
			Guard l(x_state);
			NodeBucket& s = bucket_UNSAFE(node.get());
			bool removed = false;
			s.nodes.remove_if([&node, &removed](weak_ptr<NodeEntry> const& n)
			{
				if (n.lock() == node)
					removed = true;
				return removed;
			});
			
			if (s.nodes.size() >= s_bucketSize)
			{
				// It's only contested iff nodeentry exists
				contested = s.nodes.front().lock();
				if (!contested)
				{
					s.nodes.pop_front();
					s.nodes.push_back(node);
					s.touch();
					
					if (!removed && m_nodeEventHandler)
						m_nodeEventHandler->appendEvent(node->id, NodeEntryAdded);
				}
			}
			else
			{
				s.nodes.push_back(node);
				s.touch();
				
				if (!removed && m_nodeEventHandler)
					m_nodeEventHandler->appendEvent(node->id, NodeEntryAdded);
			}
		}
		
		if (contested)
			evict(contested, node);
	}
}

void NodeTable::dropNode(shared_ptr<NodeEntry> _n)
{
	// remove from nodetable
	{
		Guard l(x_state);
		NodeBucket& s = bucket_UNSAFE(_n.get());
		s.nodes.remove_if([&_n](weak_ptr<NodeEntry> n) { return n.lock() == _n; });
	}
	
	// notify host
	clog(NodeTableUpdate) << "p2p.nodes.drop " << _n->id.abridged();
	if (m_nodeEventHandler)
		m_nodeEventHandler->appendEvent(_n->id, NodeEntryDropped);
}

NodeTable::NodeBucket& NodeTable::bucket_UNSAFE(NodeEntry const* _n)
{
	return m_state[_n->distance - 1];
}

void NodeTable::onReceived(UDPSocketFace*, bi::udp::endpoint const& _from, bytesConstRef _packet)
{
	// h256 + Signature + type + RLP (smallest possible packet is empty neighbours packet which is 3 bytes)
	if (_packet.size() < h256::size + Signature::size + 1 + 3)
	{
		clog(NodeTableWarn) << "Invalid Message size from " << _from.address().to_string() << ":" << _from.port();
		return;
	}
	
	bytesConstRef hashedBytes(_packet.cropped(h256::size, _packet.size() - h256::size));
	h256 hashSigned(sha3(hashedBytes));
	if (!_packet.cropped(0, h256::size).contentsEqual(hashSigned.asBytes()))
	{
		clog(NodeTableWarn) << "Invalid Message hash from " << _from.address().to_string() << ":" << _from.port();
		return;
	}
	
	bytesConstRef signedBytes(hashedBytes.cropped(Signature::size, hashedBytes.size() - Signature::size));

	// todo: verify sig via known-nodeid and MDC
	
	bytesConstRef sigBytes(_packet.cropped(h256::size, Signature::size));
	Public nodeid(dev::recover(*(Signature const*)sigBytes.data(), sha3(signedBytes)));
	if (!nodeid)
	{
		clog(NodeTableWarn) << "Invalid Message signature from " << _from.address().to_string() << ":" << _from.port();
		return;
	}
	
	unsigned packetType = signedBytes[0];
	bytesConstRef rlpBytes(_packet.cropped(h256::size + Signature::size + 1));
	RLP rlp(rlpBytes);
	try {
		switch (packetType)
		{
			case Pong::type:
			{
				Pong in = Pong::fromBytesConstRef(_from, rlpBytes);
				
				// whenever a pong is received, check if it's in m_evictions
				Guard le(x_evictions);
				bool evictionEntry = false;
				for (auto it = m_evictions.begin(); it != m_evictions.end(); it++)
					if (it->first.first == nodeid && it->first.second > std::chrono::steady_clock::now())
					{
						evictionEntry = true;
						if (auto n = nodeEntry(it->second))
							dropNode(n);
						
						if (auto n = nodeEntry(it->first.first))
							n->pending = false;
						
						it = m_evictions.erase(it);
					}
				
				// if not, check if it's known/pending or a pubk discovery ping
				if (!evictionEntry)
				{
					if (auto n = nodeEntry(nodeid))
						n->pending = false;
					else if (m_pubkDiscoverPings.count(_from.address()))
					{
						{
							Guard l(x_pubkDiscoverPings);
							m_pubkDiscoverPings.erase(_from.address());
						}
						if (!haveNode(nodeid))
							addNode(nodeid, _from, bi::tcp::endpoint(_from.address(), _from.port()));
					}
					else
						return; // unsolicited pong; don't note node as active
				}
				
				clog(NodeTableConnect) << "PONG from " << nodeid.abridged() << _from;
				break;
			}
				
			case Neighbours::type:
			{
				Neighbours in = Neighbours::fromBytesConstRef(_from, rlpBytes);
				for (auto n: in.nodes)
					addNode(n.node, bi::udp::endpoint(bi::address::from_string(n.ipAddress), n.port), bi::tcp::endpoint(bi::address::from_string(n.ipAddress), n.port));
				break;
			}

			case FindNode::type:
			{
				FindNode in = FindNode::fromBytesConstRef(_from, rlpBytes);

				vector<shared_ptr<NodeEntry>> nearest = nearestNodeEntries(in.target);
				static unsigned const nlimit = (m_socketPointer->maxDatagramSize - 11) / 86;
				for (unsigned offset = 0; offset < nearest.size(); offset += nlimit)
				{
					Neighbours out(_from, nearest, offset, nlimit);
					out.sign(m_secret);
					if (out.data.size() > 1280)
						clog(NetWarn) << "Sending truncated datagram, size: " << out.data.size();
					m_socketPointer->send(out);
				}
				break;
			}

			case PingNode::type:
			{
				PingNode in = PingNode::fromBytesConstRef(_from, rlpBytes);
				if (in.version != dev::p2p::c_protocolVersion)
				{
					if (auto n = nodeEntry(nodeid))
						dropNode(n);
					return;
				}
				
				addNode(nodeid, _from, bi::tcp::endpoint(bi::address::from_string(in.ipAddress), in.port));
				
				Pong p(_from);
				p.echo = sha3(rlpBytes);
				p.sign(m_secret);
				m_socketPointer->send(p);
				break;
			}
				
			default:
				clog(NodeTableWarn) << "Invalid Message, " << hex << packetType << ", received from " << _from.address().to_string() << ":" << dec << _from.port();
				return;
		}

		noteActiveNode(nodeid, _from);
	}
	catch (...)
	{
		clog(NodeTableWarn) << "Exception processing message from " << _from.address().to_string() << ":" << _from.port();
	}
}

void NodeTable::doCheckEvictions(boost::system::error_code const& _ec)
{
	if (_ec || !m_socketPointer->isOpen())
		return;

	auto self(shared_from_this());
	m_evictionCheckTimer.expires_from_now(c_evictionCheckInterval);
	m_evictionCheckTimer.async_wait([this, self](boost::system::error_code const& _ec)
	{
		if (_ec)
			return;
		
		bool evictionsRemain = false;
		list<shared_ptr<NodeEntry>> drop;
		{
			Guard ln(x_nodes);
			Guard le(x_evictions);
			for (auto& e: m_evictions)
				if (chrono::steady_clock::now() - e.first.second > c_reqTimeout)
					if (m_nodes.count(e.second))
						drop.push_back(m_nodes[e.second]);
			evictionsRemain = m_evictions.size() - drop.size() > 0;
		}
		
		drop.unique();
		for (auto n: drop)
			dropNode(n);
		
		if (evictionsRemain)
			doCheckEvictions(boost::system::error_code());
	});
}

void NodeTable::doRefreshBuckets(boost::system::error_code const& _ec)
{
	if (_ec)
		return;

	clog(NodeTableEvent) << "refreshing buckets";
	bool connected = m_socketPointer->isOpen();
	if (connected)
	{
		NodeId randNodeId;
		crypto::Nonce::get().ref().copyTo(randNodeId.ref().cropped(0, h256::size));
		crypto::Nonce::get().ref().copyTo(randNodeId.ref().cropped(h256::size, h256::size));
		discover(randNodeId);
	}

	auto runcb = [this](boost::system::error_code const& error) { doRefreshBuckets(error); };
	m_bucketRefreshTimer.expires_from_now(boost::posix_time::milliseconds(c_bucketRefresh.count()));
	m_bucketRefreshTimer.async_wait(runcb);
}

void PingNode::streamRLP(RLPStream& _s) const
{
	_s.appendList(4);
	_s << dev::p2p::c_protocolVersion << ipAddress << port << expiration;
}

void PingNode::interpretRLP(bytesConstRef _bytes)
{
	RLP r(_bytes);
	if (r.itemCountStrict() == 3)
	{
		version = 2;
		ipAddress = r[0].toString();
		port = r[1].toInt<unsigned>(RLP::Strict);
		expiration = r[2].toInt<unsigned>(RLP::Strict);
	}
	else if (r.itemCountStrict() == 4)
	{
		version = r[0].toInt<unsigned>(RLP::Strict);
		ipAddress = r[1].toString();
		port = r[2].toInt<unsigned>(RLP::Strict);
		expiration = r[3].toInt<unsigned>(RLP::Strict);
	}
	else
		BOOST_THROW_EXCEPTION(InvalidRLP());
}
