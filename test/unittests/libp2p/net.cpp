// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2018 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include <libdevcore/Assertions.h>
#include <libdevcore/Worker.h>
#include <libdevcore/concurrent_queue.h>
#include <libdevcrypto/Common.h>
#include <libp2p/NodeTable.h>
#include <libp2p/Host.h>
#include <libp2p/Network.h>
#include <libp2p/UDP.h>
#include <test/tools/libtesteth/Options.h>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <boost/test/unit_test.hpp>
#include <future>

using namespace std;
using namespace dev;
using namespace dev::test;
using namespace dev::p2p;
namespace ba = boost::asio;
namespace bi = ba::ip;

BOOST_AUTO_TEST_SUITE(network)

BOOST_FIXTURE_TEST_SUITE(net, TestOutputHelperFixture)

/**
 * Only used for testing. Not useful beyond tests.
 */
class TestHost: public Worker
{
public:
    TestHost(): Worker("test",0), m_io() {};
    ~TestHost() override { m_io.stop(); terminate(); }
    void start() { startWorking(); }
    void doWork() override { m_io.run(); }
    void doneWorking() override
    {
        m_io.restart();
        m_io.poll();
        m_io.restart();
    }

protected:
    ba::io_context m_io;
};

struct TestNodeTable: public NodeTable
{
    /// Constructor
    TestNodeTable(
        ba::io_context& _io, KeyPair _alias, bi::address const& _addr, uint16_t _port = 30311)
      : NodeTable(_io, _alias, NodeIPEndpoint(_addr, _port, _port),
            IdentitySchemeV4::createENR(_alias.secret(), _addr, _port, _port),
            true /* discovery enabled */, true /* allow local discovery */)
    {}

    static vector<pair<Public, uint16_t>> createTestNodes(unsigned _count)
    {
        vector<pair<Public, uint16_t>> ret;
        asserts(_count <= 2000);
        
        ret.clear();
        for (unsigned i = 0; i < _count; i++)
        {
            KeyPair k = KeyPair::create();
            ret.push_back(make_pair(k.pub(), randomPortNumber()));
        }

        return ret;
    }

    void populateTestNodes(vector<pair<Public, uint16_t>> const& _testNodes, size_t _count = 0)
    {
        if (!_count)
            _count = _testNodes.size();

        bi::address ourIp = bi::make_address(c_localhostIp);
        for (auto& n: _testNodes)
            if (_count--)
            {
                // manually add node for test
                auto entry = make_shared<NodeEntry>(m_hostNodeIDHash, n.first,
                    NodeIPEndpoint(ourIp, n.second, n.second),
                    RLPXDatagramFace::secondsSinceEpoch(), RLPXDatagramFace::secondsSinceEpoch());
                noteActiveNode(move(entry));
            }
            else
                break;
    }

    // populate NodeTable until one of the buckets reaches the size of _bucketSize
    // return the index of this bucket
    int populateUntilBucketSize(
        vector<pair<Public, uint16_t>> const& _testNodes, size_t _bucketSize)
    {
        auto testNode = _testNodes.begin();

        bi::address ourIp = bi::make_address(c_localhostIp);
        while (testNode != _testNodes.end())
        {
            // manually add node for test
            auto node(make_shared<NodeEntry>(m_hostNodeIDHash, testNode->first,
                NodeIPEndpoint(ourIp, testNode->second, testNode->second),
                RLPXDatagramFace::secondsSinceEpoch(), RLPXDatagramFace::secondsSinceEpoch()));
            auto distance = node->distance;
            noteActiveNode(move(node));

            {
                Guard stateGuard(x_state);
                auto const bucketIndex = distance - 1;
                if (m_buckets[bucketIndex].nodes.size() >= _bucketSize)
                    return bucketIndex;
            }

            ++testNode;
        }

        // not enough testNodes
        return -1;
    }

    // populate NodeTable until bucket _bucket reaches the size of _bucketSize
    // return true if success, false if not enought test nodes
    bool populateUntilSpecificBucketSize(
        vector<pair<Public, uint16_t>> const& _testNodes, size_t _bucket, size_t _bucketSize)
    {
        auto testNode = _testNodes.begin();

        bi::address ourIp = bi::make_address(c_localhostIp);
        while (testNode != _testNodes.end() && bucketSize(_bucket) < _bucketSize)
        {
            // manually add node for test
            // skip the nodes for other buckets
            size_t const dist = distance(m_hostNodeIDHash, sha3(testNode->first));
            if (dist != _bucket + 1)
            {
                ++testNode;
                continue;
            }

            auto entry = make_shared<NodeEntry>(m_hostNodeIDHash, testNode->first,
                NodeIPEndpoint(ourIp, testNode->second, testNode->second),
                RLPXDatagramFace::secondsSinceEpoch(), RLPXDatagramFace::secondsSinceEpoch());
            noteActiveNode(move(entry));

            ++testNode;
        }

        return testNode != _testNodes.end();
    }

    void reset()
    {
        Guard l(x_state);
        for (auto& n : m_buckets)
            n.nodes.clear();
    }

    void onPacketReceived(
        UDPSocketFace* _socket, bi::udp::endpoint const& _from, bytesConstRef _packet) override
    {
        NodeTable::onPacketReceived(_socket, _from, _packet);

        packetsReceived.push(_packet.toBytes());
    }

    bool nodeExists(NodeID const& _id) const
    {
        Guard l(x_nodes);
        return contains(m_allNodes, _id);
    }

    size_t bucketSize(size_t _bucket) const
    {
        Guard l(x_state);
        return m_buckets[_bucket].nodes.size();
    }

    shared_ptr<NodeEntry> bucketFirstNode(size_t _bucket)
    {
        Guard l(x_state);
        return m_buckets[_bucket].nodes.front().lock();
    }

    shared_ptr<NodeEntry> bucketLastNode(size_t _bucket)
    {
        Guard l(x_state);
        return m_buckets[_bucket].nodes.back().lock();
    }

    boost::optional<NodeValidation> nodeValidation(bi::udp::endpoint const& _endpoint)
    {
        promise<boost::optional<NodeValidation>> promise;
        post(m_io, [this, &promise, _endpoint] {
            auto validation = m_sentPings.find(_endpoint);
            if (validation != m_sentPings.end())
                promise.set_value(validation->second);
            else
                promise.set_value({});
        });
        return promise.get_future().get();
    }

    bool addKnownNode(Node const& _node)
    {
        // current time as lastPongReceivedTime makes node table think that endpoint proof has been
        // completed
        return NodeTable::addKnownNode(
            _node, RLPXDatagramFace::secondsSinceEpoch(), RLPXDatagramFace::secondsSinceEpoch());
    }


    concurrent_queue<bytes> packetsReceived;


    using NodeTable::m_allowLocalDiscovery;
    using NodeTable::m_hostNodeEndpoint;
    using NodeTable::m_hostNodeID;
    using NodeTable::m_hostNodeIDHash;
    using NodeTable::m_socket;
    using NodeTable::nearestNodeEntries;
    using NodeTable::nodeEntry;
    using NodeTable::noteActiveNode;
    using NodeTable::setRequestTimeToLive;
};

/**
 * Only used for testing. Not useful beyond tests.
 */
struct TestNodeTableHost : public TestHost
{
    TestNodeTableHost(unsigned _count = 8, uint16_t _port = 30310)
      : m_alias(KeyPair::create()), testNodes(TestNodeTable::createTestNodes(_count)), timer(m_io)
    {
        // find free port
        do
        {
            nodeTable.reset(
                new TestNodeTable(m_io, m_alias, bi::make_address(c_localhostIp), _port));
            ++_port;
        } while (!nodeTable->m_socket->isOpen());
    }
    ~TestNodeTableHost()
    {
        nodeTable->stop();
        m_io.stop();
        terminate();
    }

