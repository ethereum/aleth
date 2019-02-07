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
/** @file peer.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 * Peer Network test functions.
 */

#include <libp2p/Capability.h>
#include <libp2p/Host.h>
#include <test/tools/libtesteth/Options.h>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <boost/test/unit_test.hpp>
#include <chrono>
#include <thread>

using namespace std;
using namespace dev;
using namespace dev::test;
using namespace dev::p2p;

class TestCap : public CapabilityFace, public Worker
{
public:
    std::string name() const override { return "p2pTestCapability"; }
    unsigned version() const override { return 2; }
    unsigned messageCount() const override { return UserPacket + 1; }

    void onStarting() override {}
    void onStopping() override {}

    void onConnect(NodeID const&, u256 const&) override {}
    bool interpretCapabilityPacket(NodeID const&, unsigned _id, RLP const& _r) override
    {
        return _id > 0 || _r.size() > 0;
    }
    void onDisconnect(NodeID const&) override {}
};

BOOST_AUTO_TEST_SUITE(libp2p)
BOOST_FIXTURE_TEST_SUITE(p2p, TestOutputHelperFixture)

BOOST_AUTO_TEST_CASE(host)
{
    Host host1("Test", NetworkConfig(c_localhostIp, 0, false /* upnp */, true /* allow local discovery */));
    host1.registerCapability(make_shared<TestCap>());
    host1.start();
    auto host1port = host1.listenPort();
    BOOST_REQUIRE(host1port);

    Host host2("Test", NetworkConfig(c_localhostIp, 0, false /* upnp */, true /* allow local discovery */));
    host2.registerCapability(make_shared<TestCap>());
    host2.start();
    auto host2port = host2.listenPort();
    BOOST_REQUIRE(host2port);
    
    BOOST_REQUIRE_NE(host1port, host2port);

    auto node2 = host2.id();
    int const step = 10;

    // Wait for up to 6 seconds, to give the hosts time to start.
    for (unsigned i = 0; i < 6000; i += step)
    {
        this_thread::sleep_for(chrono::milliseconds(step));

        if (host1.isStarted() && host2.isStarted())
            break;
    }

    BOOST_REQUIRE(host1.isStarted() && host2.isStarted());
    
    // Wait for up to 6 seconds, to give the hosts time to get their network connection established
    for (unsigned i = 0; i < 6000; i += step)
    {
        this_thread::sleep_for(chrono::milliseconds(step));

        if (host1.haveNetwork() && host2.haveNetwork())
            break;
    }

    BOOST_REQUIRE(host1.haveNetwork() && host2.haveNetwork());
    host1.addNode(node2, NodeIPEndpoint(bi::address::from_string(c_localhostIp), host2port, host2port));

    // Wait for up to 24 seconds, to give the hosts time to find each other
    for (unsigned i = 0; i < 24000; i += step)
    {
        this_thread::sleep_for(chrono::milliseconds(step));

        if ((host1.peerCount() > 0) && (host2.peerCount() > 0))
            break;
    }

    BOOST_REQUIRE_EQUAL(host1.peerCount(), 1);
    BOOST_REQUIRE_EQUAL(host2.peerCount(), 1);
}

BOOST_AUTO_TEST_CASE(attemptNetworkRestart)
{
    Host host("Test",
        NetworkConfig(c_localhostIp, randomPortNumber(), false /* upnp */, true /* allow local discovery */));
    host.start();
    BOOST_REQUIRE(host.listenPort());
    BOOST_REQUIRE(host.haveNetwork());
    host.stop();
    BOOST_REQUIRE(!host.haveNetwork());
    BOOST_REQUIRE_THROW(host.start(), NetworkRestartNotSupported);
}

BOOST_AUTO_TEST_CASE(networkConfig)
{
    Host save("Test", NetworkConfig(false));
    bytes store(save.saveNetwork());
    
    Host restore("Test", NetworkConfig(false), bytesConstRef(&store));
    BOOST_REQUIRE(save.id() == restore.id());
}

BOOST_AUTO_TEST_CASE(registerCapabilityAfterNetworkStart)
{
    Host host("Test",
        NetworkConfig(c_localhostIp, 0, false /* upnp */, true /* allow local discovery */));
    host.start();

    // Wait for up to 6 seconds, to give the hosts time to get their network connection established
    int const step = 10;
    for (unsigned i = 0; i < 6000; i += step)
    {
        this_thread::sleep_for(chrono::milliseconds(step));

        if (host.haveNetwork())
            break;
    }
    BOOST_REQUIRE(host.haveNetwork());
    BOOST_REQUIRE(!host.haveCapabilities());
    host.registerCapability(make_shared<TestCap>());
    BOOST_REQUIRE(!host.haveCapabilities());
}

