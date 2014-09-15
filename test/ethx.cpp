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

#include <libwebthree/NetConnection.h>
#include <libwebthree/WebThreeServer.h>
#include <libethereumx/Ethereum.h>
#include <future>

using namespace std;
using namespace dev;

BOOST_AUTO_TEST_SUITE( rlpnet )

BOOST_AUTO_TEST_CASE(test_rlpnet_connectionin)
{
	return;
	boost::asio::io_service io;
	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("127.0.0.1"), 30310);
	NetConnection* n = new NetConnection(io, ep);
	assert(!n->connectionOpen());
	delete n;
	
	auto sharedconn = make_shared<NetConnection>(io, ep, 0, nullptr, nullptr);
	sharedconn.reset();
	
	// nullptr handler
	assert(!nullptr);
	messageHandlers({make_pair(0,make_shared<messageHandler>(nullptr))});
	
	auto testNoConnection = make_shared<NetConnection>(io, ep, 0, nullptr, nullptr);
	testNoConnection->start();
	io.run();
	testNoConnection.reset();
	io.reset();
}

void acceptConnection(boost::asio::ip::tcp::acceptor& _a, boost::asio::io_service &io, boost::asio::ip::tcp::endpoint ep, std::atomic<int>& connected)
{
	return;
	static std::vector<shared_ptr<NetConnection> > conns;
	std::vector<shared_ptr<NetConnection> >* cp = &conns;
	
	auto newConn = make_shared<NetConnection>(io, ep);
	_a.async_accept(newConn->socket(), [newConn, &_a, &io, ep, cp, &connected](boost::system::error_code _ec)
	{
		if (!_ec)
			newConn->start();
		else
			cout << "error accepting socket: " << _ec.message() << "\n";
		connected++;
		cp->push_back(newConn);
		usleep(rand() % 250);
		acceptConnection(_a, io, ep, connected);
	});
};

BOOST_AUTO_TEST_CASE(test_rlpnet_rapid_connections)
{
	boost::asio::io_service io;
	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("127.0.0.1"), 30310);
	
	boost::asio::ip::tcp::acceptor acceptor(io, ep);
	
	auto connIn = make_shared<NetConnection>(io, ep);
	std::atomic<int> acceptedConnections(0);
	acceptConnection(acceptor, io, ep, acceptedConnections);
	
	std::thread ioThread([&]()
	{
		io.run();
	});
	
	for (auto i = 0; i < 20; i++)
	{
		auto connOut = make_shared<NetConnection>(io, ep, 0, nullptr, nullptr);
		connOut->start();
	}
	
	usleep(200000);
	
	io.stop();
	usleep(1000); // boost/kqueue doesn't like halting 200 connections
	ioThread.join();
	
	cout << "Connections: " << acceptedConnections;
}

BOOST_AUTO_TEST_CASE(test_rlpnet_connection)
{
	boost::asio::io_service io;
	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("127.0.0.1"), 30310);
	boost::asio::ip::tcp::acceptor acceptor(io, ep);
	
	for (auto i = 0; i < 300; i++)
	{
		std::promise<bool> p_connAccepted;
		std::future<bool> f_connAccepted = p_connAccepted.get_future();

// net handler lifecycle:
		auto connIn = make_shared<NetConnection>(io, ep);
		acceptor.async_accept(connIn->socket(), [connIn, &p_connAccepted](boost::system::error_code _ec)
		{
			if (!_ec)
				connIn->start();
			else
				cout << "error accepting socket: " << _ec.message() << "\n";
			
			// TODO: issue causing parallel operations setting each other's futures
			try {
				p_connAccepted.set_value(true);
			} catch(...){
				
			}
		});
		
		std::thread ioThread([&]()
		{
			io.run();
		});

		auto connOut = make_shared<NetConnection>(io, ep, 0, nullptr, nullptr);
		connOut->start();

		while (!connOut->connectionOpen() && !connOut->connectionError());
		while (!connOut->connectionError());
			if (connIn->connectionOpen())
				break;
		
		// listening side
		if (connIn->connectionOpen() && connOut->connectionOpen())
			assert(f_connAccepted.get() == true);

		// manually shutdown; destructor shutdown doesn't wait for pertinent IO to complete
		connOut->shutdown();
		connIn->shutdown();

		// let there be a deallocation fight
		connIn.reset();
		connOut.reset();
		
		io.stop();
		ioThread.join();
		io.reset();
	}

}
BOOST_AUTO_TEST_SUITE_END() // rlpnet


BOOST_AUTO_TEST_SUITE( webthree_net )
BOOST_AUTO_TEST_CASE(test_webthree_net_eth)
{
	WebThreeServer s;
	s.setMessageHandler(EthereumService, [=](NetMsgType _type, RLP const& _request){
		// handle eth requests
	});
}

BOOST_AUTO_TEST_CASE(test_webthree_net_shh)
{
	WebThreeServer s;
	s.setMessageHandler(EthereumService, [=](NetMsgType _type, RLP const& _request){
		// shh requests
	});
}

BOOST_AUTO_TEST_CASE(test_webthree_net_bzz)
{
	WebThreeServer s;
	s.setMessageHandler(EthereumService, [=](NetMsgType _type, RLP const& _request){
		// bzz requests
	});
}
BOOST_AUTO_TEST_SUITE_END() // webthree_net


BOOST_AUTO_TEST_CASE(ethx_test_server_going_away)
{
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

