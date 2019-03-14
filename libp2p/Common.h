// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

//
// Miscellanea required for the Host/Session/NodeTable classes.
//

#pragma once

#include <atomic>
#include <string>
#include <set>
#include <vector>

// Make sure boost/asio.hpp is included before windows.h.
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>

#include <chrono>
#include <libdevcrypto/Common.h>
#include <libdevcore/Log.h>
#include <libdevcore/Exceptions.h>
#include <libdevcore/RLP.h>
#include <libdevcore/Guards.h>
namespace ba = boost::asio;
namespace bi = boost::asio::ip;

namespace dev
{

class RLP;
class RLPStream;

namespace p2p
{

/// Peer network protocol version.
constexpr unsigned c_protocolVersion = 4;

class NodeIPEndpoint;
class Node;
extern const NodeIPEndpoint UnspecifiedNodeIPEndpoint;
extern const Node UnspecifiedNode;

using NodeID = h512;

bool isPrivateAddress(bi::address const& _addressToCheck);
bool isPrivateAddress(std::string const& _addressToCheck);
bool isLocalHostAddress(bi::address const& _addressToCheck);
bool isLocalHostAddress(std::string const& _addressToCheck);
bool isPublicAddress(bi::address const& _addressToCheck);
bool isPublicAddress(std::string const& _addressToCheck);
bool isAllowedAddress(bool _allowLocalDiscovery, bi::address const& _addressToCheck);
bool isAllowedEndpoint(bool _allowLocalDiscovery, NodeIPEndpoint const& _endpointToCheck);

class UPnP;
class Host;
class Session;

struct NetworkStartRequired: virtual dev::Exception {};
struct InvalidPublicIPAddress: virtual dev::Exception {};
struct NetworkRestartNotSupported : virtual dev::Exception {};

/// The ECDHE agreement failed during RLPx handshake.
struct ECDHEError: virtual Exception {};

#define NET_GLOBAL_LOGGER(NAME, SEVERITY)                      \
    BOOST_LOG_INLINE_GLOBAL_LOGGER_CTOR_ARGS(g_##NAME##Logger, \
        boost::log::sources::severity_channel_logger_mt<>,     \
        (boost::log::keywords::severity = SEVERITY)(boost::log::keywords::channel = "net"))

NET_GLOBAL_LOGGER(netnote, VerbosityInfo)
#define cnetnote LOG(dev::p2p::g_netnoteLogger::get())
NET_GLOBAL_LOGGER(netlog, VerbosityDebug)
#define cnetlog LOG(dev::p2p::g_netlogLogger::get())
NET_GLOBAL_LOGGER(netdetails, VerbosityTrace)
#define cnetdetails LOG(dev::p2p::g_netdetailsLogger::get())

enum PacketType
{
    HelloPacket = 0,
    DisconnectPacket,
    PingPacket,
    PongPacket,
    GetPeersPacket,
    PeersPacket,
    UserPacket = 0x10
};

enum DisconnectReason
{
    DisconnectRequested = 0,
    TCPError,
    BadProtocol,
    UselessPeer,
    TooManyPeers,
    DuplicatePeer,
    IncompatibleProtocol,
    NullIdentity,
    ClientQuit,
    UnexpectedIdentity,
    LocalIdentity,
    PingTimeout,
    UserReason = 0x10,
    NoDisconnect = 0xffff
};

/// @returns the string form of the given disconnection reason.
std::string reasonOf(DisconnectReason _r);

using CapDesc = std::pair<std::string, unsigned>;
using CapDescSet = std::set<CapDesc>;
using CapDescs = std::vector<CapDesc>;

/*
 * Used by Host to pass negotiated information about a connection to a
 * new Peer Session; PeerSessionInfo is then maintained by Session and can
 * be queried for point-in-time status information via Host.
 */
struct PeerSessionInfo
{
    NodeID const id;
    std::string const clientVersion;
    std::string const host;
    unsigned short const port;
    std::chrono::steady_clock::duration lastPing;
    std::set<CapDesc> const caps;
    std::map<std::string, std::string> notes;
};

using PeerSessionInfos = std::vector<PeerSessionInfo>;

enum class PeerType
{
    Optional,
    Required
};

/**
 * @brief IPv4,UDP/TCP endpoints.
 */
class NodeIPEndpoint
{
public:
    enum RLPAppend
    {
        StreamList,
        StreamInline
    };

