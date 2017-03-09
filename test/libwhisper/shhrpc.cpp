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
/** @file shhrpc.cpp
* @author Vladislav Gluhovsky <vlad@ethdev.com>
* @date July 2015
*/

#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <libdevcore/Log.h>
#include <libdevcore/CommonIO.h>
#include <libethcore/CommonJS.h>
#include <libwebthree/WebThree.h>
#include <libweb3jsonrpc/ModularServer.h>
#include <libweb3jsonrpc/Whisper.h>
#include <libweb3jsonrpc/Net.h>
#include <libweb3jsonrpc/Web3.h>
#include <libweb3jsonrpc/Eth.h>
#include <libweb3jsonrpc/AccountHolder.h>
#include <libweb3jsonrpc/SessionManager.h>
#include <libweb3jsonrpc/AdminNet.h>
#include <libweb3jsonrpc/AdminUtils.h>
#include <jsonrpccpp/server/connectors/httpserver.h>
#include <jsonrpccpp/client/connectors/httpclient.h>
#include <json_spirit/JsonSpiritHeaders.h>
#include <test/libtesteth/TestHelper.h>
#include <test/libweb3jsonrpc/WebThreeStubClient.h>
#include <libethcore/KeyManager.h>
#include <libp2p/Common.h>
#include <libwhisper/WhisperHost.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::p2p;
using namespace dev::shh;
using namespace dev::test;
namespace js = json_spirit;

WebThreeDirect* web3;

unique_ptr<ModularServer<rpc::EthFace, rpc::WhisperFace, rpc::NetFace, rpc::Web3Face, rpc::AdminNetFace, rpc::AdminUtilsFace>> modularServer;
rpc::WhisperFace* whisperFace;
rpc::NetFace* netFace;
rpc::Web3Face* w3Face;
rpc::SessionManager sm;
rpc::EthFace* ethFace;
rpc::AdminNetFace* adminNetFace;
rpc::AdminUtilsFace* adminUtilsFace;
unique_ptr<AccountHolder> accountHolder;
unique_ptr<WebThreeStubClient> jsonrpcClient;
static string const c_version("shhrpc-web3");
static unsigned const c_ttl = 777000;

struct Setup: public TestOutputHelper
{
	Setup()
	{
		dev::p2p::NodeIPEndpoint::test_allowLocal = true;
		static bool setup = false;
		if (!setup)
		{
			setup = true;
			NetworkPreferences nprefs("127.0.0.1", 0, false);
			web3 = new WebThreeDirect(c_version, "", ChainParams(), WithExisting::Trust, {"shh"}, nprefs);
			web3->setIdealPeerCount(9);
			//auto server = new jsonrpc::HttpServer(8080);
			KeyManager keyMan;
			TrivialGasPricer gp;
			accountHolder.reset(new FixedAccountHolder([&](){return web3->ethereum();}, {}));
			whisperFace = new rpc::Whisper(*web3, {});
			//ethFace = new rpc::Eth(*web3->ethereum(), *accountHolder.get());
			netFace = new rpc::Net(*web3);
			w3Face = new rpc::Web3(web3->clientVersion());
			adminNetFace = new rpc::AdminNet(*web3, sm);
			adminUtilsFace = new rpc::AdminUtils(sm);
			//modularServer.reset(new ModularServer<rpc::EthFace, rpc::WhisperFace, rpc::NetFace, rpc::Web3Face, rpc::AdminNetFace, rpc::AdminUtilsFace>(ethFace, whisperFace, netFace, w3Face, adminNetFace, adminUtilsFace));
			//modularServer->addConnector(server);
			//modularServer->StartListening();
			auto client = new jsonrpc::HttpClient("http://localhost:8080");
			jsonrpcClient = unique_ptr<WebThreeStubClient>(new WebThreeStubClient(*client));
		}
	}

	~Setup()
	{
		dev::p2p::NodeIPEndpoint::test_allowLocal = false;
	}

	TestOutputHelper testHelper;
};

Json::Value createMessage(string const& _from, string const& _to, string const& _topic = "", string _payload = "")
{
	Json::Value msg;
	msg["from"] = _from;
	msg["to"] = _to;
	msg["ttl"] = toJS(c_ttl);

	if (_payload.empty())
		_payload = string("0x") + h256::random().hex();

	msg["payload"] = _payload;
	
	if (!_topic.empty())
	{
		Json::Value t(Json::arrayValue);
		t.append(_topic);
		msg["topics"] = t;
	}

	return msg;
}

