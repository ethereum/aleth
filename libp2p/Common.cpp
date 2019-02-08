// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "Common.h"
#include "Network.h"
#include <regex>

namespace dev
{
namespace p2p
{
using namespace std;

const NodeIPEndpoint UnspecifiedNodeIPEndpoint = NodeIPEndpoint{{}, 0, 0};
const Node UnspecifiedNode = Node{{}, UnspecifiedNodeIPEndpoint};

bool isPublicAddress(std::string const& _addressToCheck)
{
    return _addressToCheck.empty() ? false :
                                     isPublicAddress(bi::address::from_string(_addressToCheck));
}

bool isPublicAddress(bi::address const& _addressToCheck)
{
    return !(isPrivateAddress(_addressToCheck) || isLocalHostAddress(_addressToCheck));
}

bool isAllowedAddress(bool _allowLocalDiscovery, bi::address const& _addressToCheck)
{
    return _allowLocalDiscovery ? !_addressToCheck.is_unspecified() :
                                  isPublicAddress(_addressToCheck);
}

bool isAllowedEndpoint(bool _allowLocalDiscovery, NodeIPEndpoint const& _endpointToCheck)
{
    return isAllowedAddress(_allowLocalDiscovery, _endpointToCheck.address());
}

// Helper function to determine if an address falls within one of the reserved ranges
// For V4:
// Class A "10.*", Class B "172.[16->31].*", Class C "192.168.*"
bool isPrivateAddress(bi::address const& _addressToCheck)
{
    if (_addressToCheck.is_v4())
    {
        bi::address_v4 v4Address = _addressToCheck.to_v4();
        bi::address_v4::bytes_type bytesToCheck = v4Address.to_bytes();
        if (bytesToCheck[0] == 10 || bytesToCheck[0] == 127)
            return true;
        if (bytesToCheck[0] == 172 && (bytesToCheck[1] >= 16 && bytesToCheck[1] <= 31))
            return true;
        if (bytesToCheck[0] == 192 && bytesToCheck[1] == 168)
            return true;
    }
    else if (_addressToCheck.is_v6())
    {
        bi::address_v6 v6Address = _addressToCheck.to_v6();
        bi::address_v6::bytes_type bytesToCheck = v6Address.to_bytes();
        if (bytesToCheck[0] == 0xfd && bytesToCheck[1] == 0)
            return true;
        if (!bytesToCheck[0] && !bytesToCheck[1] && !bytesToCheck[2] && !bytesToCheck[3] &&
            !bytesToCheck[4] && !bytesToCheck[5] && !bytesToCheck[6] && !bytesToCheck[7] &&
            !bytesToCheck[8] && !bytesToCheck[9] && !bytesToCheck[10] && !bytesToCheck[11] &&
            !bytesToCheck[12] && !bytesToCheck[13] && !bytesToCheck[14] &&
            (bytesToCheck[15] == 0 || bytesToCheck[15] == 1))
            return true;
    }
    return false;
}

bool isPrivateAddress(std::string const& _addressToCheck)
{
    return _addressToCheck.empty() ? false :
                                     isPrivateAddress(bi::address::from_string(_addressToCheck));
}

// Helper function to determine if an address is localhost
bool isLocalHostAddress(bi::address const& _addressToCheck)
{
    // @todo: ivp6 link-local adresses (macos), ex: fe80::1%lo0
    static const set<bi::address> c_rejectAddresses = {
        {bi::address_v4::from_string(c_localhostIp)},
        {bi::address_v4::from_string("0.0.0.0")},
        {bi::address_v6::from_string("::1")},
        {bi::address_v6::from_string("::")},
    };

    return c_rejectAddresses.find(_addressToCheck) != c_rejectAddresses.end();
}

bool isLocalHostAddress(std::string const& _addressToCheck)
{
    return _addressToCheck.empty() ? false :
                                     isLocalHostAddress(bi::address::from_string(_addressToCheck));
}

std::string reasonOf(DisconnectReason _r)
{
    switch (_r)
    {
    case DisconnectRequested:
        return "Disconnect was requested.";
    case TCPError:
        return "Low-level TCP communication error.";
    case BadProtocol:
        return "Data format error.";
    case UselessPeer:
        return "Peer had no use for this node.";
    case TooManyPeers:
        return "Peer had too many connections.";
    case DuplicatePeer:
        return "Peer was already connected.";
    case IncompatibleProtocol:
        return "Peer protocol versions are incompatible.";
    case NullIdentity:
        return "Null identity given.";
    case ClientQuit:
        return "Peer is exiting.";
    case UnexpectedIdentity:
        return "Unexpected identity given.";
    case LocalIdentity:
        return "Connected to ourselves.";
    case UserReason:
        return "Subprotocol reason.";
    case NoDisconnect:
        return "(No disconnect has happened.)";
    default:
        return "Unknown reason.";
    }
}

void NodeIPEndpoint::streamRLP(RLPStream& _s, RLPAppend _append) const
{
    if (_append == StreamList)
        _s.appendList(3);
    if (m_address.is_v4())
        _s << bytesConstRef(&m_address.to_v4().to_bytes()[0], 4);
    else if (m_address.is_v6())
        _s << bytesConstRef(&m_address.to_v6().to_bytes()[0], 16);
    else
        _s << bytes();
    _s << m_udpPort << m_tcpPort;
}

void NodeIPEndpoint::interpretRLP(RLP const& _r)
{
    if (_r[0].size() == 4)
        m_address = bi::address_v4(*(bi::address_v4::bytes_type*)_r[0].toBytes().data());
    else if (_r[0].size() == 16)
        m_address = bi::address_v6(*(bi::address_v6::bytes_type*)_r[0].toBytes().data());
    else
        m_address = bi::address();
    m_udpPort = _r[1].toInt<uint16_t>();
    m_tcpPort = _r[2].toInt<uint16_t>();
}

void DeadlineOps::reap()
{
    if (m_stopped)
        return;

    Guard l(x_timers);
    auto t = m_timers.begin();
    while (t != m_timers.end())
        if (t->expired())
        {
            t->wait();
            t = m_timers.erase(t);
        }
        else
            t++;

    m_timers.emplace_back(m_io, m_reapIntervalMs, [this](boost::system::error_code const& ec) {
        if (!ec && !m_stopped)
            reap();
    });
}

Node::Node(Node const& _original)
  : id(_original.id), endpoint(_original.endpoint), peerType(_original.peerType.load())
{}

Node::Node(NodeSpec const& _s, PeerType _p)
  : id(_s.id()), endpoint(_s.nodeIPEndpoint()), peerType(_p)
{}

NodeSpec::NodeSpec(string const& _user)
{
    // Format described here: https://github.com/ethereum/wiki/wiki/enode-url-format
    // Example valid URLs:
    //      enode://6332792c4a00e3e4ee0926ed89e0d27ef985424d97b6a45bf0f23e51f0dcb5e66b875777506458aea7af6f9e4ffb69f43f3778ee73c81ed9d34c51c4b16b0b0f@52.232.243.152:30305
    //      enode://6332792c4a00e3e4ee0926ed89e0d27ef985424d97b6a45bf0f23e51f0dcb5e66b875777506458aea7af6f9e4ffb69f43f3778ee73c81ed9d34c51c4b16b0b0f@52.232.243.152:30305?discport=30303
    //      enode://6332792c4a00e3e4ee0926ed89e0d27ef985424d97b6a45bf0f23e51f0dcb5e66b875777506458aea7af6f9e4ffb69f43f3778ee73c81ed9d34c51c4b16b0b0f@52.232.243.152
    const char* peerPattern =
        R"(^(enode://)([\dabcdef]{128})@(\d{1,3}.\d{1,3}.\d{1,3}.\d{1,3})((:\d{2,5})(\?discport=(\d{2,5}))?)?$)";
    regex rx(peerPattern);
    smatch match;

    m_tcpPort = m_udpPort = c_defaultListenPort;
    if (regex_match(_user, match, rx))
    {
        m_id = NodeID(match.str(2));
        m_address = match.str(3);
        if (match[5].matched)
        {
            m_udpPort = m_tcpPort = stoi(match.str(5).substr(1));
            if (match[7].matched)
                m_udpPort = stoi(match.str(7));
        }
    }
}

NodeIPEndpoint NodeSpec::nodeIPEndpoint() const
{
    return {Network::resolveHost(m_address).address(), m_udpPort, m_tcpPort};
}

std::string NodeSpec::enode() const
{
    string ret = m_address;

    if (m_tcpPort)
    {
        if (m_udpPort && m_tcpPort != m_udpPort)
            ret += ":" + toString(m_tcpPort) + "?discport=" + toString(m_udpPort);
        else
            ret += ":" + toString(m_tcpPort);
    }

    if (m_id)
        return "enode://" + m_id.hex() + "@" + ret;
    return ret;
}

bool NodeSpec::isValid() const
{
    return m_id && !m_address.empty();
}

std::ostream& operator<<(std::ostream& _out, NodeIPEndpoint const& _ep)
{
    _out << _ep.address() << ':' << _ep.tcpPort();
    // It rarely happens that TCP and UDP are different, so save space
    // and only display the UDP one when different.
    if (_ep.udpPort() != _ep.tcpPort())
        _out << ":udp" << _ep.udpPort();
    return _out;
}

boost::log::formatting_ostream& operator<<(boost::log::formatting_ostream& _log, const Node& _node)
{
    return _log << _node.id << '@' << _node.endpoint;
}

}  // namespace p2p
}  // namespace dev