    void populate(size_t _count = 0) { nodeTable->populateTestNodes(testNodes, _count); }

    int populateUntilBucketSize(size_t _size)
    {
        return nodeTable->populateUntilBucketSize(testNodes, _size);
    }

    bool populateUntilSpecificBucketSize(size_t _bucket, size_t _size)
    {
        return nodeTable->populateUntilSpecificBucketSize(testNodes, _bucket, _size);
    }

    void processEvents(boost::system::error_code const& _e)
    {
        if (_e)
            return;
        nodeTable->processEvents();

        timer.expires_from_now(boost::posix_time::milliseconds(100));
        timer.async_wait([this](boost::system::error_code const& _e) { processEvents(_e); });
    }

    KeyPair m_alias;
    shared_ptr<TestNodeTable> nodeTable;
    vector<pair<Public, uint16_t>> testNodes;  // keypair and port
    ba::deadline_timer timer;
};

class TestUDPSocketHost : UDPSocketEvents, public TestHost
{
public:
    TestUDPSocketHost(unsigned _port = randomPortNumber()) : port(_port)
    {
        // find free port
        while (!socket || !socket->isOpen())
        {
            socket.reset(new UDPSocket<TestUDPSocketHost, 1024>(m_io, *this, port));
            try
            {
                socket->connect();
            }
            catch (exception const&)
            {
                port = randomPortNumber();
            }
        }
    }
    ~TestUDPSocketHost()
    {
        m_io.stop();
        terminate();
    }

    void onSocketDisconnected(UDPSocketFace*){};
    void onPacketReceived(UDPSocketFace*, bi::udp::endpoint const&, bytesConstRef _packet)
    {
        if (_packet.toString() == "AAAA")
            success = true;

        packetsReceived.push(_packet.toBytes());
    }

    shared_ptr<UDPSocket<TestUDPSocketHost, 1024>> socket;
    uint16_t port = 0;

    atomic<bool> success{false};

    concurrent_queue<bytes> packetsReceived;
};

struct TestNodeTableEventHandler : NodeTableEventHandler
{
    void processEvent(NodeID const& _n, NodeTableEventType const& _e) override
    {
        if (_e == NodeEntryScheduledForEviction)
            scheduledForEviction.push(_n);
    }

    concurrent_queue<NodeID> scheduledForEviction;
};

template <class ReceiverType>
std::unique_ptr<DiscoveryDatagram> waitForPacketReceived(ReceiverType& _receiver,
    std::string const& _receiverType, chrono::seconds const& _timeout = chrono::seconds{5})
{
    auto const dataReceived = _receiver.packetsReceived.pop(_timeout);
    auto datagram = DiscoveryDatagram::interpretUDP(bi::udp::endpoint{}, dev::ref(dataReceived));
    BOOST_REQUIRE_EQUAL(datagram->typeName(), _receiverType);

    return datagram;
}

BOOST_AUTO_TEST_CASE(isIPAddressType)
{
    string wildcard = "0.0.0.0";
    BOOST_REQUIRE(bi::make_address(wildcard).is_unspecified());

    string empty = "";
    BOOST_REQUIRE_THROW(bi::make_address(empty).is_unspecified(), exception);

    string publicAddress192 = "192.169.0.0";
    BOOST_REQUIRE(isPublicAddress(publicAddress192));
    BOOST_REQUIRE(!isPrivateAddress(publicAddress192));
    BOOST_REQUIRE(!isLocalHostAddress(publicAddress192));

    string publicAddress172 = "172.32.0.0";
    BOOST_REQUIRE(isPublicAddress(publicAddress172));
    BOOST_REQUIRE(!isPrivateAddress(publicAddress172));
    BOOST_REQUIRE(!isLocalHostAddress(publicAddress172));

    string privateAddress192 = "192.168.1.0";
    BOOST_REQUIRE(isPrivateAddress(privateAddress192));
    BOOST_REQUIRE(!isPublicAddress(privateAddress192));
    BOOST_REQUIRE(!isLocalHostAddress(privateAddress192));

    string privateAddress172 = "172.16.0.0";
    BOOST_REQUIRE(isPrivateAddress(privateAddress172));
    BOOST_REQUIRE(!isPublicAddress(privateAddress172));
    BOOST_REQUIRE(!isLocalHostAddress(privateAddress172));

    string privateAddress10 = "10.0.0.0";
    BOOST_REQUIRE(isPrivateAddress(privateAddress10));
    BOOST_REQUIRE(!isPublicAddress(privateAddress10));
    BOOST_REQUIRE(!isLocalHostAddress(privateAddress10));
}

BOOST_AUTO_TEST_CASE(neighboursPacketLength)
{
    KeyPair k = KeyPair::create();
    vector<pair<Public, uint16_t>> testNodes(TestNodeTable::createTestNodes(16));
    bi::udp::endpoint to(boost::asio::ip::make_address(c_localhostIp), randomPortNumber());

    // hash(32), signature(65), overhead: packetSz(3), type(1), nodeListSz(3), ts(5),
    static unsigned constexpr nlimit = (1280 - 109) / 90; // neighbour: 2 + 65 + 3 + 3 + 17
    for (unsigned offset = 0; offset < testNodes.size(); offset += nlimit)
    {
        Neighbours out(to);

        auto limit = nlimit ? min(testNodes.size(), (size_t)(offset + nlimit)) : testNodes.size();
        for (auto i = offset; i < limit; i++)
        {
            Node n(
                testNodes[i].first, NodeIPEndpoint(boost::asio::ip::make_address("200.200.200.200"),
                                        testNodes[i].second, testNodes[i].second));
            Neighbours::Neighbour neighbour(n);
            out.neighbours.push_back(neighbour);
        }

        out.sign(k.secret());
        BOOST_REQUIRE_LE(out.data.size(), 1280);
    }
}

BOOST_AUTO_TEST_CASE(neighboursPacket)
{
    KeyPair k = KeyPair::create();
    vector<pair<Public, uint16_t>> testNodes(TestNodeTable::createTestNodes(16));
    bi::udp::endpoint to(boost::asio::ip::make_address(c_localhostIp), randomPortNumber());

    Neighbours out(to);
    for (auto n: testNodes)
    {
        Node node(n.first,
            NodeIPEndpoint(boost::asio::ip::make_address("200.200.200.200"), n.second, n.second));
        Neighbours::Neighbour neighbour(node);
        out.neighbours.push_back(neighbour);
    }
    out.sign(k.secret());

    bytesConstRef packet(out.data.data(), out.data.size());
    auto in = DiscoveryDatagram::interpretUDP(to, packet);
    int count = 0;
    for (auto& n : dynamic_cast<Neighbours&>(*in).neighbours)
    {
        BOOST_REQUIRE_EQUAL(testNodes[count].second, n.endpoint.udpPort());
        BOOST_REQUIRE_EQUAL(testNodes[count].first, n.node);
        BOOST_REQUIRE_EQUAL(sha3(testNodes[count].first), sha3(n.node));
        count++;
    }
}

BOOST_AUTO_TEST_CASE(kademlia)
{
    TestNodeTableHost node(8);
    node.start();
    node.populate();
    auto nodes = node.nodeTable->nodes();
    nodes.sort();
    node.nodeTable->reset();
    node.populate(1);
    this_thread::sleep_for(chrono::seconds(2));
    BOOST_REQUIRE_EQUAL(node.nodeTable->count(), 8);
}