BOOST_FIXTURE_TEST_SUITE(shhrpc, Setup)

//
// Disabled tests as they are unstable and tend to stall the test suite.
//

//BOOST_AUTO_TEST_CASE(basic)
//{
//	cnote << "Testing web3 basic functionality...";

//	web3->startNetwork();
//	unsigned const step = 10;
//	for (unsigned i = 0; i < 3000 && !web3->haveNetwork(); i += step)
//		this_thread::sleep_for(chrono::milliseconds(step));

//	BOOST_REQUIRE(web3->haveNetwork());

//	NetworkPreferences prefs2("127.0.0.1", 0, false);
//	string const version2 = "shhrpc-host2";
//	Host host2(version2, prefs2);
//	host2.start();
//	auto port2 = host2.listenPort();
//	BOOST_REQUIRE(port2);
//	BOOST_REQUIRE_NE(port2, web3->nodeInfo().port);
//	auto whost2 = host2.registerCapability(make_shared<WhisperHost>());

//	for (unsigned i = 0; i < 3000 && !host2.haveNetwork(); i += step)
//		this_thread::sleep_for(chrono::milliseconds(step));

//	BOOST_REQUIRE(host2.haveNetwork());

//	web3->addNode(host2.id(), NodeIPEndpoint(bi::address::from_string("127.0.0.1"), port2, port2));

//	for (unsigned i = 0; i < 3000 && (!web3->peerCount() || !host2.peerCount()); i += step)
//		this_thread::sleep_for(chrono::milliseconds(step));

//	BOOST_REQUIRE_EQUAL(host2.peerCount(), 1);
//	BOOST_REQUIRE_EQUAL(web3->peerCount(), 1);

//	vector<PeerSessionInfo> vpeers = web3->peers();
//	BOOST_REQUIRE(!vpeers.empty());
//	PeerSessionInfo const& peer = vpeers.back();
//	BOOST_REQUIRE_EQUAL(peer.id, host2.id());
//	BOOST_REQUIRE_EQUAL(peer.port, port2);
//	BOOST_REQUIRE_EQUAL(peer.clientVersion, version2);

//	web3->stopNetwork();

//	for (unsigned i = 0; i < 3000 && (web3->haveNetwork() || host2.haveNetwork()); i += step)
//		this_thread::sleep_for(chrono::milliseconds(step));

//	BOOST_REQUIRE(!web3->peerCount());
//	BOOST_REQUIRE(!host2.peerCount());
//}

//BOOST_AUTO_TEST_CASE(send)
//{
//	cnote << "Testing web3 send...";

//	bool sent = false;
//	bool ready = false;
//	unsigned result = 0;
//	unsigned const messageCount = 10;
//	unsigned const step = 10;

//	Host host2("shhrpc-host2", NetworkPreferences("127.0.0.1", 0, false));
//	host2.setIdealPeerCount(1);
//	auto whost2 = host2.registerCapability(make_shared<WhisperHost>());
//	host2.start();
//	web3->startNetwork();
//	auto port2 = host2.listenPort();
//	BOOST_REQUIRE(port2);
//	BOOST_REQUIRE_NE(port2, web3->nodeInfo().port);

//	std::thread listener([&]()
//	{
//		setThreadName("listener");
//		ready = true;
//		auto w = whost2->installWatch(BuildTopicMask("odd"));
//		set<unsigned> received;
//		for (unsigned x = 0; x < 7000 && !sent; x += step)
//			this_thread::sleep_for(chrono::milliseconds(step));

//		for (unsigned x = 0, last = 0; x < 100 && received.size() < messageCount; ++x)
//		{
//			this_thread::sleep_for(chrono::milliseconds(50));
//			for (auto i: whost2->checkWatch(w))
//			{
//				Message msg = whost2->envelope(i).open(whost2->fullTopics(w));
//				last = RLP(msg.payload()).toInt<unsigned>();
//				if (received.insert(last).second)
//					result += last;
//			}
//		}
//	});

