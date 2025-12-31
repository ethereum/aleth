// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include "Common.h"
#include "ENR.h"
#include "EndpointTracker.h"
#include <libp2p/UDP.h>
#include <boost/integer/static_log2.hpp>
#include <algorithm>

namespace dev
{
namespace p2p
{

enum NodeTableEventType
{
    NodeEntryAdded,
    NodeEntryDropped,
    NodeEntryScheduledForEviction
};

class NodeTable;
class NodeTableEventHandler
{
public:
    friend class NodeTable;

    virtual ~NodeTableEventHandler() = default;

    virtual void processEvent(NodeID const& _n, NodeTableEventType const& _e) = 0;

protected:
    /// Called by NodeTable on behalf of an implementation (Host) to process new events without blocking nodetable.
    void processEvents()
    {
        std::list<std::pair<NodeID, NodeTableEventType>> events;
        {
            Guard l(x_events);
            if (!m_nodeEventHandler.size())
                return;
            m_nodeEventHandler.unique();
            for (auto const& n: m_nodeEventHandler)
                events.push_back(std::make_pair(n,m_events[n]));
            m_nodeEventHandler.clear();
            m_events.clear();
        }
        for (auto const& e: events)
            processEvent(e.first, e.second);
    }

    /// Called by NodeTable to append event.
    virtual void appendEvent(NodeID _n, NodeTableEventType _e) { Guard l(x_events); m_nodeEventHandler.push_back(_n); m_events[_n] = _e; }

    Mutex x_events;
    std::list<NodeID> m_nodeEventHandler;
    std::unordered_map<NodeID, NodeTableEventType> m_events;
};

class NodeTable;
inline std::ostream& operator<<(std::ostream& _out, NodeTable const& _nodeTable);

struct NodeEntry;
struct DiscoveryDatagram;

/**
 * NodeTable using modified kademlia for node discovery and preference.
 * Node table requires an IO service, creates a socket for incoming
 * UDP messages and implements a kademlia-like protocol. Node requests and
 * responses are used to build a node table which can be queried to
 * obtain a list of potential nodes to connect to, and, passes events to
 * Host whenever a node is added or removed to/from the table.
 *
 * Thread-safety is ensured by modifying NodeEntry details via
 * shared_ptr replacement instead of mutating values.
 *
 * NodeTable accepts a port for UDP and will listen to the port on all available
 * interfaces.
 *
 * [Optimization]
 * @todo serialize evictions per-bucket
 * @todo store evictions in map, unit-test eviction logic
 * @todo store root node in table
 * @todo encapsulate discover into NetworkAlgorithm (task)
 * @todo expiration and sha3(id) 'to' for messages which are replies (prevents replay)
 * @todo cache Ping and FindSelf
 *
 * [Networking]
 * @todo eth/upnp/natpmp/stun/ice/etc for public-discovery
 * @todo firewall
 *
 * [Protocol]
 * @todo optimize knowledge at opposite edges; eg, s_bitsPerStep lookups. (Can be done via pointers to NodeBucket)
 * @todo ^ s_bitsPerStep = 8; // Denoted by b in [Kademlia]. Bits by which address space is divided.
 */
class NodeTable : UDPSocketEvents
{
    friend std::ostream& operator<<(std::ostream& _out, NodeTable const& _nodeTable);
    using NodeSocket = UDPSocket<NodeTable, 1280>;
    using TimePoint = std::chrono::steady_clock::time_point;	///< Steady time point.
    using NodeIdTimePoint = std::pair<NodeID, TimePoint>;

public:
    // Period during which we consider last PONG results to be valid before sending new PONG
    static constexpr uint32_t c_bondingTimeSeconds{12 * 60 * 60};

    /// Constructor requiring host for I/O, credentials, and IP Address, port to listen on 
    /// and host ENR.
    NodeTable(ba::io_context& _io, KeyPair const& _alias, NodeIPEndpoint const& _endpoint,
        ENR const& _enr, bool _enabled = true, bool _allowLocalDiscovery = false);

    /// Returns distance based on xor metric two node ids. Used by NodeEntry and NodeTable.
    static int distance(h256 const& _a, h256 const& _b)
    {
        u256 d = _a ^ _b;
        unsigned ret = 0;
        while (d >>= 1)
            ++ret;
        return ret;
    }

    void stop()
    {
        if (m_socket->isOpen())
        {
            cancelTimer(m_discoveryTimer);
            cancelTimer(m_timeoutsTimer);
            cancelTimer(m_endpointTrackingTimer);
            m_socket->disconnect();
        }
    }

