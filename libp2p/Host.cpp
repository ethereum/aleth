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
/** @file Host.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "Host.h"
#include "CapabilityHost.h"
#include "Common.h"
#include "HostCapability.h"
#include "RLPxHandshake.h"
#include "Session.h"
#include "UPnP.h"
#include <libdevcore/Assertions.h>
#include <libdevcore/Common.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Exceptions.h>
#include <libdevcore/FileSystem.h>
#include <boost/algorithm/string.hpp>
#include <chrono>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
using namespace std;
using namespace dev;
using namespace dev::p2p;

namespace
{
class CapabilityHost : public CapabilityHostFace
{
public:
    explicit CapabilityHost(Host& _host) : m_host{_host} {}

    boost::optional<PeerSessionInfo> peerSessionInfo(NodeID const& _nodeID) const override
    {
        auto session = m_host.peerSession(_nodeID);
        return session ? session->info() : boost::optional<PeerSessionInfo>{};
    }

    void disconnect(NodeID const& _nodeID, DisconnectReason _reason) override
    {
        auto session = m_host.peerSession(_nodeID);
        if (session)
            session->disconnect(_reason);
    }

    void disableCapability(NodeID const& _nodeID, std::string const& _capabilityName,
        std::string const& _problem) override
    {
        auto session = m_host.peerSession(_nodeID);
        if (session)
            session->disableCapability(_capabilityName, _problem);
    }

    void addRating(NodeID const& _nodeID, int _r) override
    {
        auto session = m_host.peerSession(_nodeID);
        if (session)
            session->addRating(_r);
    }

    RLPStream& prep(NodeID const& _nodeID, std::string const& _capabilityName, RLPStream& _s,
        unsigned _id, unsigned _args = 0) override
    {
        auto session = m_host.peerSession(_nodeID);
        if (!session)
            return _s;

        unsigned const offset = session->peer()->capabilityOffset(_capabilityName);
        return _s.appendRaw(bytes(1, _id + offset)).appendList(_args);
    }

    void sealAndSend(NodeID const& _nodeID, RLPStream& _s) override
    {
        auto session = m_host.peerSession(_nodeID);
        if (session)
            session->sealAndSend(_s);
    }

    void addNote(NodeID const& _nodeID, std::string const& _k, std::string const& _v) override
    {
        auto session = m_host.peerSession(_nodeID);
        if (session)
            session->addNote(_k, _v);
    }

    bool isRude(NodeID const& _nodeID, std::string const& _capability) const override
    {
        auto s = m_host.peerSession(_nodeID);
        if (s)
            return s->repMan().isRude(*s, _capability);
        return false;
    }

    void foreachPeer(std::string const& _name, u256 const& _version,
        std::function<bool(NodeID const&)> _f) const override
    {
        m_host.forEachPeer(_name, _version, _f);
    }

private:
    Host& m_host;
};
}  // namespace

/// Interval at which Host::run will call keepAlivePeers to ping peers.
std::chrono::seconds const c_keepAliveInterval = std::chrono::seconds(30);

/// Disconnect timeout after failure to respond to keepAlivePeers ping.
std::chrono::milliseconds const c_keepAliveTimeOut = std::chrono::milliseconds(1000);

HostNodeTableHandler::HostNodeTableHandler(Host& _host): m_host(_host) {}

void HostNodeTableHandler::processEvent(NodeID const& _n, NodeTableEventType const& _e)
{
    m_host.onNodeTableEvent(_n, _e);
}

ReputationManager::ReputationManager()
{
}

void ReputationManager::noteRude(SessionFace const& _s, std::string const& _sub)
{
    DEV_WRITE_GUARDED(x_nodes)
        m_nodes[make_pair(_s.id(), _s.info().clientVersion)].subs[_sub].isRude = true;
}

bool ReputationManager::isRude(SessionFace const& _s, std::string const& _sub) const
{
    DEV_READ_GUARDED(x_nodes)
    {
        auto nit = m_nodes.find(make_pair(_s.id(), _s.info().clientVersion));
        if (nit == m_nodes.end())
            return false;
        auto sit = nit->second.subs.find(_sub);
        bool ret = sit == nit->second.subs.end() ? false : sit->second.isRude;
        return _sub.empty() ? ret : (ret || isRude(_s));
    }
    return false;
}

void ReputationManager::setData(SessionFace const& _s, std::string const& _sub, bytes const& _data)
{
    DEV_WRITE_GUARDED(x_nodes)
        m_nodes[make_pair(_s.id(), _s.info().clientVersion)].subs[_sub].data = _data;
}

bytes ReputationManager::data(SessionFace const& _s, std::string const& _sub) const
{
    DEV_READ_GUARDED(x_nodes)
    {
        auto nit = m_nodes.find(make_pair(_s.id(), _s.info().clientVersion));
        if (nit == m_nodes.end())
            return bytes();
        auto sit = nit->second.subs.find(_sub);
        return sit == nit->second.subs.end() ? bytes() : sit->second.data;
    }
    return bytes();
}

Host::Host(string const& _clientVersion, KeyPair const& _alias, NetworkConfig const& _n)
  : Worker("p2p", 0),
    m_clientVersion(_clientVersion),
    m_netConfig(_n),
    m_ifAddresses(Network::getInterfaceAddresses()),
    m_ioService(2),
    m_tcp4Acceptor(m_ioService),
    m_alias(_alias),
    m_lastPing(chrono::steady_clock::time_point::min()),
    m_capabilityHost(make_shared<CapabilityHost>(*this))
{
    cnetnote << "Id: " << id();
}

Host::Host(string const& _clientVersion, NetworkConfig const& _n, bytesConstRef _restoreNetwork):
    Host(_clientVersion, networkAlias(_restoreNetwork), _n)
{
    m_restoreNetwork = _restoreNetwork.toBytes();
}

Host::~Host()
{
    stop();
    terminate();
}

void Host::start()
{
    DEV_TIMED_FUNCTION_ABOVE(500);
    startWorking();
    while (isWorking() && !haveNetwork())
        this_thread::sleep_for(chrono::milliseconds(10));
    
    // network start failed!
    if (isWorking())
        return;

    cwarn << "Network start failed!";
    doneWorking();
}

void Host::stop()
{
    // called to force io_service to kill any remaining tasks it might have -
    // such tasks may involve socket reads from Capabilities that maintain references
    // to resources we're about to free.

    // ignore if already stopped/stopping, at the same time,
    // signal run() to prepare for shutdown and reset m_timer
    if (!m_run.exchange(false))
        return;

    {
        unique_lock<mutex> l(x_runTimer);
        while (m_timer)
            m_timerReset.wait(l);
    }

    // stop worker thread
    if (isWorking())
        stopWorking();
}

void Host::doneWorking()
{
    // reset ioservice (cancels all timers and allows manually polling network, below)
    m_ioService.reset();

    DEV_GUARDED(x_timers)
        m_timers.clear();
    
    // shutdown acceptor
    m_tcp4Acceptor.cancel();
    if (m_tcp4Acceptor.is_open())
        m_tcp4Acceptor.close();

    // There maybe an incoming connection which started but hasn't finished.
    // Wait for acceptor to end itself instead of assuming it's complete.
    // This helps ensure a peer isn't stopped at the same time it's starting
    // and that socket for pending connection is closed.
    while (m_accepting)
        m_ioService.poll();

    // stop capabilities (eth: stops syncing or block/tx broadcast)
    for (auto const& h: m_capabilities)
        h.second->onStopping();

    // disconnect pending handshake, before peers, as a handshake may create a peer
    for (unsigned n = 0;; n = 0)
    {
        DEV_GUARDED(x_connecting)
            for (auto const& i: m_connecting)
                if (auto h = i.lock())
                {
                    h->cancel();
                    n++;
                }
        if (!n)
            break;
        m_ioService.poll();
    }
    
    // disconnect peers
    for (unsigned n = 0;; n = 0)
    {
        DEV_RECURSIVE_GUARDED(x_sessions)
            for (auto i: m_sessions)
                if (auto p = i.second.lock())
                    if (p->isConnected())
                    {
                        p->disconnect(ClientQuit);
                        n++;
                    }
        if (!n)
            break;

        // poll so that peers send out disconnect packets
        m_ioService.poll();
    }

    // stop network (again; helpful to call before subsequent reset())
    m_ioService.stop();

    // reset network (allows reusing ioservice in future)
    m_ioService.reset();

    // finally, clear out peers (in case they're lingering)
    RecursiveGuard l(x_sessions);
    m_sessions.clear();
}

bool Host::isRequiredPeer(NodeID const& _id) const
{
    Guard l(x_requiredPeers);
    return m_requiredPeers.count(_id);
}

// called after successful handshake
void Host::startPeerSession(Public const& _id, RLP const& _rlp, unique_ptr<RLPXFrameCoder>&& _io, std::shared_ptr<RLPXSocket> const& _s)
{
    // session maybe ingress or egress so m_peers and node table entries may not exist
    shared_ptr<Peer> peer;
    DEV_RECURSIVE_GUARDED(x_sessions)
    {
        if (m_peers.count(_id))
            peer = m_peers[_id];
        else
        {
            // peer doesn't exist, try to get port info from node table
            if (Node n = nodeFromNodeTable(_id))
                peer = make_shared<Peer>(n);

            if (!peer)
                peer = make_shared<Peer>(Node(_id, UnspecifiedNodeIPEndpoint));

            m_peers[_id] = peer;
        }
    }
    if (peer->isOffline())
        peer->m_lastConnected = std::chrono::system_clock::now();
    peer->endpoint.setAddress(_s->remoteEndpoint().address());

    auto protocolVersion = _rlp[0].toInt<unsigned>();
    auto clientVersion = _rlp[1].toString();
    auto caps = _rlp[2].toVector<CapDesc>();
    auto listenPort = _rlp[3].toInt<unsigned short>();
    auto pub = _rlp[4].toHash<Public>();

    if (pub != _id)
    {
        cdebug << "Wrong ID: " << pub << " vs. " << _id;
        return;
    }

    // clang error (previously: ... << hex << caps ...)
    // "'operator<<' should be declared prior to the call site or in an associated namespace of one of its arguments"
    stringstream capslog;

    // leave only highset mutually supported capability version
    caps.erase(remove_if(caps.begin(), caps.end(), [&](CapDesc const& _r){ return !haveCapability(_r) || any_of(caps.begin(), caps.end(), [&](CapDesc const& _o){ return _r.first == _o.first && _o.second > _r.second && haveCapability(_o); }); }), caps.end());

    for (auto cap: caps)
        capslog << "(" << cap.first << "," << dec << cap.second << ")";

    cnetlog << "Hello: " << clientVersion << " V[" << protocolVersion << "]"
            << " " << _id << " " << showbase << capslog.str() << " " << dec << listenPort;

    // create session so disconnects are managed
    shared_ptr<SessionFace> session = make_shared<Session>(this, move(_io), _s, peer,
        PeerSessionInfo({_id, clientVersion, peer->endpoint.address().to_string(), listenPort,
            chrono::steady_clock::duration(), _rlp[2].toSet<CapDesc>(), 0, map<string, string>(),
            protocolVersion}));
    if (protocolVersion < dev::p2p::c_protocolVersion - 1)
    {
        session->disconnect(IncompatibleProtocol);
        return;
    }
    if (caps.empty())
    {
        session->disconnect(UselessPeer);
        return;
    }

    if (m_netConfig.pin && !isRequiredPeer(_id))
    {
        cdebug << "Unexpected identity from peer (got" << _id << ", must be one of " << m_requiredPeers << ")";
        session->disconnect(UnexpectedIdentity);
        return;
    }
    
    {
        RecursiveGuard l(x_sessions);
        if (m_sessions.count(_id) && !!m_sessions[_id].lock())
            if (auto s = m_sessions[_id].lock())
                if(s->isConnected())
                {
                    // Already connected.
                    cnetlog << "Session already exists for peer with id " << _id;
                    session->disconnect(DuplicatePeer);
                    return;
                }
        
        if (!peerSlotsAvailable())
        {
            cnetdetails << "Too many peers, can't connect. peer count: " << peerCount()
                        << " pending peers: " << m_pendingPeerConns.size();
            session->disconnect(TooManyPeers);
            return;
        }

        unsigned offset = (unsigned)UserPacket;

        // todo: mutex Session::m_capabilities and move for(:caps) out of mutex.
        for (auto const& capDesc : caps)
        {
            auto pcap = m_capabilities[capDesc];
            if (!pcap)
                return session->disconnect(IncompatibleProtocol);

            peer->setCapabilityOffset(capDesc.first, offset);
            session->registerCapability(capDesc, pcap);

            pcap->onConnect(_id, capDesc.second);

            offset += pcap->messageCount();
        }

        session->start();
        m_sessions[_id] = session;
    }
    
    LOG(m_logger) << "p2p.host.peer.register " << _id;
}

void Host::onNodeTableEvent(NodeID const& _n, NodeTableEventType const& _e)
{
    if (_e == NodeEntryAdded)
    {
        LOG(m_logger) << "p2p.host.nodeTable.events.nodeEntryAdded " << _n;
        if (Node n = nodeFromNodeTable(_n))
        {
            shared_ptr<Peer> p;
            DEV_RECURSIVE_GUARDED(x_sessions)
            {
                if (m_peers.count(_n))
                {
                    p = m_peers[_n];
                    p->endpoint = n.endpoint;
                }
                else
                {
                    p = make_shared<Peer>(n);
                    m_peers[_n] = p;
                    LOG(m_logger) << "p2p.host.peers.events.peerAdded " << _n << " " << p->endpoint;
                }
            }
            if (peerSlotsAvailable(Egress))
                connect(p);
        }
    }
    else if (_e == NodeEntryDropped)
    {
        LOG(m_logger) << "p2p.host.nodeTable.events.NodeEntryDropped " << _n;
        RecursiveGuard l(x_sessions);
        if (m_peers.count(_n) && m_peers[_n]->peerType == PeerType::Optional)
            m_peers.erase(_n);
    }
}

void Host::determinePublic()
{
    // set m_tcpPublic := listenIP (if public) > public > upnp > unspecified address.
    
    auto ifAddresses = Network::getInterfaceAddresses();
    auto laddr = m_netConfig.listenIPAddress.empty() ? bi::address() : bi::address::from_string(m_netConfig.listenIPAddress);
    auto lset = !laddr.is_unspecified();
    auto paddr = m_netConfig.publicIPAddress.empty() ? bi::address() : bi::address::from_string(m_netConfig.publicIPAddress);
    auto pset = !paddr.is_unspecified();
    
    bool listenIsPublic = lset && isPublicAddress(laddr);
    bool publicIsHost = !lset && pset && ifAddresses.count(paddr);
    
    bi::tcp::endpoint ep(bi::address(), m_listenPort);
    if (m_netConfig.traverseNAT && listenIsPublic)
    {
        cnetnote << "Listen address set to Public address: " << laddr << ". UPnP disabled.";
        ep.address(laddr);
    }
    else if (m_netConfig.traverseNAT && publicIsHost)
    {
        cnetnote << "Public address set to Host configured address: " << paddr << ". UPnP disabled.";
        ep.address(paddr);
    }
    else if (m_netConfig.traverseNAT)
    {
        bi::address natIFAddr;
        ep = Network::traverseNAT(lset && ifAddresses.count(laddr) ? std::set<bi::address>({laddr}) : ifAddresses, m_listenPort, natIFAddr);
        
        if (lset && natIFAddr != laddr)
            // if listen address is set, Host will use it, even if upnp returns different
            cwarn << "Listen address " << laddr << " differs from local address " << natIFAddr
                  << " returned by UPnP!";

        if (pset && ep.address() != paddr)
        {
            // if public address is set, Host will advertise it, even if upnp returns different
            cwarn << "Specified public address " << paddr << " differs from external address "
                  << ep.address() << " returned by UPnP!";
            ep.address(paddr);
        }
    }
    else if (pset)
        ep.address(paddr);

    m_tcpPublic = ep;
}

void Host::runAcceptor()
{
    assert(m_listenPort > 0);

    if (m_run && !m_accepting)
    {
        cnetdetails << "Listening on local port " << m_listenPort << " (public: " << m_tcpPublic
                    << ")";
        m_accepting = true;

        auto socket = make_shared<RLPXSocket>(m_ioService);
        m_tcp4Acceptor.async_accept(socket->ref(), [=](boost::system::error_code ec)
        {
            m_accepting = false;
            if (ec || !m_run)
            {
                socket->close();
                return;
            }
            if (peerCount() > peerSlots(Ingress))
            {
                cnetdetails << "Dropping incoming connect due to maximum peer count (" << Ingress
                            << " * ideal peer count): " << socket->remoteEndpoint();
                socket->close();
                if (ec.value() < 1)
                    runAcceptor();
                return;
            }
            
            bool success = false;
            try
            {
                // incoming connection; we don't yet know nodeid
                auto handshake = make_shared<RLPXHandshake>(this, socket);
                m_connecting.push_back(handshake);
                handshake->start();
                success = true;
            }
            catch (Exception const& _e)
            {
                cwarn << "ERROR: " << diagnostic_information(_e);
            }
            catch (std::exception const& _e)
            {
                cwarn << "ERROR: " << _e.what();
            }

            if (!success)
                socket->ref().close();
            runAcceptor();
        });
    }
}

std::unordered_map<Public, std::string> Host::pocHosts()
{
    return {
        // Mainnet:
        { Public("a979fb575495b8d6db44f750317d0f4622bf4c2aa3365d6af7c284339968eef29b69ad0dce72a4d8db5ebb4968de0e3bec910127f134779fbcb0cb6d3331163c"), "52.16.188.185:30303" },
        { Public("3f1d12044546b76342d59d4a05532c14b85aa669704bfe1f864fe079415aa2c02d743e03218e57a33fb94523adb54032871a6c51b2cc5514cb7c7e35b3ed0a99"), "13.93.211.84:30303" },
        { Public("78de8a0916848093c73790ead81d1928bec737d565119932b98c6b100d944b7a95e94f847f689fc723399d2e31129d182f7ef3863f2b4c820abbf3ab2722344d"), "191.235.84.50:30303" },
        { Public("158f8aab45f6d19c6cbf4a089c2670541a8da11978a2f90dbf6a502a4a3bab80d288afdbeb7ec0ef6d92de563767f3b1ea9e8e334ca711e9f8e2df5a0385e8e6"), "13.75.154.138:30303" },
        { Public("1118980bf48b0a3640bdba04e0fe78b1add18e1cd99bf22d53daac1fd9972ad650df52176e7c7d89d1114cfef2bc23a2959aa54998a46afcf7d91809f0855082"), "52.74.57.123:30303" },
        // Ropsten:
        { Public("30b7ab30a01c124a6cceca36863ece12c4f5fa68e3ba9b0b51407ccc002eeed3b3102d20a88f1c1d3c3154e2449317b8ef95090e77b312d5cc39354f86d5d606"), "52.176.7.10:30303" },
        { Public("865a63255b3bb68023b6bffd5095118fcc13e79dcf014fe4e47e065c350c7cc72af2e53eff895f11ba1bbb6a2b33271c1116ee870f266618eadfc2e78aa7349c"), "52.176.100.77:30303" },
        { Public("6332792c4a00e3e4ee0926ed89e0d27ef985424d97b6a45bf0f23e51f0dcb5e66b875777506458aea7af6f9e4ffb69f43f3778ee73c81ed9d34c51c4b16b0b0f"), "52.232.243.152:30303" },
        { Public("94c15d1b9e2fe7ce56e458b9a3b672ef11894ddedd0c6f247e0f1d3487f52b66208fb4aeb8179fce6e3a749ea93ed147c37976d67af557508d199d9594c35f09"), "192.81.208.223:30303" },
    };
}

void Host::registerCapability(std::shared_ptr<HostCapabilityFace> const& _cap)
{
    registerCapability(_cap, _cap->name(), _cap->version());
}

void Host::registerCapability(
    std::shared_ptr<HostCapabilityFace> const& _cap, std::string const& _name, u256 const& _version)
{
    m_capabilities[std::make_pair(_name, _version)] = _cap;
}

void Host::addPeer(NodeSpec const& _s, PeerType _t)
{
    if (_t == PeerType::Optional)
        addNode(_s.id(), _s.nodeIPEndpoint());
    else
        requirePeer(_s.id(), _s.nodeIPEndpoint());
}

void Host::addNode(NodeID const& _node, NodeIPEndpoint const& _endpoint)
{
    // return if network is stopped while waiting on Host::run() or nodeTable to start
    while (!haveNetwork())
        if (isWorking())
            this_thread::sleep_for(chrono::milliseconds(50));
        else
            return;

    if (_node == id())
    {
        cnetdetails << "Ingoring the request to connect to self " << _node;
        return;
    }

    if (_endpoint.tcpPort() < 30300 || _endpoint.tcpPort() > 30305)
        cnetdetails << "Non-standard port being recorded: " << _endpoint.tcpPort();

    addNodeToNodeTable(Node(_node, _endpoint));
}

void Host::requirePeer(NodeID const& _n, NodeIPEndpoint const& _endpoint)
{
    {
        Guard l(x_requiredPeers);
        m_requiredPeers.insert(_n);
    }

    if (!m_run)
        return;
    
    if (_n == id())
    {
        cnetdetails << "Ingoring the request to connect to self " << _n;
        return;
    }

    Node node(_n, _endpoint, PeerType::Required);
    if (_n)
    {
        // create or update m_peers entry
        shared_ptr<Peer> p;
        DEV_RECURSIVE_GUARDED(x_sessions)
            if (m_peers.count(_n))
            {
                p = m_peers[_n];
                p->endpoint = node.endpoint;
                p->peerType = PeerType::Required;
            }
            else
            {
                p = make_shared<Peer>(node);
                m_peers[_n] = p;
            }
        // required for discovery
        addNodeToNodeTable(*p, NodeTable::NodeRelation::Unknown);
    }
    else
    {
        if (!addNodeToNodeTable(node))
            return;
        auto t = make_shared<boost::asio::deadline_timer>(m_ioService);
        t->expires_from_now(boost::posix_time::milliseconds(600));
        t->async_wait([this, _n](boost::system::error_code const& _ec)
        {
            if (!_ec)
                if (auto n = nodeFromNodeTable(_n))
                    requirePeer(n.id, n.endpoint);
        });
        DEV_GUARDED(x_timers)
            m_timers.push_back(t);
    }
}

void Host::relinquishPeer(NodeID const& _node)
{
    Guard l(x_requiredPeers);
    if (m_requiredPeers.count(_node))
        m_requiredPeers.erase(_node);
}

void Host::connect(std::shared_ptr<Peer> const& _p)
{
    if (!m_run)
        return;
    
    if (havePeerSession(_p->id))
    {
        cnetdetails << "Aborted connect. Node already connected.";
        return;
    }

    if (!nodeTableHasNode(_p->id) && _p->peerType == PeerType::Optional)
        return;

    // prevent concurrently connecting to a node
    Peer *nptr = _p.get();
    if (m_pendingPeerConns.count(nptr))
        return;
    m_pendingPeerConns.insert(nptr);

    _p->m_lastAttempted = std::chrono::system_clock::now();
    
    bi::tcp::endpoint ep(_p->endpoint);
    cnetdetails << "Attempting connection to node " << _p->id << "@" << ep << " from " << id();
    auto socket = make_shared<RLPXSocket>(m_ioService);
    socket->ref().async_connect(ep, [=](boost::system::error_code const& ec)
    {
        _p->m_lastAttempted = std::chrono::system_clock::now();
        _p->m_failedAttempts++;
        
        if (ec)
        {
            cnetdetails << "Connection refused to node " << _p->id << "@" << ep << " ("
                        << ec.message() << ")";
            // Manually set error (session not present)
            _p->m_lastDisconnect = TCPError;
        }
        else
        {
            cnetdetails << "Connecting to " << _p->id << "@" << ep;
            auto handshake = make_shared<RLPXHandshake>(this, socket, _p->id);
            {
                Guard l(x_connecting);
                m_connecting.push_back(handshake);
            }

            handshake->start();
        }
        
        m_pendingPeerConns.erase(nptr);
    });
}

PeerSessionInfos Host::peerSessionInfo() const
{
    if (!m_run)
        return PeerSessionInfos();

    std::vector<PeerSessionInfo> ret;
    RecursiveGuard l(x_sessions);
    for (auto& i: m_sessions)
        if (auto j = i.second.lock())
            if (j->isConnected())
                ret.push_back(j->info());
    return ret;
}

size_t Host::peerCount() const
{
    unsigned retCount = 0;
    RecursiveGuard l(x_sessions);
    for (auto& i: m_sessions)
        if (std::shared_ptr<SessionFace> j = i.second.lock())
            if (j->isConnected())
                retCount++;
    return retCount;
}

void Host::run(boost::system::error_code const&)
{
    if (!m_run)
    {
        // reset NodeTable
        DEV_GUARDED(x_nodeTable)
            m_nodeTable.reset();

        // stopping io service allows running manual network operations for shutdown
        // and also stops blocking worker thread, allowing worker thread to exit
        m_ioService.stop();

        // resetting timer signals network that nothing else can be scheduled to run
        DEV_GUARDED(x_runTimer)
            m_timer.reset();

        m_timerReset.notify_all();
        return;
    }

    if (auto nodeTable = this->nodeTable()) // This again requires x_nodeTable, which is why an additional variable nodeTable is used.
        nodeTable->processEvents();

    // cleanup zombies
    DEV_GUARDED(x_connecting)
        m_connecting.remove_if([](std::weak_ptr<RLPXHandshake> h){ return h.expired(); });
    DEV_GUARDED(x_timers)
        m_timers.remove_if([](std::shared_ptr<boost::asio::deadline_timer> t)
        {
            return t->expires_from_now().total_milliseconds() < 0;
        });

    keepAlivePeers();
    
    // At this time peers will be disconnected based on natural TCP timeout.
    // disconnectLatePeers needs to be updated for the assumption that Session
    // is always live and to ensure reputation and fallback timers are properly
    // updated. // disconnectLatePeers();

    // todo: update peerSlotsAvailable()
    
    list<shared_ptr<Peer>> toConnect;
    unsigned reqConn = 0;
    {
        RecursiveGuard l(x_sessions);
        for (auto const& p: m_peers)
        {
            bool haveSession = havePeerSession(p.second->id);
            bool required = p.second->peerType == PeerType::Required;
            if (haveSession && required)
                reqConn++;
            else if (!haveSession && p.second->shouldReconnect() && (!m_netConfig.pin || required))
                toConnect.push_back(p.second);
        }
    }

    for (auto p: toConnect)
        if (p->peerType == PeerType::Required && reqConn++ < m_idealPeerCount)
            connect(p);
    
    if (!m_netConfig.pin)
    {
        unsigned const maxSlots = m_idealPeerCount + reqConn;
        unsigned occupiedSlots = peerCount() + m_pendingPeerConns.size();
        for (auto peerToConnect = toConnect.cbegin();
             occupiedSlots <= maxSlots && peerToConnect != toConnect.cend(); ++peerToConnect)
        {
            if ((*peerToConnect)->peerType == PeerType::Optional)
            {
                connect(*peerToConnect);
                ++occupiedSlots;
            }
        }
    }

    auto runcb = [this](boost::system::error_code const& error) { run(error); };
    m_timer->expires_from_now(boost::posix_time::milliseconds(c_timerInterval));
    m_timer->async_wait(runcb);
}

void Host::startedWorking()
{
    asserts(!m_timer);

    {
        // prevent m_run from being set to true at same time as set to false by stop()
        // don't release mutex until m_timer is set so in case stop() is called at same
        // time, stop will wait on m_timer and graceful network shutdown.
        Guard l(x_runTimer);
        // create deadline timer
        m_timer.reset(new boost::asio::deadline_timer(m_ioService));
        m_run = true;
    }

    // start capability threads (ready for incoming connections)
    for (auto const& h: m_capabilities)
        h.second->onStarting();
    
    // try to open acceptor (todo: ipv6)
    int port = Network::tcp4Listen(m_tcp4Acceptor, m_netConfig);
    if (port > 0)
    {
        m_listenPort = port;
        determinePublic();
        runAcceptor();
    }
    else
        LOG(m_logger) << "p2p.start.notice id: " << id() << " TCP Listen port is invalid or unavailable.";

    auto nodeTable = make_shared<NodeTable>(
        m_ioService,
        m_alias,
        NodeIPEndpoint(bi::address::from_string(listenAddress()), listenPort(), listenPort()),
        m_netConfig.discovery
    );
    nodeTable->setEventHandler(new HostNodeTableHandler(*this));
    DEV_GUARDED(x_nodeTable)
        m_nodeTable = nodeTable;
    restoreNetwork(&m_restoreNetwork);

    LOG(m_logger) << "p2p.started id: " << id();

    run(boost::system::error_code());
}

void Host::doWork()
{
    try
    {
        if (m_run)
            m_ioService.run();
    }
    catch (std::exception const& _e)
    {
        cwarn << "Exception in Network Thread: " << _e.what();
        cwarn << "Network Restart is Recommended.";
    }
}

void Host::keepAlivePeers()
{
    if (chrono::steady_clock::now() - c_keepAliveInterval < m_lastPing)
        return;

    RecursiveGuard l(x_sessions);
    for (auto it = m_sessions.begin(); it != m_sessions.end();)
        if (auto p = it->second.lock())
        {
            p->ping();
            ++it;
        }
        else
            it = m_sessions.erase(it);

    m_lastPing = chrono::steady_clock::now();
}

void Host::disconnectLatePeers()
{
    auto now = chrono::steady_clock::now();
    if (now - c_keepAliveTimeOut < m_lastPing)
        return;

    RecursiveGuard l(x_sessions);
    for (auto p: m_sessions)
        if (auto pp = p.second.lock())
            if (now - c_keepAliveTimeOut > m_lastPing && pp->lastReceived() < m_lastPing)
                pp->disconnect(PingTimeout);
}

bytes Host::saveNetwork() const
{
    std::list<Peer> peers;
    {
        RecursiveGuard l(x_sessions);
        for (auto p: m_peers)
            if (p.second)
                peers.push_back(*p.second);
    }
    peers.sort();

    RLPStream network;
    int count = 0;
    for (auto const& p: peers)
    {
        // todo: ipv6
        if (!p.endpoint.address().is_v4())
            continue;

        // Only save peers which have connected within 2 days, with properly-advertised port and public IP address
        if (chrono::system_clock::now() - p.m_lastConnected < chrono::seconds(3600 * 48) && !!p.endpoint && p.id != id() && (p.peerType == PeerType::Required || p.endpoint.isAllowed()))
        {
            network.appendList(11);
            p.endpoint.streamRLP(network, NodeIPEndpoint::StreamInline);
            network << p.id << (p.peerType == PeerType::Required ? true : false)
                << chrono::duration_cast<chrono::seconds>(p.m_lastConnected.time_since_epoch()).count()
                << chrono::duration_cast<chrono::seconds>(p.m_lastAttempted.time_since_epoch()).count()
                << p.m_failedAttempts.load() << (unsigned)p.m_lastDisconnect << p.m_score.load() << p.m_rating.load();
            count++;
        }
    }

    if (auto nodeTable = this->nodeTable())
    {
        auto state = nodeTable->snapshot();
        state.sort();
        for (auto const& entry: state)
        {
            network.appendList(4);
            entry.endpoint.streamRLP(network, NodeIPEndpoint::StreamInline);
            network << entry.id;
            count++;
        }
    }
    // else: TODO: use previous configuration if available

    RLPStream ret(3);
    ret << dev::p2p::c_protocolVersion << m_alias.secret().ref();
    ret.appendList(count);
    if (!!count)
        ret.appendRaw(network.out(), count);
    return ret.out();
}

void Host::restoreNetwork(bytesConstRef _b)
{
    if (!_b.size())
        return;
    
    // nodes can only be added if network is added
    if (!isStarted())
        BOOST_THROW_EXCEPTION(NetworkStartRequired());

    if (m_dropPeers)
        return;
    
    RecursiveGuard l(x_sessions);
    RLP r(_b);
    unsigned fileVersion = r[0].toInt<unsigned>();
    if (r.itemCount() > 0 && r[0].isInt() && fileVersion >= dev::p2p::c_protocolVersion - 1)
    {
        // r[0] = version
        // r[1] = key
        // r[2] = nodes

        for (auto i: r[2])
        {
            // todo: ipv6
            if (i[0].itemCount() != 4 && i[0].size() != 4)
                continue;

            if (i.itemCount() == 4 || i.itemCount() == 11)
            {
                Node n((NodeID)i[3], NodeIPEndpoint(i));
                if (i.itemCount() == 4 && n.endpoint.isAllowed())
                {
                    addNodeToNodeTable(n);
                }
                else if (i.itemCount() == 11)
                {
                    n.peerType = i[4].toInt<bool>() ? PeerType::Required : PeerType::Optional;
                    if (!n.endpoint.isAllowed() && n.peerType == PeerType::Optional)
                        continue;
                    shared_ptr<Peer> p = make_shared<Peer>(n);
                    p->m_lastConnected = chrono::system_clock::time_point(chrono::seconds(i[5].toInt<unsigned>()));
                    p->m_lastAttempted = chrono::system_clock::time_point(chrono::seconds(i[6].toInt<unsigned>()));
                    p->m_failedAttempts = i[7].toInt<unsigned>();
                    p->m_lastDisconnect = (DisconnectReason)i[8].toInt<unsigned>();
                    p->m_score = (int)i[9].toInt<unsigned>();
                    p->m_rating = (int)i[10].toInt<unsigned>();
                    m_peers[p->id] = p;
                    if (p->peerType == PeerType::Required)
                        requirePeer(p->id, n.endpoint);
                    else 
                        addNodeToNodeTable(*p.get(), NodeTable::NodeRelation::Known);
                }
            }
            else if (i.itemCount() == 3 || i.itemCount() == 10)
            {
                Node n((NodeID)i[2], NodeIPEndpoint(bi::address_v4(i[0].toArray<byte, 4>()), i[1].toInt<uint16_t>(), i[1].toInt<uint16_t>()));
                if (i.itemCount() == 3 && n.endpoint.isAllowed())
                    addNodeToNodeTable(n);
                else if (i.itemCount() == 10)
                {
                    n.peerType = i[3].toInt<bool>() ? PeerType::Required : PeerType::Optional;
                    if (!n.endpoint.isAllowed() && n.peerType == PeerType::Optional)
                        continue;
                    shared_ptr<Peer> p = make_shared<Peer>(n);
                    p->m_lastConnected = chrono::system_clock::time_point(chrono::seconds(i[4].toInt<unsigned>()));
                    p->m_lastAttempted = chrono::system_clock::time_point(chrono::seconds(i[5].toInt<unsigned>()));
                    p->m_failedAttempts = i[6].toInt<unsigned>();
                    p->m_lastDisconnect = (DisconnectReason)i[7].toInt<unsigned>();
                    p->m_score = (int)i[8].toInt<unsigned>();
                    p->m_rating = (int)i[9].toInt<unsigned>();
                    m_peers[p->id] = p;
                    if (p->peerType == PeerType::Required)
                        requirePeer(p->id, n.endpoint);
                    else
                        addNodeToNodeTable(*p.get(), NodeTable::NodeRelation::Known);
                }
            }
        }
    }
}

bool Host::peerSlotsAvailable(Host::PeerSlotType _type /*= Ingress*/)
{
    return peerCount() + m_pendingPeerConns.size() < peerSlots(_type);
}

