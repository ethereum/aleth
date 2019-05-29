// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2018 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "Host.h"
#include "Capability.h"
#include "CapabilityHost.h"
#include "Common.h"
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
/// Interval at which Host::run will call keepAlivePeers to ping peers.
constexpr chrono::seconds c_keepAliveInterval{30};

/// Disconnect timeout after failure to respond to keepAlivePeers ping.
constexpr chrono::seconds c_keepAliveTimeOut{1};

/// Interval which m_runTimer is run when network is connected.
constexpr chrono::milliseconds c_runTimerInterval{100};

/// Interval at which active peer info is logged
constexpr chrono::seconds c_logActivePeersInterval{30};
}  // namespace

HostNodeTableHandler::HostNodeTableHandler(Host& _host): m_host(_host) {}

void HostNodeTableHandler::processEvent(NodeID const& _n, NodeTableEventType const& _e)
{
    m_host.onNodeTableEvent(_n, _e);
}

void ReputationManager::noteRude(SessionFace const& _s, string const& _sub)
{
    DEV_WRITE_GUARDED(x_nodes)
        m_nodes[make_pair(_s.id(), _s.info().clientVersion)].subs[_sub].isRude = true;
}

bool ReputationManager::isRude(SessionFace const& _s, string const& _sub) const
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

void ReputationManager::setData(SessionFace const& _s, string const& _sub, bytes const& _data)
{
    DEV_WRITE_GUARDED(x_nodes)
        m_nodes[make_pair(_s.id(), _s.info().clientVersion)].subs[_sub].data = _data;
}

bytes ReputationManager::data(SessionFace const& _s, string const& _sub) const
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

Host::Host(
    string const& _clientVersion, pair<Secret, ENR> const& _secretAndENR, NetworkConfig const& _n)
  : Worker("p2p", 0),
    m_clientVersion(_clientVersion),
    m_netConfig(_n),
    m_ifAddresses(Network::getInterfaceAddresses()),
    m_ioService(2),  // concurrency hint, suggests how many threads it should allow to run
                     // simultaneously
    m_tcp4Acceptor(m_ioService),
    m_runTimer(m_ioService),
    m_alias{_secretAndENR.first},
    m_restoredENR{_secretAndENR.second},
    m_lastPing(chrono::steady_clock::time_point::min()),
    m_capabilityHost(createCapabilityHost(*this)),
    m_lastPeerLogMessage(chrono::steady_clock::time_point::min())
{
    LOG(m_infoLogger) << "Id: " << id();
    LOG(m_infoLogger) << "ENR: " << m_restoredENR;
}

Host::Host(string const& _clientVersion, NetworkConfig const& _n, bytesConstRef _restoreNetwork)
  : Host(_clientVersion, restoreENR(_restoreNetwork, _n), _n)
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
    if (m_nodeTable)
        BOOST_THROW_EXCEPTION(NetworkRestartNotSupported());

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

    // ignore if already stopped/stopping, at the same time
    // indicates that the network is shutting down
    if (!m_run.exchange(false))
        return;

    // stopping io service allows running manual network operations for shutdown
    // and also stops blocking worker thread, allowing worker thread to exit
    m_ioService.stop();

    // Close the node table socket and cancel deadline timers. This effectively stops
    // discovery, even during subsequent io service polling
    nodeTable()->stop();

    // stop worker thread
    if (isWorking())
        stopWorking();
}

void Host::startCapabilities()
{
    for (auto const& itCap : m_capabilities)
    {
        scheduleCapabilityWorkLoop(*itCap.second.capability, itCap.second.backgroundWorkTimer);
    }
}

