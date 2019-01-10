// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2018 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include <libdevcore/Assertions.h>
#include <libdevcore/Worker.h>
#include <libdevcore/concurrent_queue.h>
#include <libdevcrypto/Common.h>
#include <libp2p/NodeTable.h>
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
    void doneWorking() override { m_io.reset(); m_io.poll(); m_io.reset(); }

protected:
    ba::io_service m_io;
};

struct TestNodeTable: public NodeTable
{
    /// Constructor
    TestNodeTable(
        ba::io_service& _io, KeyPair _alias, bi::address const& _addr, uint16_t _port = 30311)
      : NodeTable(_io, _alias, NodeIPEndpoint(_addr, _port, _port), true /* discovery enabled */,
            true /* allow local discovery */)
    {}

    static std::vector<std::pair<Public, uint16_t>> createTestNodes(unsigned _count)
    {
        std::vector<std::pair<Public, uint16_t>> ret;
        asserts(_count < 1000);
        static constexpr uint16_t c_basePort = 30500;

        ret.clear();
        for (unsigned i = 0; i < _count; i++)
        {
            KeyPair k = KeyPair::create();
            ret.push_back(make_pair(k.pub(), c_basePort + i));
        }

        return ret;
    }

    void populateTestNodes(
        std::vector<std::pair<Public, uint16_t>> const& _testNodes, size_t _count = 0)
    {
        if (!_count)
            _count = _testNodes.size();

        bi::address ourIp = bi::address::from_string("127.0.0.1");
        for (auto& n: _testNodes)
            if (_count--)
            {
                // manually add node for test
                {
                    Guard ln(x_nodes);
                    shared_ptr<NodeEntry> node(new NodeEntry(
                        m_hostNodeID, n.first, NodeIPEndpoint(ourIp, n.second, n.second)));
                    node->lastPongReceivedTime = RLPXDatagramFace::secondsSinceEpoch();
                    m_allNodes[node->id] = node;
                }
                noteActiveNode(n.first, bi::udp::endpoint(ourIp, n.second));
            }
            else
                break;
    }

    // populate NodeTable until one of the buckets reaches the size of _bucketSize
    // return the index of this bucket
    int populateUntilBucketSize(
        std::vector<std::pair<Public, uint16_t>> const& _testNodes, size_t _bucketSize)
    {
        auto testNode = _testNodes.begin();

        bi::address ourIp = bi::address::from_string("127.0.0.1");
        while (testNode != _testNodes.end())
        {
            unsigned distance = 0;
            // manually add node for test
            {
                Guard ln(x_nodes);
                shared_ptr<NodeEntry> node(new NodeEntry(m_hostNodeID, testNode->first,
                    NodeIPEndpoint(ourIp, testNode->second, testNode->second)));
                node->lastPongReceivedTime = RLPXDatagramFace::secondsSinceEpoch();
                m_allNodes[node->id] = node;
                distance = node->distance;
            }
            noteActiveNode(testNode->first, bi::udp::endpoint(ourIp, testNode->second));

            auto const bucketIndex = distance - 1;
            if (m_buckets[bucketIndex].nodes.size() >= _bucketSize)
                return bucketIndex;

            ++testNode;
        }

        // not enough testNodes
        return -1;
    }

    // populate NodeTable until bucket _bucket reaches the size of _bucketSize
    // return true if success, false if not enought test nodes
    bool populateUntilSpecificBucketSize(std::vector<std::pair<Public, uint16_t>> const& _testNodes,
        size_t _bucket, size_t _bucketSize)
    {
        auto testNode = _testNodes.begin();

        bi::address ourIp = bi::address::from_string("127.0.0.1");
        while (testNode != _testNodes.end() && m_buckets[_bucket].nodes.size() < _bucketSize)
        {
            // manually add node for test
            {
                // skip the nodes for other buckets
                size_t const dist = distance(m_hostNodeID, testNode->first);
                if (dist != _bucket + 1)
                {
                    ++testNode;
                    continue;
                }

                Guard ln(x_nodes);
                shared_ptr<NodeEntry> node(new NodeEntry(m_hostNodeID, testNode->first,
                    NodeIPEndpoint(ourIp, testNode->second, testNode->second)));
                node->lastPongReceivedTime = RLPXDatagramFace::secondsSinceEpoch();
                m_allNodes[node->id] = node;
            }
            noteActiveNode(testNode->first, bi::udp::endpoint(ourIp, testNode->second));

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

    concurrent_queue<bytes> packetsReceived;


    using NodeTable::m_allNodes;
    using NodeTable::m_buckets;
    using NodeTable::m_hostNodeID;
    using NodeTable::m_hostNodeEndpoint;
    using NodeTable::m_sentPings;
    using NodeTable::m_socket;
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
                new TestNodeTable(m_io, m_alias, bi::address::from_string("127.0.0.1"), _port));
            ++_port;
        } while (!nodeTable->m_socket->isOpen());
    }
    ~TestNodeTableHost()
    {
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
    std::vector<std::pair<Public, uint16_t>> testNodes;  // keypair and port
    ba::deadline_timer timer;
};

