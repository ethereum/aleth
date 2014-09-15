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
/** @file NetConnection.h
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <boost/asio.hpp>
#include "Common.h"

namespace dev
{

class NetMsg;
	
/**
 * @brief Interface for listening on or connecting sockets for
 *
 * @todo Random sequence initialization.
 */
class NetConnection: public std::enable_shared_from_this<NetConnection>
{
public:
	// Constructor for incoming connections.
	NetConnection(boost::asio::io_service& _io_service, boost::asio::ip::tcp::endpoint _ep);
	
	// Constructor for outgoing connections (multiple services).
	NetConnection(boost::asio::io_service& _io_service, boost::asio::ip::tcp::endpoint _ep, messageHandlers _svcMsgHandlers, messageHandlers _dataMsgHandlers);
	
	// Constructor for outgoing connections (single service, twoway).
	NetConnection(boost::asio::io_service& _io_service, boost::asio::ip::tcp::endpoint _ep, NetMsgServiceType _svc, messageHandler* _svcMsgHandler, messageHandler* _dataMsgHandler);
	
	// Destructor.
	~NetConnection();
	
	/// Send handhsake and start connection read loop
	void start();
	
	/// Use at your own risk!
	boost::asio::ip::tcp::socket& socket();
	
	/// Send message
	/// @todo Defer shutdown until send completes
	void send(NetMsg const& _msg);
	
	/// @returns if connection is open; returns false if connection is shutting down
	bool connectionOpen() const;
	
	bool connectionError() const;
	
	/// Gracefully shutdown connection
	void shutdown(bool _wait = true);

protected:
//	/// Build and send message
//	void send(NetMsgServiceType _svc, NetMsgSequence _seq, NetMsgType _type, RLP const& _msg);
	
private:
	/// @returns if RLP message size is valid and matches length from 4-byte header
	/// @todo check service, sequence, packet type
	bool checkPacket(bytesConstRef _netMsg) const;

	
	void doRead(size_t _rlpLen = 0);
	
	
	void handshake(size_t _rlpLen = 0);

	
	/// Shutdown due to error
	void shutdownWithError(boost::system::error_code _ec);

	/// Close socket. Used by shutdown methods.
	void closeSocket();
	
	messageHandlers m_serviceMsgHandlers;
	messageHandlers m_dataMsgHandlers;
	
	boost::asio::ip::tcp::socket m_socket;
	boost::asio::ip::tcp::endpoint m_endpoint;
	bytes m_recvBuffer;						///< Buffer for bytes of new messages
	size_t m_recvdBytes = 0;						///< Incoming bytes of new message
	
	std::atomic<bool> m_stopped;				///< Set when connection is stopping or stopped. Handshake cannot occur unless m_stopped is true.
	std::atomic<bool> m_started;				///< Atomically ensure connection is started once. Start cannot occur unless m_started is false. Managed by start() and shutdown(bool).
	bool m_originateConnection;
	
	boost::system::error_code m_socketError;
	mutable std::mutex x_socketError;
};


}

