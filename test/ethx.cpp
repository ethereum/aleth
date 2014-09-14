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
BOOST_AUTO_TEST_CASE(test_rlpnet_connection)
{
	boost::asio::io_service io;
	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("127.0.0.1"), 30310);
	boost::asio::ip::tcp::acceptor acceptor(io, ep);

	std::promise<bool> p_connAccepted;
	std::future<bool> f_connAccepted = p_connAccepted.get_future();
	
	shared_ptr<NetConnection> connIn(new NetConnection(io));
	acceptor.async_accept(connIn->socket(), [connIn, &p_connAccepted](boost::system::error_code _ec)
	{
		p_connAccepted.set_value(true);
	});
	connIn.reset();
	
	
	NetConnection *connOut = new NetConnection(io, ep, 0, nullptr, nullptr);
	while (!connOut->connectionOpen())
		usleep(100);
	delete connOut;

	// listening side
	assert(f_connAccepted.get() == true);
	
	
	// connecting side
	
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

