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
/** @file net.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 */

#include <boost/test/unit_test.hpp>
#include <test/tools/libtesteth/Options.h>
#include <test/tools/libtesteth/TestOutputHelper.h>

#include <libdevcore/Worker.h>
#include <libdevcore/Assertions.h>
#include <libdevcrypto/Common.h>
#include <libp2p/UDP.h>
#include <libp2p/NodeTable.h>

using namespace std;
using namespace dev;
using namespace dev::test;
using namespace dev::p2p;
namespace ba = boost::asio;
namespace bi = ba::ip;

struct NetFixture: public TestOutputHelper
{
	NetFixture() { dev::p2p::NodeIPEndpoint::test_allowLocal = true; }
	~NetFixture() { dev::p2p::NodeIPEndpoint::test_allowLocal = false; }
};

BOOST_AUTO_TEST_SUITE(network)

BOOST_FIXTURE_TEST_SUITE(net, NetFixture)

/**
 * Only used for testing. Not useful beyond tests.
 */
class TestHost: public Worker
{
public:
	TestHost(): Worker("test",0), m_io() {};
	virtual ~TestHost() { m_io.stop(); stopWorking(); }
	void start() { startWorking(); }
	void doWork() { m_io.run(); }
	void doneWorking() { m_io.reset(); m_io.poll(); m_io.reset(); }

protected:
	ba::io_service m_io;
};

struct TestNodeTable: public NodeTable
{
	/// Constructor
	TestNodeTable(ba::io_service& _io, KeyPair _alias, bi::address const& _addr, uint16_t _port = 30300): NodeTable(_io, _alias, NodeIPEndpoint(_addr, _port, _port)) {}

	static std::vector<std::pair<KeyPair,unsigned>> createTestNodes(unsigned _count)
	{
		std::vector<std::pair<KeyPair,unsigned>> ret;
		asserts(_count < 1000);
		static uint16_t s_basePort = 30500;

		ret.clear();
		for (unsigned i = 0; i < _count; i++)
		{
			KeyPair k = KeyPair::create();
			ret.push_back(make_pair(k,s_basePort+i));
		}

		return ret;
	}

	void pingTestNodes(std::vector<std::pair<KeyPair,unsigned>> const& _testNodes)
	{
		bi::address ourIp = bi::address::from_string("127.0.0.1");
		for (auto& n: _testNodes)
		{
			ping(NodeIPEndpoint(ourIp, n.second, n.second));
			this_thread::sleep_for(chrono::milliseconds(2));
		}
	}

	void populateTestNodes(std::vector<std::pair<KeyPair,unsigned>> const& _testNodes, size_t _count = 0)
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
					shared_ptr<NodeEntry> node(new NodeEntry(m_node.id, n.first.pub(), NodeIPEndpoint(ourIp, n.second, n.second)));
					node->pending = false;
					m_nodes[node->id] = node;
				}
				noteActiveNode(n.first.pub(), bi::udp::endpoint(ourIp, n.second));
			}
			else
				break;
	}

	void reset()
	{
		Guard l(x_state);
		for (auto& n: m_state) n.nodes.clear();
	}
};

/**
 * Only used for testing. Not useful beyond tests.
 */
struct TestNodeTableHost: public TestHost
{
	TestNodeTableHost(unsigned _count = 8): m_alias(KeyPair::create()), nodeTable(new TestNodeTable(m_io, m_alias, bi::address::from_string("127.0.0.1"))), testNodes(TestNodeTable::createTestNodes(_count)) {};
	~TestNodeTableHost() { m_io.stop(); stopWorking(); }

	void setup() { for (auto n: testNodes) nodeTables.push_back(make_shared<TestNodeTable>(m_io,n.first, bi::address::from_string("127.0.0.1"),n.second)); }

	void pingAll() { for (auto& t: nodeTables) t->pingTestNodes(testNodes); }

	void populateAll(size_t _count = 0) { for (auto& t: nodeTables) t->populateTestNodes(testNodes, _count); }

	void populate(size_t _count = 0) { nodeTable->populateTestNodes(testNodes, _count); }

	KeyPair m_alias;
	shared_ptr<TestNodeTable> nodeTable;
	std::vector<std::pair<KeyPair,unsigned>> testNodes; // keypair and port
	std::vector<shared_ptr<TestNodeTable>> nodeTables;
};

class TestUDPSocket: UDPSocketEvents, public TestHost
{
public:
	TestUDPSocket(): m_socket(new UDPSocket<TestUDPSocket, 1024>(m_io, *this, 30300)) {}

	void onDisconnected(UDPSocketFace*) {};
	void onReceived(UDPSocketFace*, bi::udp::endpoint const&, bytesConstRef _packet) { if (_packet.toString() == "AAAA") success = true; }

	shared_ptr<UDPSocket<TestUDPSocket, 1024>> m_socket;

	bool success = false;
};

