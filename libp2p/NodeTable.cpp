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

constexpr unsigned c_handleTimeoutsIntervalMs = 5000;

}  // namespace

constexpr std::chrono::seconds DiscoveryDatagram::c_timeToLive;
constexpr std::chrono::milliseconds NodeTable::c_reqTimeout;
constexpr std::chrono::milliseconds NodeTable::c_bucketRefresh;
constexpr std::chrono::milliseconds NodeTable::c_evictionCheckInterval;

inline bool operator==(
    std::weak_ptr<NodeEntry> const& _weak, std::shared_ptr<NodeEntry> const& _shared)
{
    return !_weak.owner_before(_shared) && !_shared.owner_before(_weak);
}

NodeEntry::NodeEntry(NodeID const& _src, Public const& _pubk, NodeIPEndpoint const& _gw): Node(_pubk, _gw), distance(NodeTable::distance(_src, _pubk)) {}

NodeTable::NodeTable(
    ba::io_service& _io, KeyPair const& _alias, NodeIPEndpoint const& _endpoint, bool _enabled)
  : m_hostNodeID(_alias.pub()),
    m_hostNodeEndpoint(_endpoint),
    m_secret(_alias.secret()),
    m_socket(make_shared<NodeSocket>(
        _io, static_cast<UDPSocketEvents&>(*this), (bi::udp::endpoint)m_hostNodeEndpoint)),
    m_timers(_io)
{
    for (unsigned i = 0; i < s_bins; i++)
        m_buckets[i].distance = i;

    if (!_enabled)
        return;

    try
    {
        m_socket->connect();
        doDiscovery();
        doHandleTimeouts();
    }
    catch (std::exception const& _e)
    {
        cwarn << "Exception connecting NodeTable socket: " << _e.what();
        cwarn << "Discovery disabled.";
    }
}

NodeTable::~NodeTable()
{
    m_socket->disconnect();
    m_timers.stop();
}

void NodeTable::processEvents()
{
    if (m_nodeEventHandler)
        m_nodeEventHandler->processEvents();
}

void NodeTable::addNode(Node const& _node, NodeRelation _relation)
{
    if (_relation == Known)
    {
        auto nodeEntry = make_shared<NodeEntry>(m_hostNodeID, _node.id, _node.endpoint);
        // mark as validated
        // TODO get last pong time as input, ping if needed
        nodeEntry->lastPongReceivedTime = RLPXDatagramFace::secondsSinceEpoch();
        DEV_GUARDED(x_nodes) { m_allNodes[_node.id] = nodeEntry; }
        noteActiveNode(_node.id, _node.endpoint);
        return;
    }

    if (!_node.endpoint || !_node.id)
        return;

    DEV_GUARDED(x_nodes)
    {
        if (m_allNodes.count(_node.id))
            return;
    }

    auto nodeEntry = make_shared<NodeEntry>(m_hostNodeID, _node.id, _node.endpoint);
    DEV_GUARDED(x_nodes) { m_allNodes[_node.id] = nodeEntry; }
    LOG(m_logger) << "Pending node " << _node.id << "@" << _node.endpoint;
    ping(*nodeEntry);
}

list<NodeID> NodeTable::nodes() const
{
    list<NodeID> nodes;
    DEV_GUARDED(x_nodes)
    {
        for (auto& i : m_allNodes)
            nodes.push_back(i.second->id);
    }
    return nodes;
}

list<NodeEntry> NodeTable::snapshot() const
{
    list<NodeEntry> ret;
    DEV_GUARDED(x_state)
    {
        for (auto const& s : m_buckets)
            for (auto const& np : s.nodes)
                if (auto n = np.lock())
                    ret.push_back(*n);
    }
    return ret;
}

Node NodeTable::node(NodeID const& _id)
{
    Guard l(x_nodes);
    auto const it = m_allNodes.find(_id);
    if (it != m_allNodes.end())
    {
        auto const& entry = it->second;
        return Node(_id, entry->endpoint, entry->peerType);
    }
    return UnspecifiedNode;
}

shared_ptr<NodeEntry> NodeTable::nodeEntry(NodeID _id)
{
    Guard l(x_nodes);
    auto const it = m_allNodes.find(_id);
    return it != m_allNodes.end() ? it->second : shared_ptr<NodeEntry>();
}