//	for (unsigned i = 0; i < 2000 && (!host2.haveNetwork() || !web3->haveNetwork()); i += step)
//		this_thread::sleep_for(chrono::milliseconds(step));

//	BOOST_REQUIRE(host2.haveNetwork());
//	BOOST_REQUIRE(web3->haveNetwork());

//	web3->requirePeer(host2.id(), NodeIPEndpoint(bi::address::from_string("127.0.0.1"), port2, port2));

//	for (unsigned i = 0; i < 3000 && (!web3->peerCount() || !host2.peerCount()); i += step)
//		this_thread::sleep_for(chrono::milliseconds(step));

//	BOOST_REQUIRE_EQUAL(host2.peerCount(), 1);
//	BOOST_REQUIRE_EQUAL(web3->peerCount(), 1);
	
//	KeyPair us = KeyPair::create();
//	for (unsigned i = 0; i < messageCount; ++i)
//	{
//		web3->whisper()->post(us.secret(), RLPStream().append(i * i).out(), BuildTopic(i)(i % 2 ? "odd" : "even"), 777000, 1);
//		this_thread::sleep_for(chrono::milliseconds(50));
//	}
	
//	sent = true;
//	auto messages = web3->whisper()->all();
//	BOOST_REQUIRE_EQUAL(messages.size(), messageCount);

//	listener.join();
//	BOOST_REQUIRE_EQUAL(result, 1 + 9 + 25 + 49 + 81);
//}

//BOOST_AUTO_TEST_CASE(receive)
//{
//	cnote << "Testing web3 receive...";

//	bool sent = false;
//	bool ready = false;
//	unsigned result = 0;
//	unsigned const messageCount = 6;
//	unsigned const step = 10;
//	Host host2("shhrpc-host2", NetworkPreferences("127.0.0.1", 0, false));
//	host2.setIdealPeerCount(1);
//	auto whost2 = host2.registerCapability(make_shared<WhisperHost>());
//	host2.start();
//	web3->startNetwork();
//	auto port2 = host2.listenPort();
//	BOOST_REQUIRE(port2);
//	BOOST_REQUIRE_NE(port2, web3->nodeInfo().port);

//	std::thread listener([&]()
//	{
//		setThreadName("listener");
//		ready = true;
//		auto w = web3->whisper()->installWatch(BuildTopicMask("odd"));
		
//		set<unsigned> received;
//		for (unsigned x = 0; x < 7000 && !sent; x += step)
//			this_thread::sleep_for(chrono::milliseconds(step));

//		for (unsigned x = 0, last = 0; x < 100 && received.size() < messageCount; ++x)
//		{
//			this_thread::sleep_for(chrono::milliseconds(50));
//			for (auto i: web3->whisper()->checkWatch(w))
//			{
//				Message msg = web3->whisper()->envelope(i).open(web3->whisper()->fullTopics(w));
//				last = RLP(msg.payload()).toInt<unsigned>();
//				if (received.insert(last).second)
//					result += last;
//			}
//		}

//		web3->whisper()->uninstallWatch(w);
//	});

//	for (unsigned i = 0; i < 2000 && (!host2.haveNetwork() || !web3->haveNetwork()); i += step)
//		this_thread::sleep_for(chrono::milliseconds(step));

//	BOOST_REQUIRE(host2.haveNetwork());
//	BOOST_REQUIRE(web3->haveNetwork());

//	auto port1 = web3->nodeInfo().port;
//	host2.addNode(web3->id(), NodeIPEndpoint(bi::address::from_string("127.0.0.1"), port1, port1));

//	for (unsigned i = 0; i < 3000 && (!web3->peerCount() || !host2.peerCount()); i += step)
//		this_thread::sleep_for(chrono::milliseconds(step));

//	BOOST_REQUIRE_EQUAL(host2.peerCount(), 1);
//	BOOST_REQUIRE_EQUAL(web3->peerCount(), 1);
	
//	KeyPair us = KeyPair::create();
//	for (unsigned i = 0; i < messageCount; ++i)
//	{
//		web3->whisper()->post(us.secret(), RLPStream().append(i * i * i).out(), BuildTopic(i)(i % 2 ? "odd" : "even"), c_ttl, 1);
//		this_thread::sleep_for(chrono::milliseconds(50));
//	}
	
//	sent = true;
//	listener.join();
//	BOOST_REQUIRE_EQUAL(result, 1 + 27 + 125);
//}