void Host::scheduleCapabilityWorkLoop(CapabilityFace& _cap, shared_ptr<ba::steady_timer> _timer)
{
    _timer->expires_from_now(_cap.backgroundWorkInterval());
    _timer->async_wait([this, _timer, &_cap](boost::system::error_code _ec) {
        if (_timer->expires_at() == c_steadyClockMin ||
            _ec == boost::asio::error::operation_aborted)
        {
            LOG(m_logger) << "Timer was probably cancelled for capability: " << _cap.descriptor();
            return;
        }
        else if (_ec)
        {
            LOG(m_logger) << "Timer error detected for capability: " << _cap.descriptor();
            return;
        }

        _cap.doBackgroundWork();
        scheduleCapabilityWorkLoop(_cap, move(_timer));
    });
}

void Host::stopCapabilities()
{
    for (auto const& itCap : m_capabilities)
    {
        auto timer = itCap.second.backgroundWorkTimer;
        m_ioService.post([timer] { timer->expires_at(c_steadyClockMin); });
    }
}

void Host::doneWorking()
{
    // Return early if we have no capabilities since there's nothing to do. We've already stopped
    // the io service and cleared the node table timers which means discovery is no longer running.
    if (!haveCapabilities())
        return;

    // reset ioservice (allows manually polling network, below)
    m_ioService.reset();

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

    // (eth: stops syncing or block / tx broadcast). Capabilities will be cancelled when ioservice
    // is polled on pending handshake / peer disconnect
    stopCapabilities();

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

    // finally, clear out peers (in case they're lingering)
    RecursiveGuard l(x_sessions);
    m_sessions.clear();
}

// Starts a new peer session after a successful handshake - agree on mutually-supported capablities,
// start each mutually-supported capability, and send a ping to the node.
void Host::startPeerSession(Public const& _id, RLP const& _hello,
    unique_ptr<RLPXFrameCoder>&& _io, shared_ptr<RLPXSocket> const& _s)
{
    // session maybe ingress or egress so m_peers and node table entries may not exist
    shared_ptr<Peer> peer;
    DEV_RECURSIVE_GUARDED(x_sessions)
    {
        auto const remoteAddress = _s->remoteEndpoint().address();
        auto const remoteTcpPort = _s->remoteEndpoint().port();

        peer = findPeer(_id, remoteAddress, remoteTcpPort);
        if (!peer)
        {
            peer = make_shared<Peer>(Node{_id, NodeIPEndpoint{remoteAddress, 0, remoteTcpPort}});
            m_peers[_id] = peer;
        }
    }
    if (peer->isOffline())
        peer->m_lastConnected = std::chrono::system_clock::now();

    auto const protocolVersion = _hello[0].toInt<unsigned>();
    auto const clientVersion = _hello[1].toString();
    auto caps = _hello[2].toVector<CapDesc>();
    auto const listenPort = _hello[3].toInt<unsigned short>();
    auto const pub = _hello[4].toHash<Public>();

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

    cnetlog << "Starting peer session with " << clientVersion << " (protocol: V" << protocolVersion
            << ") " << _id << " " << showbase << "capabilities: " << capslog.str() << " " << dec
            << "port: " << listenPort;

    // create session so disconnects are managed
    shared_ptr<SessionFace> session = make_shared<Session>(this, move(_io), _s, peer,
        PeerSessionInfo({_id, clientVersion, peer->endpoint.address().to_string(), listenPort,
            chrono::steady_clock::duration(), _hello[2].toSet<CapDesc>(),
            map<string, string>()}));
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

        m_sessions[_id] = session;

        unsigned offset = (unsigned)UserPacket;

        // todo: mutex Session::m_capabilities and move for(:caps) out of mutex.
        for (auto const& capDesc : caps)
        {
            auto itCap = m_capabilities.find(capDesc);
            if (itCap == m_capabilities.end())
                return session->disconnect(IncompatibleProtocol);

            auto capability = itCap->second.capability;
            session->registerCapability(capDesc, offset, capability);

            cnetlog << "New session for capability " << capDesc.first << "; idOffset: " << offset
                    << " with " << _id << "@" << _s->remoteEndpoint();

            capability->onConnect(_id, capDesc.second);

            offset += capability->messageCount();
        }

        session->start();
    }

    LOG(m_logger) << "Peer connection successfully established with " << _id << "@"
                  << _s->remoteEndpoint();
}

