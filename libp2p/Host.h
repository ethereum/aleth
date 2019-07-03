// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include "Common.h"
#include "ENR.h"
#include "Network.h"
#include "NodeTable.h"
#include "Peer.h"
#include "RLPXSocket.h"
#include <libdevcore/Guards.h>
#include <libdevcore/Worker.h>
#include <libdevcrypto/Common.h>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <utility>
#include <vector>
namespace io = boost::asio;
namespace bi = io::ip;

namespace std
{
template<> struct hash<pair<dev::p2p::NodeID, string>>
{
    size_t operator()(pair<dev::p2p::NodeID, string> const& _value) const
    {
        size_t ret = hash<dev::p2p::NodeID>()(_value.first);
        return ret ^ (hash<string>()(_value.second) + 0x9e3779b9 + (ret << 6) + (ret >> 2));
    }
};
}

namespace dev
{

namespace p2p
{
class CapabilityFace;
class CapabilityHostFace;
class Host;
class SessionFace;
class RLPXFrameCoder;
class RLPXHandshake;

class HostNodeTableHandler: public NodeTableEventHandler
{
public:
    HostNodeTableHandler(Host& _host);

    Host const& host() const { return m_host; }

private:
    virtual void processEvent(NodeID const& _n, NodeTableEventType const& _e);

    Host& m_host;
};

struct SubReputation
{
    bool isRude = false;
    int utility = 0;
    bytes data;
};

struct Reputation
{
    std::unordered_map<std::string, SubReputation> subs;
};

class ReputationManager
{
public:
    void noteRude(SessionFace const& _s, std::string const& _sub = std::string());
    bool isRude(SessionFace const& _s, std::string const& _sub = std::string()) const;
    void setData(SessionFace const& _s, std::string const& _sub, bytes const& _data);
    bytes data(SessionFace const& _s, std::string const& _subs) const;

private:
    std::unordered_map<std::pair<p2p::NodeID, std::string>, Reputation> m_nodes;	///< Nodes that were impolite while syncing. We avoid syncing from these if possible.
    SharedMutex mutable x_nodes;
};

struct NodeInfo
{
    NodeInfo() = default;
    NodeInfo(NodeID const& _id, std::string const& _address, unsigned _port, std::string const& _version):
        id(_id), address(_address), port(_port), version(_version) {}

    std::string enode() const { return "enode://" + id.hex() + "@" + address + ":" + toString(port); }

    NodeID id;
    std::string address;
    unsigned port;
    std::string version;
};

/**
 * @brief The Host class
 * Capabilities should be registered prior to startNetwork, since m_capabilities is not thread-safe.
 *
 * @todo determinePublic: ipv6, udp
 * @todo per-session keepalive/ping instead of broadcast; set ping-timeout via median-latency
 */
class Host: public Worker
{
    friend class HostNodeTableHandler;
    friend class RLPXHandshake;
    
    friend class Session;

public:
    /// Start server, listening for connections on the given port.
    Host(
        std::string const& _clientVersion,
        NetworkConfig const& _n = NetworkConfig{},
        bytesConstRef _restoreNetwork = bytesConstRef()
    );

    /// Alternative constructor that allows providing the node key directly
    /// without restoring the network.
    Host(std::string const& _clientVersion, std::pair<Secret, ENR> const& _secretAndENR,
        NetworkConfig const& _n = NetworkConfig{});

    /// Will block on network process events.
    virtual ~Host();

    /// Register a host capability; all new peer connections will see this capability.
    void registerCapability(std::shared_ptr<CapabilityFace> const& _cap);

    /// Register a host capability with arbitrary name and version.
    /// Might be useful when you want to handle several subprotocol versions with a single
    /// capability class.
    void registerCapability(std::shared_ptr<CapabilityFace> const& _cap, std::string const& _name,
        unsigned _version);

