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

namespace dev
{
namespace p2p
{
namespace
{
// global thread-safe logger for static methods
BOOST_LOG_INLINE_GLOBAL_LOGGER_CTOR_ARGS(g_discoveryWarnLogger,
    boost::log::sources::severity_channel_logger_mt<>,
    (boost::log::keywords::severity = 0)(boost::log::keywords::channel = "discov"))
}  // namespace

inline bool operator==(
    std::weak_ptr<NodeEntry> const& _weak, std::shared_ptr<NodeEntry> const& _shared)
{
    return !_weak.owner_before(_shared) && !_shared.owner_before(_weak);
}

NodeEntry::NodeEntry(NodeID const& _src, Public const& _pubk, NodeIPEndpoint const& _gw): Node(_pubk, _gw), distance(NodeTable::distance(_src, _pubk)) {}

NodeTable::NodeTable(ba::io_service& _io, KeyPair const& _alias, NodeIPEndpoint const& _endpoint, bool _enabled):
    m_node(Node(_alias.pub(), _endpoint)),
    m_secret(_alias.secret()),
    m_socket(make_shared<NodeSocket>(_io, *reinterpret_cast<UDPSocketEvents*>(this), (bi::udp::endpoint)m_node.endpoint)),
    m_socketPointer(m_socket.get()),
    m_timers(_io)
{
    for (unsigned i = 0; i < s_bins; i++)
        m_state[i].distance = i;
    
    if (!_enabled)
        return;
    
    try
    {
        m_socketPointer->connect();
        doDiscovery();
    }
    catch (std::exception const& _e)
    {
        cwarn << "Exception connecting NodeTable socket: " << _e.what();
        cwarn << "Discovery disabled.";
    }
}
    
NodeTable::~NodeTable()
{
    m_socketPointer->disconnect();
    m_timers.stop();
}

void NodeTable::processEvents()
{
    if (m_nodeEventHandler)
        m_nodeEventHandler->processEvents();
}

shared_ptr<NodeEntry> NodeTable::addNode(Node const& _node, NodeRelation _relation)
{
    if (_relation == Known)
    {
        auto ret = make_shared<NodeEntry>(m_node.id, _node.id, _node.endpoint);
        ret->pending = false;
        DEV_GUARDED(x_nodes)
            m_nodes[_node.id] = ret;
        noteActiveNode(_node.id, _node.endpoint);
        return ret;
    }
    
    if (!_node.endpoint)
        return shared_ptr<NodeEntry>();
    
    // ping address to recover nodeid if nodeid is empty
    if (!_node.id)
    {
        DEV_GUARDED(x_nodes)
        LOG(m_logger) << "Sending public key discovery Ping to "
                      << (bi::udp::endpoint)_node.endpoint
                      << " (Advertising: " << (bi::udp::endpoint)m_node.endpoint << ")";
        DEV_GUARDED(x_pubkDiscoverPings)
            m_pubkDiscoverPings[_node.endpoint.address] = std::chrono::steady_clock::now();
        ping(_node.endpoint);
        return shared_ptr<NodeEntry>();
    }
    
    DEV_GUARDED(x_nodes)
        if (m_nodes.count(_node.id))
            return m_nodes[_node.id];
    
    auto ret = make_shared<NodeEntry>(m_node.id, _node.id, _node.endpoint);
    DEV_GUARDED(x_nodes)
        m_nodes[_node.id] = ret;
        LOG(m_logger) << "addNode pending for " << _node.endpoint;
        ping(_node.endpoint);
        return ret;
}

list<NodeID> NodeTable::nodes() const
{
    list<NodeID> nodes;
    DEV_GUARDED(x_nodes)
        for (auto& i: m_nodes)
            nodes.push_back(i.second->id);
    return nodes;
}

list<NodeEntry> NodeTable::snapshot() const
{
    list<NodeEntry> ret;
    DEV_GUARDED(x_state)
        for (auto const& s: m_state)
            for (auto const& np: s.nodes)
                if (auto n = np.lock())
                    ret.push_back(*n);
    return ret;
}

Node NodeTable::node(NodeID const& _id)
{
    Guard l(x_nodes);
    if (m_nodes.count(_id))
    {
        auto entry = m_nodes[_id];
        return Node(_id, entry->endpoint, entry->peerType);
    }
    return UnspecifiedNode;
}

shared_ptr<NodeEntry> NodeTable::nodeEntry(NodeID _id)
{
    Guard l(x_nodes);
    return m_nodes.count(_id) ? m_nodes[_id] : shared_ptr<NodeEntry>();
}

void NodeTable::doDiscover(NodeID _node, unsigned _round, shared_ptr<set<shared_ptr<NodeEntry>>> _tried)
{
    // NOTE: ONLY called by doDiscovery!
    
    if (!m_socketPointer->isOpen())
        return;
    
    if (_round == s_maxSteps)
    {
        LOG(m_logger) << "Terminating discover after " << _round << " rounds.";
        doDiscovery();
        return;
    }
    else if (!_round && !_tried)
        // initialized _tried on first round
        _tried = make_shared<set<shared_ptr<NodeEntry>>>();
    
    auto nearest = nearestNodeEntries(_node);
    list<shared_ptr<NodeEntry>> tried;
    for (unsigned i = 0; i < nearest.size() && tried.size() < s_alpha; i++)
        if (!_tried->count(nearest[i]))
        {
            auto r = nearest[i];
            tried.push_back(r);
            FindNode p(r->endpoint, _node);
            p.sign(m_secret);
            DEV_GUARDED(x_findNodeTimeout)
                m_findNodeTimeout.push_back(make_pair(r->id, chrono::steady_clock::now()));
            m_socketPointer->send(p);
        }
    
    if (tried.empty())
    {
        LOG(m_logger) << "Terminating discover after " << _round << " rounds.";
        doDiscovery();
        return;
    }
        
    while (!tried.empty())
    {
        _tried->insert(tried.front());
        tried.pop_front();
    }

    m_timers.schedule(c_reqTimeout.count() * 2, [this, _node, _round, _tried](boost::system::error_code const& _ec)
    {
        if (_ec)
            // we can't use m_logger here, because captured this might be already destroyed
            clog(VerbosityDebug, "discov")
                << "Discovery timer was probably cancelled: " << _ec.value() << " "
                << _ec.message();

        if (_ec.value() == boost::asio::error::operation_aborted || m_timers.isStopped())
            return;

        // error::operation_aborted means that the timer was probably aborted. 
        // It usually happens when "this" object is deallocated, in which case 
        // subsequent call to doDiscover() would cause a crash. We can not rely on 
        // m_timers.isStopped(), because "this" pointer was captured by the lambda,
        // and therefore, in case of deallocation m_timers object no longer exists.

        doDiscover(_node, _round + 1, _tried);
    });
}

vector<shared_ptr<NodeEntry>> NodeTable::nearestNodeEntries(NodeID _target)
{
    // send s_alpha FindNode packets to nodes we know, closest to target
    static unsigned lastBin = s_bins - 1;
    unsigned head = distance(m_node.id, _target);
    unsigned tail = head == 0 ? lastBin : (head - 1) % s_bins;
    
    map<unsigned, list<shared_ptr<NodeEntry>>> found;
    
    // if d is 0, then we roll look forward, if last, we reverse, else, spread from d
    if (head > 1 && tail != lastBin)
        while (head != tail && head < s_bins)
        {
            Guard l(x_state);
            for (auto const& n: m_state[head].nodes)
                if (auto p = n.lock())
                    found[distance(_target, p->id)].push_back(p);

            if (tail)
                for (auto const& n: m_state[tail].nodes)
                    if (auto p = n.lock())
                        found[distance(_target, p->id)].push_back(p);

            head++;
            if (tail)
                tail--;
        }
    else if (head < 2)
        while (head < s_bins)
        {
            Guard l(x_state);
            for (auto const& n: m_state[head].nodes)
                if (auto p = n.lock())
                    found[distance(_target, p->id)].push_back(p);
            head++;
        }
    else
        while (tail > 0)
        {
            Guard l(x_state);
            for (auto const& n: m_state[tail].nodes)
                if (auto p = n.lock())
                    found[distance(_target, p->id)].push_back(p);
            tail--;
        }
    
    vector<shared_ptr<NodeEntry>> ret;
    for (auto& nodes: found)
        for (auto const& n: nodes.second)
            if (ret.size() < s_bucketSize && !!n->endpoint && n->endpoint.isAllowed())
                ret.push_back(n);
    return ret;
}

void NodeTable::ping(NodeIPEndpoint _to) const
{
    NodeIPEndpoint src;
    DEV_GUARDED(x_nodes)
        src = m_node.endpoint;
    PingNode p(src, _to);
    p.sign(m_secret);
    m_socketPointer->send(p);
}

void NodeTable::ping(NodeEntry* _n) const
{
    if (_n)
        ping(_n->endpoint);
}

void NodeTable::evict(shared_ptr<NodeEntry> _leastSeen, shared_ptr<NodeEntry> _new)
{
    if (!m_socketPointer->isOpen())
        return;
    
    unsigned evicts = 0;
    DEV_GUARDED(x_evictions)
    {
        EvictionTimeout evictTimeout{_new->id, chrono::steady_clock::now()};  
        m_evictions.emplace(_leastSeen->id, evictTimeout);
        evicts = m_evictions.size();
    }

    if (evicts == 1)
        doCheckEvictions();
    ping(_leastSeen.get());
}

void NodeTable::noteActiveNode(Public const& _pubk, bi::udp::endpoint const& _endpoint)
{
    if (_pubk == m_node.address() || !NodeIPEndpoint(_endpoint.address(), _endpoint.port(), _endpoint.port()).isAllowed())
        return;

    shared_ptr<NodeEntry> newNode = nodeEntry(_pubk);
    if (newNode && !newNode->pending)
    {
        LOG(m_logger) << "Noting active node: " << _pubk << " " << _endpoint.address().to_string()
                      << ":" << _endpoint.port();
        newNode->endpoint.address = _endpoint.address();
        newNode->endpoint.udpPort = _endpoint.port();

        shared_ptr<NodeEntry> nodeToEvict;
        {
            Guard l(x_state);
            // Find a bucket to put a node to
            NodeBucket& s = bucket_UNSAFE(newNode.get());
            auto& nodes = s.nodes;

            // check if the node is already in the bucket
            auto it = std::find(nodes.begin(), nodes.end(), newNode);
            if (it != nodes.end())
            {
                // if it was in the bucket, move it to the last position
                nodes.splice(nodes.end(), nodes, it);
            }
            else
            {
                if (nodes.size() < s_bucketSize)
                {
                    // if it was not there, just add it as a most recently seen node
                    // (i.e. to the end of the list)
                    nodes.push_back(newNode);
                    if (m_nodeEventHandler)
                        m_nodeEventHandler->appendEvent(newNode->id, NodeEntryAdded);
                }
                else
                {
                    // if bucket is full, start eviction process for the least recently seen node
                    nodeToEvict = nodes.front().lock();
                    // It could have been replaced in addNode(), then weak_ptr is expired.
                    // If so, just add a new one instead of expired
                    if (!nodeToEvict)
                    {
                        nodes.pop_front();
                        nodes.push_back(newNode);
                        if (m_nodeEventHandler)
                            m_nodeEventHandler->appendEvent(newNode->id, NodeEntryAdded);
                    }
                }
            }
        }

        if (nodeToEvict)
            evict(nodeToEvict, newNode);
    }
}

void NodeTable::dropNode(shared_ptr<NodeEntry> _n)
{
    // remove from nodetable
    {
        Guard l(x_state);
        NodeBucket& s = bucket_UNSAFE(_n.get());
        s.nodes.remove_if(
            [_n](weak_ptr<NodeEntry> const& _bucketEntry) { return _bucketEntry == _n; });
    }
    
    // notify host
    LOG(m_logger) << "p2p.nodes.drop " << _n->id;
    if (m_nodeEventHandler)
        m_nodeEventHandler->appendEvent(_n->id, NodeEntryDropped);
}

NodeTable::NodeBucket& NodeTable::bucket_UNSAFE(NodeEntry const* _n)
{
    return m_state[_n->distance - 1];
}

void NodeTable::onReceived(UDPSocketFace*, bi::udp::endpoint const& _from, bytesConstRef _packet)
{
    try {
        unique_ptr<DiscoveryDatagram> packet = DiscoveryDatagram::interpretUDP(_from, _packet);
        if (!packet)
            return;
        if (packet->isExpired())
        {
            LOG(m_logger) << "Invalid packet (timestamp in the past) from "
                          << _from.address().to_string() << ":" << _from.port();
            return;
        }

        switch (packet->packetType())
        {
            case Pong::type:
            {
                auto in = dynamic_cast<Pong const&>(*packet);
                // whenever a pong is received, check if it's in m_evictions
                bool found = false;
                NodeID leastSeenID;
                EvictionTimeout evictionEntry;
                DEV_GUARDED(x_evictions)
                { 
                    auto e = m_evictions.find(in.sourceid);
                    if (e != m_evictions.end())
                    { 
                        if (e->second.evictedTimePoint > std::chrono::steady_clock::now())
                        {
                            found = true;
                            leastSeenID = e->first;
                            evictionEntry = e->second;
                            m_evictions.erase(e);
                        }
                    }
                }

                if (found)
                {
                    if (auto n = nodeEntry(evictionEntry.newNodeID))
                        dropNode(n);
                    if (auto n = nodeEntry(leastSeenID))
                        n->pending = false;
                }
                else
                {
                    // if not, check if it's known/pending or a pubk discovery ping
                    if (auto n = nodeEntry(in.sourceid))
                        n->pending = false;
                    else
                    {
                        DEV_GUARDED(x_pubkDiscoverPings)
                        {
                            if (!m_pubkDiscoverPings.count(_from.address()))
                                return; // unsolicited pong; don't note node as active
                            m_pubkDiscoverPings.erase(_from.address());
                        }
                        if (!haveNode(in.sourceid))
                            addNode(Node(in.sourceid, NodeIPEndpoint(_from.address(), _from.port(), _from.port())));
                    }
                }
                
                // update our endpoint address and UDP port
                DEV_GUARDED(x_nodes)
                {
                    if ((!m_node.endpoint || !m_node.endpoint.isAllowed()) && isPublicAddress(in.destination.address))
                        m_node.endpoint.address = in.destination.address;
                    m_node.endpoint.udpPort = in.destination.udpPort;
                }

                LOG(m_logger) << "PONG from " << in.sourceid << " " << _from;
                break;
            }
                
            case Neighbours::type:
            {
                auto in = dynamic_cast<Neighbours const&>(*packet);
                bool expected = false;
                auto now = chrono::steady_clock::now();
                DEV_GUARDED(x_findNodeTimeout)
                    m_findNodeTimeout.remove_if([&](NodeIdTimePoint const& t)
                    {
                        if (t.first == in.sourceid && now - t.second < c_reqTimeout)
                            expected = true;
                        else if (t.first == in.sourceid)
                            return true;
                        return false;
                    });
                if (!expected)
                {
                    cnetdetails << "Dropping unsolicited neighbours packet from "
                                << _from.address();
                    break;
                }

                for (auto n: in.neighbours)
                    addNode(Node(n.node, n.endpoint));
                break;
            }

            case FindNode::type:
            {
                auto in = dynamic_cast<FindNode const&>(*packet);
                vector<shared_ptr<NodeEntry>> nearest = nearestNodeEntries(in.target);
                static unsigned const nlimit = (m_socketPointer->maxDatagramSize - 109) / 90;
                for (unsigned offset = 0; offset < nearest.size(); offset += nlimit)
                {
                    Neighbours out(_from, nearest, offset, nlimit);
                    out.sign(m_secret);
                    if (out.data.size() > 1280)
                        cnetlog << "Sending truncated datagram, size: " << out.data.size();
                    m_socketPointer->send(out);
                }
                break;
            }

            case PingNode::type:
            {
                auto in = dynamic_cast<PingNode const&>(*packet);
                in.source.address = _from.address();
                in.source.udpPort = _from.port();
                addNode(Node(in.sourceid, in.source));
                
                Pong p(in.source);
                p.echo = in.echo;
                p.sign(m_secret);
                m_socketPointer->send(p);
                break;
            }
        }

        noteActiveNode(packet->sourceid, _from);
    }
    catch (std::exception const& _e)
    {
        LOG(m_logger) << "Exception processing message from " << _from.address().to_string() << ":"
                      << _from.port() << ": " << _e.what();
    }
    catch (...)
    {
        LOG(m_logger) << "Exception processing message from " << _from.address().to_string() << ":"
                      << _from.port();
    }
}

void NodeTable::doCheckEvictions()
{
    m_timers.schedule(c_evictionCheckInterval.count(), [this](boost::system::error_code const& _ec)
    {
        if (_ec)
            // we can't use m_logger here, because captured this might be already destroyed
            clog(VerbosityDebug, "discov")
                << "Check Evictions timer was probably cancelled: " << _ec.value() << " "
                << _ec.message();

        if (_ec.value() == boost::asio::error::operation_aborted || m_timers.isStopped())
            return;
        
        bool evictionsRemain = false;
        list<shared_ptr<NodeEntry>> drop;
        {
            Guard le(x_evictions);
            Guard ln(x_nodes);
            for (auto& e: m_evictions)
                if (chrono::steady_clock::now() - e.second.evictedTimePoint > c_reqTimeout)
                    if (m_nodes.count(e.second.newNodeID))
                        drop.push_back(m_nodes[e.second.newNodeID]);
            evictionsRemain = (m_evictions.size() - drop.size() > 0);
        }
        
        drop.unique();
        for (auto n: drop)
            dropNode(n);
        
        if (evictionsRemain)
            doCheckEvictions();
    });
}

void NodeTable::doDiscovery()
{
    m_timers.schedule(c_bucketRefresh.count(), [this](boost::system::error_code const& _ec)
    {
        if (_ec)
            // we can't use m_logger here, because captured this might be already destroyed
            clog(VerbosityDebug, "discov")
                << "Discovery timer was probably cancelled: " << _ec.value() << " "
                << _ec.message();

        if (_ec.value() == boost::asio::error::operation_aborted || m_timers.isStopped())
            return;

        LOG(m_logger) << "performing random discovery";
        NodeID randNodeId;
        crypto::Nonce::get().ref().copyTo(randNodeId.ref().cropped(0, h256::size));
        crypto::Nonce::get().ref().copyTo(randNodeId.ref().cropped(h256::size, h256::size));
        doDiscover(randNodeId);
    });
}

unique_ptr<DiscoveryDatagram> DiscoveryDatagram::interpretUDP(bi::udp::endpoint const& _from, bytesConstRef _packet)
{
    unique_ptr<DiscoveryDatagram> decoded;
    // h256 + Signature + type + RLP (smallest possible packet is empty neighbours packet which is 3 bytes)
    if (_packet.size() < h256::size + Signature::size + 1 + 3)
    {
        LOG(g_discoveryWarnLogger::get()) << "Invalid packet (too small) from "
                                          << _from.address().to_string() << ":" << _from.port();
        return decoded;
    }
    bytesConstRef hashedBytes(_packet.cropped(h256::size, _packet.size() - h256::size));
    bytesConstRef signedBytes(hashedBytes.cropped(Signature::size, hashedBytes.size() - Signature::size));
    bytesConstRef signatureBytes(_packet.cropped(h256::size, Signature::size));
    bytesConstRef bodyBytes(_packet.cropped(h256::size + Signature::size + 1));

    h256 echo(sha3(hashedBytes));
    if (!_packet.cropped(0, h256::size).contentsEqual(echo.asBytes()))
    {
        LOG(g_discoveryWarnLogger::get()) << "Invalid packet (bad hash) from "
                                          << _from.address().to_string() << ":" << _from.port();
        return decoded;
    }
    Public sourceid(dev::recover(*(Signature const*)signatureBytes.data(), sha3(signedBytes)));
    if (!sourceid)
    {
        LOG(g_discoveryWarnLogger::get()) << "Invalid packet (bad signature) from "
                                          << _from.address().to_string() << ":" << _from.port();
        return decoded;
    }
    switch (signedBytes[0])
    {
    case PingNode::type:
        decoded.reset(new PingNode(_from, sourceid, echo));
        break;
    case Pong::type:
        decoded.reset(new Pong(_from, sourceid, echo));
        break;
    case FindNode::type:
        decoded.reset(new FindNode(_from, sourceid, echo));
        break;
    case Neighbours::type:
        decoded.reset(new Neighbours(_from, sourceid, echo));
        break;
    default:
        LOG(g_discoveryWarnLogger::get()) << "Invalid packet (unknown packet type) from "
                                          << _from.address().to_string() << ":" << _from.port();
        return decoded;
    }
    decoded->interpretRLP(bodyBytes);
    return decoded;
}

}  // namespace p2p
}  // namespace dev