void Host::onNodeTableEvent(NodeID const& _nodeID, NodeTableEventType const& _e)
{
    if (_e == NodeEntryAdded)
    {
        LOG(m_logger) << "p2p.host.nodeTable.events.nodeEntryAdded " << _nodeID;
        if (Node node = nodeFromNodeTable(_nodeID))
        {
            shared_ptr<Peer> peer;
            DEV_RECURSIVE_GUARDED(x_sessions)
            {
                peer = findPeer(_nodeID, node.endpoint.address(), node.endpoint.tcpPort());
                if (!peer)
                {
                    peer = make_shared<Peer>(node);
                    m_peers[_nodeID] = peer;
                    LOG(m_logger) << "p2p.host.peers.events.peerAdded " << _nodeID << " "
                                  << peer->endpoint;
                }
            }
            if (peerSlotsAvailable(Egress))
                connect(peer);
        }
    }
    else if (_e == NodeEntryDropped)
    {
        LOG(m_logger) << "p2p.host.nodeTable.events.NodeEntryDropped " << _nodeID;
        RecursiveGuard l(x_sessions);
        if (m_peers.count(_nodeID) && m_peers[_nodeID]->peerType == PeerType::Optional)
            m_peers.erase(_nodeID);
    }
}

bool Host::isHandshaking(NodeID const& _id) const
{
    Guard l(x_connecting);
    for (auto const& cIter : m_connecting)
    {
        std::shared_ptr<RLPXHandshake> const connecting = cIter.lock();
        if (connecting && connecting->remote() == _id)
            return true;
    }
    return false;
}

bi::tcp::endpoint Host::determinePublic() const
{
    // return listenIP (if public) > public > upnp > unspecified address.

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
        ep = Network::traverseNAT(lset && ifAddresses.count(laddr) ? set<bi::address>({laddr}) : ifAddresses, m_listenPort, natIFAddr);
        
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

    return ep;
}

ENR Host::updateENR(
    ENR const& _restoredENR, bi::tcp::endpoint const& _tcpPublic, uint16_t const& _listenPort)
{
    auto const address =
        _tcpPublic.address().is_unspecified() ? _restoredENR.ip() : _tcpPublic.address();

    if (_restoredENR.ip() == address && _restoredENR.tcpPort() == _listenPort &&
        _restoredENR.udpPort() == _listenPort)
        return _restoredENR;

    ENR const newENR = IdentitySchemeV4::updateENR(
        _restoredENR, m_alias.secret(), address, _listenPort, _listenPort);
    LOG(m_infoLogger) << "ENR updated: " << newENR;

    return newENR;
}

void Host::runAcceptor()
{
    assert(m_listenPort > 0);

    if (m_tcp4Acceptor.is_open() && !m_accepting)
    {
        cnetdetails << "Listening on local port " << m_listenPort;
        m_accepting = true;

        auto socket = make_shared<RLPXSocket>(m_ioService);
        m_tcp4Acceptor.async_accept(socket->ref(), [=](boost::system::error_code ec)
        {
            m_accepting = false;
            if (ec || !m_tcp4Acceptor.is_open())
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
            catch (exception const& _e)
            {
                cwarn << "ERROR: " << _e.what();
            }

            if (!success)
                socket->ref().close();
            runAcceptor();
        });
    }
}

void Host::registerCapability(shared_ptr<CapabilityFace> const& _cap)
{
    registerCapability(_cap, _cap->name(), _cap->version());
}

void Host::registerCapability(
    shared_ptr<CapabilityFace> const& _cap, string const& _name, unsigned _version)
{
    if (haveNetwork())
    {
        cwarn << "Capabilities must be registered before the network is started";
        return;
    }
    m_capabilities[{_name, _version}] = {_cap, make_shared<ba::steady_timer>(m_ioService)};
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
        cnetdetails << "Ignoring the request to connect to self " << _node;
        return;
    }

    addNodeToNodeTable(Node(_node, _endpoint));
}