    bool haveCapability(CapDesc const& _name) const { return m_capabilities.count(_name) != 0; }
    bool haveCapabilities() const { return !caps().empty(); }
    CapDescs caps() const { CapDescs ret; for (auto const& i: m_capabilities) ret.push_back(i.first); return ret; }

    /// Add a potential peer.
    void addPeer(NodeSpec const& _s, PeerType _t);

    /// Add node as a peer candidate. Node is added if discovery ping is successful and table has capacity.
    void addNode(NodeID const& _node, NodeIPEndpoint const& _endpoint);
    
    /// Create Peer and attempt keeping peer connected.
    void requirePeer(NodeID const& _node, NodeIPEndpoint const& _endpoint);

    /// Create Peer and attempt keeping peer connected.
    void requirePeer(NodeID const& _node, bi::address const& _addr, unsigned short _udpPort, unsigned short _tcpPort) { requirePeer(_node, NodeIPEndpoint(_addr, _udpPort, _tcpPort)); }

    /// returns true if a member of m_requiredPeers
    bool isRequiredPeer(NodeID const&) const;

    /// Note peer as no longer being required.
    void relinquishPeer(NodeID const& _node);
    
    /// Set ideal number of peers.
    void setIdealPeerCount(unsigned _n) { m_idealPeerCount = _n; }

    /// Set multipier for max accepted connections.
    void setPeerStretch(unsigned _n) { m_stretchPeers = _n; }
    
    /// Get peer information.
    PeerSessionInfos peerSessionInfos() const;

    /// Get number of peers connected.
    size_t peerCount() const;

    /// Get the address we're listening on currently.
    std::string listenAddress() const { return m_tcpPublic.address().is_unspecified() ? "0.0.0.0" : m_tcpPublic.address().to_string(); }

    /// Get the port we're listening on currently.
    unsigned short listenPort() const { return std::max(0, m_listenPort.load()); }

    /// Serialise the set of known peers.
    bytes saveNetwork() const;

    // TODO: P2P this should be combined with peers into a HostStat object of some kind; coalesce data, as it's only used for status information.
    Peers getPeers() const { RecursiveGuard l(x_sessions); Peers ret; for (auto const& i: m_peers) ret.push_back(*i.second); return ret; }

    NetworkConfig const& networkConfig() const { return m_netConfig; }

    /// Start network. @threadsafe
    void start();

    /// Stop network. @threadsafe
    /// Resets acceptor, socket, and IO service. Called by deallocator.
    void stop();

    /// @returns if network has been started.
    bool isStarted() const { return isWorking(); }

    /// @returns our reputation manager.
    ReputationManager& repMan() { return m_repMan; }

    /// @returns if network is started and interactive.
    bool haveNetwork() const { return m_run; }
    
    /// Validates and starts peer session, taking ownership of _io. Disconnects and returns false upon error.
    void startPeerSession(Public const& _id, RLP const& _hello,
        std::unique_ptr<RLPXFrameCoder>&& _io, std::shared_ptr<RLPXSocket> const& _s);

    /// Get session by id
    std::shared_ptr<SessionFace> peerSession(NodeID const& _id) const;

    /// Set a handshake failure reason for a peer
    void onHandshakeFailed(NodeID const& _n, HandshakeFailureReason _r);

    /// Get our current node ID.
    NodeID id() const { return m_alias.pub(); }

    /// Get the public TCP endpoint.
    bi::tcp::endpoint const& tcpPublic() const { return m_tcpPublic; }

    /// Get the endpoint information.
    std::string enode() const
    {
        std::string address;
        if (!m_netConfig.publicIPAddress.empty())
            address = m_netConfig.publicIPAddress;
        else if (!m_tcpPublic.address().is_unspecified())
            address = m_tcpPublic.address().to_string();
        else
            address = c_localhostIp;

        std::string port;
        if (m_tcpPublic.port())
            port = toString(m_tcpPublic.port());
        else
            port = toString(m_netConfig.listenPort);

        return "enode://" + id().hex() + "@" + address + ":" + port;
    }