    /// Set event handler for NodeEntryAdded and NodeEntryDropped events.
    void setEventHandler(NodeTableEventHandler* _handler) { m_nodeEventHandler.reset(_handler); }

    /// Called by implementation which provided handler to process NodeEntryAdded/NodeEntryDropped events. Events are coalesced by type whereby old events are ignored.
    void processEvents();

    /// Starts async node add to the node table by pinging it to trigger the endpoint proof.
    /// In case the node is already in the node table, pings only if the endpoint proof expired.
    ///
    /// @return True if the node is valid.
    bool addNode(Node const& _node);

    /// Add node to the list of all nodes and add it to the node table.
    /// In case the node's endpoint proof expired, pings it.
    /// In case the nodes is already in the node table, ignores add request.
    ///
    /// @return True if the node is valid.
    bool addKnownNode(
        Node const& _node, uint32_t _lastPongReceivedTime, uint32_t _lastPongSentTime);

    /// Returns list of node ids active in node table.
    std::list<NodeID> nodes() const;

    /// Returns node count.
    unsigned count() const
    {
        Guard l(x_nodes);
        return m_allNodes.size();
    }

    /// Returns snapshot of table.
    std::list<NodeEntry> snapshot() const;

    /// Returns true if node id is in node table.
    bool haveNode(NodeID const& _id)
    {
        Guard l(x_nodes);
        return m_allNodes.count(_id) > 0;
    }

    /// Returns the Node to the corresponding node id or the empty Node if that id is not found.
    Node node(NodeID const& _id);

    ENR hostENR() const
    {
        Guard l(m_hostENRMutex);
        return m_hostENR;
    }

    void runBackgroundTask(std::chrono::milliseconds const& _period,
        std::shared_ptr<ba::steady_timer> _timer, std::function<void()> _f);

    void cancelTimer(std::shared_ptr<ba::steady_timer> _timer);

    // protected only for derived classes in tests
protected:
    /**
     * NodeValidation is used to record information about the nodes that we have sent Ping to.
     */
    struct NodeValidation
    {
        // Public key of the target node
        NodeID nodeID;
        // We receive TCP port in the Ping packet, and store it here until it passes endpoint proof
        // (answers with Pong), then it will be added to the bucket of the node table
        uint16_t tcpPort = 0;
        // Time we sent Ping - used to handle timeouts
        TimePoint pingSentTime;
        // Hash of the sent Ping packet - used to validate received Pong
        h256 pingHash;
        // Replacement is put into the node table,
        // if original pinged node doesn't answer after timeout
        std::shared_ptr<NodeEntry> replacementNodeEntry;

        NodeValidation(NodeID const& _nodeID, uint16_t _tcpPort, TimePoint const& _pingSentTime,
            h256 const& _pingHash, std::shared_ptr<NodeEntry> _replacementNodeEntry)
          : nodeID{_nodeID},
            tcpPort{_tcpPort},
            pingSentTime{_pingSentTime},
            pingHash{_pingHash},
            replacementNodeEntry{std::move(_replacementNodeEntry)}
        {}
    };

    /// Constants for Kademlia, derived from address space.

    static constexpr unsigned s_addressByteSize = h256::size;					///< Size of address type in bytes.
    static constexpr unsigned s_bits = 8 * s_addressByteSize;					///< Denoted by n in [Kademlia].
    static constexpr unsigned s_bins = s_bits - 1;  ///< Size of m_buckets (excludes root, which is us).
    static constexpr unsigned s_maxSteps = boost::static_log2<s_bits>::value;	///< Max iterations of discovery. (discover)

    /// Chosen constants

    static constexpr unsigned s_bucketSize = 16;			///< Denoted by k in [Kademlia]. Number of nodes stored in each bucket.
    static constexpr unsigned s_alpha = 3;				///< Denoted by \alpha in [Kademlia]. Number of concurrent FindNode requests.

    /// Intervals

    /// How long to wait for requests (evict, find iterations).
    static constexpr std::chrono::milliseconds c_reqTimeoutMs{300};
    /// How long to wait before starting a new discovery round
    static constexpr std::chrono::milliseconds c_discoveryRoundIntervalMs{c_reqTimeoutMs * 2};
    /// Refresh interval prevents bucket from becoming stale. [Kademlia]
    static constexpr std::chrono::milliseconds c_bucketRefreshMs{7200};

    struct NodeBucket
    {
        unsigned distance;
        std::list<std::weak_ptr<NodeEntry>> nodes;
    };

