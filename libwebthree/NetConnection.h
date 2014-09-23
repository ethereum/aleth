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
#include "NetCommon.h"

namespace dev
{
	
class NetMsg;
class NetProtocol;

/**
 * @brief Interface for managing incoming/outgoing sockets.
 *
 * @todo write queue
 * @todo protocol error handling (separate from network/shutdown errors)
 * @todo replace x_socketError in handshake
 */
class NetConnection: public std::enable_shared_from_this<NetConnection>
{
	/// Accesses socket()
	friend class NetEndpoint;
	
public:
	/// Constructor for incoming connections.
	NetConnection(boost::asio::io_service& _io_service);
	
	/// Constructor for outgoing connections.
	NetConnection(boost::asio::io_service& _io_service, boost::asio::ip::tcp::endpoint _ep);

	/// Destructor.
	~NetConnection();

	void setServiceMessageHandler(NetMsgServiceType _svc, messageHandler _h);
	void setDataMessageHandler(NetMsgServiceType _svc, messageHandler _h);
	
	/// Send handhsake and start connection read loop upon success.
	void start();
	
	/// Use at your own risk! (currently public for tests)
	boost::asio::ip::tcp::socket* socket();
	
	/// Send message
	void send(NetMsg const& _msg);
	
	/// @returns if connection is open; returns false if connection is shutting down
	bool connectionOpen() const;

	/// @returns if connection closed with error
	bool connectionError() const;

	/// Attempt to gracefully shutdown connection
	void shutdown(bool _wait = true);

protected:
//	/// Build and send message
//	void send(NetMsgServiceType _svc, NetMsgSequence _seq, NetMsgType _type, RLP const& _msg);
	
private:
//	/// @returns if RLP message size is valid and matches length from 4-byte header
//	bool checkPacket(bytesConstRef _netMsg) const;

	void doWrite();
	
	/// Continously read messages until shutdown. Messages are passed to appropriate service handler or, for control messages, handled directly.
	void doRead(size_t _rlpLen = 0);
	
	/// Called by start(); disconnects if handshake fails.
	void handshake(size_t _rlpLen = 0);

	/// Shutdown due to error
	void shutdownWithError(boost::system::error_code _ec);

	/// Close socket. Used by shutdown methods.
	void closeSocket();
	
	messageHandlers m_serviceMsgHandlers;		///< Called with service messages.
	messageHandlers m_dataMsgHandlers;		///< Called with data messages.
	
	boost::asio::ip::tcp::socket m_socket;
	boost::asio::ip::tcp::endpoint m_endpoint;
	bytes m_recvBuffer;						///< Buffer for bytes of new messages
	size_t m_recvdBytes = 0;					///< Incoming bytes of new message
	
	std::atomic<bool> m_stopped;				///< Set when connection is stopping or stopped. Handshake cannot occur unless m_stopped is true.
	std::atomic<bool> m_started;				///< Atomically ensure connection is started once. Start cannot occur unless m_started is false. Managed by start() and shutdown(bool).
	bool m_originateConnection;				///< True if connection is outbound, otherwise false.
	
	boost::system::error_code m_socketError;	///< Set when shutdown performed w/error.
	mutable std::mutex x_socketError;			///< Mutex for error which can occur from host or IO thread.
};


}