    /// Get the node information.
    p2p::NodeInfo nodeInfo() const { return NodeInfo(id(), (networkConfig().publicIPAddress.empty() ? m_tcpPublic.address().to_string() : networkConfig().publicIPAddress), m_tcpPublic.port(), m_clientVersion); }

    /// Get Ethereum Node Record of the host
    ENR enr() const
    {
        Guard l(x_nodeTable);
        return m_nodeTable ? m_nodeTable->hostENR() : m_restoredENR;
    }

    /// Apply function to each session
    void forEachPeer(
        std::string const& _capabilityName, std::function<bool(NodeID const&)> _f) const;

    /// Execute work on the network thread
    void postWork(std::function<void()> _f) { post(m_ioContext, std::move(_f)); }

    std::shared_ptr<CapabilityHostFace> capabilityHost() const { return m_capabilityHost; }

protected:
    /*
     * Used by the host to run a capability's background work loop
     */
    struct CapabilityRuntime
    {
        std::shared_ptr<CapabilityFace> capability;
        std::shared_ptr<ba::steady_timer> backgroundWorkTimer;
    };

    void onNodeTableEvent(NodeID const& _n, NodeTableEventType const& _e);

    /// Deserialise the data and populate the set of known peers.
    void restoreNetwork(bytesConstRef _b);

private:
    enum PeerSlotType { Egress, Ingress };
    
    unsigned peerSlots(PeerSlotType _type) { return _type == Egress ? m_idealPeerCount : m_idealPeerCount * m_stretchPeers; }
    
    bool havePeerSession(NodeID const& _id) { return !!peerSession(_id); }

    bool isHandshaking(NodeID const& _id) const;

    /// Determines publicly advertised address.
    bi::tcp::endpoint determinePublic() const;

    ENR updateENR(
        ENR const& _restoredENR, bi::tcp::endpoint const& _tcpPublic, uint16_t const& _listenPort);

    void connect(std::shared_ptr<Peer> const& _p);

    /// Returns true if pending and connected peer count is less than maximum
    bool peerSlotsAvailable(PeerSlotType _type = Ingress);
    
    /// Ping the peers to update the latency information and disconnect peers which have timed out.
    void keepAlivePeers();

    /// Log count of active peers and information about each peer
    void logActivePeers();

    /// Disconnect peers which didn't respond to keepAlivePeers ping prior to c_keepAliveTimeOut.
    void disconnectLatePeers();

    /// Called only from startedWorking().
    void runAcceptor();

    /// Called by Worker. Not thread-safe; to be called only by worker.
    virtual void startedWorking();
    /// Called by startedWorking. Not thread-safe; to be called only be Worker.
    void run(boost::system::error_code const& error);			///< Run network. Called serially via ASIO deadline timer. Manages connection state transitions.

    /// Run network. Not thread-safe; to be called only by worker.
    virtual void doWork();

    /// Shutdown network. Not thread-safe; to be called only by worker.
    virtual void doneWorking();

    /// Get or create host's Ethereum Node record.
    std::pair<Secret, ENR> restoreENR(bytesConstRef _b, NetworkConfig const& _networkConfig);

    bool nodeTableHasNode(Public const& _id) const;
    Node nodeFromNodeTable(Public const& _id) const;
    bool addNodeToNodeTable(Node const& _node);

    bool addKnownNodeToNodeTable(
        Node const& _node, uint32_t _lastPongReceivedTime, uint32_t _lastPongSentTime);

    /// Determines if a node with the supplied endpoint should be included in or restored from the
    /// serialized network configuration data
    bool isAllowedEndpoint(NodeIPEndpoint const& _endpointToCheck) const
    {
        return dev::p2p::isAllowedEndpoint(m_netConfig.allowLocalDiscovery, _endpointToCheck);
    }

    /// Start registered capabilities, typically done on network start
    void startCapabilities();