    /// @return true if the node is valid to be added to the node table.
    /// (validates node ID and endpoint)
    bool isValidNode(Node const& _node) const;

    /// Used to ping a node to initiate the endpoint proof. Used when contacting neighbours if they
    /// don't have a valid endpoint proof (see doDiscoveryRound), refreshing buckets and as part of
    /// eviction process (see evict). Synchronous, has to be called only from the network thread.
    void ping(Node const& _node, std::shared_ptr<NodeEntry> _replacementNodeEntry = {});

    /// Schedules ping() method to be called from the network thread.
    /// Not synchronous - the ping operation is queued via a timer.
    void schedulePing(Node const& _node);

    /// Used by asynchronous operations to return NodeEntry which is active and managed by node table.
    std::shared_ptr<NodeEntry> nodeEntry(NodeID const& _id);

    /// Used to discovery nodes on network which are close to the given target.
    /// Sends s_alpha concurrent requests to nodes nearest to target, for nodes nearest to target, up to s_maxSteps rounds.
    void doDiscoveryRound(NodeID _target, unsigned _round,
        std::shared_ptr<std::set<std::shared_ptr<NodeEntry>>> _tried);

    /// Returns s_bucketSize nodes from node table which are closest to target.
    std::vector<std::shared_ptr<NodeEntry>> nearestNodeEntries(NodeID const& _target);

    /// Asynchronously drops _leastSeen node if it doesn't reply and adds _replacement node,
    /// otherwise _replacement is thrown away.
    void evict(NodeEntry const& _leastSeen, std::shared_ptr<NodeEntry> _replacement);

    /// Called whenever activity is received from a node in order to maintain node table. Only
    /// called for nodes for which we've completed an endpoint proof.
    void noteActiveNode(std::shared_ptr<NodeEntry> _nodeEntry);

    /// Used to drop node when timeout occurs or when evict() result is to keep previous node.
    void dropNode(std::shared_ptr<NodeEntry> _n);

    /// Returns references to bucket which corresponds to distance of node id.
    /// @warning Only use the return reference locked x_state mutex.
    // TODO p2p: Remove this method after removing offset-by-one functionality.
    NodeBucket& bucket_UNSAFE(NodeEntry const* _n);

    /// General Network Events

    /// Called by m_socket when packet is received.
    void onPacketReceived(
        UDPSocketFace*, bi::udp::endpoint const& _from, bytesConstRef _packet) override;

    std::shared_ptr<NodeEntry> handlePong(
        bi::udp::endpoint const& _from, DiscoveryDatagram const& _packet);
    std::shared_ptr<NodeEntry> handleNeighbours(
        bi::udp::endpoint const& _from, DiscoveryDatagram const& _packet);
    std::shared_ptr<NodeEntry> handleFindNode(
        bi::udp::endpoint const& _from, DiscoveryDatagram const& _packet);
    std::shared_ptr<NodeEntry> handlePingNode(
        bi::udp::endpoint const& _from, DiscoveryDatagram const& _packet);
    std::shared_ptr<NodeEntry> handleENRRequest(
        bi::udp::endpoint const& _from, DiscoveryDatagram const& _packet);
    std::shared_ptr<NodeEntry> handleENRResponse(
        bi::udp::endpoint const& _from, DiscoveryDatagram const& _packet);

    /// Called by m_socket when socket is disconnected.
    void onSocketDisconnected(UDPSocketFace*) override {}

    /// Tasks

    /// Looks up a random node at @c_bucketRefresh interval.
    void doDiscovery();

    /// Clear timed-out pings and drop nodes from the node table which haven't responded to ping and
    /// bring in their replacements
    void doHandleTimeouts();

    // Remove old records in m_endpointTracker.
    void doEndpointTracking();

    // Useful only for tests.
    void setRequestTimeToLive(std::chrono::seconds const& _time) { m_requestTimeToLive = _time; }
    uint32_t nextRequestExpirationTime() const
    {
        return RLPXDatagramFace::futureFromEpoch(m_requestTimeToLive);
    }

    /// Determines if a node with the supplied endpoint is allowed to participate in discovery.
    bool isAllowedEndpoint(NodeIPEndpoint const& _endpointToCheck) const
    {
        return dev::p2p::isAllowedEndpoint(m_allowLocalDiscovery, _endpointToCheck);
    }

    std::unique_ptr<NodeTableEventHandler> m_nodeEventHandler;		///< Event handler for node events.

