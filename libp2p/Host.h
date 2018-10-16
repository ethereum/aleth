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
/** @file Host.h
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include "Common.h"
#include "Network.h"
#include "NodeTable.h"
#include "Peer.h"
#include "RLPXFrameCoder.h"
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
namespace ba = boost::asio;
namespace bi = ba::ip;

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
class CapabilityHostFace;
class Host;
class HostCapabilityFace;
class SessionFace;

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
    ReputationManager();

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
    Host(
        std::string const& _clientVersion,
        KeyPair const& _alias,
        NetworkConfig const& _n = NetworkConfig{}
    );

    /// Will block on network process events.
    virtual ~Host();

    /// Default hosts for current version of client.
    static std::unordered_map<Public, std::string> pocHosts();

    /// Register a host capability; all new peer connections will see this capability.
    void registerCapability(std::shared_ptr<HostCapabilityFace> const& _cap);

    /// Register a host capability with arbitrary name and version.
    /// Might be useful when you want to handle several subprotocol versions with a single
    /// capability class.
    void registerCapability(std::shared_ptr<HostCapabilityFace> const& _cap,
        std::string const& _name, u256 const& _version);

    bool haveCapability(CapDesc const& _name) const { return m_capabilities.count(_name) != 0; }
    CapDescs caps() const { CapDescs ret; for (auto const& i: m_capabilities) ret.push_back(i.first); return ret; }

    /// Add a potential peer.
    void addPeer(NodeSpec const& _s, PeerType _t);

    /// Add node as a peer candidate. Node is added if discovery ping is successful and table has capacity.
    void addNode(NodeID const& _node, NodeIPEndpoint const& _endpoint);
    
    /// Create Peer and attempt keeping peer connected.
    void requirePeer(NodeID const& _node, NodeIPEndpoint const& _endpoint);

    /// Create Peer and attempt keeping peer connected.
    void requirePeer(NodeID const& _node, bi::address const& _addr, unsigned short _udpPort, unsigned short _tcpPort) { requirePeer(_node, NodeIPEndpoint(_addr, _udpPort, _tcpPort)); }

    /// Note peer as no longer being required.
    void relinquishPeer(NodeID const& _node);
    
    /// Set ideal number of peers.
    void setIdealPeerCount(unsigned _n) { m_idealPeerCount = _n; }

    /// Set multipier for max accepted connections.
    void setPeerStretch(unsigned _n) { m_stretchPeers = _n; }
    
    /// Get peer information.
    PeerSessionInfos peerSessionInfo() const;

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

    void setNetworkConfig(NetworkConfig const& _p, bool _dropPeers = false) { m_dropPeers = _dropPeers; auto had = isStarted(); if (had) stop(); m_netConfig = _p; if (had) start(); }

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
    bool haveNetwork() const { Guard l(x_runTimer); Guard ll(x_nodeTable); return m_run && !!m_nodeTable; }
    
    /// Validates and starts peer session, taking ownership of _io. Disconnects and returns false upon error.
    void startPeerSession(Public const& _id, RLP const& _hello, std::unique_ptr<RLPXFrameCoder>&& _io, std::shared_ptr<RLPXSocket> const& _s);

    /// Get session by id
    std::shared_ptr<SessionFace> peerSession(NodeID const& _id) const
    {
        RecursiveGuard l(x_sessions);
        return m_sessions.count(_id) ? m_sessions[_id].lock() : std::shared_ptr<SessionFace>();
    }

    /// Get our current node ID.
    NodeID id() const { return m_alias.pub(); }

    /// Get the public TCP endpoint.
    bi::tcp::endpoint const& tcpPublic() const { return m_tcpPublic; }

    /// Get the public endpoint information.
    std::string enode() const { return "enode://" + id().hex() + "@" + (networkConfig().publicIPAddress.empty() ? m_tcpPublic.address().to_string() : networkConfig().publicIPAddress) + ":" + toString(m_tcpPublic.port()); }

    /// Get the node information.
    p2p::NodeInfo nodeInfo() const { return NodeInfo(id(), (networkConfig().publicIPAddress.empty() ? m_tcpPublic.address().to_string() : networkConfig().publicIPAddress), m_tcpPublic.port(), m_clientVersion); }

    /// Apply function to each session
    void forEachPeer(std::string const& _name, u256 const& _version,
        std::function<bool(NodeID const&)> _f) const;

    std::shared_ptr<CapabilityHostFace> capabilityHost() const { return m_capabilityHost; }

protected:
    void onNodeTableEvent(NodeID const& _n, NodeTableEventType const& _e);

    /// Deserialise the data and populate the set of known peers.
    void restoreNetwork(bytesConstRef _b);

private:
    enum PeerSlotType { Egress, Ingress };
    
    unsigned peerSlots(PeerSlotType _type) { return _type == Egress ? m_idealPeerCount : m_idealPeerCount * m_stretchPeers; }
    
    bool havePeerSession(NodeID const& _id) { return !!peerSession(_id); }

    /// Determines and sets m_tcpPublic to publicly advertised address.
    void determinePublic();

    void connect(std::shared_ptr<Peer> const& _p);

    /// Returns true if pending and connected peer count is less than maximum
    bool peerSlotsAvailable(PeerSlotType _type = Ingress);
    
    /// Ping the peers to update the latency information and disconnect peers which have timed out.
    void keepAlivePeers();

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

    /// Get or create host identifier (KeyPair).
    static KeyPair networkAlias(bytesConstRef _b);

    /// returns true if a member of m_requiredPeers
    bool isRequiredPeer(NodeID const&) const;

    bool nodeTableHasNode(Public const& _id) const;
    Node nodeFromNodeTable(Public const& _id) const;
    bool addNodeToNodeTable(Node const& _node, NodeTable::NodeRelation _relation = NodeTable::NodeRelation::Unknown);

    bytes m_restoreNetwork;										///< Set by constructor and used to set Host key and restore network peers & nodes.

    std::atomic<bool> m_run{false};													///< Whether network is running.

    std::string m_clientVersion;											///< Our version string.

    NetworkConfig m_netConfig;										        ///< Network settings.

    /// Interface addresses (private, public)
    std::set<bi::address> m_ifAddresses;								///< Interface addresses.

    std::atomic<int> m_listenPort{-1};												///< What port are we listening on. -1 means binding failed or acceptor hasn't been initialized.

    ba::io_service m_ioService;											///< IOService for network stuff.
    bi::tcp::acceptor m_tcp4Acceptor;										///< Listening acceptor.

    std::unique_ptr<boost::asio::deadline_timer> m_timer;					///< Timer which, when network is running, calls scheduler() every c_timerInterval ms.
    mutable std::mutex x_runTimer;	///< Start/stop mutex.
    static const unsigned c_timerInterval = 100;							///< Interval which m_timer is run when network is connected.
    std::condition_variable m_timerReset;

    std::set<Peer*> m_pendingPeerConns;									/// Used only by connect(Peer&) to limit concurrently connecting to same node. See connect(shared_ptr<Peer>const&).

    bi::tcp::endpoint m_tcpPublic;											///< Our public listening endpoint.
    KeyPair m_alias;															///< Alias for network communication. Network address is k*G. k is key material. TODO: Replace KeyPair.
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
    
    std::list<std::weak_ptr<RLPXHandshake>> m_connecting;					///< Pending connections.
    Mutex x_connecting;													///< Mutex for m_connecting.

    unsigned m_idealPeerCount = 11;										///< Ideal number of peers to be connected to.
    unsigned m_stretchPeers = 7;										///< Accepted connection multiplier (max peers = ideal*stretch).

    std::map<CapDesc, std::shared_ptr<HostCapabilityFace>> m_capabilities;	///< Each of the capabilities we support.
    
    /// Deadline timers used for isolated network events. GC'd by run.
    std::list<std::shared_ptr<boost::asio::deadline_timer>> m_timers;
    Mutex x_timers;

    std::chrono::steady_clock::time_point m_lastPing;						///< Time we sent the last ping to all peers.
    bool m_accepting = false;
    bool m_dropPeers = false;

    ReputationManager m_repMan;

    std::shared_ptr<CapabilityHostFace> m_capabilityHost;

    Logger m_logger{createLogger(VerbosityDebug, "net")};
};

}
}