BOOST_AUTO_TEST_CASE(hostNoCapsNoTcpListener)
{
    Host host("Test", NetworkConfig(c_localhostIp, randomPortNumber(), false /* upnp */, true /* allow local discovery */));
    host.start();

    // Wait 6 seconds for network to come up
    uint32_t const step = 10;
    for (unsigned i = 0; i < 6000; i += step)
    {
        this_thread::sleep_for(chrono::milliseconds(step));
        if (host.haveNetwork())
            break;
    }

    BOOST_REQUIRE(host.haveNetwork());
    auto const hostPort = host.listenPort();
    BOOST_REQUIRE(hostPort);
    BOOST_REQUIRE(host.caps().empty());

    {
        // Verify no TCP listener on the host port
        io::io_context ioService;
        bi::tcp::acceptor tcp4Acceptor{ioService};
        auto const tcpListenPort = Network::tcp4Listen(tcp4Acceptor, NetworkConfig{ c_localhostIp, hostPort});
        BOOST_REQUIRE_EQUAL(tcpListenPort, hostPort);
    }

    // Verify discovery is running - ping the host and verify a response is received
    TestUDPSocketHost nodeSocketHost;
    nodeSocketHost.start();
    auto const sourcePort = nodeSocketHost.port;
    NodeIPEndpoint sourceEndpoint{
        boost::asio::ip::make_address(c_localhostIp), sourcePort, sourcePort};
    NodeIPEndpoint targetEndpoint{boost::asio::ip::make_address(c_localhostIp), hostPort, hostPort};
    PingNode ping(sourceEndpoint, targetEndpoint);
    ping.sign(KeyPair::create().secret());
    nodeSocketHost.socket->send(ping);

    BOOST_REQUIRE_NO_THROW(nodeSocketHost.packetsReceived.pop(chrono::seconds(5)));
}

BOOST_AUTO_TEST_CASE(udpOnce)
{
    TestUDPSocketHost a;
    auto const port = a.port;
    a.start();
    UDPDatagram d(bi::udp::endpoint(boost::asio::ip::make_address(c_localhostIp), port),
        bytes({65, 65, 65, 65}));
    a.socket->send(d);
    this_thread::sleep_for(chrono::seconds(1));
    BOOST_REQUIRE_EQUAL(true, a.success);
}

BOOST_AUTO_TEST_CASE(noteActiveNodeAppendsNewNode)
{
    TestNodeTableHost nodeTableHost(1);
    nodeTableHost.populate(1);

    auto& nodeTable = nodeTableHost.nodeTable;
    shared_ptr<NodeEntry> newNode = nodeTable->nodeEntry(nodeTableHost.testNodes.front().first);

    BOOST_REQUIRE_GT(nodeTable->bucketSize(newNode->distance - 1), 0);

    auto firstNodeSharedPtr = nodeTable->bucketFirstNode(newNode->distance - 1);
    BOOST_REQUIRE_EQUAL(firstNodeSharedPtr, newNode);
}

BOOST_AUTO_TEST_CASE(noteActiveNodeUpdatesKnownNode)
{
    TestNodeTableHost nodeTableHost(512);
    int const bucketIndex = nodeTableHost.populateUntilBucketSize(3);
    BOOST_REQUIRE(bucketIndex >= 0);

    auto& nodeTable = nodeTableHost.nodeTable;
    auto knownNode = nodeTable->bucketFirstNode(bucketIndex);

    nodeTable->noteActiveNode(knownNode);

    // check that node was moved to the back of the bucket
    BOOST_CHECK_NE(nodeTable->bucketFirstNode(bucketIndex), knownNode);
    BOOST_CHECK_EQUAL(nodeTable->bucketLastNode(bucketIndex), knownNode);
}

BOOST_AUTO_TEST_CASE(noteActiveNodeEvictsTheNodeWhenBucketIsFull)
{
    TestNodeTableHost nodeTableHost(512);
    int const bucketIndex = nodeTableHost.populateUntilBucketSize(16);
    BOOST_REQUIRE(bucketIndex >= 0);

    unique_ptr<TestNodeTableEventHandler> eventHandler(new TestNodeTableEventHandler);
    concurrent_queue<NodeID>& evictEvents = eventHandler->scheduledForEviction;

    auto& nodeTable = nodeTableHost.nodeTable;
    nodeTable->setEventHandler(eventHandler.release());

    nodeTableHost.start();
    nodeTableHost.processEvents({});

    // generate new address for the same bucket
    NodeID newNodeId;
    do
    {
        KeyPair k = KeyPair::create();
        newNodeId = k.pub();
    } while (NodeTable::distance(nodeTable->m_hostNodeIDHash, sha3(newNodeId)) != bucketIndex + 1);

    auto leastRecentlySeenNode = nodeTable->bucketFirstNode(bucketIndex);

    auto const port = randomPortNumber();
    nodeTable->addKnownNode(
        Node(newNodeId, NodeIPEndpoint(bi::make_address(c_localhostIp), port, port)));

    // wait for eviction
    evictEvents.pop();

    // the bucket is still max size
    BOOST_CHECK_EQUAL(nodeTable->bucketSize(bucketIndex), 16);
    // least recently seen node not removed yet
    BOOST_CHECK_EQUAL(nodeTable->bucketFirstNode(bucketIndex), leastRecentlySeenNode);
    // but added to evictions
    auto evicted = nodeTable->nodeValidation(leastRecentlySeenNode->endpoint());
    BOOST_REQUIRE(evicted.is_initialized());
    BOOST_REQUIRE(evicted->replacementNodeEntry);
    BOOST_CHECK_EQUAL(evicted->replacementNodeEntry->id(), newNodeId);
}

BOOST_AUTO_TEST_CASE(nearestNodeEntriesOneNode)
{
    TestNodeTableHost nodeTableHost(1);
    nodeTableHost.populate(1);

    auto& nodeTable = nodeTableHost.nodeTable;
    vector<shared_ptr<NodeEntry>> const nearest = nodeTable->nearestNodeEntries(NodeID::random());

    BOOST_REQUIRE_EQUAL(nearest.size(), 1);
    BOOST_REQUIRE_EQUAL(nearest.front()->id(), nodeTableHost.testNodes.front().first);
}

BOOST_AUTO_TEST_CASE(nearestNodeEntriesOneDistantNode)
{
    // specific case that was failing - one node in bucket #252, target corresponding to bucket #253
    unique_ptr<TestNodeTableHost> nodeTableHost;
    do
    {
        nodeTableHost.reset(new TestNodeTableHost(1));
        nodeTableHost->populate(1);
    } while (nodeTableHost->nodeTable->bucketSize(252) != 1);

    auto& nodeTable = nodeTableHost->nodeTable;

    h256 const hostNodeIDHash = nodeTable->m_hostNodeIDHash;

    NodeID target = NodeID::random();
    while (NodeTable::distance(hostNodeIDHash, sha3(target)) != 254)
        target = NodeID::random();

    vector<shared_ptr<NodeEntry>> const nearest = nodeTable->nearestNodeEntries(target);

    BOOST_REQUIRE_EQUAL(nearest.size(), 1);
    BOOST_REQUIRE_EQUAL(nearest.front()->id(), nodeTableHost->testNodes.front().first);
}