BOOST_AUTO_TEST_CASE(saveNodes)
{
    std::list<Host*> hosts;
    unsigned const c_step = 10;
    unsigned const c_nodes = 6;
    unsigned const c_peers = c_nodes - 1;
    std::set<short unsigned> ports;

    for (unsigned i = 0; i < c_nodes; ++i)
    {
        Host* h = new Host("Test", NetworkConfig(c_localhostIp, 0, false /* upnp */, true /* allow local discovery */));
        h->setIdealPeerCount(10);
        h->registerCapability(make_shared<TestCap>());
        h->start(); // starting host is required so listenport is available
        while (!h->haveNetwork())
            this_thread::sleep_for(chrono::milliseconds(c_step));

        BOOST_REQUIRE(h->listenPort());
        bool inserted = ports.insert(h->listenPort()).second;
        BOOST_REQUIRE(inserted);
        h->registerCapability(make_shared<TestCap>());
        hosts.push_back(h);
    }
    
    Host& host = *hosts.front();
    for (auto const& h: hosts)
        host.addNode(h->id(), NodeIPEndpoint(bi::address::from_string(c_localhostIp), h->listenPort(), h->listenPort()));

    for (unsigned i = 0; i < c_peers * 1000 && host.peerCount() < c_peers; i += c_step)
        this_thread::sleep_for(chrono::milliseconds(c_step));

    Host& host2 = *hosts.back();
    for (auto const& h: hosts)
        host2.addNode(h->id(), NodeIPEndpoint(bi::address::from_string(c_localhostIp), h->listenPort(), h->listenPort()));

    for (unsigned i = 0; i < c_peers * 2000 && host2.peerCount() < c_peers; i += c_step)
        this_thread::sleep_for(chrono::milliseconds(c_step));

    BOOST_CHECK_EQUAL(host.peerCount(), c_peers);
    BOOST_CHECK_EQUAL(host2.peerCount(), c_peers);

    host.stop();

    // Wait for up to 6 seconds, to give the host time to shut down
    int const step = 10;
    for (unsigned i = 0; i < 6000; i += step)
    {
        this_thread::sleep_for(chrono::milliseconds(step));
        if (!host.haveNetwork())
            break;
    }

    bytes firstHostNetwork(host.saveNetwork());
    bytes secondHostNetwork(host.saveNetwork());	
    BOOST_REQUIRE_EQUAL(sha3(firstHostNetwork), sha3(secondHostNetwork));	
    
    RLP r(firstHostNetwork);
    BOOST_REQUIRE(r.itemCount() == 3);
    BOOST_REQUIRE(r[0].toInt<unsigned>() == dev::p2p::c_protocolVersion);
    BOOST_REQUIRE_EQUAL(r[1].toBytes().size(), 32); // secret
    BOOST_REQUIRE(r[2].itemCount() >= c_nodes);
    
    for (auto i: r[2])
    {
        BOOST_REQUIRE(i.itemCount() == 6 || i.itemCount() == 11);
        BOOST_REQUIRE(i[0].size() == 4 || i[0].size() == 16);
    }

    for (auto host: hosts)
        delete host;
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(p2pPeer, TestOutputHelperFixture)

BOOST_AUTO_TEST_CASE(requirePeer)
{
    unsigned const step = 10;
    const char* const localhost = c_localhostIp;
    NetworkConfig prefs1(localhost, 0, false /* upnp */, true /* allow local discovery */);
    NetworkConfig prefs2(localhost, 0, false /* upnp */, true /* allow local discovery */);
    Host host1("Test", prefs1);
    Host host2("Test", prefs2);
    host1.registerCapability(make_shared<TestCap>());
    host2.registerCapability(make_shared<TestCap>());
    host1.start();
    host2.start();
    auto node2 = host2.id();
    auto port1 = host1.listenPort();
    auto port2 = host2.listenPort();
    BOOST_REQUIRE(port1);
    BOOST_REQUIRE(port2);
    BOOST_REQUIRE_NE(port1, port2);

    host1.requirePeer(node2, NodeIPEndpoint(bi::address::from_string(localhost), port2, port2));

    // Wait for up to 12 seconds, to give the hosts time to connect to each other.
    for (unsigned i = 0; i < 12000; i += step)
    {
        this_thread::sleep_for(chrono::milliseconds(step));

        if ((host1.peerCount() > 0) && (host2.peerCount() > 0))
            break;
    }

    auto host1peerCount = host1.peerCount();
    auto host2peerCount = host2.peerCount();
    BOOST_REQUIRE_EQUAL(host1peerCount, 1);
    BOOST_REQUIRE_EQUAL(host2peerCount, 1);

    PeerSessionInfos sis1 = host1.peerSessionInfo();
    PeerSessionInfos sis2 = host2.peerSessionInfo();

    BOOST_REQUIRE_EQUAL(sis1.size(), 1);
    BOOST_REQUIRE_EQUAL(sis2.size(), 1);

    Peers peers1 = host1.getPeers();
    Peers peers2 = host2.getPeers();
    BOOST_REQUIRE_EQUAL(peers1.size(), 1);
    BOOST_REQUIRE_EQUAL(peers2.size(), 1);

    DisconnectReason disconnect1 = peers1[0].lastDisconnect();
    DisconnectReason disconnect2 = peers2[0].lastDisconnect();
    BOOST_REQUIRE_EQUAL(disconnect1, disconnect2);

    host1.relinquishPeer(node2);

    // Wait for up to 6 seconds, to give the hosts time to disconnect from each other.
    for (unsigned i = 0; i < 6000; i += step)
    {
        this_thread::sleep_for(chrono::milliseconds(step));

        if ((host1.peerCount() == 0) && (host2.peerCount() == 0))
            break;
    }

    host1peerCount = host1.peerCount();
    host2peerCount = host2.peerCount();
    BOOST_REQUIRE_EQUAL(host1peerCount, 1);
    BOOST_REQUIRE_EQUAL(host2peerCount, 1);
}

BOOST_AUTO_TEST_CASE(requirePeerNoNetwork)
{
    NetworkConfig prefs1(c_localhostIp, 0, false /* upnp */, true /* allow local discovery */);
    NetworkConfig prefs2(c_localhostIp, 0, false /* upnp */, true /* allow local discovery */);
    Host host1("Test", prefs1);
    Host host2("Test", prefs2);
    auto node2 = host2.id();

    // Both hosts' ports should be 0 since we haven't started their network threads
    BOOST_REQUIRE_EQUAL(host1.listenPort(), 0);
    BOOST_REQUIRE_EQUAL(host2.listenPort(), 0);

    host1.registerCapability(make_shared<TestCap>());
    host2.registerCapability(make_shared<TestCap>());

    host1.requirePeer(node2, NodeIPEndpoint(bi::address::from_string(c_localhostIp),
                                 0 /* tcp port */, 0 /* udp port */));

    BOOST_REQUIRE(!host1.isRequiredPeer(node2));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(peerTypes, TestOutputHelperFixture)

BOOST_AUTO_TEST_CASE(emptySharedPeer)
{
    shared_ptr<Peer> p;
    BOOST_REQUIRE(!p);
    
    std::map<NodeID, std::shared_ptr<Peer>> peers;
    p = peers[NodeID()];
    BOOST_REQUIRE(!p);
    
    p.reset(new Peer(UnspecifiedNode));
    BOOST_REQUIRE(!p->id);
    BOOST_REQUIRE(!*p);
    
    p.reset(new Peer(Node(NodeID(EmptySHA3), UnspecifiedNodeIPEndpoint)));
    BOOST_REQUIRE(!(!*p));
    BOOST_REQUIRE(*p);
    BOOST_REQUIRE(p);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

int peerTest(int argc, char** argv)
{
    Public remoteAlias;
    short listenPort = 30304;
    string remoteHost;
    short remotePort = 30304;
    
    for (int i = 1; i < argc; ++i)
    {
        string arg = argv[i];
        if (arg == "-l" && i + 1 < argc)
            listenPort = (short)atoi(argv[++i]);
        else if (arg == "-r" && i + 1 < argc)
            remoteHost = argv[++i];
        else if (arg == "-p" && i + 1 < argc)
            remotePort = (short)atoi(argv[++i]);
        else if (arg == "-ra" && i + 1 < argc)
            remoteAlias = Public(dev::fromHex(argv[++i]));
        else
            remoteHost = argv[i];
    }

    Host ph("Test", NetworkConfig(listenPort));

    if (!remoteHost.empty() && !remoteAlias)
        ph.addNode(remoteAlias, NodeIPEndpoint(bi::address::from_string(remoteHost), remotePort, remotePort));

    this_thread::sleep_for(chrono::milliseconds(200));

    return 0;
}