    NodeID const m_hostNodeID;
    h256 const m_hostNodeIDHash;
    // Host IP address given to constructor
    bi::address const m_hostStaticIP;
    // Dynamically updated host endpoint
    NodeIPEndpoint m_hostNodeEndpoint;
    ENR m_hostENR;
    mutable Mutex m_hostENRMutex;
    Secret m_secret;												///< This nodes secret key.

    mutable Mutex x_nodes;											///< LOCK x_state first if both locks are required. Mutable for thread-safe copy in nodes() const.

    /// Node endpoints. Includes all nodes that were added into node table's buckets
    /// and have not been evicted yet.
    std::unordered_map<NodeID, std::shared_ptr<NodeEntry>> m_allNodes;

    mutable Mutex x_state;											///< LOCK x_state first if both x_nodes and x_state locks are required.

    /// State of p2p node network. Only includes nodes for which we've completed the endpoint proof
    std::array<NodeBucket, s_bins> m_buckets;

    std::list<NodeIdTimePoint> m_sentFindNodes;					///< Timeouts for FindNode requests.

    std::shared_ptr<NodeSocket> m_socket;							///< Shared pointer for our UDPSocket; ASIO requires shared_ptr.

    // The info about PING packets we've sent to other nodes and haven't received PONG yet
    std::map<bi::udp::endpoint, NodeValidation> m_sentPings;

    // Expiration time of sent discovery packets.
    std::chrono::seconds m_requestTimeToLive;

    mutable Logger m_logger{createLogger(VerbosityDebug, "discov")};

    bool m_allowLocalDiscovery;                                     ///< Allow nodes with local addresses to be included in the discovery process

    EndpointTracker m_endpointTracker;

    std::shared_ptr<ba::steady_timer> m_discoveryTimer;
    std::shared_ptr<ba::steady_timer> m_timeoutsTimer;
    std::shared_ptr<ba::steady_timer> m_endpointTrackingTimer;

    ba::io_context& m_io;
};

/**
 * NodeEntry
 * @brief Entry in Node Table
 */
struct NodeEntry
{
    NodeEntry(h256 const& _hostNodeIDHash, Public const& _pubk, NodeIPEndpoint const& _gw,
        uint32_t _pongReceivedTime, uint32_t _pongSentTime)
      : node(_pubk, _gw),
        nodeIDHash(sha3(_pubk)),
        distance(NodeTable::distance(_hostNodeIDHash, nodeIDHash)),
        lastPongReceivedTime(_pongReceivedTime),
        lastPongSentTime(_pongSentTime)
    {}
    bool hasValidEndpointProof() const
    {
        return RLPXDatagramFace::secondsSinceEpoch() <
               lastPongReceivedTime + NodeTable::c_bondingTimeSeconds;
    }
    NodeID const& id() const { return node.id; }
    NodeIPEndpoint const& endpoint() const { return node.endpoint; }
    PeerType peerType() const { return node.peerType; }

    Node node;
    h256 const nodeIDHash;
    int const distance = 0;  ///< Node's distance (xor of _hostNodeIDHash as integer).
    uint32_t lastPongReceivedTime = 0;
    uint32_t lastPongSentTime = 0;
};

inline std::ostream& operator<<(std::ostream& _out, NodeTable const& _nodeTable)
{
    _out << _nodeTable.m_hostNodeID << "\t"
         << "0\t" << _nodeTable.m_hostNodeEndpoint.address() << ":"
         << _nodeTable.m_hostNodeEndpoint.udpPort() << std::endl;
    auto s = _nodeTable.snapshot();
    for (auto n: s)
        _out << n.id() << "\t" << n.distance << "\t" << n.endpoint() << "\n";
    return _out;
}

struct DiscoveryDatagram: public RLPXDatagramFace
{
    static constexpr std::chrono::seconds c_timeToLiveS{60};

    /// Constructor used for sending.
    DiscoveryDatagram(bi::udp::endpoint const& _to)
      : RLPXDatagramFace(_to), expiration(futureFromEpoch(c_timeToLiveS))
    {}

    /// Constructor used for parsing inbound packets.
    DiscoveryDatagram(bi::udp::endpoint const& _from, NodeID const& _fromid, h256 const& _echo): RLPXDatagramFace(_from), sourceid(_fromid), echo(_echo) {}

    // These two are set for inbound packets only.
    NodeID sourceid; // sender public key (from signature)
    h256 echo;       // hash of encoded packet, for reply tracking

