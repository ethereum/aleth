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
/** @file RLPConnection.h
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <boost/asio.hpp>
#include "Common.h"

namespace dev
{

class RLPMessage;
	
class RLPConnection: public std::enable_shared_from_this<RLPConnection>
{
	friend class WebThreeServer;
	friend class WebThreeClient;
	
public:
	// Constructor for incoming connections.
	RLPConnection(boost::asio::io_service& io_service);
	
	// Constructor for outgoing connections.
	RLPConnection(boost::asio::io_service& io_service, boost::asio::ip::tcp::endpoint _ep);
	
	// Destructor.
	~RLPConnection() {}
	
	void setMessageHandler(RLPMessageServiceType _serviceId, messageHandler _responder);
	
	/// Send handhsake and start connection read loop
	void start() { handshake(); };
	
	/// Send message
	void send(RLPMessage& _msg);
	
	/// @returns if connection is open; returns false if connection is shutting down
	bool connectionOpen() const;

private:
	/// Build and send message
	void send(RLPMessageServiceType _svc, RLPMessageSequence _seq, RLPMessageType _type, RLP const& _msg);
	
	/// @returns if RLP message size is valid and matches length from 4-byte header
	/// @todo check service, sequence, packet type
	bool checkPacket(bytesConstRef _netMsg) const;

	void handshake(size_t _rlpLen = 0);
	void doRead(size_t _rlpLen = 0);
	
	/// Shutdown connection
	void shutdown(bool _wait = true);
	
	std::map<RLPMessageServiceType,std::shared_ptr<messageHandler>> m_msgHandlers;		///< Services' responder methods.
	std::mutex x_msgHandlers;											///< m_responders mutex.
	
	boost::asio::ip::tcp::socket m_socket;
	bytes m_recvBuffer;						///< Buffer for bytes of new messages
	size_t m_recvdBytes;						///< Incoming bytes of new message
	
	std::atomic<bool> m_stopped;				///< Set when connection is stopping or stopped.
	std::mutex x_stopped;
};


}