BOOST_AUTO_TEST_CASE(nearestNodeEntriesManyNodes)
{
    unsigned constexpr nodeCount = 128;
    TestNodeTableHost nodeTableHost(nodeCount);
    nodeTableHost.populate(nodeCount);

    auto& nodeTable = nodeTableHost.nodeTable;

    NodeID const target = NodeID::random();
    vector<shared_ptr<NodeEntry>> nearest = nodeTable->nearestNodeEntries(target);

    BOOST_REQUIRE_EQUAL(nearest.size(), 16);

    // get all nodes sorted by distance to target
    list<NodeEntry> const allNodeEntries = nodeTable->snapshot();
    h256 const targetNodeIDHash = sha3(target);
    vector<pair<unsigned, NodeID>> nodesByDistanceToTarget;
    for (auto const& nodeEntry : allNodeEntries)
    {
        NodeID const& nodeID = nodeEntry.id();
        nodesByDistanceToTarget.emplace_back(
            NodeTable::distance(targetNodeIDHash, sha3(nodeID)), nodeID);
    }
    // stable sort to keep them in the order as they are in buckets
    // (the same order they are iterated in nearestNodeEntries implementation)
    std::stable_sort(nodesByDistanceToTarget.begin(), nodesByDistanceToTarget.end(),
        [](pair<unsigned, NodeID> const& _n1, pair<unsigned, NodeID> const& _n2) {
            return _n1.first < _n2.first;
        });
    // get 16 with lowest distance
    std::vector<NodeID> expectedNearestIDs;
    std::transform(nodesByDistanceToTarget.begin(), nodesByDistanceToTarget.begin() + 16,
        std::back_inserter(expectedNearestIDs),
        [](pair<unsigned, NodeID> const& _n) { return _n.second; });


    vector<NodeID> nearestIDs;
    for (auto const& nodeEntry : nearest)
        nearestIDs.push_back(nodeEntry->id());

    BOOST_REQUIRE(nearestIDs == expectedNearestIDs);
}

BOOST_AUTO_TEST_CASE(unexpectedPong)
{
    // NodeTable receiving PONG
    TestNodeTableHost nodeTableHost(0);
    nodeTableHost.start();
    auto& nodeTable = nodeTableHost.nodeTable;

    Pong pong(nodeTable->m_hostNodeEndpoint);
    auto nodeKeyPair = KeyPair::create();
    pong.sign(nodeKeyPair.secret());

    TestUDPSocketHost a;
    a.start();
    a.socket->send(pong);

    // wait for PONG to be received and handled
    nodeTable->packetsReceived.pop();

    BOOST_REQUIRE(!nodeTable->nodeExists(nodeKeyPair.pub()));
}

BOOST_AUTO_TEST_CASE(invalidPong)
{
    // NodeTable sending PING
    TestNodeTableHost nodeTableHost(0);
    nodeTableHost.start();
    auto& nodeTable = nodeTableHost.nodeTable;

    // socket answering with PONG
    TestUDPSocketHost nodeSocketHost;
    nodeSocketHost.start();
    auto const nodePort = nodeSocketHost.port;

    // add a node to node table, initiating PING
    auto const nodeEndpoint = NodeIPEndpoint{bi::make_address(c_localhostIp), nodePort, nodePort};
    auto const nodeKeyPair = KeyPair::create();
    auto const nodePubKey = nodeKeyPair.pub();
    nodeTable->addNode(Node{nodePubKey, nodeEndpoint});

    // validate received ping
    waitForPacketReceived(nodeSocketHost, "Ping");

    // send invalid pong (pong without ping hash)
    Pong pong(nodeTable->m_hostNodeEndpoint);
    pong.sign(nodeKeyPair.secret());

    nodeSocketHost.socket->send(pong);

    // wait for PONG to be received and handled
    nodeTable->packetsReceived.pop();

    // pending node validation should still be not deleted
    BOOST_REQUIRE(nodeTable->nodeValidation(nodeEndpoint));
    // node is not in the node table
    BOOST_REQUIRE(!nodeTable->nodeExists(nodePubKey));
}

BOOST_AUTO_TEST_CASE(validPong)
{
    // NodeTable sending PING
    TestNodeTableHost nodeTableHost(0);
    nodeTableHost.start();
    auto& nodeTable = nodeTableHost.nodeTable;

    // socket receiving PING
    TestUDPSocketHost nodeSocketHost;
    nodeSocketHost.start();
    uint16_t nodePort = nodeSocketHost.port;

    // add a node to node table, initiating PING
    auto nodeEndpoint = NodeIPEndpoint{bi::make_address(c_localhostIp), nodePort, nodePort};
    auto nodeKeyPair = KeyPair::create();
    auto nodePubKey = nodeKeyPair.pub();
    nodeTable->addNode(Node{nodePubKey, nodeEndpoint});

    // handle received PING
    auto pingDatagram = waitForPacketReceived(nodeSocketHost, "Ping");
    auto const& ping = dynamic_cast<PingNode const&>(*pingDatagram);

    // send PONG
    Pong pong(nodeTable->m_hostNodeEndpoint);
    pong.echo = ping.echo;
    pong.sign(nodeKeyPair.secret());
    nodeSocketHost.socket->send(pong);

    // wait for PONG to be received and handled
    nodeTable->packetsReceived.pop();

    BOOST_REQUIRE(nodeTable->nodeExists(nodePubKey));
    auto addedNode = nodeTable->nodeEntry(nodePubKey);
    BOOST_CHECK(addedNode->lastPongReceivedTime > 0);
}

BOOST_AUTO_TEST_CASE(pongWithChangedNodeID)
{
    // NodeTable sending PING
    TestNodeTableHost nodeTableHost(0);
    nodeTableHost.start();
    auto& nodeTable = nodeTableHost.nodeTable;
    nodeTable->setRequestTimeToLive(chrono::seconds(1));

    // socket receiving PING
    TestUDPSocketHost nodeSocketHost{randomPortNumber()};
    nodeSocketHost.start();
    uint16_t const nodePort = nodeSocketHost.port;

    // add a node to node table, initiating PING
    auto const nodeEndpoint = NodeIPEndpoint{bi::make_address(c_localhostIp), nodePort, nodePort};
    auto const nodeKeyPair = KeyPair::create();
    auto const nodePubKey = nodeKeyPair.pub();
    nodeTable->addNode(Node{nodePubKey, nodeEndpoint});

    // handle received PING
    auto pingDatagram = waitForPacketReceived(nodeSocketHost, "Ping");
    auto const& ping = dynamic_cast<PingNode const&>(*pingDatagram);

    // send PONG with different NodeID
    Pong pong(nodeTable->m_hostNodeEndpoint);
    pong.echo = ping.echo;
    auto const newNodeKeyPair = KeyPair::create();
    auto const newNodePubKey = newNodeKeyPair.pub();
    pong.sign(newNodeKeyPair.secret());
    nodeSocketHost.socket->send(pong);

    // wait for PONG to be received and handled
    nodeTable->packetsReceived.pop();

    BOOST_REQUIRE(nodeTable->nodeExists(newNodeKeyPair.pub()));
    auto const addedNode = nodeTable->nodeEntry(newNodePubKey);
    BOOST_CHECK(addedNode->lastPongReceivedTime > 0);

    BOOST_CHECK(!nodeTable->nodeExists(nodePubKey));
    auto const sentPing = nodeTable->nodeValidation(nodeEndpoint);
    BOOST_CHECK(!sentPing.is_initialized());
}