//BOOST_AUTO_TEST_CASE(serverBasic)
//{
//	cnote << "Testing basic jsonrpc server...";

//	string s = w3Face->web3_clientVersion();
//	BOOST_REQUIRE_EQUAL(s, c_version);

//	s = netFace->net_version();
//	BOOST_REQUIRE(s.empty());

//	s = w3Face->web3_sha3("some pseudo-random string here");
//	BOOST_REQUIRE_EQUAL(s.size(), h256::size * 2 + 2);
//	BOOST_REQUIRE('0' == s[0] && 'x' == s[1]);

//	s = netFace->net_peerCount();
//	BOOST_REQUIRE(!s.empty());

//	KeyPair src = KeyPair::create();
//	KeyPair dst = KeyPair::create();
//	Json::Value t1 = createMessage(toJS(src.address()), toJS(dst.address()));
//	bool b = whisperFace->shh_post(t1);
//	BOOST_REQUIRE(b);

//	string const id = whisperFace->shh_newIdentity();
//	BOOST_REQUIRE_EQUAL(id.size(), 130);
//	BOOST_REQUIRE('0' == id[0] && 'x' == id[1]);

//	b = whisperFace->shh_hasIdentity(id);
//	BOOST_REQUIRE(b);

//	Json::Value t2 = createMessage(id, id);
//	b = whisperFace->shh_post(t2);
//	BOOST_REQUIRE(b);
//}

//BOOST_AUTO_TEST_CASE(server)
//{
//	cnote << "Testing server functionality...";

//	bool b;
//	string s;
//	Json::Value j;
//	rpc::SessionPermissions permissions;
//	permissions.privileges.insert(rpc::Privilege::Admin);
//	string const text = string("0x") + h256::random().hex(); // message must be in raw form

//	string sess1 = sm.newSession(permissions);
//	string sess2("session number two");
//	sm.addSession(sess2, permissions);
	
//	int newVerbosity = 10;
//	int oldVerbosity = g_logVerbosity;
//	b = adminUtilsFace->admin_setVerbosity(newVerbosity, sess1);
//	BOOST_REQUIRE(b);
//	BOOST_REQUIRE_EQUAL(g_logVerbosity, newVerbosity);

//	b = adminUtilsFace->admin_setVerbosity(oldVerbosity, sess1);
//	BOOST_REQUIRE(b);
//	BOOST_REQUIRE_EQUAL(g_logVerbosity, oldVerbosity);

//	b = adminNetFace->admin_net_start(sess1);
//	BOOST_REQUIRE(b);

//	unsigned const step = 10;
//	for (unsigned i = 0; i < 3000 && !netFace->net_listening(); i += step)
//		this_thread::sleep_for(chrono::milliseconds(step));

//	b = netFace->net_listening();
//	BOOST_REQUIRE(b);
	
//	b = adminNetFace->admin_net_stop(sess1);
//	BOOST_REQUIRE(b);

//	b = netFace->net_listening();
//	BOOST_REQUIRE(!b);

//	j = adminNetFace->admin_net_peers(sess1);
//	BOOST_REQUIRE(j.empty());

//	j = adminNetFace->admin_net_nodeInfo(sess2);
//	BOOST_REQUIRE_EQUAL(j["id"].asString(), web3->id().hex());
//	BOOST_REQUIRE_EQUAL(j["port"].asUInt(), web3->nodeInfo().port);

//	Host host2("shhrpc-host2", NetworkPreferences("127.0.0.1", 0, false));
//	host2.setIdealPeerCount(9);
//	auto whost2 = host2.registerCapability(make_shared<WhisperHost>());
//	host2.start();
//	auto port2 = host2.listenPort();
//	BOOST_REQUIRE(port2);
//	BOOST_REQUIRE_NE(port2, web3->nodeInfo().port);

//	b = adminNetFace->admin_net_start(sess2);
//	BOOST_REQUIRE(b);
	
//	for (unsigned i = 0; i < 2000 && !host2.haveNetwork(); i += step)
//		this_thread::sleep_for(chrono::milliseconds(step));

//	for (unsigned i = 0; i < 2000 && !netFace->net_listening(); i += step)
//		this_thread::sleep_for(chrono::milliseconds(step));

