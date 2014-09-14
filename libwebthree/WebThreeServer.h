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
/** @file WebThreeServer.h
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <boost/asio.hpp>
#include "Common.h"

namespace dev
{
	
class RLPConnection;
class RLPMessage;
	
/**
 * @brief Server for interfacing between Ethereum systems (client, whisper, swarm).
 *
 * @todo: move logchannels to common
 * @todo: parameter for initializing acceptor by default
 * @todo: replace m_responders with something direct
 * @todo: handle null channel(for connection-specific messaging)
 * @todo: rename service sequence to notify sequence
 * @todo: document header, sequences, notification channels (0x00 packets are for notifying channels; header size is 4 instead of 3)
 * @todo: refactor for reflective interface
 * @todo: createServerProcess();  Creates ethereum process dedicated to providing RPC services.
 * @todo: handling exceptions (MessageTooLarge/Small/etc) thrown from boost work
 * @todo: disconnect message/packet (*only* for version mismatch)
 *
 */
class WebThreeServer: public std::enable_shared_from_this<WebThreeServer>
{
	friend class RLPMessage; // server verifies sequence
	
public:
	/// Constructor. After this, everything should be set up to go.
	WebThreeServer(boost::asio::ip::tcp::endpoint _ep = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 30310));
	
	/// Destructor.
	~WebThreeServer();

	/// Provided function will be passed RLP of incoming messages for the given service. Not thread-safe.
	/// @todo exception if responder already registered
	void setMessageHandler(WebThreeServiceType _serviceId, std::function<void(RLP const& _rlp)> _responder);
	
private:
	/// Start server.
	void startServer();
	
	/// Iterates handlers, creates supporting data structures and starts doRead loops for each registered service.
	void refreshHandlers();
	
	/// Stops server. Server sets m_stopped to true and then joins network and responder threads.
	void stopServer();
	
	void doAccept();
	
	boost::asio::ip::tcp::endpoint m_endpoint;							///< Default endpoint is 127.0.0.1:30310

	boost::asio::io_service m_io;										///< Boost IO Service
	boost::asio::ip::tcp::acceptor m_acceptor;							///< Socket acceptor for incoming client connections.
	std::thread m_ioThread;											///< Thread for run()'ing boost I/O
	
	std::map<WebThreeServiceType,std::shared_ptr<messageHandler>> m_responders;		///< Services' responder methods.
	std::mutex x_responders;											///< m_responders mutex.
	std::vector<std::shared_ptr<RLPConnection>> m_connections;		///< Connected sessions.
	std::mutex x_connections;											///< m_sessions mutex.

	/// Setting true causes responder-process to set a deadline timer. Acceptor and read loops halt when deadline timer occurs.
	std::atomic<bool> m_stopped;
	std::mutex x_stopped;
};


}