BOOST_AUTO_TEST_CASE(pingTimeout)
{
    // NodeTable sending PING
    TestNodeTableHost nodeTableHost(0);

    nodeTableHost.start();
    auto& nodeTable = nodeTableHost.nodeTable;
    nodeTable->setRequestTimeToLive(chrono::seconds(1));

    // socket receiving PING
    TestUDPSocketHost nodeSocketHost;
    nodeSocketHost.start();
    auto const nodePort = nodeSocketHost.port;

    // add a node to node table, initiating PING
    auto nodeEndpoint = NodeIPEndpoint{bi::make_address(c_localhostIp), nodePort, nodePort};
    auto nodeKeyPair = KeyPair::create();
    auto nodePubKey = nodeKeyPair.pub();
    nodeTable->addNode(Node{nodePubKey, nodeEndpoint});

    this_thread::sleep_for(chrono::seconds(6));

    BOOST_CHECK(!nodeTable->nodeExists(nodePubKey));
    auto sentPing = nodeTable->nodeValidation(nodeEndpoint);
    BOOST_CHECK(!sentPing.is_initialized());

    // handle received PING
    auto pingDatagram = waitForPacketReceived(nodeSocketHost, "Ping");
    auto const& ping = dynamic_cast<PingNode const&>(*pingDatagram);

    // send PONG after timeout
    Pong pong(nodeTable->m_hostNodeEndpoint);
    pong.echo = ping.echo;
    pong.sign(nodeKeyPair.secret());
    nodeSocketHost.socket->send(pong);

    // wait for PONG to be received and handled
    waitForPacketReceived(*nodeTable, "Pong");

    BOOST_CHECK(!nodeTable->nodeExists(nodePubKey));
    sentPing = nodeTable->nodeValidation(nodeEndpoint);
    BOOST_CHECK(!sentPing.is_initialized());
}

BOOST_AUTO_TEST_CASE(invalidPing)
{
    // NodeTable receiving PING
    TestNodeTableHost nodeTableHost(0);
    nodeTableHost.start();
    auto& nodeTable = nodeTableHost.nodeTable;

    // port 0 is invalid
    NodeIPEndpoint endpoint{boost::asio::ip::make_address("200.200.200.200"), 0, 0};
    PingNode ping(endpoint, nodeTable->m_hostNodeEndpoint);
    ping.sign(nodeTableHost.m_alias.secret());

    TestUDPSocketHost nodeSocketHost;
    nodeSocketHost.start();
    nodeSocketHost.socket->send(ping);

    // wait for PING to be received and handled
    nodeTable->packetsReceived.pop();

    // Verify that no PONG response is received
    BOOST_CHECK_THROW(nodeSocketHost.packetsReceived.pop(chrono::seconds(3)), WaitTimeout);
}

BOOST_AUTO_TEST_CASE(neighboursSentAfterFindNode)
{
    unsigned constexpr nodeCount = 8;
    TestNodeTableHost nodeTableHost(nodeCount);
    nodeTableHost.start();
    nodeTableHost.populate(nodeCount);
    BOOST_REQUIRE_EQUAL(nodeTableHost.nodeTable->count(), nodeCount);

    // Create and add a node to the node table. We will use this "node" to send the FindNode query
    // to the node table
    TestUDPSocketHost nodeSocketHost;
    auto const listenPort = nodeSocketHost.port;
    nodeSocketHost.start();
    auto const& nodeTable = nodeTableHost.nodeTable;
    KeyPair newNodeKeyPair = KeyPair::create();
    NodeID newNodeId = newNodeKeyPair.pub();
    nodeTable->addKnownNode(
        Node(newNodeId, NodeIPEndpoint(bi::make_address(c_localhostIp), listenPort, listenPort)));

    // Create and send the FindNode packet from the new "node"
    KeyPair target = KeyPair::create();
    FindNode findNode(nodeTable->m_hostNodeEndpoint, target.pub());
    findNode.sign(newNodeKeyPair.secret());
    nodeSocketHost.socket->send(findNode);

    // Wait for FindNode to be received and handled
    nodeTable->packetsReceived.pop(chrono::seconds(5));

    // Wait for the Neighbours packet to be received
    waitForPacketReceived(nodeSocketHost, "Neighbours");

    // TODO: Validate the contents of the neighbours packet
}

BOOST_AUTO_TEST_CASE(unexpectedFindNode)
{
    unsigned constexpr nodeCount = 8;
    TestNodeTableHost nodeTableHost(nodeCount);
    nodeTableHost.start();
    nodeTableHost.populate(nodeCount);
    BOOST_REQUIRE_EQUAL(nodeTableHost.nodeTable->count(), nodeCount);

    // Create and send the FindNode packet
    TestUDPSocketHost nodeSocketHost;
    nodeSocketHost.start();
    auto nodeTable = nodeTableHost.nodeTable;
    FindNode findNode(nodeTable->m_hostNodeEndpoint, KeyPair::create().pub() /* target */);
    findNode.sign(KeyPair::create().secret());
    nodeSocketHost.socket->send(findNode);

    // Wait for FindNode to be received
    nodeTable->packetsReceived.pop(chrono::seconds(5));

    // Verify that no neighbours response is received
    BOOST_CHECK_THROW(nodeSocketHost.packetsReceived.pop(chrono::seconds(5)), WaitTimeout);
}

BOOST_AUTO_TEST_CASE(evictionWithOldNodeAnswering)
{
    TestNodeTableHost nodeTableHost(2000);
    auto& nodeTable = nodeTableHost.nodeTable;

    // socket receiving PING
    TestUDPSocketHost nodeSocketHost;
    nodeSocketHost.start();
    auto const nodePort = nodeSocketHost.port;

    // add a node to node table
    auto nodeEndpoint = NodeIPEndpoint{bi::make_address(c_localhostIp), nodePort, nodePort};
    auto nodeKeyPair = KeyPair::create();
    auto nodeId = nodeKeyPair.pub();
    nodeTable->addKnownNode(Node{nodeId, nodeEndpoint});

    unique_ptr<TestNodeTableEventHandler> eventHandler(new TestNodeTableEventHandler);
    concurrent_queue<NodeID>& evictEvents = eventHandler->scheduledForEviction;
    nodeTable->setEventHandler(eventHandler.release());

    // add 15 nodes more to the same bucket
    BOOST_REQUIRE(nodeTable->nodeEntry(nodeId)->distance > 0);
    int bucketIndex = nodeTable->nodeEntry(nodeId)->distance - 1;
    bool populated = nodeTableHost.populateUntilSpecificBucketSize(bucketIndex, 16);
    BOOST_REQUIRE(populated);

    nodeTableHost.start();
    nodeTableHost.processEvents({});

    // generate new address for the same bucket
    NodeID newNodeId;
    do
    {
        KeyPair newNodeKeyPair = KeyPair::create();
        newNodeId = newNodeKeyPair.pub();
    } while (NodeTable::distance(nodeTable->m_hostNodeIDHash, sha3(newNodeId)) != bucketIndex + 1);

    // add a node to node table, initiating eviction of nodeId
    // port doesn't matter, it won't be pinged because we're adding it as known
    auto newNodeEndpoint = NodeIPEndpoint{bi::make_address(c_localhostIp), nodePort, nodePort};
    nodeTable->addKnownNode(Node{newNodeId, newNodeEndpoint});

    // wait for eviction
    evictEvents.pop(chrono::seconds(5));

    auto evicted = nodeTable->nodeValidation(nodeEndpoint);
    BOOST_REQUIRE(evicted.is_initialized());

    // handle received PING
    auto pingDatagram = waitForPacketReceived(nodeSocketHost, "Ping");
    auto const& ping = dynamic_cast<PingNode const&>(*pingDatagram);

    // send valid PONG
    Pong pong(nodeTable->m_hostNodeEndpoint);
    pong.echo = ping.echo;
    pong.sign(nodeKeyPair.secret());
    nodeSocketHost.socket->send(pong);

    // wait for PONG to be received and handled
    nodeTable->packetsReceived.pop();

    // check that old node is not evicted
    BOOST_REQUIRE(nodeTable->nodeExists(nodeId));
    auto addedNode = nodeTable->nodeEntry(nodeId);
    BOOST_CHECK(addedNode->lastPongReceivedTime);
    auto sentPing = nodeTable->nodeValidation(nodeEndpoint);
    BOOST_CHECK(!sentPing.is_initialized());
    // check that old node is most recently seen in the bucket
    BOOST_CHECK(nodeTable->bucketLastNode(bucketIndex)->id() == nodeId);
    // check that replacement node is dropped
    BOOST_CHECK(!nodeTable->nodeExists(newNodeId));
}