KeyPair Host::networkAlias(bytesConstRef _b)
{
    RLP r(_b);
    if (r.itemCount() == 3 && r[0].isInt() && r[0].toInt<unsigned>() >= 3)
        return KeyPair(Secret(r[1].toBytes()));
    else
        return KeyPair::create();
}

bool Host::nodeTableHasNode(Public const& _id) const
{
    auto nodeTable = this->nodeTable();
    return nodeTable && nodeTable->haveNode(_id);
}

Node Host::nodeFromNodeTable(Public const& _id) const
{
    auto nodeTable = this->nodeTable();
    return nodeTable ? nodeTable->node(_id) : Node{};
}

bool Host::addNodeToNodeTable(Node const& _node, NodeTable::NodeRelation _relation /* = NodeTable::NodeRelation::Unknown */)
{
    auto nodeTable = this->nodeTable();
    if (!nodeTable)
        return false;

    nodeTable->addNode(_node, _relation);
    return true;
}

void Host::forEachPeer(
    std::string const& _name, u256 const& _version, std::function<bool(NodeID const&)> _f) const
{
    RecursiveGuard l(x_sessions);
    std::vector<std::pair<std::shared_ptr<SessionFace>, std::shared_ptr<Peer>>> sessions;
    for (auto const& i : m_sessions)
        if (std::shared_ptr<SessionFace> s = i.second.lock())
            if (s->capabilities().count(std::make_pair(_name, _version)))
                sessions.push_back(make_pair(s, s->peer()));

    // order peers by protocol, rating, connection age
    auto sessionLess =
        [](std::pair<std::shared_ptr<SessionFace>, std::shared_ptr<Peer>> const& _left,
            std::pair<std::shared_ptr<SessionFace>, std::shared_ptr<Peer>> const& _right) {
            return _left.first->rating() == _right.first->rating() ?
                       _left.first->connectionTime() < _right.first->connectionTime() :
                       _left.first->rating() > _right.first->rating();
        };
    std::sort(sessions.begin(), sessions.end(), sessionLess);

    for (auto s : sessions)
        if (!_f(s.first->id()))
            return;
}