void NodeTable::doDiscover(NodeID _node, unsigned _round, shared_ptr<set<shared_ptr<NodeEntry>>> _tried)
{
    // NOTE: ONLY called by doDiscovery!
    
    if (!m_socket->isOpen())
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
                m_findNodeTimeout.emplace_back(r->id, chrono::steady_clock::now());
            LOG(m_logger) << p.typeName() << " to " << _node << "@" << r->endpoint;
            m_socket->send(p);
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
    unsigned head = distance(m_hostNodeID, _target);
    unsigned tail = head == 0 ? lastBin : (head - 1) % s_bins;
    
    map<unsigned, list<shared_ptr<NodeEntry>>> found;
    
    // if d is 0, then we roll look forward, if last, we reverse, else, spread from d
    if (head > 1 && tail != lastBin)
        while (head != tail && head < s_bins)
        {
            Guard l(x_state);
            for (auto const& n : m_buckets[head].nodes)
                if (auto p = n.lock())
                    found[distance(_target, p->id)].push_back(p);

            if (tail)
                for (auto const& n : m_buckets[tail].nodes)
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
            for (auto const& n : m_buckets[head].nodes)
                if (auto p = n.lock())
                    found[distance(_target, p->id)].push_back(p);
            head++;
        }
    else
        while (tail > 0)
        {
            Guard l(x_state);
            for (auto const& n : m_buckets[tail].nodes)
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

void NodeTable::ping(NodeEntry const& _nodeEntry, boost::optional<NodeID> const& _replacementNodeID)
{
    m_timers.schedule(0, [this, _nodeEntry, _replacementNodeID](
                             boost::system::error_code const& _ec) {
        if (_ec || m_timers.isStopped())
            return;

        NodeIPEndpoint src;
        src = m_hostNodeEndpoint;
        PingNode p(src, _nodeEntry.endpoint);
        auto const pingHash = p.sign(m_secret);
        LOG(m_logger) << p.typeName() << " to " << _nodeEntry.id << "@" << p.destination;
        m_socket->send(p);

        m_sentPings[_nodeEntry.id] = {chrono::steady_clock::now(), pingHash, _replacementNodeID};
    });
}

void NodeTable::evict(NodeEntry const& _leastSeen, NodeEntry const& _new)
{
    if (!m_socket->isOpen())
        return;

    ping(_leastSeen, _new.id);
}

void NodeTable::noteActiveNode(Public const& _pubk, bi::udp::endpoint const& _endpoint)
{
    if (_pubk == m_hostNodeID ||
        !NodeIPEndpoint(_endpoint.address(), _endpoint.port(), _endpoint.port()).isAllowed())
        return;

    shared_ptr<NodeEntry> newNode = nodeEntry(_pubk);
    if (newNode && RLPXDatagramFace::secondsSinceEpoch() <
                       newNode->lastPongReceivedTime + c_bondingTimeSeconds)
    {
        LOG(m_logger) << "Active node " << _pubk << '@' << _endpoint;
        newNode->endpoint.setAddress(_endpoint.address());
        newNode->endpoint.setUdpPort(_endpoint.port());


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
            evict(*nodeToEvict, *newNode);
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

    DEV_GUARDED(x_nodes) { m_allNodes.erase(_n->id); }

    // notify host
    LOG(m_logger) << "p2p.nodes.drop " << _n->id;
    if (m_nodeEventHandler)
        m_nodeEventHandler->appendEvent(_n->id, NodeEntryDropped);
}

NodeTable::NodeBucket& NodeTable::bucket_UNSAFE(NodeEntry const* _n)
{
    return m_buckets[_n->distance - 1];
}

void NodeTable::onPacketReceived(
    UDPSocketFace*, bi::udp::endpoint const& _from, bytesConstRef _packet)
{
    try {
        unique_ptr<DiscoveryDatagram> packet = DiscoveryDatagram::interpretUDP(_from, _packet);
        if (!packet)
            return;
        if (packet->isExpired())
        {
            LOG(m_logger) << "Expired " << packet->typeName() << " from " << packet->sourceid << "@"
                          << _from;
            return;
        }

        LOG(m_logger) << packet->typeName() << " from " << packet->sourceid << "@" << _from;
        switch (packet->packetType())
        {
            case Pong::type:
            {
                auto const& pong = dynamic_cast<Pong const&>(*packet);
                auto const& sourceId = pong.sourceid;

                // validate pong
                auto const sentPing = m_sentPings.find(sourceId);
                if (sentPing == m_sentPings.end())
                {
                    LOG(m_logger) << "Unexpected PONG from " << _from.address().to_string() << ":"
                                  << _from.port();
                    return;
                }

                if (pong.echo != sentPing->second.pingHash)
                {
                    LOG(m_logger) << "Invalid PONG from " << _from.address().to_string() << ":"
                                  << _from.port();
                    return;
                }

                auto const sourceNodeEntry = nodeEntry(sourceId);
                assert(sourceNodeEntry.get());
                sourceNodeEntry->lastPongReceivedTime = RLPXDatagramFace::secondsSinceEpoch();

                // Valid PONG received, so we don't want to evict this node,
                // and we don't need to remember replacement node anymore
                auto const& optionalReplacementID = sentPing->second.replacementNodeID;
                if (optionalReplacementID)
                    if (auto replacementNode = nodeEntry(*optionalReplacementID))
                        dropNode(move(replacementNode));

                m_sentPings.erase(sentPing);

                // update our endpoint address and UDP port
                DEV_GUARDED(x_nodes)
                {
                    if ((!m_hostNodeEndpoint || !m_hostNodeEndpoint.isAllowed()) &&
                        isPublicAddress(pong.destination.address()))
                        m_hostNodeEndpoint.setAddress(pong.destination.address());
                    m_hostNodeEndpoint.setUdpPort(pong.destination.udpPort());
                }
                break;
            }

            case Neighbours::type:
            {
                auto const& in = dynamic_cast<Neighbours const&>(*packet);
                bool expected = false;
                auto now = chrono::steady_clock::now();
                DEV_GUARDED(x_findNodeTimeout)
                {
                    m_findNodeTimeout.remove_if([&](NodeIdTimePoint const& _t) noexcept {
                        if (_t.first != in.sourceid)
                            return false;
                        if (now - _t.second < c_reqTimeout)
                            expected = true;
                        return true;
                    });
                }
                if (!expected)
                {
                    cnetdetails << "Dropping unsolicited neighbours packet from "
                                << _from.address();
                    break;
                }

                for (auto const& n : in.neighbours)
                    addNode(Node(n.node, n.endpoint));
                break;
            }

            case FindNode::type:
            {
                auto const& in = dynamic_cast<FindNode const&>(*packet);
                vector<shared_ptr<NodeEntry>> nearest = nearestNodeEntries(in.target);
                static unsigned constexpr nlimit = (NodeSocket::maxDatagramSize - 109) / 90;
                for (unsigned offset = 0; offset < nearest.size(); offset += nlimit)
                {
                    Neighbours out(_from, nearest, offset, nlimit);
                    LOG(m_logger) << out.typeName() << " to " << in.sourceid << "@" << _from;
                    out.sign(m_secret);
                    if (out.data.size() > 1280)
                        cnetlog << "Sending truncated datagram, size: " << out.data.size();
                    m_socket->send(out);
                }
                break;
            }

            case PingNode::type:
            {
                auto& in = dynamic_cast<PingNode&>(*packet);
                in.source.setAddress(_from.address());
                in.source.setUdpPort(_from.port());
                addNode(Node(in.sourceid, in.source));
                
                Pong p(in.source);
                LOG(m_logger) << p.typeName() << " to " << in.sourceid << "@" << _from;
                p.echo = in.echo;
                p.sign(m_secret);
                m_socket->send(p);
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

void NodeTable::doHandleTimeouts()
{
    m_timers.schedule(c_handleTimeoutsIntervalMs, [this](boost::system::error_code const& _ec) {
        if ((_ec && _ec.value() == boost::asio::error::operation_aborted) || m_timers.isStopped())
            return;

        vector<shared_ptr<NodeEntry>> nodesToActivate;
        for (auto it = m_sentPings.begin(); it != m_sentPings.end();)
        {
            if (chrono::steady_clock::now() >
                it->second.pingSendTime + DiscoveryDatagram::c_timeToLive)
            {
                if (auto node = nodeEntry(it->first))
                {
                    dropNode(move(node));

                    // save the replacement node that should be activated
                    if (it->second.replacementNodeID)
                        if (auto replacement = nodeEntry(*it->second.replacementNodeID))
                            nodesToActivate.emplace_back(replacement);
                }

                it = m_sentPings.erase(it);
            }
            else
                ++it;
        }

        // activate replacement nodes and put them into buckets
        for (auto const& n : nodesToActivate)
            noteActiveNode(n->id, n->endpoint);

        doHandleTimeouts();
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