BOOST_AUTO_TEST_CASE(evictionWithOldNodeDropped)
{
    TestNodeTableHost nodeTableHost(512);
    int const bucketIndex = nodeTableHost.populateUntilBucketSize(16);
    BOOST_REQUIRE(bucketIndex >= 0);

    auto& nodeTable = nodeTableHost.nodeTable;
    nodeTable->setRequestTimeToLive(chrono::seconds(1));

    nodeTableHost.start();

    auto oldNode = nodeTable->bucketFirstNode(bucketIndex);

    // generate new address for the same bucket
    NodeID newNodeId;
    do
    {
        KeyPair newNodeKeyPair = KeyPair::create();
        newNodeId = newNodeKeyPair.pub();
    } while (NodeTable::distance(nodeTable->m_hostNodeIDHash, sha3(newNodeId)) != bucketIndex + 1);

    // add new node to node table
    // port doesn't matter, it won't be pinged because we're adding it as known
    auto const port = randomPortNumber();
    auto newNodeEndpoint = NodeIPEndpoint{bi::make_address(c_localhostIp), port, port};
    nodeTable->addKnownNode(Node{newNodeId, newNodeEndpoint});

    // wait for PING time out
    this_thread::sleep_for(chrono::seconds(6));

    // check that old node is evicted
    BOOST_CHECK(!nodeTable->nodeExists(oldNode->id()));
    BOOST_CHECK(!nodeTable->nodeValidation(oldNode->endpoint()).is_initialized());
    // check that replacement node is active
    BOOST_CHECK(nodeTable->nodeExists(newNodeId));
    auto newNode = nodeTable->nodeEntry(newNodeId);
    BOOST_CHECK(newNode->lastPongReceivedTime > 0);
    BOOST_CHECK(nodeTable->bucketLastNode(bucketIndex)->id() == newNodeId);
}

BOOST_AUTO_TEST_CASE(pingFromLocalhost)
{
    TestNodeTableHost nodeTableHost(0);
    nodeTableHost.start();
    auto& nodeTable = nodeTableHost.nodeTable;

    size_t expectedNodeCount = 0;
    BOOST_REQUIRE(nodeTable->count() == expectedNodeCount);

    // Need to disable local discovery otherwise node table will allow
    // nodes with localhost IPs to be added
    nodeTable->m_allowLocalDiscovery = false;

    // Ping from localhost and verify node isn't added to node table
    TestUDPSocketHost nodeSocketHost{randomPortNumber()};
    nodeSocketHost.start();
    auto const nodePort = nodeSocketHost.port;
    auto nodeEndpoint = NodeIPEndpoint{bi::make_address(c_localhostIp), nodePort, nodePort};

    PingNode ping(nodeEndpoint, nodeTable->m_hostNodeEndpoint);
    ping.sign(KeyPair::create().secret());

    nodeSocketHost.socket->send(ping);

    // Wait for the node table to receive and process the ping
    nodeTable->packetsReceived.pop(chrono::seconds(5));

    // Verify the node wasn't added to the node table
    BOOST_REQUIRE(nodeTable->count() == expectedNodeCount);

    // Verify that the node table doesn't respond with a pong
    BOOST_REQUIRE_THROW(
        nodeSocketHost.packetsReceived.pop(chrono::seconds(5)), WaitTimeout);
}

BOOST_AUTO_TEST_CASE(addSelf)
{
    TestNodeTableHost nodeTableHost(0);
    nodeTableHost.start();
    auto& nodeTable = nodeTableHost.nodeTable;

    size_t expectedNodeCount = 0;
    BOOST_REQUIRE(nodeTable->count() == expectedNodeCount);

    TestUDPSocketHost nodeSocketHost;
    auto nodePort = nodeSocketHost.port;
    auto nodeEndpoint = NodeIPEndpoint{bi::make_address(c_localhostIp), nodePort, nodePort};

    // Create arbitrary node and verify it can be pinged
    auto nodePubKey = KeyPair::create().pub();
    Node node(nodePubKey, nodeEndpoint);
    nodeTable->addNode(node);
    BOOST_CHECK(nodeTable->nodeValidation(nodeEndpoint));

    // Create self node and verify it isn't pinged
    Node self(nodeTableHost.m_alias.pub(), nodeTable->m_hostNodeEndpoint);
    nodeTable->addNode(self);
    BOOST_CHECK(!nodeTable->nodeValidation(nodeTable->m_hostNodeEndpoint));
}

BOOST_AUTO_TEST_CASE(findNodeIsSentAfterPong)
{
    TestNodeTableHost nodeTableHost(0);
    nodeTableHost.start();
    auto& nodeTable = nodeTableHost.nodeTable;

    // We use a TestUDPSocketHost rather than another node table because
    // node tables schedule discovery with timers and run code in other threads
    // so there's no guarantee that one node table will run discovery before the
    // other which can cause seemingly random test failures
    TestUDPSocketHost nodeSocketHost{randomPortNumber()};
    nodeSocketHost.start();
    auto const nodePort = nodeSocketHost.port;
    auto const nodeEndpoint = NodeIPEndpoint{bi::make_address(c_localhostIp), nodePort, nodePort};
    auto const nodeKeyPair = KeyPair::create();
    auto const nodeId = nodeKeyPair.pub();

    // Add the TestUDPSocketHost to the node table, triggering a ping
    nodeTable->addNode(Node{nodeId, nodeEndpoint});

    auto pingDatagram = waitForPacketReceived(nodeSocketHost, "Ping");
    auto const& ping = dynamic_cast<PingNode const&>(*pingDatagram);

    // Reply to the ping (which should trigger the node table to send a FindNode packet once the
    // next discovery pass starts)
    Pong pong(nodeTable->m_hostNodeEndpoint);
    pong.echo = ping.echo;
    pong.sign(nodeKeyPair.secret());
    nodeSocketHost.socket->send(pong);

    waitForPacketReceived(nodeSocketHost, "FindNode", chrono::seconds(20));
}

