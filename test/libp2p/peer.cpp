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

#include <boost/test/unit_test.hpp>
#include <chrono>
#include <thread>
#include <libp2p/Host.h>
using namespace std;
using namespace dev;
using namespace dev::p2p;

struct P2PFixture
{
	P2PFixture() { dev::p2p::NodeIPEndpoint::test_allowLocal = true; }
	~P2PFixture() { dev::p2p::NodeIPEndpoint::test_allowLocal = false; }
};

BOOST_FIXTURE_TEST_SUITE(p2p, P2PFixture)

BOOST_AUTO_TEST_CASE(host)
{
	auto oldLogVerbosity = g_logVerbosity;
	g_logVerbosity = 10;
	
	NetworkPreferences host1prefs("127.0.0.1", 30301, false);
	NetworkPreferences host2prefs("127.0.0.1", 30302, false);
	
	Host host1("Test", host1prefs);
	host1.start();
		
	Host host2("Test", host2prefs);
	auto node2 = host2.id();
	host2.start();
	
	host1.addNode(node2, NodeIPEndpoint(bi::address::from_string("127.0.0.1"), host2prefs.listenPort, host2prefs.listenPort));
	
	this_thread::sleep_for(chrono::seconds(3));
	
	auto host1peerCount = host1.peerCount();
	auto host2peerCount = host2.peerCount();
	BOOST_REQUIRE_EQUAL(host1peerCount, 1);
	BOOST_REQUIRE_EQUAL(host2peerCount, 1);
	
	g_logVerbosity = oldLogVerbosity;
}

BOOST_AUTO_TEST_CASE(networkConfig)
{
	Host save("Test", NetworkPreferences(false));
	bytes store(save.saveNetwork());
	
	Host restore("Test", NetworkPreferences(false), bytesConstRef(&store));
	BOOST_REQUIRE(save.id() == restore.id());
}

BOOST_AUTO_TEST_CASE(save_nodes)
{
	std::list<Host*> hosts;
	for (auto i:{0,1,2,3,4,5})
	{
		Host* h = new Host("Test", NetworkPreferences("127.0.0.1", 30300 + i, false));
		h->setIdealPeerCount(10);
		// starting host is required so listenport is available
		h->start();
		while (!h->haveNetwork())
			this_thread::sleep_for(chrono::milliseconds(2));
		hosts.push_back(h);
	}
	
	Host& host = *hosts.front();
	for (auto const& h: hosts)
		host.addNode(h->id(), NodeIPEndpoint(bi::address::from_string("127.0.0.1"), h->listenPort(), h->listenPort()));
	
	Host& host2 = *hosts.back();
	for (auto const& h: hosts)
		host2.addNode(h->id(), NodeIPEndpoint(bi::address::from_string("127.0.0.1"), h->listenPort(), h->listenPort()));

	this_thread::sleep_for(chrono::milliseconds(2000));
	bytes firstHostNetwork(host.saveNetwork());
	bytes secondHostNetwork(host.saveNetwork());
	
	BOOST_REQUIRE_EQUAL(sha3(firstHostNetwork), sha3(secondHostNetwork));
	
	BOOST_CHECK_EQUAL(host.peerCount(), 5);
	BOOST_CHECK_EQUAL(host2.peerCount(), 5);
	
	RLP r(firstHostNetwork);
	BOOST_REQUIRE(r.itemCount() == 3);
	BOOST_REQUIRE(r[0].toInt<unsigned>() == dev::p2p::c_protocolVersion);
	BOOST_REQUIRE_EQUAL(r[1].toBytes().size(), 32); // secret
	BOOST_REQUIRE(r[2].itemCount() >= 5);
	
	for (auto i: r[2])
	{
		BOOST_REQUIRE(i.itemCount() == 3 || i.itemCount() == 10);
		BOOST_REQUIRE(i[0].itemCount() == 4 || i[0].itemCount() == 16);
	}
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(peerTypes)

BOOST_AUTO_TEST_CASE(emptySharedPeer)
{
	shared_ptr<Peer> p;
	BOOST_REQUIRE(!p);
	
	std::map<NodeId, std::shared_ptr<Peer>> peers;
	p = peers[NodeId()];
	BOOST_REQUIRE(!p);
	
	p.reset(new Peer(UnspecifiedNode));
	BOOST_REQUIRE(!p->id);
	BOOST_REQUIRE(!*p);
	
	p.reset(new Peer(Node(NodeId(EmptySHA3), UnspecifiedNodeIPEndpoint)));
	BOOST_REQUIRE(!(!*p));
	BOOST_REQUIRE(*p);
	BOOST_REQUIRE(p);
}

BOOST_AUTO_TEST_SUITE_END()

int peerTest(int argc, char** argv)
{
	Public remoteAlias;
	short listenPort = 30303;
	string remoteHost;
	short remotePort = 30303;
	
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

	Host ph("Test", NetworkPreferences(listenPort));

	if (!remoteHost.empty() && !remoteAlias)
		ph.addNode(remoteAlias, NodeIPEndpoint(bi::address::from_string(remoteHost), remotePort, remotePort));

	this_thread::sleep_for(chrono::milliseconds(200));

	return 0;
}