class TestUDPSocketHost : UDPSocketEvents, public TestHost
{
public:
    TestUDPSocketHost(unsigned _port) : port(_port)
    {
        // find free port
        while (!socket || !socket->isOpen())
        {
            socket.reset(new UDPSocket<TestUDPSocketHost, 1024>(m_io, *this, port));
            try
            {
                socket->connect();
            }
            catch (std::exception const&)
            {
                ++port;
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

    std::atomic<bool> success{false};

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

BOOST_AUTO_TEST_CASE(isIPAddressType)
{
    string wildcard = "0.0.0.0";
    BOOST_REQUIRE(bi::address::from_string(wildcard).is_unspecified());

    string empty = "";
    BOOST_REQUIRE_THROW(bi::address::from_string(empty).is_unspecified(), std::exception);

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
    std::vector<std::pair<Public, uint16_t>> testNodes(TestNodeTable::createTestNodes(16));
    bi::udp::endpoint to(boost::asio::ip::address::from_string("127.0.0.1"), 30000);

    // hash(32), signature(65), overhead: packetSz(3), type(1), nodeListSz(3), ts(5),
    static unsigned constexpr nlimit = (1280 - 109) / 90; // neighbour: 2 + 65 + 3 + 3 + 17
    for (unsigned offset = 0; offset < testNodes.size(); offset += nlimit)
    {
        Neighbours out(to);

        auto limit =
            nlimit ? std::min(testNodes.size(), (size_t)(offset + nlimit)) : testNodes.size();
        for (auto i = offset; i < limit; i++)
        {
            Node n(testNodes[i].first,
                NodeIPEndpoint(boost::asio::ip::address::from_string("200.200.200.200"),
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
    std::vector<std::pair<Public, uint16_t>> testNodes(TestNodeTable::createTestNodes(16));
    bi::udp::endpoint to(boost::asio::ip::address::from_string("127.0.0.1"), 30000);

    Neighbours out(to);
    for (auto n: testNodes)
    {
        Node node(n.first, NodeIPEndpoint(boost::asio::ip::address::from_string("200.200.200.200"),
                               n.second, n.second));
        Neighbours::Neighbour neighbour(node);
        out.neighbours.push_back(neighbour);
    }
    out.sign(k.secret());

    bytesConstRef packet(out.data.data(), out.data.size());
    auto in = DiscoveryDatagram::interpretUDP(to, packet);
    int count = 0;
    for (auto n: dynamic_cast<Neighbours&>(*in).neighbours)
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
    this_thread::sleep_for(chrono::milliseconds(2000));
    BOOST_REQUIRE_EQUAL(node.nodeTable->count(), 8);
}

BOOST_AUTO_TEST_CASE(udpOnce)
{
    unsigned short port = 30333;
    UDPDatagram d(bi::udp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port), bytes({65,65,65,65}));
    TestUDPSocketHost a{port};
    a.start();
    a.socket->send(d);
    this_thread::sleep_for(chrono::seconds(1));
    BOOST_REQUIRE_EQUAL(true, a.success);
}

BOOST_AUTO_TEST_CASE(noteActiveNodeAppendsNewNode)
{
    TestNodeTableHost nodeTableHost(1);
    nodeTableHost.populate(1);

    auto& nodeTable = nodeTableHost.nodeTable;
    std::shared_ptr<NodeEntry> newNode = nodeTable->m_allNodes.begin()->second;

    auto const& nodeBucketArray = nodeTable->m_buckets;
    auto const& nodeBucket = nodeBucketArray[newNode->distance - 1];
    auto const& nodes = nodeBucket.nodes;
    BOOST_REQUIRE(!nodes.empty());

    auto firstNodeSharedPtr = nodes.front().lock();
    BOOST_REQUIRE_EQUAL(firstNodeSharedPtr, newNode);
}

BOOST_AUTO_TEST_CASE(noteActiveNodeUpdatesKnownNode)
{
    TestNodeTableHost nodeTableHost(512);
    int const bucketIndex = nodeTableHost.populateUntilBucketSize(3);
    BOOST_REQUIRE(bucketIndex >= 0);

    auto& nodeTable = nodeTableHost.nodeTable;
    auto const& nodeBucketArray = nodeTable->m_buckets;
    auto const& nodes = nodeBucketArray[bucketIndex].nodes;
    auto knownNode = nodes.front().lock();

    nodeTable->noteActiveNode(knownNode->id, knownNode->endpoint);

    // check that node was moved to the back of the bucket
    BOOST_CHECK_NE(nodes.front().lock(), knownNode);
    BOOST_CHECK_EQUAL(nodes.back().lock(), knownNode);
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
    } while (NodeTable::distance(nodeTableHost.m_alias.pub(), newNodeId) != bucketIndex + 1);

    auto const& nodeBucketArray = nodeTable->m_buckets;
    auto const& nodes = nodeBucketArray[bucketIndex].nodes;
    auto leastRecentlySeenNode = nodes.front().lock();

    nodeTable->addNode(
        Node(newNodeId, NodeIPEndpoint(bi::address::from_string("127.0.0.1"), 30000, 30000)),
        NodeTable::Known);

    // wait for eviction
    evictEvents.pop();

    // the bucket is still max size
    BOOST_CHECK_EQUAL(nodes.size(), 16);
    // least recently seen node not removed yet
    BOOST_CHECK_EQUAL(nodes.front().lock(), leastRecentlySeenNode);
    // but added to evictions
    auto evicted = nodeTable->m_sentPings.find(leastRecentlySeenNode->id);
    BOOST_REQUIRE(evicted != nodeTable->m_sentPings.end());
    BOOST_REQUIRE(evicted->second.replacementNodeID);
    BOOST_CHECK_EQUAL(*evicted->second.replacementNodeID, newNodeId);
}

BOOST_AUTO_TEST_CASE(noteActiveNodeReplacesNodeInFullBucketWhenEndpointChanged)
{
    TestNodeTableHost nodeTableHost(512);
    int const bucketIndex = nodeTableHost.populateUntilBucketSize(16);
    BOOST_REQUIRE(bucketIndex >= 0);

    auto& nodeTable = nodeTableHost.nodeTable;
    auto const& nodeBucketArray = nodeTable->m_buckets;
    auto const& nodes = nodeBucketArray[bucketIndex].nodes;
    auto leastRecentlySeenNodeId = nodes.front().lock()->id;

    // addNode will replace the node in the m_allNodes map, because it's the same id with enother
    // endpoint
    NodeIPEndpoint newEndpoint{bi::address::from_string("127.0.0.1"), 30000, 30000};
    nodeTable->addNode(Node(leastRecentlySeenNodeId, newEndpoint), NodeTable::Known);

    // the bucket is still max size
    BOOST_CHECK_EQUAL(nodes.size(), 16);
    // least recently seen node removed
    BOOST_CHECK_NE(nodes.front().lock()->id, leastRecentlySeenNodeId);
    // but added as most recently seen with new endpoint
    auto mostRecentNodeEntry = nodes.back().lock();
    BOOST_CHECK_EQUAL(mostRecentNodeEntry->id, leastRecentlySeenNodeId);
    BOOST_CHECK_EQUAL(mostRecentNodeEntry->endpoint, newEndpoint);
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

    TestUDPSocketHost a{30333};
    a.start();
    a.socket->send(pong);

    // wait for PONG to be received and handled
    nodeTable->packetsReceived.pop();

    auto addedNode = nodeTable->m_allNodes.find(nodeKeyPair.pub());
    BOOST_REQUIRE(addedNode == nodeTable->m_allNodes.end());
}

BOOST_AUTO_TEST_CASE(invalidPong)
{
    // NodeTable sending PING
    TestNodeTableHost nodeTableHost(0);
    nodeTableHost.start();
    auto& nodeTable = nodeTableHost.nodeTable;

    // socket answering with PONG
    TestUDPSocketHost nodeSocketHost{30500};
    nodeSocketHost.start();
    uint16_t nodePort = nodeSocketHost.port;

    // add a node to node table, initiating PING
    auto nodeEndpoint = NodeIPEndpoint{bi::address::from_string("127.0.0.1"), nodePort, nodePort};
    auto nodeKeyPair = KeyPair::create();
    auto nodePubKey = nodeKeyPair.pub();
    nodeTable->addNode(Node{nodePubKey, nodeEndpoint});

    // send PONG
    Pong pong(nodeTable->m_hostNodeEndpoint);
    pong.sign(nodeKeyPair.secret());

    nodeSocketHost.socket->send(pong);

    // wait for PONG to be received and handled
    nodeTable->packetsReceived.pop();

    auto addedNode = nodeTable->m_allNodes.find(nodePubKey);
    BOOST_REQUIRE(addedNode != nodeTable->m_allNodes.end());
    BOOST_CHECK_EQUAL(addedNode->second->lastPongReceivedTime, 0);
}

BOOST_AUTO_TEST_CASE(validPong)
{
    // NodeTable sending PING
    TestNodeTableHost nodeTableHost(0);
    nodeTableHost.start();
    auto& nodeTable = nodeTableHost.nodeTable;

    // socket receiving PING
    TestUDPSocketHost nodeSocketHost{30500};
    nodeSocketHost.start();
    uint16_t nodePort = nodeSocketHost.port;

    // add a node to node table, initiating PING
    auto nodeEndpoint = NodeIPEndpoint{bi::address::from_string("127.0.0.1"), nodePort, nodePort};
    auto nodeKeyPair = KeyPair::create();
    auto nodePubKey = nodeKeyPair.pub();
    nodeTable->addNode(Node{nodePubKey, nodeEndpoint});

    // handle received PING
    auto pingDataReceived = nodeSocketHost.packetsReceived.pop();
    auto pingDatagram =
        DiscoveryDatagram::interpretUDP(bi::udp::endpoint{}, dev::ref(pingDataReceived));
    auto ping = dynamic_cast<PingNode const&>(*pingDatagram);

    // send PONG
    Pong pong(nodeTable->m_hostNodeEndpoint);
    pong.echo = ping.echo;
    pong.sign(nodeKeyPair.secret());
    nodeSocketHost.socket->send(pong);

    // wait for PONG to be received and handled
    nodeTable->packetsReceived.pop();

    auto addedNode = nodeTable->m_allNodes.find(nodePubKey);
    BOOST_REQUIRE(addedNode != nodeTable->m_allNodes.end());
    BOOST_CHECK(addedNode->second->lastPongReceivedTime > 0);
}

BOOST_AUTO_TEST_CASE(pingTimeout)
{
    // NodeTable sending PING
    TestNodeTableHost nodeTableHost(0);

    nodeTableHost.start();
    auto& nodeTable = nodeTableHost.nodeTable;
    nodeTable->setRequestTimeToLive(std::chrono::seconds(1));

    // socket receiving PING
    TestUDPSocketHost nodeSocketHost{30500};
    nodeSocketHost.start();
    uint16_t nodePort = nodeSocketHost.port;

    // add a node to node table, initiating PING
    auto nodeEndpoint = NodeIPEndpoint{bi::address::from_string("127.0.0.1"), nodePort, nodePort};
    auto nodeKeyPair = KeyPair::create();
    auto nodePubKey = nodeKeyPair.pub();
    nodeTable->addNode(Node{nodePubKey, nodeEndpoint});

    this_thread::sleep_for(std::chrono::seconds(6));

    auto addedNode = nodeTable->m_allNodes.find(nodePubKey);
    BOOST_CHECK(addedNode == nodeTable->m_allNodes.end());
    auto sentPing = nodeTable->m_sentPings.find(nodePubKey);
    BOOST_CHECK(sentPing == nodeTable->m_sentPings.end());

    // handle received PING
    auto pingDataReceived = nodeSocketHost.packetsReceived.pop();
    auto pingDatagram =
        DiscoveryDatagram::interpretUDP(bi::udp::endpoint{}, dev::ref(pingDataReceived));
    auto ping = dynamic_cast<PingNode const&>(*pingDatagram);

    // send PONG after timeout
    Pong pong(nodeTable->m_hostNodeEndpoint);
    pong.echo = ping.echo;
    pong.sign(nodeKeyPair.secret());
    nodeSocketHost.socket->send(pong);

    // wait for PONG to be received and handled
    nodeTable->packetsReceived.pop();

    addedNode = nodeTable->m_allNodes.find(nodePubKey);
    BOOST_CHECK(addedNode == nodeTable->m_allNodes.end());
    sentPing = nodeTable->m_sentPings.find(nodePubKey);
    BOOST_CHECK(sentPing == nodeTable->m_sentPings.end());
}

BOOST_AUTO_TEST_CASE(invalidPing)
{
    // NodeTable receiving PING
    TestNodeTableHost nodeTableHost(0);
    nodeTableHost.start();
    auto& nodeTable = nodeTableHost.nodeTable;

    // port 0 is invalid
    NodeIPEndpoint endpoint{boost::asio::ip::address::from_string("200.200.200.200"), 0, 0};
    PingNode ping(endpoint, nodeTable->m_hostNodeEndpoint);
    ping.sign(nodeTableHost.m_alias.secret());

    TestUDPSocketHost nodeSocketHost{30333};
    nodeSocketHost.start();
    nodeSocketHost.socket->send(ping);

    // wait for PING to be received and handled
    nodeTable->packetsReceived.pop();

    // Verify that no PONG response is received
    BOOST_CHECK_THROW(nodeSocketHost.packetsReceived.pop(chrono::milliseconds(3000)), WaitTimeout);
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
    TestUDPSocketHost nodeSocketHost{30000};
    auto const listenPort = nodeSocketHost.port;
    nodeSocketHost.start();
    auto const& nodeTable = nodeTableHost.nodeTable;
    KeyPair newNodeKeyPair = KeyPair::create();
    NodeID newNodeId = newNodeKeyPair.pub();
    nodeTable->addNode(Node(newNodeId, NodeIPEndpoint(bi::address::from_string("127.0.0.1"),
                                           listenPort, listenPort)),
        NodeTable::Known /* Makes node table think that endpoint proof has been completed */);

    // Create and send the FindNode packet from the new "node"
    KeyPair target = KeyPair::create();
    FindNode findNode(nodeTable->m_hostNodeEndpoint, target.pub());
    findNode.sign(newNodeKeyPair.secret());
    nodeSocketHost.socket->send(findNode);

    // Wait for FindNode to be received and handled
    nodeTable->packetsReceived.pop(chrono::milliseconds(5000));

    // Wait for the Neighbours packet to be received
    auto packetReceived = nodeSocketHost.packetsReceived.pop(chrono::milliseconds(5000));
    auto datagram = DiscoveryDatagram::interpretUDP(bi::udp::endpoint{}, dev::ref(packetReceived));
    BOOST_CHECK_EQUAL(datagram->typeName(), "Neighbours");

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
    TestUDPSocketHost nodeSocketHost{30000};
    nodeSocketHost.start();
    auto nodeTable = nodeTableHost.nodeTable;
    FindNode findNode(nodeTable->m_hostNodeEndpoint, KeyPair::create().pub() /* target */);
    findNode.sign(KeyPair::create().secret());
    nodeSocketHost.socket->send(findNode);

    // Wait for FindNode to be received
    nodeTable->packetsReceived.pop(chrono::milliseconds(5000));

    // Verify that no neighbours response is received
    BOOST_CHECK_THROW(nodeSocketHost.packetsReceived.pop(chrono::milliseconds(5000)), WaitTimeout);
}

BOOST_AUTO_TEST_CASE(evictionWithOldNodeAnswering)
{
    TestNodeTableHost nodeTableHost(512);
    auto& nodeTable = nodeTableHost.nodeTable;

    // socket receiving PING
    TestUDPSocketHost nodeSocketHost{30500};
    nodeSocketHost.start();
    uint16_t nodePort = nodeSocketHost.port;

    // add a node to node table
    auto nodeEndpoint = NodeIPEndpoint{bi::address::from_string("127.0.0.1"), nodePort, nodePort};
    auto nodeKeyPair = KeyPair::create();
    auto nodeId = nodeKeyPair.pub();
    nodeTable->addNode(Node{nodeId, nodeEndpoint}, NodeTable::Known);

    unique_ptr<TestNodeTableEventHandler> eventHandler(new TestNodeTableEventHandler);
    concurrent_queue<NodeID>& evictEvents = eventHandler->scheduledForEviction;
    nodeTable->setEventHandler(eventHandler.release());

    // add 15 nodes more to the same bucket
    BOOST_REQUIRE(nodeTable->m_allNodes.find(nodeId)->second->distance > 0);
    int bucketIndex = nodeTable->m_allNodes[nodeId]->distance - 1;
    nodeTableHost.populateUntilSpecificBucketSize(bucketIndex, 16);

    nodeTableHost.start();
    nodeTableHost.processEvents({});

    // generate new address for the same bucket
    NodeID newNodeId;
    do
    {
        KeyPair newNodeKeyPair = KeyPair::create();
        newNodeId = newNodeKeyPair.pub();
    } while (NodeTable::distance(nodeTableHost.m_alias.pub(), newNodeId) != bucketIndex + 1);

    // add a node to node table, initiating eviction of nodeId
    // port doesn't matter, it won't be pinged because we're adding it as known
    auto newNodeEndpoint =
        NodeIPEndpoint{bi::address::from_string("127.0.0.1"), nodePort, nodePort};
    nodeTable->addNode(Node{newNodeId, newNodeEndpoint}, NodeTable::Known);

    // wait for eviction
    evictEvents.pop();

    auto evicted = nodeTable->m_sentPings.find(nodeId);
    BOOST_REQUIRE(evicted != nodeTable->m_sentPings.end());

    // handle received PING
    auto pingDataReceived = nodeSocketHost.packetsReceived.pop();
    auto pingDatagram =
        DiscoveryDatagram::interpretUDP(bi::udp::endpoint{}, dev::ref(pingDataReceived));
    auto ping = dynamic_cast<PingNode const&>(*pingDatagram);

    // send valid PONG
    Pong pong(nodeTable->m_hostNodeEndpoint);
    pong.echo = ping.echo;
    pong.sign(nodeKeyPair.secret());
    nodeSocketHost.socket->send(pong);

    // wait for PONG to be received and handled
    nodeTable->packetsReceived.pop();

    // check that old node is not evicted
    auto addedNode = nodeTable->m_allNodes.find(nodeId);
    BOOST_REQUIRE(addedNode != nodeTable->m_allNodes.end());
    BOOST_CHECK(addedNode->second->lastPongReceivedTime);
    auto sentPing = nodeTable->m_sentPings.find(nodeId);
    BOOST_CHECK(sentPing == nodeTable->m_sentPings.end());
    // check that old node is most recently seen in the bucket
    BOOST_CHECK(nodeTable->m_buckets[bucketIndex].nodes.back().lock()->id == nodeId);
    // check that replacement node is dropped
    BOOST_CHECK(nodeTable->m_allNodes.find(newNodeId) == nodeTable->m_allNodes.end());
}

BOOST_AUTO_TEST_CASE(evictionWithOldNodeDropped)
{
    TestNodeTableHost nodeTableHost(512);
    int const bucketIndex = nodeTableHost.populateUntilBucketSize(16);
    BOOST_REQUIRE(bucketIndex >= 0);

    auto& nodeTable = nodeTableHost.nodeTable;
    nodeTable->setRequestTimeToLive(std::chrono::seconds(1));

    nodeTableHost.start();

    auto const& nodeBucketArray = nodeTable->m_buckets;
    auto const& nodes = nodeBucketArray[bucketIndex].nodes;
    auto oldNodeId = nodes.front().lock()->id;

    // generate new address for the same bucket
    NodeID newNodeId;
    do
    {
        KeyPair newNodeKeyPair = KeyPair::create();
        newNodeId = newNodeKeyPair.pub();
    } while (NodeTable::distance(nodeTableHost.m_alias.pub(), newNodeId) != bucketIndex + 1);

    // add new node to node table
    // port doesn't matter, it won't be pinged because we're adding it as known
    auto newNodeEndpoint = NodeIPEndpoint{bi::address::from_string("127.0.0.1"), 30500, 30500};
    nodeTable->addNode(Node{newNodeId, newNodeEndpoint}, NodeTable::Known);

    // wait for PING time out
    this_thread::sleep_for(std::chrono::seconds(6));

    // check that old node is evicted
    BOOST_CHECK(nodeTable->m_allNodes.find(oldNodeId) == nodeTable->m_allNodes.end());
    BOOST_CHECK(nodeTable->m_sentPings.find(oldNodeId) == nodeTable->m_sentPings.end());
    // check that replacement node is active
    auto newNode = nodeTable->m_allNodes.find(newNodeId);
    BOOST_CHECK(newNode != nodeTable->m_allNodes.end());
    BOOST_CHECK(newNode->second->lastPongReceivedTime > 0);
    BOOST_CHECK(nodes.back().lock()->id == newNodeId);
}

BOOST_AUTO_TEST_CASE(addSelf)
{
    TestNodeTableHost nodeTableHost(512);
    auto& nodeTable = nodeTableHost.nodeTable;

    size_t expectedNodeCount = 0;
    BOOST_REQUIRE(nodeTable->count() == expectedNodeCount);

    TestUDPSocketHost nodeSocketHost{ 30500 };
    auto nodePort = nodeSocketHost.port;
    auto nodeEndpoint = NodeIPEndpoint{ bi::address::from_string("127.0.0.1"), nodePort, nodePort };

    // Create arbitrary node and verify it can be added to the node table
    auto nodeKeyPair = KeyPair::create();
    Node node(nodeKeyPair.pub(), nodeEndpoint);
    nodeTable->addNode(node);
    BOOST_CHECK(nodeTable->count() == ++expectedNodeCount);

    // Create self node and verify it isn't added to the node table
    Node self(nodeTableHost.m_alias.pub(), nodeEndpoint);
    nodeTable->addNode(self);
    BOOST_CHECK(nodeTable->count() == ++expectedNodeCount - 1);
}

BOOST_AUTO_TEST_CASE(findNodeIsSentAfterPong)
{
    // Node Table receiving Ping and sending FindNode
    TestNodeTableHost nodeTableHost1(15);
    nodeTableHost1.populate();
    nodeTableHost1.start();
    auto& nodeTable1 = nodeTableHost1.nodeTable;

    TestNodeTableHost nodeTableHost2(512, nodeTable1->m_hostNodeEndpoint.udpPort() + 1);
    nodeTableHost2.populate();
    nodeTableHost2.start();
    auto& nodeTable2 = nodeTableHost2.nodeTable;

    // add node1 to table2 initiating PING from table2 to table1
    nodeTable2->addNode(Node{nodeTable1->m_hostNodeID, nodeTable1->m_hostNodeEndpoint});

    auto packetReceived1 = nodeTable2->packetsReceived.pop();
    auto datagram1 =
        DiscoveryDatagram::interpretUDP(bi::udp::endpoint{}, dev::ref(packetReceived1));
    BOOST_CHECK_EQUAL(datagram1->typeName(), "Pong");

    auto packetReceived2 = nodeTable2->packetsReceived.pop();
    auto datagram2 =
        DiscoveryDatagram::interpretUDP(bi::udp::endpoint{}, dev::ref(packetReceived2));
    BOOST_CHECK_EQUAL(datagram2->typeName(), "Ping");

    auto packetReceived3 = nodeTable2->packetsReceived.pop();
    auto datagram3 =
        DiscoveryDatagram::interpretUDP(bi::udp::endpoint{}, dev::ref(packetReceived3));
    BOOST_CHECK_EQUAL(datagram3->typeName(), "FindNode");
}

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
    ba::io_service io;
    ba::deadline_timer t(io);
    bool start = false;
    boost::system::error_code ec;
    std::atomic<unsigned> fired(0);

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
    BOOST_REQUIRE(n != node);

    Node nodeEq(Public(sha3("0")), NodeIPEndpoint(bi::address(), 0, 0));
    BOOST_REQUIRE_EQUAL(node, nodeEq);
}

BOOST_AUTO_TEST_CASE(nodeTableReturnsUnspecifiedNode)
{
    ba::io_service io;
    NodeTable t(io, KeyPair::create(), NodeIPEndpoint(bi::address::from_string("127.0.0.1"), 30303, 30303));
    if (Node n = t.node(NodeID()))
        BOOST_REQUIRE(false);
    else
        BOOST_REQUIRE(n == UnspecifiedNode);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