BOOST_AUTO_TEST_CASE(pingNotSentAfterPongForKnownNode)
{
    // Validate 3 scenarios:
    // * A ping is sent after a pong for a new node
    // * No ping is sent after a pong for a known node with a valid endpoint proof
    // * A ping is sent after a pong for a known node with an invalid endpoint proof

    TestNodeTableHost nodeTableHost1(0);
    nodeTableHost1.start();
    auto& nodeTable1 = nodeTableHost1.nodeTable;

    TestUDPSocketHost nodeSocketHost2;
    nodeSocketHost2.start();
    auto nodePort2 = nodeSocketHost2.port;
    auto nodeEndpoint2 = NodeIPEndpoint{bi::make_address(c_localhostIp), nodePort2, nodePort2};

    // Add node to the node table to trigger ping
    auto nodeKeyPair2 = KeyPair::create();
    Node node2(nodeKeyPair2.pub(), nodeEndpoint2);
    nodeTable1->addNode(node2);

    // Verify ping is received
    auto datagram1 = waitForPacketReceived(nodeSocketHost2, "Ping");

    // Send pong to trigger endpoint proof in node table
    Pong pong(nodeTable1->m_hostNodeEndpoint);
    auto const& ping1 = dynamic_cast<PingNode&>(*datagram1);
    pong.echo = ping1.echo;
    pong.sign(nodeKeyPair2.secret());
    nodeSocketHost2.socket->send(pong);

    // Receive FindNode packet
    waitForPacketReceived(nodeSocketHost2, "FindNode", chrono::seconds{20});

    // Ping the node table and verify that a pong is received but no ping is received after
    // (since the endpoint proof has already been completed)
    PingNode ping2(nodeEndpoint2, nodeTable1->m_hostNodeEndpoint);
    ping2.sign(nodeKeyPair2.secret());
    nodeSocketHost2.socket->send(ping2);

    waitForPacketReceived(nodeSocketHost2, "Pong");

    // Verify that another ping isn't sent
    BOOST_REQUIRE_THROW(nodeSocketHost2.packetsReceived.pop(chrono::seconds(3)), WaitTimeout);

    // Force the endpoint proof to be invalid
    auto newNode = nodeTable1->nodeEntry(nodeKeyPair2.pub());
    newNode->lastPongReceivedTime = 0;
    BOOST_REQUIRE(!newNode->hasValidEndpointProof());

    // Ping the node table and verify you get a pong followed by a ping
    PingNode ping3(nodeEndpoint2, nodeTable1->m_hostNodeEndpoint);
    ping3.sign(nodeKeyPair2.secret());
    nodeSocketHost2.socket->send(ping3);

    waitForPacketReceived(nodeSocketHost2, "Pong");

    waitForPacketReceived(nodeSocketHost2, "Ping");
}

BOOST_AUTO_TEST_CASE(addNodePingsNodeOnlyOnce)
{
    // NodeTable sending PING
    TestNodeTableHost nodeTableHost(0);
    nodeTableHost.start();
    auto& nodeTable = nodeTableHost.nodeTable;

    // add a node to node table, initiating PING
    auto const nodePort = randomPortNumber();
    auto nodeEndpoint = NodeIPEndpoint{bi::make_address(c_localhostIp), nodePort, nodePort};
    auto nodePubKey = KeyPair::create().pub();
    nodeTable->addNode(Node{nodePubKey, nodeEndpoint});

    auto sentPing = nodeTable->nodeValidation(nodeEndpoint);
    BOOST_REQUIRE(sentPing.is_initialized());

    this_thread::sleep_for(chrono::milliseconds(2000));

    // add it for the second time
    nodeTable->addNode(Node{nodePubKey, nodeEndpoint});

    auto sentPing2 = nodeTable->nodeValidation(nodeEndpoint);
    BOOST_REQUIRE(sentPing2.is_initialized());

    // check that Ping was sent only once, so Ping hash didn't change
    BOOST_REQUIRE_EQUAL(sentPing->pingHash, sentPing2->pingHash);
}

BOOST_AUTO_TEST_CASE(validENRRequest)
{
    // NodeTable receiving ENRRequest
    TestNodeTableHost nodeTableHost(0);
    nodeTableHost.start();
    auto& nodeTable = nodeTableHost.nodeTable;

    // socket sending ENRRequest
    TestUDPSocketHost nodeSocketHost;
    nodeSocketHost.start();
    uint16_t const nodePort = nodeSocketHost.port;

    // Exchange Ping/Pongs before sending ENRRequest

    // add a node to node table, initiating PING
    auto const nodeEndpoint = NodeIPEndpoint{bi::make_address(c_localhostIp), nodePort, nodePort};
    auto const nodeKeyPair = KeyPair::create();
    auto const nodePubKey = nodeKeyPair.pub();
    nodeTable->addNode(Node{nodePubKey, nodeEndpoint});

    // handle received PING
    auto pingDatagram = waitForPacketReceived(nodeSocketHost, "Ping");
    auto const& ping = dynamic_cast<PingNode const&>(*pingDatagram);

    // send PONG
    Pong pong(nodeTable->m_hostNodeEndpoint);
    pong.echo = ping.echo;
    pong.sign(nodeKeyPair.secret());
    nodeSocketHost.socket->send(pong);

    // wait for PONG to be received and handled
    waitForPacketReceived(*nodeTable, "Pong");

    // send ENRRequest
    ENRRequest enrRequest(nodeTable->m_hostNodeEndpoint);
    enrRequest.sign(nodeKeyPair.secret());
    nodeSocketHost.socket->send(enrRequest);

    // wait for ENRRequest to be received and handled
    waitForPacketReceived(*nodeTable, "ENRRequest");

    auto datagram = waitForPacketReceived(nodeSocketHost, "ENRResponse");

    auto const& enrResponse = dynamic_cast<ENRResponse const&>(*datagram);
    auto const& keyValuePairs = enrResponse.enr->keyValuePairs();
    BOOST_REQUIRE_EQUAL(RLP{keyValuePairs.at("id")}.toString(), "v4");
    PublicCompressed publicCompressed{RLP{keyValuePairs.at("secp256k1")}.toBytesConstRef()};
    BOOST_REQUIRE(toPublic(publicCompressed) == nodeTable->m_hostNodeID);
    bytes const localhostBytes{127, 0, 0, 1};
    BOOST_REQUIRE(RLP{keyValuePairs.at("ip")}.toBytes() == localhostBytes);
    BOOST_REQUIRE_EQUAL(
        RLP{keyValuePairs.at("tcp")}.toInt(), nodeTable->m_hostNodeEndpoint.tcpPort());
    BOOST_REQUIRE_EQUAL(
        RLP{keyValuePairs.at("udp")}.toInt(), nodeTable->m_hostNodeEndpoint.udpPort());
}

BOOST_AUTO_TEST_CASE(changingHostEndpoint)
{
    // NodeTable sending PING
    TestNodeTableHost nodeTableHost(0);
    nodeTableHost.start();
    auto& nodeTable = nodeTableHost.nodeTable;

    auto const originalHostEndpoint = nodeTable->m_hostNodeEndpoint;
    auto const originalHostENR = nodeTable->hostENR();
    uint16_t const newPort = originalHostEndpoint.udpPort() + 1;
    auto const newHostEndpoint = NodeIPEndpoint{
        bi::make_address(c_localhostIp), newPort, nodeTable->m_hostNodeEndpoint.tcpPort()};

    for (int i = 0; i < 10; ++i)
    {
        BOOST_CHECK_EQUAL(nodeTable->m_hostNodeEndpoint, originalHostEndpoint);
        BOOST_CHECK(nodeTable->hostENR().signature() == originalHostENR.signature());

        // socket receiving PING
        TestUDPSocketHost nodeSocketHost;
        nodeSocketHost.start();
        uint16_t nodePort = nodeSocketHost.port;

        // add a node to node table, initiating PING
        auto nodeEndpoint = NodeIPEndpoint{bi::make_address(c_localhostIp), nodePort, nodePort};
        auto nodeKeyPair = KeyPair::create();
        auto nodePubKey = nodeKeyPair.pub();
        nodeTable->addNode(Node{nodePubKey, nodeEndpoint});

        // handle received PING
        auto pingDatagram = waitForPacketReceived(nodeSocketHost, "Ping");
        auto const& ping = dynamic_cast<PingNode const&>(*pingDatagram);

        // send PONG
        Pong pong(originalHostEndpoint);
        pong.destination = newHostEndpoint;
        pong.echo = ping.echo;
        pong.sign(nodeKeyPair.secret());
        nodeSocketHost.socket->send(pong);

        // wait for PONG to be received and handled
        nodeTable->packetsReceived.pop();
    }

    BOOST_CHECK_EQUAL(nodeTable->m_hostNodeEndpoint, newHostEndpoint);

    ENR const newENR = nodeTable->hostENR();
    BOOST_CHECK_EQUAL(newENR.ip(), newHostEndpoint.address());
    BOOST_CHECK_EQUAL(newENR.udpPort(), newHostEndpoint.udpPort());
    BOOST_CHECK_EQUAL(newENR.tcpPort(), newHostEndpoint.tcpPort());
}


