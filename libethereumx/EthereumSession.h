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
/** @file EthereumSession.h
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <thread>
#include <mutex>
#include <libp2p/Common.h>
#include "Common.h"

namespace dev
{
namespace eth
{

using namespace dev::p2p;
class Ethereum;
	
/**
 * @brief API Session for interfacing with Ethereum.
 * Handles RPC requests and connectivity of a single Client.
 *
 */
class EthereumSession: public std::enable_shared_from_this<EthereumSession>
{
public:
	EthereumSession(Ethereum* _s, boost::asio::ip::tcp::socket* _socket);
	
	/// Verifies version of remote, and starts read loop.
	/// Network code within start() is same as doReadAndRespond() except that it
	/// handles handshake-related concerns.
	void start();
	
	/// @returns if client connection is open. @returns false if disconnecting.
	bool connectionOpen() const;
	
	/// Stops responding to requests. If a request is pending, disconnect() will wait until response is written to network.
	void disconnect();
	
	unsigned socketId() const { return m_socket.native_handle(); }
	
private:
	/// Begin reading and responding to client requests.
	void doReadAndRespond();
	
	/// Handles incoming request packet, passed to interpret.
	void handleRead(const boost::system::error_code& _ec, size_t _len);
	
	/// Interprets packet, performs procedure-call, and sends response.
	bool interpretRequest(RLP const& _r);
	
	/// Send response from server to client. Only called in reponse to a request from within handleRead().
	/// write() is performed synchronously to ensure all requests are responded to.
	void sendResponse(RPCResponsePacketType _type, bytesConstRef _rlpmsg);
	
	Ethereum* m_ethereum;										///< Called by session to peform Client calls.
	
	mutable boost::asio::ip::tcp::socket m_socket;				///< Connection to client.
	bytes m_recvBuffer;										///< Receive buffer for all incoming data.
	bytes m_received;										///< Receive buffer for complete messages.
	std::chrono::steady_clock::time_point m_disconnectAt;		///< Used to signal disconnect.
	mutable std::mutex x_disconnectAt;						///< m_disconnectAt mutex.
};

}
}