    // Most discovery packets carry a timestamp, which must be greater
    // than the current local time. This prevents replay attacks.
    // Optional because some packets (ENRResponse) don't have it
    boost::optional<uint32_t> expiration;
    bool isExpired() const
    {
        return expiration.is_initialized() && secondsSinceEpoch() > *expiration;
    }

    /// Decodes UDP packets.
    static std::unique_ptr<DiscoveryDatagram> interpretUDP(bi::udp::endpoint const& _from, bytesConstRef _packet);
};

/**
 * Ping packet: Sent to check if node is alive.
 * PingNode is cached and regenerated after timestamp + t, where t is timeout.
 *
 * Ping is used to implement evict. When a new node is seen for
 * a given bucket which is full, the least-responsive node is pinged.
 * If the pinged node doesn't respond, then it is removed and the new
 * node is inserted.
 */
struct PingNode: DiscoveryDatagram
{
    using DiscoveryDatagram::DiscoveryDatagram;
    PingNode(NodeIPEndpoint const& _src, NodeIPEndpoint const& _dest): DiscoveryDatagram(_dest), source(_src), destination(_dest) {}
    PingNode(bi::udp::endpoint const& _from, NodeID const& _fromid, h256 const& _echo): DiscoveryDatagram(_from, _fromid, _echo) {}

    static constexpr uint8_t type = 1;
    uint8_t packetType() const override { return type; }

    unsigned version = 0;
    NodeIPEndpoint source;
    NodeIPEndpoint destination;
    boost::optional<uint64_t> seq;

    void streamRLP(RLPStream& _s) const override
    {
        _s.appendList(seq.is_initialized() ? 5 : 4);
        _s << dev::p2p::c_protocolVersion;
        source.streamRLP(_s);
        destination.streamRLP(_s);
        _s << *expiration;
        if (seq.is_initialized())
            _s << *seq;
    }
    void interpretRLP(bytesConstRef _bytes) override
    {
        RLP r(_bytes, RLP::AllowNonCanon|RLP::ThrowOnFail);
        version = r[0].toInt<unsigned>();
        source.interpretRLP(r[1]);
        destination.interpretRLP(r[2]);
        expiration = r[3].toInt<uint32_t>();
        if (r.itemCount() > 4 && r[4].isInt())
            seq = r[4].toInt<uint64_t>();
    }

    std::string typeName() const override { return "Ping"; }
};

/**
 * Pong packet: Sent in response to ping
 */
struct Pong: DiscoveryDatagram
{
    Pong(NodeIPEndpoint const& _dest): DiscoveryDatagram((bi::udp::endpoint)_dest), destination(_dest) {}
    Pong(bi::udp::endpoint const& _from, NodeID const& _fromid, h256 const& _echo): DiscoveryDatagram(_from, _fromid, _echo) {}

    static constexpr uint8_t type = 2;
    uint8_t packetType() const override { return type; }

    NodeIPEndpoint destination;
    boost::optional<uint64_t> seq;

    void streamRLP(RLPStream& _s) const override
    {
        _s.appendList(seq.is_initialized() ? 4 : 3);
        destination.streamRLP(_s);
        _s << echo;
        _s << *expiration;
        if (seq.is_initialized())
            _s << *seq;
    }
    void interpretRLP(bytesConstRef _bytes) override
    {
        RLP r(_bytes, RLP::AllowNonCanon|RLP::ThrowOnFail);
        destination.interpretRLP(r[0]);
        echo = (h256)r[1];
        expiration = r[2].toInt<uint32_t>();
        if (r.itemCount() > 3 && r[3].isInt())
            seq = r[3].toInt<uint64_t>();
    }

    std::string typeName() const override { return "Pong"; }
};

/**
 * FindNode Packet: Request k-nodes, closest to the target.
 * FindNode is cached and regenerated after timestamp + t, where t is timeout.
 * FindNode implicitly results in finding neighbours of a given node.
 *
 * RLP Encoded Items: 2
 * Minimum Encoded Size: 21 bytes
 * Maximum Encoded Size: 30 bytes
 *
 * target: NodeID of node. The responding node will send back nodes closest to the target.
 *
 */
struct FindNode: DiscoveryDatagram
{
    FindNode(bi::udp::endpoint _to, h512 _target): DiscoveryDatagram(_to), target(_target) {}
    FindNode(bi::udp::endpoint const& _from, NodeID const& _fromid, h256 const& _echo): DiscoveryDatagram(_from, _fromid, _echo) {}

    static constexpr uint8_t type = 3;
    uint8_t packetType() const override { return type; }

    h512 target;