void Host::requirePeer(NodeID const& _nodeID, NodeIPEndpoint const& _endpoint)
{
    if (!m_run)
    {
        cwarn << "Network not running so node (" << _nodeID << ") with endpoint (" << _endpoint
              << ") cannot be added as a required peer";
        return;
    }
    if (!haveCapabilities())
    {
        cwarn << "No capabilities registered so node (" << _nodeID << ") with endpoint (" << _endpoint
            << ") cannot be added as a required peer";
        return;
    }

    if (_nodeID == id())
    {
        cnetdetails << "Ignoring the request to connect to self " << _nodeID;
        return;
    }

    if (!_nodeID)
    {
        cnetdetails << "Ignoring the request to connect to null node id.";
        return;
    }

    {
        Guard l(x_requiredPeers);
        m_requiredPeers.insert(_nodeID);
    }

    Node node(_nodeID, _endpoint, PeerType::Required);
    // create or update m_peers entry
    shared_ptr<Peer> peer;
    DEV_RECURSIVE_GUARDED(x_sessions)
    {
        peer = findPeer(_nodeID, node.endpoint.address(), node.endpoint.tcpPort());
        if (peer)
            peer->peerType = PeerType::Required;
        else
        {
            peer = make_shared<Peer>(node);
            m_peers[_nodeID] = peer;
        }
    }
    // required for discovery
    addNodeToNodeTable(*peer);
}

bool Host::isRequiredPeer(NodeID const& _id) const
{
    Guard l(x_requiredPeers);
    return m_requiredPeers.count(_id);
}

void Host::relinquishPeer(NodeID const& _node)
{
    Guard l(x_requiredPeers);
    if (m_requiredPeers.count(_node))
        m_requiredPeers.erase(_node);
}

