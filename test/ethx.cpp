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
/** @file ethx.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 * State test functions.
 */

#include <boost/test/unit_test.hpp>

#include <libwebthree/NetMsg.h>
#include <libwebthree/NetConnection.h>
#include <libwebthree/NetEndpoint.h>
#include <libwebthree/WebThreeServer.h>
#include <libethereumx/Ethereum.h>
#include <libp2p/Host.h>
#include <libethereum/Client.h>

#include "rpcprotocol.h"
#include "rpcservice.h"

using namespace std;
using namespace dev;

BOOST_AUTO_TEST_SUITE( netproto )

BOOST_AUTO_TEST_CASE(test_netproto_simple)
{
	cout << "test_netproto_simple" << endl;
	
	EthereumRPCService s(nullptr);
	EthereumRPCServer p((NetConnection *)nullptr, &s);
	
	std::string sA = s.test();
	std::string pA = p.test();
	assert(sA == pA);
	assert(sA == "a");
	
	assert((NetMsgServiceType)2 == EthereumRPCService::serviceId());
	
}

BOOST_AUTO_TEST_CASE(test_netendpoint)
{
	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("127.0.0.1"), 30310);

	shared_ptr<NetEndpoint> netEp(new NetEndpoint(ep));
	netEp->start();
	
	auto testConn = make_shared<NetConnection>(netEp->get_io_service(), ep);
	testConn->start();
	
	while (!testConn->connectionOpen() && !testConn->connectionError());
	
	netEp->stop();
	testConn.reset();
	
	netEp->start();
	testConn.reset(new NetConnection(netEp->get_io_service(), ep));
	testConn->start();
	while (!testConn->connectionOpen() && !testConn->connectionError());
	testConn.reset();
	netEp->stop();
	netEp.reset();
	testConn.reset();
}

BOOST_AUTO_TEST_CASE(test_netservice)
{
	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("127.0.0.1"), 30310);

	p2p::Host net("test", p2p::NetworkPreferences());
	unique_ptr<eth::Client> eth(new eth::Client(&net));
	clog(RPCNote) << "Blockchain opened. Starting RPC Server.";
	
	unique_ptr<EthereumRPCService> server(new EthereumRPCService(eth.get()));
	
	shared_ptr<NetEndpoint> netEp(new NetEndpoint(ep));
	netEp->registerService(server.get());
	netEp->start();
	
	// rpc client requries connection and server
	
	auto clientConn = make_shared<NetConnection>(netEp->get_io_service(), ep);
	clientConn->start();
	EthereumRPCClient client(clientConn.get(), nullptr);
	
	Address a(fromHex("0x1a26338f0d905e295fccb71fa9ea849ffa12aaf4"));
	u256 balance = client.balanceAt(a);
	cout << "Got balanceAt: " << balance << endl;
}
	
BOOST_AUTO_TEST_SUITE_END()

	
BOOST_AUTO_TEST_SUITE( rlpnet )

BOOST_AUTO_TEST_CASE(test_rlpnet_messages)
{
	// service message:
	RLPStream s;
	s << "version";
	
	NetMsg version((NetMsgServiceType)0, 0, (NetMsgType)0, RLP(s.out()));
	
	bytes rlpbytes = version.payload();
	RLP rlp = RLP(bytesConstRef(&rlpbytes).cropped(4));
	assert("version" == rlp[2].toString());
	assert("version" == RLP(rlp[2].data().toBytes()).toString());
	assert("version" == RLP(version.rlp()).toString());
	
	// application message:
	NetMsg appthing((NetMsgServiceType)1, 0, (NetMsgType)1, RLP(s.out()));
	
	rlpbytes = appthing.payload();
	rlp = RLP(bytesConstRef(&rlpbytes).cropped(4));
	
	assert("version" == rlp[3].toString());
	
	// converting to const storage (this fails)
	RLP rlpFromBytes = RLP(rlp[3].data().toBytes());
	assert("version" == rlpFromBytes.toString());

}

BOOST_AUTO_TEST_CASE(test_rlpnet_connectionin)
{
	cout << "test_rlpnet_connectionin" << endl;
	
	boost::asio::io_service io;
	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("127.0.0.1"), 30310);
	NetConnection* n = new NetConnection(io, ep);
	assert(!n->connectionOpen());
	delete n;
	
	auto sharedconn = make_shared<NetConnection>(io);
	sharedconn.reset();;
	
	auto testNoConnection = make_shared<NetConnection>(io, ep);
	testNoConnection->start();
	io.run();
	testNoConnection.reset();
	io.reset();
}

void acceptConnection(boost::asio::ip::tcp::acceptor& _a, boost::asio::io_service &acceptIo, boost::asio::ip::tcp::endpoint ep, std::atomic<size_t>* connected)
{
	static std::vector<shared_ptr<NetConnection> > conns;
	
	auto newConn = make_shared<NetConnection>(acceptIo);
	conns.push_back(newConn);
	_a.async_accept(*newConn->socket(), [newConn, &_a, &acceptIo, ep, connected](boost::system::error_code _ec)
	{
		acceptConnection(_a, acceptIo, ep, connected);
		connected->fetch_add(1);
		
		if (!_ec)
			newConn->start();
		else
			clog(RPCNote) << "error accepting socket: " << _ec.message();
	});
};