class PacketsWithChangedEndpointFixture : public TestOutputHelperFixture
{
public:
    PacketsWithChangedEndpointFixture()
    {
        nodeTableHost.start();
        nodeSocketHost1.start();
        nodePort1 = nodeSocketHost1.port;
        nodeSocketHost2.start();
        nodePort2 = nodeSocketHost2.port;

        nodeEndpoint1 = NodeIPEndpoint{bi::make_address(c_localhostIp), nodePort1, nodePort1};
        nodeEndpoint2 = NodeIPEndpoint{bi::make_address(c_localhostIp), nodePort2, nodePort2};

        // add a node to node table, initiating PING
        nodeTable->addNode(Node{nodePubKey, nodeEndpoint1});

        // handle received PING
        auto pingDatagram = waitForPacketReceived(nodeSocketHost1, "Ping");
        auto const& ping = dynamic_cast<PingNode const&>(*pingDatagram);

        // send PONG
        Pong pong{nodeTable->m_hostNodeEndpoint};
        pong.echo = ping.echo;
        pong.sign(nodeKeyPair.secret());
        nodeSocketHost1.socket->send(pong);

        // wait for PONG to be received and handled
        nodeTable->packetsReceived.pop(chrono::seconds(5));

        nodeEntry = nodeTable->nodeEntry(nodePubKey);
    }

    TestNodeTableHost nodeTableHost{0};
    shared_ptr<TestNodeTable>& nodeTable = nodeTableHost.nodeTable;

    // socket representing initial peer node
    TestUDPSocketHost nodeSocketHost1;
    uint16_t nodePort1 = 0;

    // socket representing peer with changed endpoint
    TestUDPSocketHost nodeSocketHost2;
    uint16_t nodePort2 = 0;

    NodeIPEndpoint nodeEndpoint1;
    NodeIPEndpoint nodeEndpoint2;
    KeyPair nodeKeyPair = KeyPair::create();
    NodeID nodePubKey = nodeKeyPair.pub();

    shared_ptr<NodeEntry> nodeEntry;
};

BOOST_FIXTURE_TEST_SUITE(packetsWithChangedEndpointSuite, PacketsWithChangedEndpointFixture)

BOOST_AUTO_TEST_CASE(addNode)
{
    // this should initiate Ping to endpoint 2
    nodeTable->addNode(Node{nodePubKey, nodeEndpoint2});

    // handle received PING
    auto pingDatagram = waitForPacketReceived(nodeSocketHost2, "Ping");
    auto const& ping = dynamic_cast<PingNode const&>(*pingDatagram);

    // send PONG
    Pong pong{nodeTable->m_hostNodeEndpoint};
    pong.echo = ping.echo;
    pong.sign(nodeKeyPair.secret());
    nodeSocketHost2.socket->send(pong);

    // wait for PONG to be received and handled
    waitForPacketReceived(*nodeTable, "Pong");

    BOOST_REQUIRE_EQUAL(nodeEntry->endpoint(), nodeEndpoint2);
}

BOOST_AUTO_TEST_CASE(findNode)
{
    // Create and send the FindNode packet through endpoint 2
    FindNode findNode{nodeTable->m_hostNodeEndpoint, KeyPair::create().pub() /* target */};
    findNode.sign(nodeKeyPair.secret());
    nodeSocketHost2.socket->send(findNode);

    // Wait for FindNode to be received
    waitForPacketReceived(*nodeTable, "FindNode");

    // Verify that no neighbours response is received
    BOOST_CHECK_THROW(nodeSocketHost2.packetsReceived.pop(chrono::seconds(5)), WaitTimeout);
}

BOOST_AUTO_TEST_CASE(neighbours)
{
    // Wait for FindNode arriving to endpoint 1
    waitForPacketReceived(nodeSocketHost1, "FindNode", chrono::seconds{10});

    // send Neighbours through endpoint 2
    NodeIPEndpoint neighbourEndpoint{
        boost::asio::ip::make_address("200.200.200.200"), c_defaultListenPort, c_defaultListenPort};
    vector<shared_ptr<NodeEntry>> nearest{
        make_shared<NodeEntry>(nodeTable->m_hostNodeIDHash, KeyPair::create().pub(),
            neighbourEndpoint, RLPXDatagramFace::secondsSinceEpoch(), 0 /* pongSentTime */)};
    Neighbours neighbours{nodeTable->m_hostNodeEndpoint, nearest};
    neighbours.sign(nodeKeyPair.secret());
    nodeSocketHost2.socket->send(neighbours);

    // Wait for Neighbours to be received
    waitForPacketReceived(*nodeTable, "Neighbours");

    // no Ping is sent to neighbour
    auto sentPing = nodeTable->nodeValidation(neighbourEndpoint);
    BOOST_REQUIRE(!sentPing.is_initialized());
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(netTypes, TestOutputHelperFixture)

/*

 Test disabled by Bob Summerwill on 1st Sep 2016, because it is
 consistently failing on Ubuntu within TravisCI, and also appears
 to be testing timing-specific behaviour within Boost code which
 we did not author.

 See https://github.com/ethereum/cpp-ethereum/issues/3253.

 TODO - Work out why this test was written in the first place,
 and why it has started failing.   Re-add it or remove it.


(deadlineTimer)
{
    ba::io_context io;
    ba::deadline_timer t(io);
    bool start = false;
    boost::system::error_code ec;
    atomic<unsigned> fired(0);

    thread thread([&](){ while(!start) this_thread::sleep_for(chrono::milliseconds(10)); io.run();
}); t.expires_from_now(boost::posix_time::milliseconds(200)); start = true;
    t.async_wait([&](boost::system::error_code const& _ec){ ec = _ec; fired++; });
    BOOST_REQUIRE_NO_THROW(t.wait());
    this_thread::sleep_for(chrono::milliseconds(250));
    auto expire = t.expires_from_now().total_milliseconds();
    BOOST_REQUIRE(expire <= 0);
    BOOST_REQUIRE(fired == 1);
    BOOST_REQUIRE(!ec);
    io.stop();
    thread.join();
}
*/

BOOST_AUTO_TEST_CASE(unspecifiedNode)
{
    Node n = UnspecifiedNode;
    BOOST_REQUIRE(!n);

    Node node(Public(sha3("0")), NodeIPEndpoint(bi::address(), 0, 0));
    BOOST_REQUIRE(node);
}

BOOST_AUTO_TEST_CASE(nodeTableReturnsUnspecifiedNode)
{
    ba::io_context io;
    auto const port = randomPortNumber();
    auto const keyPair = KeyPair::create();
    auto const addr = bi::make_address(c_localhostIp);
    NodeTable t(io, keyPair, NodeIPEndpoint(addr, port, port),
        IdentitySchemeV4::createENR(keyPair.secret(), addr, port, port));
    if (Node n = t.node(NodeID()))
        BOOST_REQUIRE(false);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
