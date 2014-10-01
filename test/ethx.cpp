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

#include <libdevnet/NetMsg.h>
#include <libdevnet/NetConnection.h>
#include <libdevnet/NetEndpoint.h>
#include <libp2p/Host.h>
#include <libethereum/Client.h>

#include <libdevcrypto/FileSystem.h>
#include <libwebthree/WebThree.h>
#include "rpcservice.h"
#include "rpcprotocol.h"

using namespace std;
using namespace dev;

// NOTE: Tests are currently selectively-disabled (return; at top), as acceptor holds socket even after acceptor/sockets are cancelled and closed (causing tests to lock and/or fail).

BOOST_AUTO_TEST_SUITE( netproto )

BOOST_AUTO_TEST_CASE(test_netproto_simple)
{
	return;
	cout << "test_netproto_simple" << endl;
	
	// Shared
	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("127.0.0.1"), 30310);
	
	
	/// Service<Protocol> && Protocol<Service>
	TestService s(nullptr);
	TestServiceProtocol p((NetConnection *)nullptr, &s);
	assert(TestServiceProtocol::serviceId() == 255);
	assert(TestService::serviceId() == TestServiceProtocol::serviceId());
	
	// sA: To be used by client request test...
	std::string sA = s.serviceString();
	assert(sA == "serviceString");
	
	// pA: To be used by server request test...
	std::string pA = p.protocolString();
	assert(pA == "protocolString");

	// Access interface through service (used by protocol)
	assert(s.m_interface->stringTest() == "string");

	// Client request test:
	
	// Register service, start endpoint
	shared_ptr<NetEndpoint> netEp(new NetEndpoint(ep));
	netEp->registerService(&s);
	netEp->start();
	
	// Client connection
	auto clientConn = make_shared<NetConnection>(netEp->get_io_service(), ep);
	TestRPCProtocol clientProtocol(clientConn.get());
	clientConn->start();
	
	// wait for handshake
	// todo: this may be removable as requests are queued
	while (!clientConn->connectionOpen() && !clientConn->connectionError());
	
	
	
}

BOOST_AUTO_TEST_CASE(test_netendpoint)
{
	return;
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

BOOST_AUTO_TEST_CASE(test_webthree)
{
	WebThreeDirect direct(string("Test/v") + dev::Version + "/" DEV_QUOTED(ETH_BUILD_TYPE) "/" DEV_QUOTED(ETH_BUILD_PLATFORM), getDataDir() + "/Test", false, {"eth", "shh"});
	
	Address a(fromHex("1a26338f0d905e295fccb71fa9ea849ffa12aaf4"));
	u256 directBalance = direct.ethereum()->balanceAt(a);
	assert(directBalance.str() == "1606938044258990275541962092341162602522202993782792835301376");
	
	WebThree client;
	u256 clientBalance = client.ethereum()->balanceAt(a);
	clientBalance = client.ethereum()->balanceAt(a);

	assert(clientBalance == directBalance);
	cout << "Got balanceAt: " << clientBalance << endl;
}
	
BOOST_AUTO_TEST_CASE(test_netservice)
{
	return;
	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("127.0.0.1"), 30310);

	p2p::Host net("test", p2p::NetworkPreferences());
	unique_ptr<eth::Client> eth(new eth::Client(&net));
	clog(RPCNote) << "Blockchain opened. Starting RPC Server.";
	
	unique_ptr<EthereumRPC> server(new EthereumRPC(eth.get()));
	
	shared_ptr<NetEndpoint> netEp(new NetEndpoint(ep));
	netEp->registerService(server.get());
	netEp->start();
	
	// rpc client requries connection and server
	
	auto clientConn = make_shared<NetConnection>(netEp->get_io_service(), ep);
	EthereumRPCClient client(clientConn.get());
	clientConn->start();
	
	Address a(fromHex("1a26338f0d905e295fccb71fa9ea849ffa12aaf4"));
	u256 balance = client.balanceAt(a, -1);
	cout << "Got balanceAt: " << balance << endl;

	assert(balance.str() == "1606938044258990275541962092341162602522202993782792835301376");
}
	
BOOST_AUTO_TEST_SUITE_END()

	
BOOST_AUTO_TEST_SUITE( rlpnet )

BOOST_AUTO_TEST_CASE(test_rlpnet_messages)
{
	// service message:
	RLPStream s;
	s.appendList(1);
	s << "version";
	
	NetMsg version((NetMsgServiceType)0, 0, (NetMsgType)0, RLP(s.out()));
	
	bytes rlpbytes = version.payload();
	RLP rlp = RLP(bytesConstRef(&rlpbytes).cropped(4));
	assert("version" == rlp[2][0].toString());
	assert("version" == RLP(rlp[2][0].data().toBytes()).toString());
	assert("version" == RLP(version.rlp())[0].toString());
	
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
	// Acceptor is to be replaced with Net implementation.
//	static std::vector<shared_ptr<NetConnection> > conns;
//	
//	auto newConn = make_shared<NetConnection>(acceptIo);
//	conns.push_back(newConn);
//	_a.async_accept(*newConn->socket(), [newConn, &_a, &acceptIo, ep, connected](boost::system::error_code _ec)
//	{
//		acceptConnection(_a, acceptIo, ep, connected);
//		connected->fetch_add(1);
//		
//		if (!_ec)
//			newConn->start();
//		else
//			clog(RPCNote) << "error accepting socket: " << _ec.message();
//	});
};

BOOST_AUTO_TEST_CASE(test_rlpnet_connections)
{
	
	// Disabled: Acceptor is to be replaced with Net implementation.
	return;
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
	
BOOST_AUTO_TEST_SUITE_END()