BOOST_AUTO_TEST_CASE(test_rlpnet_connections)
{
	cout << "test_rlpnet_rapid_connections" << endl;
	
	// Shared:
	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("127.0.0.1"), 30310);
	std::atomic<bool> stopThread(false);
	
	// Client IO:
	boost::asio::io_service io;
	std::vector<shared_ptr<NetConnection> > conns;
	
	// Acceptor IO, Thread:
	std::atomic<size_t> acceptedConnections(0);
	boost::asio::io_service acceptorIo;
	std::thread acceptorIoThread([&]()
	{
		boost::asio::ip::tcp::acceptor acceptor(acceptorIo, ep);
		acceptConnection(acceptor, acceptorIo, ep, &acceptedConnections);
		
		// Continue to run when out of work
		while (!stopThread)
		{
			clog(RPCNote) << "acceptorIoThread started";
			
			if (acceptorIo.stopped())
				acceptorIo.reset();
			acceptorIo.run();
			usleep(1000);
		}
		clog(RPCNote) << "acceptorIoThread stopped";
	});
	
	
	// Client Connections:
	
	// create first connection so IO has work
	auto connOut = make_shared<NetConnection>(acceptorIo, ep);
	connOut->start();
	conns.push_back(connOut);
	
//	std::thread ioThread([&]()
//	{
//		while (!stopThread)
//		{
//			clog(RPCNote) << "ioThread started";
//			
//			if (io.stopped())
//				io.reset();
//			io.run();
//			usleep(1000);
//		}
//		
//		clog(RPCNote) << "ioThread stopped";
//	});

	for (auto i = 0; i < 256; i++)
	{
		usleep(2000);
		auto connOut = make_shared<NetConnection>(acceptorIo, ep);
		connOut->start();
		conns.push_back(connOut);
	}

	std::set<NetConnection*> connErrs;
	while (acceptedConnections + connErrs.size() != conns.size())
	{
		for (auto c: conns)
			if (c->connectionError())
				connErrs.insert(c.get());
		
		usleep(250);
	}
	assert(acceptedConnections + connErrs.size() == conns.size());
	
	// tailend connections likely won't even start
	for (auto c: conns)
	{
		// wait for connection to open or error
		while (!c->connectionOpen() && !c->connectionError());
		
		// shutdown
		c->shutdown();
		
		// wait until shutdown
		while (c->connectionOpen() && !c->connectionError());
		
		if (c->connectionError())
			connErrs.insert(c.get());
	}

	
	// Before shutting down IO service, all connections need to be deleted
	// so connections are deallocate prior to IO service deallocation
	for (auto c: conns)
		c.reset();
	connOut.reset();
	
	stopThread.store(true);

	acceptorIo.stop();
	acceptorIoThread.join();
	
	io.stop();
//	ioThread.join();

	clog(RPCNote) << "Connections: " << acceptedConnections << "Failed: " << connErrs.size();
	
}
	
BOOST_AUTO_TEST_CASE(test_rlpnet_connectHandler)
{
//	boost::asio::io_service acceptorIo;
//	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("127.0.0.1"), 30310);
//	boost::asio::ip::tcp::acceptor acceptor(acceptorIo, ep);
//
//	std::atomic<size_t> acceptedConnections(0);
//	acceptConnection(acceptor, acceptorIo, ep, &acceptedConnections);
	
	
	
}
	
	
BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE( webthree_net )
BOOST_AUTO_TEST_CASE(test_webthree_net_eth)
{
	WebThreeServer s;
	s.setMessageHandler(EthereumService, [=](NetMsg const& _msg){
		// handle eth requests
	});
}

BOOST_AUTO_TEST_CASE(test_webthree_net_shh)
{
	WebThreeServer s;
	s.setMessageHandler(EthereumService, [=](NetMsg const& _msg){
		// shh requests
	});
}

BOOST_AUTO_TEST_CASE(test_webthree_net_bzz)
{
	WebThreeServer s;
	s.setMessageHandler(EthereumService, [=](NetMsg const& _msg){
		// bzz requests
	});
}
BOOST_AUTO_TEST_SUITE_END() // webthree_net


BOOST_AUTO_TEST_CASE(ethx_test_server_going_away)
{
	return;
	using namespace dev::eth;
	
	cnote << "Testing EthereumX...";

	cnote << "ethx: Starting first client";
	Ethereum* client1 = new Ethereum();
	
	cnote << "ethx: Starting second client";
	Ethereum* client2 = new Ethereum();
	
	usleep(100 * 1000);
	
	// returns without network
	size_t c1peerCount = client1->peerCount();
	cnote << "ethx: client1 peerCount() " << c1peerCount;
	delete client1;
	
	// returns without network (should become RPC Server)
	size_t c2peerCount = client2->peerCount();
	cnote << "ethx: client2 peerCount()" << c2peerCount;
	
	// returns w/network connected to client2
	cnote << "ethx: Starting three client";
	Ethereum* client3 = new Ethereum();

	cnote << "ethx: client3 requesting peerCount()";
	size_t c3peerCount = client3->peerCount();
	cnote << "ethx: client3 peerCount()" << c3peerCount;
	
	client3->connect("54.76.56.74");
	client3->flushTransactions();

	cnote << "ethx: Connected peerCount()" << client3->peerCount();
	
	usleep(2000 * 1000);
	
	delete client2;
	delete client3;
}