//	BOOST_REQUIRE(host2.haveNetwork());
//	BOOST_REQUIRE(netFace->net_listening());

//	string node("enode://");
//	node += host2.id().hex();
//	node += "@127.0.0.1:";
//	node += toString(port2);
//	b = adminNetFace->admin_net_connect(node, sess2);

//	for (unsigned i = 0; i < 3000 && !host2.peerCount(); i += step)
//		this_thread::sleep_for(chrono::milliseconds(step));

//	BOOST_REQUIRE_EQUAL(host2.peerCount(), 1);
//	this_thread::sleep_for(chrono::milliseconds(step));

//	j = adminNetFace->admin_net_peers(sess2);
//	BOOST_REQUIRE_EQUAL(j.size(), 1);
//	Json::Value peer = j[0];
//	s = peer["id"].asString();
//	BOOST_REQUIRE_EQUAL(s, host2.id().hex());
//	BOOST_REQUIRE_EQUAL(peer["port"].asUInt(), port2);

//	s = netFace->net_peerCount();
//	BOOST_REQUIRE_EQUAL(s, "0x1");

//	KeyPair src = KeyPair::create();
//	KeyPair dst = KeyPair::create();

//	Json::Value t1 = createMessage(toJS(src.address()), toJS(dst.address()));
//	b = whisperFace->shh_post(t1);
//	BOOST_REQUIRE(b);

//	string const id = whisperFace->shh_newIdentity();
//	BOOST_REQUIRE_EQUAL(id.size(), 130);
//	BOOST_REQUIRE(whisperFace->shh_hasIdentity(id));

//	Json::Value t2 = createMessage(id, id);
//	b = whisperFace->shh_post(t2);
//	BOOST_REQUIRE(b);

//	string const nonexistent = "123456789";
//	b = whisperFace->shh_uninstallFilter(nonexistent);
//	BOOST_REQUIRE(b);

//	j = whisperFace->shh_getMessages(nonexistent);
//	BOOST_REQUIRE(j.empty());

//	string const topic = "unicorns";
//	Json::Value t(Json::arrayValue);
//	t.append(topic);
//	Json::Value f;
//	f["to"] = id;
//	f["topics"] = t;
//	string const filter = whisperFace->shh_newFilter(f);

//	j = whisperFace->shh_getFilterChanges(filter);
//	BOOST_REQUIRE(j.empty());

//	j = whisperFace->shh_getMessages(filter);
//	BOOST_REQUIRE(j.empty());

//	Json::Value msg = createMessage(id, id, topic, text);
//	b = whisperFace->shh_post(msg);
//	BOOST_REQUIRE(b);
//	this_thread::sleep_for(chrono::milliseconds(50));

//	j = whisperFace->shh_getFilterChanges(filter);
//	BOOST_REQUIRE(!j.empty());
//	Json::Value m1 = j[0];
//	BOOST_REQUIRE_EQUAL(m1["ttl"], toJS(c_ttl));
//	BOOST_REQUIRE_EQUAL(m1["from"], id);
//	BOOST_REQUIRE_EQUAL(m1["to"], id);
//	BOOST_REQUIRE_EQUAL(m1["payload"], text);

//	j = whisperFace->shh_getMessages(filter);
//	BOOST_REQUIRE(!j.empty());
//	Json::Value m2 = j[0];
//	BOOST_REQUIRE_EQUAL(m2["ttl"], toJS(c_ttl));
//	BOOST_REQUIRE_EQUAL(m2["from"], id);
//	BOOST_REQUIRE_EQUAL(m2["to"], id);
//	BOOST_REQUIRE_EQUAL(m2["payload"], text);

//	j = whisperFace->shh_getFilterChanges(filter);
//	BOOST_REQUIRE(j.empty());

//	j = whisperFace->shh_getMessages(filter);
//	BOOST_REQUIRE(!j.empty());
//	m1 = j[0];
//	BOOST_REQUIRE_EQUAL(m1["ttl"], toJS(c_ttl));
//	BOOST_REQUIRE_EQUAL(m1["from"], id);
//	BOOST_REQUIRE_EQUAL(m1["to"], id);
//	BOOST_REQUIRE_EQUAL(m1["payload"], text);
//	cnote << "Testing web3 receive...";

