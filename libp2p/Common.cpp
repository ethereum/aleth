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

char const* p2pPacketTypeToString(P2pPacketType _packetType)
{
    switch (_packetType)
    {
    case HelloPacket:
        return "Hello";
    case DisconnectPacket:
        return "Disconnect";
    case PingPacket:
        return "Ping";
    case PongPacket:
        return "Pong";
    default:
        return "Unknown";
    }
}

bool isPublicAddress(string const& _addressToCheck)
{
    return _addressToCheck.empty() ? false : isPublicAddress(bi::make_address(_addressToCheck));
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

bool isPrivateAddress(string const& _addressToCheck)
{
    return _addressToCheck.empty() ? false : isPrivateAddress(bi::make_address(_addressToCheck));
}

// Helper function to determine if an address is localhost
bool isLocalHostAddress(bi::address const& _addressToCheck)
{
    // @todo: ivp6 link-local adresses (macos), ex: fe80::1%lo0
    static const set<bi::address> c_rejectAddresses = {
        {bi::make_address_v4(c_localhostIp)},
        {bi::make_address_v4("0.0.0.0")},
        {bi::make_address_v6("::1")},
        {bi::make_address_v6("::")},
    };

    return c_rejectAddresses.find(_addressToCheck) != c_rejectAddresses.end();
}

bool isLocalHostAddress(string const& _addressToCheck)
{
    return _addressToCheck.empty() ? false : isLocalHostAddress(bi::make_address(_addressToCheck));
}

string reasonOf(DisconnectReason _r)
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

string NodeSpec::enode() const
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

ostream& operator<<(ostream& _out, NodeIPEndpoint const& _ep)
{
    _out << _ep.address() << ':' << _ep.tcpPort();
    // It rarely happens that TCP and UDP are different, so save space
    // and only display the UDP one when different.
    if (_ep.udpPort() != _ep.tcpPort())
        _out << ":udp" << _ep.udpPort();
    return _out;
}

vector<pair<Public, const char*>> defaultBootNodes()
{
    // TODO: Use full string enode representation, maybe via Node, NodeSpec or other type.
    // clang-format off
    return {
        // Mainnet:
        // --> Ethereum Foundation Go Bootnodes
        {Public("a979fb575495b8d6db44f750317d0f4622bf4c2aa3365d6af7c284339968eef29b69ad0dce72a4d8db5ebb4968de0e3bec910127f134779fbcb0cb6d3331163c"), "52.16.188.185:30303"}, // IE
        {Public("3f1d12044546b76342d59d4a05532c14b85aa669704bfe1f864fe079415aa2c02d743e03218e57a33fb94523adb54032871a6c51b2cc5514cb7c7e35b3ed0a99"), "13.93.211.84:30303"}, // US-WEST
        {Public("78de8a0916848093c73790ead81d1928bec737d565119932b98c6b100d944b7a95e94f847f689fc723399d2e31129d182f7ef3863f2b4c820abbf3ab2722344d"), "191.235.84.50:30303"}, // BR
        {Public("158f8aab45f6d19c6cbf4a089c2670541a8da11978a2f90dbf6a502a4a3bab80d288afdbeb7ec0ef6d92de563767f3b1ea9e8e334ca711e9f8e2df5a0385e8e6"), "13.75.154.138:30303"}, // AU
        {Public("1118980bf48b0a3640bdba04e0fe78b1add18e1cd99bf22d53daac1fd9972ad650df52176e7c7d89d1114cfef2bc23a2959aa54998a46afcf7d91809f0855082"), "52.74.57.123:30303"}, // SG
        
        // Ropsten:
        {Public("30b7ab30a01c124a6cceca36863ece12c4f5fa68e3ba9b0b51407ccc002eeed3b3102d20a88f1c1d3c3154e2449317b8ef95090e77b312d5cc39354f86d5d606"), "52.176.7.10:30303"}, // US-Azure geth
        {Public("865a63255b3bb68023b6bffd5095118fcc13e79dcf014fe4e47e065c350c7cc72af2e53eff895f11ba1bbb6a2b33271c1116ee870f266618eadfc2e78aa7349c"), "52.176.100.77:30303"}, // US-Azure parity
        {Public("6332792c4a00e3e4ee0926ed89e0d27ef985424d97b6a45bf0f23e51f0dcb5e66b875777506458aea7af6f9e4ffb69f43f3778ee73c81ed9d34c51c4b16b0b0f"), "52.232.243.152:30303"}, // Parity
        {Public("94c15d1b9e2fe7ce56e458b9a3b672ef11894ddedd0c6f247e0f1d3487f52b66208fb4aeb8179fce6e3a749ea93ed147c37976d67af557508d199d9594c35f09"), "192.81.208.223:30303"}, // @gpip
        
        // Rinkeby:
        {Public("a24ac7c5484ef4ed0c5eb2d36620ba4e4aa13b8c84684e1b4aab0cebea2ae45cb4d375b77eab56516d34bfbd3c1a833fc51296ff084b770b94fb9028c4d25ccf"), "52.169.42.101:30303"}, // IE
        {Public("343149e4feefa15d882d9fe4ac7d88f885bd05ebb735e547f12e12080a9fa07c8014ca6fd7f373123488102fe5e34111f8509cf0b7de3f5b44339c9f25e87cb8"), "52.3.158.184:30303"}, // INFURA
        {Public("b6b28890b006743680c52e64e0d16db57f28124885595fa03a562be1d2bf0f3a1da297d56b13da25fb992888fd556d4c1a27b1f39d531bde7de1921c90061cc6"), "159.89.28.211:30303"}, // AKASHA
        
        // Goerli
        // --> Upstream bootnodes
        {Public("011f758e6552d105183b1761c5e2dea0111bc20fd5f6422bc7f91e0fabbec9a6595caf6239b37feb773dddd3f87240d99d859431891e4a642cf2a0a9e6cbb98a"), "51.141.78.53:30303"},
        {Public("176b9417f511d05b6b2cf3e34b756cf0a7096b3094572a8f6ef4cdcb9d1f9d00683bf0f83347eebdf3b81c3521c2332086d9592802230bf528eaf606a1d9677b"), "13.93.54.137:30303"},
        {Public("46add44b9f13965f7b9875ac6b85f016f341012d84f975377573800a863526f4da19ae2c620ec73d11591fa9510e992ecc03ad0751f53cc02f7c7ed6d55c7291"), "94.237.54.114:30313"},
        {Public("c1f8b7c2ac4453271fa07d8e9ecf9a2e8285aa0bd0c07df0131f47153306b0736fd3db8924e7a9bf0bed6b1d8d4f87362a71b033dc7c64547728d953e43e59b2"), "52.64.155.147:30303"},
        {Public("f4a9c6ee28586009fb5a96c8af13a58ed6d8315a9eee4772212c1d4d9cebe5a8b8a78ea4434f318726317d04a3f531a1ef0420cf9752605a562cfe858c46e263"), "213.186.16.82:30303"},
        
        // --> Ethereum Foundation bootnode
        {Public("573b6607cd59f241e30e4c4943fd50e99e2b6f42f9bd5ca111659d309c06741247f4f1e93843ad3e8c8c18b6e2d94c161b7ef67479b3938780a97134b618b5ce"), "52.56.136.200:30303"},
    };
    // clang-format on
}

}  // namespace p2p
}  // namespace dev