    NodeIPEndpoint() = default;
    NodeIPEndpoint(bi::address _addr, uint16_t _udp, uint16_t _tcp)
      : m_address(std::move(_addr)), m_udpPort(_udp), m_tcpPort(_tcp)
    {}
    explicit NodeIPEndpoint(RLP const& _r) { interpretRLP(_r); }

    operator bi::udp::endpoint() const { return {m_address, m_udpPort}; }
    operator bi::tcp::endpoint() const { return {m_address, m_tcpPort}; }

    explicit operator bool() const
    {
        return !m_address.is_unspecified() && m_udpPort > 0 && m_tcpPort > 0;
    }

    bool operator==(NodeIPEndpoint const& _cmp) const {
        return m_address == _cmp.m_address && m_udpPort == _cmp.m_udpPort &&
               m_tcpPort == _cmp.m_tcpPort;
    }
    bool operator!=(NodeIPEndpoint const& _cmp) const {
        return !operator==(_cmp);
    }
    
    void streamRLP(RLPStream& _s, RLPAppend _append = StreamList) const;
    void interpretRLP(RLP const& _r);

    bi::address address() const { return m_address; }

    void setAddress(bi::address _addr) { m_address = std::move(_addr); }

    uint16_t udpPort() const { return m_udpPort; }

    void setUdpPort(uint16_t _udp) { m_udpPort = _udp; }

    uint16_t tcpPort() const { return m_tcpPort; }

    void setTcpPort(uint16_t _tcp) { m_tcpPort = _tcp; }

private:
    bi::address m_address;
    uint16_t m_udpPort = 0;
    uint16_t m_tcpPort = 0;
};

struct NodeSpec
{
    NodeSpec() {}

    /// Accepts user-readable strings in the form defined here: https://github.com/ethereum/wiki/wiki/enode-url-format
    NodeSpec(std::string const& _user);

    NodeSpec(std::string const& _addr, uint16_t _port, int _udpPort = -1):
        m_address(_addr),
        m_tcpPort(_port),
        m_udpPort(_udpPort == -1 ? _port : (uint16_t)_udpPort)
    {}

    NodeID id() const { return m_id; }

    NodeIPEndpoint nodeIPEndpoint() const;

    std::string enode() const;

    bool isValid() const;

private:
    std::string m_address;
    uint16_t m_tcpPort = 0;
    uint16_t m_udpPort = 0;
    NodeID m_id;
};

class Node
{
public:
    Node() = default;
    virtual ~Node() = default;
    Node(Node const&);
    Node(Public _publicKey, NodeIPEndpoint const& _ip, PeerType _peerType = PeerType::Optional): id(_publicKey), endpoint(_ip), peerType(_peerType) {}
    Node(NodeSpec const& _s, PeerType _peerType = PeerType::Optional);

    NodeID const& address() const { return id; }

    explicit operator bool() const { return static_cast<bool>(id); }

    // TODO: make private, give accessors and rename m_...
    NodeID id;

    /// Endpoints by which we expect to reach node.
    // TODO: make private, give accessors and rename m_...
    NodeIPEndpoint endpoint;

    // TODO: p2p implement
    std::atomic<PeerType> peerType{PeerType::Optional};
};

std::ostream& operator<<(std::ostream& _out, const Node& _node);

/// Simple stream output for a NodeIPEndpoint.
std::ostream& operator<<(std::ostream& _out, NodeIPEndpoint const& _ep);

/// Official Ethereum boot nodes
std::vector<std::pair<Public, const char*>> defaultBootNodes();
}
}

/// std::hash for asio::adress
namespace std
{

template <> struct hash<bi::address>
{
    size_t operator()(bi::address const& _a) const
    {
        if (_a.is_v4())
            return std::hash<unsigned long>()(_a.to_v4().to_ulong());
        if (_a.is_v6())
        {
            auto const& range = _a.to_v6().to_bytes();
            return boost::hash_range(range.begin(), range.end());
        }
        if (_a.is_unspecified())
            return static_cast<size_t>(0x3487194039229152ull);  // Chosen by fair dice roll, guaranteed to be random
        return std::hash<std::string>()(_a.to_string());
    }
};
}  // namespace std