//	bool sent = false;
//	bool ready = false;
//	unsigned result = 0;
//	unsigned const messageCount = 6;
//	unsigned const step = 10;
//	Host host2("shhrpc-host2", NetworkPreferences("127.0.0.1", 0, false));
//	host2.setIdealPeerCount(1);
//	auto whost2 = host2.registerCapability(make_shared<WhisperHost>());
//	host2.start();
//	web3->startNetwork();
//	auto port2 = host2.listenPort();
//	BOOST_REQUIRE(port2);
//	BOOST_REQUIRE_NE(port2, web3->nodeInfo().port);

//	std::thread listener([&]()
//	{
//		setThreadName("listener");
//		ready = true;
//		auto w = web3->whisper()->installWatch(BuildTopicMask("odd"));

//		set<unsigned> received;
//		for (unsigned x = 0; x < 7000 && !sent; x += step)
//			this_thread::sleep_for(chrono::milliseconds(step));

//		for (unsigned x = 0, last = 0; x < 100 && received.size() < messageCount; ++x)
//		{
//			this_thread::sleep_for(chrono::milliseconds(50));
//			for (auto i: web3->whisper()->checkWatch(w))
//			{
//				Message msg = web3->whisper()->envelope(i).open(web3->whisper()->fullTopics(w));
//				last = RLP(msg.payload()).toInt<unsigned>();
//				if (received.insert(last).second)
//					result += last;
//			}
//		}

//		web3->whisper()->uninstallWatch(w);
//	});

//	for (unsigned i = 0; i < 2000 && (!host2.haveNetwork() || !web3->haveNetwork()); i += step)
//		this_thread::sleep_for(chrono::milliseconds(step));

//	BOOST_REQUIRE(host2.haveNetwork());
//	BOOST_REQUIRE(web3->haveNetwork());

//	auto port1 = web3->nodeInfo().port;
//	host2.addNode(web3->id(), NodeIPEndpoint(bi::address::from_string("127.0.0.1"), port1, port1));

//	for (unsigned i = 0; i < 3000 && (!web3->peerCount() || !host2.peerCount()); i += step)
//		this_thread::sleep_for(chrono::milliseconds(step));

//	BOOST_REQUIRE_EQUAL(host2.peerCount(), 1);
//	BOOST_REQUIRE_EQUAL(web3->peerCount(), 1);

//	KeyPair us = KeyPair::create();
//	for (unsigned i = 0; i < messageCount; ++i)
//	{
//		web3->whisper()->post(us.secret(), RLPStream().append(i * i * i).out(), BuildTopic(i)(i % 2 ? "odd" : "even"), c_ttl, 1);
//		this_thread::sleep_for(chrono::milliseconds(50));
//	}

//	sent = true;
//	listener.join();
//	BOOST_REQUIRE_EQUAL(result, 1 + 27 + 125);
//	msg = createMessage(id, id, topic);
//	b = whisperFace->shh_post(msg);
//	BOOST_REQUIRE(b);
//	this_thread::sleep_for(chrono::milliseconds(50));

//	j = whisperFace->shh_getFilterChanges(filter);
//	BOOST_REQUIRE_EQUAL(j.size(), 1);

//	j = whisperFace->shh_getMessages(filter);
//	BOOST_REQUIRE_EQUAL(j.size(), 2);

//	b = whisperFace->shh_uninstallFilter(filter);
//	BOOST_REQUIRE(b);

//	j = whisperFace->shh_getFilterChanges(filter);
//	BOOST_REQUIRE(j.empty());

//	j = whisperFace->shh_getMessages(filter);
//	BOOST_REQUIRE(j.empty());

//	msg = createMessage(id, id, topic);
//	b = whisperFace->shh_post(msg);
//	BOOST_REQUIRE(b);
//	this_thread::sleep_for(chrono::milliseconds(50));

//	j = whisperFace->shh_getFilterChanges(filter);
//	BOOST_REQUIRE(j.empty());

//	j = whisperFace->shh_getMessages(filter);
//	BOOST_REQUIRE(j.empty());

//	b = adminNetFace->admin_net_stop(sess2);
//	BOOST_REQUIRE(b);

//	b = netFace->net_listening();
//	BOOST_REQUIRE(!b);
//}

BOOST_AUTO_TEST_SUITE_END()
