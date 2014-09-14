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

#include <libethereumx/Ethereum.h>

#include <libwebthree/WebThreeServer.h>

using namespace dev;
using namespace dev::eth;


BOOST_AUTO_TEST_SUITE( webthree_server )

BOOST_AUTO_TEST_CASE(test_webthree_server_lifecycle)
{
	WebThreeServer s;
	s.setMessageHandler(EthereumService, [=](NetMsgType _type, RLP const& _request){
		// handle eth requests
	});
	
	s.setMessageHandler(WhisperService, [=](NetMsgType _type, RLP const& _request){
		// handle shh requests
	});

	s.setMessageHandler(SwarmService, [=](NetMsgType _type, RLP const& _request){
		// handle bzz requests
	});
	
	s.setMessageHandler(WebThreeService, [=](NetMsgType _type, RLP const& _request){
		// handle general requests
	});
	
}

BOOST_AUTO_TEST_SUITE_END() // webthree_server


BOOST_AUTO_TEST_CASE(ethx_test_server_going_away)
{
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