void Host::connect(shared_ptr<Peer> const& _p)
{
    if (!m_run)
    {
        cwarn << "Network not running so cannot connect to peer " << _p->id << "@" << _p->address();
        return;
    }

    if (!haveCapabilities())
    {
        cwarn << "No capabilities registered so cannot connect to peer " << _p->id << "@" << _p->address();
        return;
    }

    if (isHandshaking(_p->id))
    {
        cwarn << "Aborted connection. RLPX handshake with peer already in progress: " << _p->id
              << "@" << _p->endpoint;
        return;
    }

    if (havePeerSession(_p->id))
    {
        cnetdetails << "Aborted connection. Peer already connected: " << _p->id << "@"
                    << _p->endpoint;
        return;
    }

    if (!nodeTableHasNode(_p->id) && _p->peerType == PeerType::Optional)
        return;

    // prevent concurrently connecting to a node
    Peer *nptr = _p.get();
    if (m_pendingPeerConns.count(nptr))
        return;
    m_pendingPeerConns.insert(nptr);

    _p->m_lastAttempted = chrono::system_clock::now();
    
    bi::tcp::endpoint ep(_p->endpoint);
    cnetdetails << "Attempting connection to " << _p->id << "@" << ep << " from " << id();
    auto socket = make_shared<RLPXSocket>(m_ioService);
    socket->ref().async_connect(ep, [=](boost::system::error_code const& ec)
    {
        _p->m_lastAttempted = chrono::system_clock::now();
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
            cnetdetails << "Starting RLPX handshake with " << _p->id << "@" << ep;
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

PeerSessionInfos Host::peerSessionInfos() const
{
    if (!m_run)
        return PeerSessionInfos();

    vector<PeerSessionInfo> ret;
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
        if (shared_ptr<SessionFace> j = i.second.lock())
            if (j->isConnected())
                retCount++;
    return retCount;
}

void Host::run(boost::system::error_code const& _ec)
{
    if (!m_run || _ec)
        return;

    // This again requires x_nodeTable, which is why an additional variable nodeTable is used.
    if (auto nodeTable = this->nodeTable())
        nodeTable->processEvents();

    // cleanup zombies
    DEV_GUARDED(x_connecting)
        m_connecting.remove_if([](weak_ptr<RLPXHandshake> h){ return h.expired(); });

    keepAlivePeers();
    logActivePeers();

    // At this time peers will be disconnected based on natural TCP timeout.
    // disconnectLatePeers needs to be updated for the assumption that Session
    // is always live and to ensure reputation and fallback timers are properly
    // updated. // disconnectLatePeers();

    // todo: update peerSlotsAvailable()

    list<shared_ptr<Peer>> toConnect;
    unsigned reqConn = 0;
    {
        RecursiveGuard l(x_sessions);
        for (auto const& p : m_peers)
        {
            bool haveSession = havePeerSession(p.second->id);
            bool required = p.second->peerType == PeerType::Required;
            if (haveSession && required)
                reqConn++;
            else if (!haveSession && p.second->shouldReconnect() &&
                        (!m_netConfig.pin || required))
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

    if (!m_run)
        return;

    auto runcb = [this](boost::system::error_code const& error) { run(error); };
    m_runTimer.expires_from_now(c_runTimerInterval);
    m_runTimer.async_wait(runcb);
}

// Called after thread has been started to perform additional class-specific state
// initialization (e.g. start capability threads, start TCP listener, and kick off timers)
void Host::startedWorking()
{
    if (haveCapabilities())
    {
        startCapabilities();

        // try to open acceptor (todo: ipv6)
        int port = Network::tcp4Listen(m_tcp4Acceptor, m_netConfig);
        if (port > 0)
        {
            m_listenPort = port;
            runAcceptor();
        }
        else
            LOG(m_logger) << "p2p.start.notice id: " << id() << " TCP Listen port is invalid or unavailable.";
    }
    else
        m_listenPort = m_netConfig.listenPort;

    m_tcpPublic = determinePublic();
    ENR const enr = updateENR(m_restoredENR, m_tcpPublic, listenPort());

    auto nodeTable = make_shared<NodeTable>(m_ioService, m_alias,
        NodeIPEndpoint(bi::address::from_string(listenAddress()), listenPort(), listenPort()), enr,
        m_netConfig.discovery, m_netConfig.allowLocalDiscovery);

    // Don't set an event handler if we don't have capabilities, because no capabilities
    // means there's no host state to update in response to node table events
    if (haveCapabilities())
        nodeTable->setEventHandler(new HostNodeTableHandler(*this));
    DEV_GUARDED(x_nodeTable)
        m_nodeTable = nodeTable;
    
    m_run = true;
    restoreNetwork(&m_restoreNetwork);
    if (haveCapabilities())
    {
        LOG(m_logger) << "devp2p started. Node id: " << id();
        run(boost::system::error_code());
    }
    else
        LOG(m_logger) << "No registered capabilities, devp2p not started.";
}

void Host::doWork()
{
    try
    {
        if (m_run)
            m_ioService.run();
    }
    catch (exception const& _e)
    {
        cwarn << "Exception in Network Thread: " << _e.what();
        cwarn << "Network Restart is Recommended.";
    }
}

void Host::keepAlivePeers()
{
    if (!m_run || chrono::steady_clock::now() - c_keepAliveInterval < m_lastPing)
        return;

    RecursiveGuard l(x_sessions);
    {
        for (auto it = m_sessions.begin(); it != m_sessions.end();)
            if (auto p = it->second.lock())
            {
                p->ping();
                ++it;
            }
            else
                it = m_sessions.erase(it);
    }

    m_lastPing = chrono::steady_clock::now();
}

void Host::logActivePeers()
{
    if (!m_run || chrono::steady_clock::now() - c_logActivePeersInterval < m_lastPeerLogMessage)
        return;

    LOG(m_infoLogger) << "Active peer count: " << peerCount();
    if (m_netConfig.discovery)
        LOG(m_infoLogger) << "Looking for peers...";

    LOG(m_logger) << "Peers: " << peerSessionInfos();
    m_lastPeerLogMessage = chrono::steady_clock::now();
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
    if (haveNetwork())
    {
        cwarn << "Cannot save network configuration while network is still running.";
        return bytes{};
    }

    RLPStream network;
    list<NodeEntry> nodeTableEntries;
    DEV_GUARDED(x_nodeTable)
    {
        if (m_nodeTable)
            nodeTableEntries = m_nodeTable->snapshot();
    }
    int count = 0;
    for (auto const& entry : nodeTableEntries)
    {
        network.appendList(6);
        entry.endpoint().streamRLP(network, NodeIPEndpoint::StreamInline);
        network << entry.id() << entry.lastPongReceivedTime << entry.lastPongSentTime;
        count++;
    }

    vector<Peer> peers;
    {
        RecursiveGuard l(x_sessions);
        for (auto const& p: m_peers)
            if (p.second)
                peers.push_back(*p.second);
    }

    for (auto const& p: peers)
    {
        // todo: ipv6
        if (!p.endpoint.address().is_v4())
            continue;

        // Only save peers which have connected within 2 days, with properly-advertised port and
        // public IP address
        if (chrono::system_clock::now() - p.m_lastConnected < chrono::seconds(3600 * 48) &&
            !!p.endpoint && p.id != id() &&
            (p.peerType == PeerType::Required || isAllowedEndpoint(p.endpoint)))
        {
            network.appendList(11);
            p.endpoint.streamRLP(network, NodeIPEndpoint::StreamInline);
            network << p.id << (p.peerType == PeerType::Required)
                    << chrono::duration_cast<chrono::seconds>(p.m_lastConnected.time_since_epoch()).count()
                    << chrono::duration_cast<chrono::seconds>(p.m_lastAttempted.time_since_epoch()).count()
                    << p.m_failedAttempts.load() << (unsigned)p.m_lastDisconnect << p.m_score.load()
                    << p.m_rating.load();
            count++;
        }
    }

    RLPStream ret(3);
    ret << dev::p2p::c_protocolVersion;

    ret.appendList(2);
    ret << m_alias.secret().ref();
    enr().streamRLP(ret);

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

    RecursiveGuard l(x_sessions);
    RLP r(_b);
    auto const protocolVersion = r[0].toInt<unsigned>();
    if (r.itemCount() > 0 && r[0].isInt() && protocolVersion >= dev::p2p::c_protocolVersion)
    {
        // r[0] = version
        // r[1] = key
        // r[2] = nodes

        for (auto const& nodeRLP : r[2])
        {
            // nodeRLP[0] - IP address
            // todo: ipv6
            if (nodeRLP[0].itemCount() != 4 && nodeRLP[0].size() != 4)
                continue;

            Node node((NodeID)nodeRLP[3], NodeIPEndpoint(nodeRLP));
            if (nodeRLP.itemCount() == 6 && isAllowedEndpoint(node.endpoint))
            {
                // node was saved from the node table
                auto const lastPongReceivedTime = nodeRLP[4].toInt<uint32_t>();
                auto const lastPongSentTime = nodeRLP[5].toInt<uint32_t>();
                addKnownNodeToNodeTable(node, lastPongReceivedTime, lastPongSentTime);
            }
            else if (nodeRLP.itemCount() == 11)
            {
                // node was saved from the connected peer list
                node.peerType = nodeRLP[4].toInt<bool>() ? PeerType::Required : PeerType::Optional;
                if (!isAllowedEndpoint(node.endpoint) && node.peerType == PeerType::Optional)
                    continue;
                shared_ptr<Peer> peer = make_shared<Peer>(node);
                peer->m_lastConnected =
                    chrono::system_clock::time_point(chrono::seconds(nodeRLP[5].toInt<unsigned>()));
                peer->m_lastAttempted =
                    chrono::system_clock::time_point(chrono::seconds(nodeRLP[6].toInt<unsigned>()));
                peer->m_failedAttempts = nodeRLP[7].toInt<unsigned>();
                peer->m_lastDisconnect = (DisconnectReason)nodeRLP[8].toInt<unsigned>();
                peer->m_score = (int)nodeRLP[9].toInt<unsigned>();
                peer->m_rating = (int)nodeRLP[10].toInt<unsigned>();
                m_peers[peer->id] = peer;
                if (peer->peerType == PeerType::Required)
                    requirePeer(peer->id, node.endpoint);
                else
                    addNodeToNodeTable(*peer);
            }
        }
    }
}

bool Host::peerSlotsAvailable(Host::PeerSlotType _type /*= Ingress*/)
{
    return peerCount() + m_pendingPeerConns.size() < peerSlots(_type);
}

std::pair<Secret, ENR> Host::restoreENR(bytesConstRef _b, NetworkConfig const& _netConfig)
{
    RLP r(_b);
    Secret secret;
    if (r.itemCount() == 3 && r[0].isInt() && r[0].toInt<unsigned>() >= 3)
    {
        if (r[1].isList())
        {
            secret = Secret{r[1][0].toBytes()};
            auto enrRlp = r[1][1];

            return make_pair(secret, IdentitySchemeV4::parseENR(enrRlp));
        }

        // Support for older format without ENR
        secret = Secret{r[1].toBytes()};
    }
    else
    {
        // no private key found, create new one
        secret = KeyPair::create().secret();
    }

    auto const address = _netConfig.publicIPAddress.empty() ?
                             bi::address{} :
                             bi::address::from_string(_netConfig.publicIPAddress);

    return make_pair(secret,
        IdentitySchemeV4::createENR(secret, address, _netConfig.listenPort, _netConfig.listenPort));
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

bool Host::addNodeToNodeTable(Node const& _node)
{
    auto nodeTable = this->nodeTable();
    if (!nodeTable)
        return false;

    return nodeTable->addNode(_node);
}

bool Host::addKnownNodeToNodeTable(
    Node const& _node, uint32_t _lastPongReceivedTime, uint32_t _lastPongSentTime)
{
    auto nt = nodeTable();
    if (!nt)
        return false;

    return nt->addKnownNode(_node, _lastPongReceivedTime, _lastPongSentTime);
}

void Host::forEachPeer(
    string const& _capabilityName, function<bool(NodeID const&)> _f) const
{
    RecursiveGuard l(x_sessions);
    vector<shared_ptr<SessionFace>> sessions;
    for (auto const& i : m_sessions)
        if (shared_ptr<SessionFace> s = i.second.lock())
        {
            vector<CapDesc> capabilities = s->capabilities();
            for (auto const& cap : capabilities)
                if (cap.first == _capabilityName)
                    sessions.emplace_back(move(s));
        }

    // order peers by rating, connection age
    auto sessionLess = [](shared_ptr<SessionFace> const& _left,
                           shared_ptr<SessionFace> const& _right) {
        return _left->rating() == _right->rating() ?
                   _left->connectionTime() < _right->connectionTime() :
                   _left->rating() > _right->rating();
    };
    sort(sessions.begin(), sessions.end(), sessionLess);

    for (auto const& s : sessions)
        if (!_f(s->id()))
            return;
}

std::shared_ptr<Peer> Host::findPeer(
    NodeID const& _nodeID, bi::address const& _address, unsigned short _tcpPort) const
{
    auto const itPeer = m_peers.find(_nodeID);
    if (itPeer != m_peers.end() && itPeer->second->endpoint.address() == _address &&
        itPeer->second->endpoint.tcpPort() == _tcpPort)
        return itPeer->second;

    return {};
}