BOOST_AUTO_TEST_CASE(requestTimeout)
{
	if (test::Options::get().nonetwork)
	{
		clog << "Skipping test network/net/requestTimeout. --nonetwork flag is set.\n";
		return;
	}

	using TimePoint = std::chrono::steady_clock::time_point;
	using RequestTimeout = std::pair<NodeID, TimePoint>;
	
	std::chrono::milliseconds timeout(300);
	std::list<RequestTimeout> timeouts;
	
	NodeID nodeA(sha3("a"));
	NodeID nodeB(sha3("b"));
	timeouts.push_back(make_pair(nodeA, chrono::steady_clock::now()));
	this_thread::sleep_for(std::chrono::milliseconds(100));
	timeouts.push_back(make_pair(nodeB, chrono::steady_clock::now()));
	this_thread::sleep_for(std::chrono::milliseconds(210));
	
	bool nodeAtriggered = false;
	bool nodeBtriggered = false;
	timeouts.remove_if([&](RequestTimeout const& t)
	{
		auto now = chrono::steady_clock::now();
		auto diff = now - t.second;
		if (t.first == nodeA && diff < timeout)
			nodeAtriggered = true;
		if (t.first == nodeB && diff < timeout)
			nodeBtriggered = true;
		return (t.first == nodeA || t.first == nodeB);
	});
	
	BOOST_REQUIRE(nodeAtriggered == false);
	BOOST_REQUIRE(nodeBtriggered == true);
	BOOST_REQUIRE(timeouts.size() == 0);
}

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
	if (test::Options::get().nonetwork)
	{
		clog << "Skipping test network/net/neighboursPacketLength. --nonetwork flag is set.\n";
		return;
	}

	KeyPair k = KeyPair::create();
	std::vector<std::pair<KeyPair,unsigned>> testNodes(TestNodeTable::createTestNodes(16));
	bi::udp::endpoint to(boost::asio::ip::address::from_string("127.0.0.1"), 30000);
	
	// hash(32), signature(65), overhead: packetSz(3), type(1), nodeListSz(3), ts(5),
	static unsigned const nlimit = (1280 - 109) / 90; // neighbour: 2 + 65 + 3 + 3 + 17
	for (unsigned offset = 0; offset < testNodes.size(); offset += nlimit)
	{
		Neighbours out(to);
		
		auto limit = nlimit ? std::min(testNodes.size(), (size_t)(offset + nlimit)) : testNodes.size();
		for (auto i = offset; i < limit; i++)
		{
			Node n(testNodes[i].first.pub(), NodeIPEndpoint(boost::asio::ip::address::from_string("200.200.200.200"), testNodes[i].second, testNodes[i].second));
			Neighbours::Neighbour neighbour(n);
			out.neighbours.push_back(neighbour);
		}
		
		out.sign(k.secret());
		BOOST_REQUIRE_LE(out.data.size(), 1280);
	}
}

BOOST_AUTO_TEST_CASE(neighboursPacket)
{
	if (test::Options::get().nonetwork)
	{
		clog << "Skipping test network/net/neighboursPacket. --nonetwork flag is set.\n";
		return;
	}

	KeyPair k = KeyPair::create();
	std::vector<std::pair<KeyPair,unsigned>> testNodes(TestNodeTable::createTestNodes(16));
	bi::udp::endpoint to(boost::asio::ip::address::from_string("127.0.0.1"), 30000);

	Neighbours out(to);
	for (auto n: testNodes)
	{
		Node node(n.first.pub(), NodeIPEndpoint(boost::asio::ip::address::from_string("200.200.200.200"), n.second, n.second));
		Neighbours::Neighbour neighbour(node);
		out.neighbours.push_back(neighbour);
	}
	out.sign(k.secret());

	bytesConstRef packet(out.data.data(), out.data.size());
	auto in = DiscoveryDatagram::interpretUDP(to, packet);
	int count = 0;
	for (auto n: dynamic_cast<Neighbours&>(*in).neighbours)
	{
		BOOST_REQUIRE_EQUAL(testNodes[count].second, n.endpoint.udpPort);
		BOOST_REQUIRE_EQUAL(testNodes[count].first.pub(), n.node);
		BOOST_REQUIRE_EQUAL(sha3(testNodes[count].first.pub()), sha3(n.node));
		count++;
	}
}

BOOST_AUTO_TEST_CASE(test_findnode_neighbours)
{
	// Executing findNode should result in a list which is serialized
	// into Neighbours packet. Neighbours packet should then be deserialized
	// into the same list of nearest nodes.
}

BOOST_AUTO_TEST_CASE(kademlia)
{
	if (test::Options::get().nonetwork)
	{
		clog << "Skipping test network/net/kademlia. --nonetwork flag is set.\n";
		return;
	}

	TestNodeTableHost node(8);
	node.start();
	node.setup();
	node.populate();
	node.populateAll();
	auto nodes = node.nodeTable->nodes();
	nodes.sort();
	node.nodeTable->reset();
	node.populate(1);
	this_thread::sleep_for(chrono::milliseconds(2000));
	BOOST_REQUIRE_EQUAL(node.nodeTable->count(), 8);
}

BOOST_AUTO_TEST_CASE(udpOnce)
{
	if (test::Options::get().nonetwork)
	{
		clog << "Skipping test network/net/udpOnce. --nonetwork flag is set.\n";
		return;
	}

	UDPDatagram d(bi::udp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 30300), bytes({65,65,65,65}));
	TestUDPSocket a; a.m_socket->connect(); a.start();
	a.m_socket->send(d);
	this_thread::sleep_for(chrono::seconds(1));
	BOOST_REQUIRE_EQUAL(true, a.success);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(netTypes, TestOutputHelper)

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
	if (test::Options::get().nonetwork)
		return;

	ba::io_service io;
	ba::deadline_timer t(io);
	bool start = false;
	boost::system::error_code ec;
	std::atomic<unsigned> fired(0);
	
	thread thread([&](){ while(!start) this_thread::sleep_for(chrono::milliseconds(10)); io.run(); });
	t.expires_from_now(boost::posix_time::milliseconds(200));
	start = true;
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
	if (test::Options::get().nonetwork)
	{
		clog << "Skipping test network/net/unspecifiedNode. --nonetwork flag is set.\n";
		return;
	}

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
	if (test::Options::get().nonetwork)
		return;

	ba::io_service io;
	NodeTable t(io, KeyPair::create(), NodeIPEndpoint(bi::address::from_string("127.0.0.1"), 30303, 30303));
	if (Node n = t.node(NodeID()))
		BOOST_REQUIRE(false);
	else
		BOOST_REQUIRE(n == UnspecifiedNode);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