    /// Schedule's a capability's work loop on the network thread
    void scheduleCapabilityWorkLoop(CapabilityFace& _cap, std::shared_ptr<ba::steady_timer> _timer);

    /// Stop registered capabilities, typically done when the network is being shut down.
    void stopCapabilities();

    std::shared_ptr<Peer> peer(NodeID const& _n) const;

    bytes m_restoreNetwork;										///< Set by constructor and used to set Host key and restore network peers & nodes.

    std::atomic<bool> m_run{false};													///< Whether network is running.

    std::string m_clientVersion;											///< Our version string.

    NetworkConfig m_netConfig;										        ///< Network settings.

    /// Interface addresses (private, public)
    std::set<bi::address> m_ifAddresses;								///< Interface addresses.

    std::atomic<int> m_listenPort{-1};												///< What port are we listening on. -1 means binding failed or acceptor hasn't been initialized.

    io::io_context m_ioContext;
    bi::tcp::acceptor m_tcp4Acceptor;										///< Listening acceptor.

    /// Timer which, when network is running, calls run() every c_timerInterval ms.
    ba::steady_timer m_runTimer;

    std::set<Peer*> m_pendingPeerConns;									/// Used only by connect(Peer&) to limit concurrently connecting to same node. See connect(shared_ptr<Peer>const&).

    bi::tcp::endpoint m_tcpPublic;											///< Our public listening endpoint.
    /// Alias for network communication.
    KeyPair m_alias;
    /// Host's Ethereum Node Record restored from network.rlp
    ENR const m_restoredENR;
    std::shared_ptr<NodeTable> m_nodeTable;									///< Node table (uses kademlia-like discovery).
    mutable std::mutex x_nodeTable;
    std::shared_ptr<NodeTable> nodeTable() const { Guard l(x_nodeTable); return m_nodeTable; }

    /// Shared storage of Peer objects. Peers are created or destroyed on demand by the Host. Active sessions maintain a shared_ptr to a Peer;
    std::unordered_map<NodeID, std::shared_ptr<Peer>> m_peers;
    
    /// Peers we try to connect regardless of p2p network.
    std::set<NodeID> m_requiredPeers;
    mutable Mutex x_requiredPeers;

    /// The nodes to which we are currently connected. Used by host to service peer requests and keepAlivePeers and for shutdown. (see run())
    /// Mutable because we flush zombie entries (null-weakptrs) as regular maintenance from a const method.
    mutable std::unordered_map<NodeID, std::weak_ptr<SessionFace>> m_sessions;
    mutable RecursiveMutex x_sessions;

    /// Pending connections. Completed handshakes are garbage-collected in run() (a handshake is
    /// complete when there are no more shared_ptrs in handlers)
    std::list<std::weak_ptr<RLPXHandshake>> m_connecting;
    mutable Mutex x_connecting;													///< Mutex for m_connecting.

    unsigned m_idealPeerCount = 11;										///< Ideal number of peers to be connected to.
    unsigned m_stretchPeers = 7;										///< Accepted connection multiplier (max peers = ideal*stretch).

    /// Each of the capabilities we support. CapabilityRuntime is used to run a capability's
    /// background work loop
    std::map<CapDesc, CapabilityRuntime> m_capabilities;

    std::chrono::steady_clock::time_point m_lastPing;						///< Time we sent the last ping to all peers.
    bool m_accepting = false;

    ReputationManager m_repMan;

    std::shared_ptr<CapabilityHostFace> m_capabilityHost;

    /// When the last "active peers" message was logged - used to throttle
    /// logging to once every c_logActivePeersInterval seconds
    std::chrono::steady_clock::time_point m_lastPeerLogMessage;

    mutable Logger m_logger{createLogger(VerbosityDebug, "net")};
    Logger m_detailsLogger{createLogger(VerbosityTrace, "net")};
    Logger m_infoLogger{createLogger(VerbosityInfo, "net")};
};

}
}