    void streamRLP(RLPStream& _s) const override
    {
        _s.appendList(2);
        _s << target << *expiration;
    }
    void interpretRLP(bytesConstRef _bytes) override
    {
        RLP r(_bytes, RLP::AllowNonCanon|RLP::ThrowOnFail);
        target = r[0].toHash<h512>();
        expiration = r[1].toInt<uint32_t>();
    }

    std::string typeName() const override { return "FindNode"; }
};

/**
 * Node Packet: One or more node packets are sent in response to FindNode.
 */
struct Neighbours: DiscoveryDatagram
{
    Neighbours(bi::udp::endpoint _to, std::vector<std::shared_ptr<NodeEntry>> const& _nearest, unsigned _offset = 0, unsigned _limit = 0): DiscoveryDatagram(_to)
    {
        auto limit = _limit ? std::min(_nearest.size(), (size_t)(_offset + _limit)) : _nearest.size();
        for (auto i = _offset; i < limit; i++)
            neighbours.push_back(Neighbour(_nearest[i]->node));
    }
    Neighbours(bi::udp::endpoint const& _to): DiscoveryDatagram(_to) {}
    Neighbours(bi::udp::endpoint const& _from, NodeID const& _fromid, h256 const& _echo): DiscoveryDatagram(_from, _fromid, _echo) {}

    struct Neighbour
    {
        Neighbour(Node const& _node): endpoint(_node.endpoint), node(_node.id) {}
        Neighbour(RLP const& _r): endpoint(_r) { node = h512(_r[3].toBytes()); }
        NodeIPEndpoint endpoint;
        NodeID node;
        void streamRLP(RLPStream& _s) const { _s.appendList(4); endpoint.streamRLP(_s, NodeIPEndpoint::StreamInline); _s << node; }
    };

    static constexpr uint8_t type = 4;
    uint8_t packetType() const override { return type; }

    std::vector<Neighbour> neighbours;

    void streamRLP(RLPStream& _s) const override
    {
        _s.appendList(2);
        _s.appendList(neighbours.size());
        for (auto const& n: neighbours)
            n.streamRLP(_s);
        _s << *expiration;
    }
    void interpretRLP(bytesConstRef _bytes) override
    {
        RLP r(_bytes, RLP::AllowNonCanon|RLP::ThrowOnFail);
        for (auto const& n: r[0])
            neighbours.emplace_back(n);
        expiration = r[1].toInt<uint32_t>();
    }

    std::string typeName() const override { return "Neighbours"; }
};

struct ENRRequest : DiscoveryDatagram
{
    // Constructor for outgoing packets
    ENRRequest(bi::udp::endpoint const& _to) : DiscoveryDatagram{_to} {}
    // Constructor for incoming packets
    ENRRequest(bi::udp::endpoint const& _from, NodeID const& _fromID, h256 const& _echo)
      : DiscoveryDatagram{_from, _fromID, _echo}
    {}

    static constexpr uint8_t type = 5;
    uint8_t packetType() const override { return type; }

    void streamRLP(RLPStream& _s) const override
    {
        _s.appendList(1);
        _s << *expiration;
    }
    void interpretRLP(bytesConstRef _bytes) override
    {
        RLP r(_bytes, RLP::AllowNonCanon | RLP::ThrowOnFail);
        expiration = r[0].toInt<uint32_t>();
    }

    std::string typeName() const override { return "ENRRequest"; }
};

struct ENRResponse : DiscoveryDatagram
{
    // Constructor for outgoing packets
    ENRResponse(bi::udp::endpoint const& _dest, ENR const& _enr)
      : DiscoveryDatagram{_dest}, enr{new ENR{_enr}}
    {}
    // Constructor for incoming packets
    ENRResponse(bi::udp::endpoint const& _from, NodeID const& _fromID, h256 const& _echo)
      : DiscoveryDatagram{_from, _fromID, _echo}
    {}

    static constexpr uint8_t type = 6;
    uint8_t packetType() const override { return type; }

    std::unique_ptr<ENR> enr;

    void streamRLP(RLPStream& _s) const override
    {
        _s.appendList(2);
        _s << echo;
        enr->streamRLP(_s);
    }
    void interpretRLP(bytesConstRef _bytes) override
    {
        RLP r(_bytes, RLP::AllowNonCanon | RLP::ThrowOnFail);
        echo = (h256)r[0];
        enr.reset(new ENR{IdentitySchemeV4::parseENR(r[1])});
    }

    std::string typeName() const override { return "ENRResponse"; }
};
}
}
