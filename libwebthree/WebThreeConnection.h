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
/** @file WebThreeConnection.h
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <boost/asio.hpp>
#include "Common.h"

namespace dev
{

class WebThreeMessage;
	
class WebThreeConnection
{
	friend class WebThreeServer;
	friend class WebThreeResponse;
	
public:
	WebThreeConnection(messageHandler* _s, boost::asio::ip::tcp::socket *_sock);
	WebThreeConnection();
	
private:
	void send(WebThreeMessage& _msg);
	
	/// Called when message is received. Message is passed to designated messageHandler.
	void messageReceived(WebThreeMessage& _msg);
	
	/// @returns if RLP message size matches length from 4-byte header and is valid. Called by payload.
	bool checkPacket(bytesConstRef _netMsg) const;
	
	void start();
	void handshake();
	void doRead();
	
	/// Schedules write; attempts to send immediately if no writes are pending.
	void write(bytes& _msg);
	void doWrite();
	
	
	// Networking:
	bool connectionOpen() const;
	
	void disconnect();
	
	messageHandler* m_responder;
	boost::asio::ip::tcp::socket *m_socket;
	
	std::chrono::steady_clock::time_point m_disconnectAt;
	mutable std::mutex x_disconnectAt;
};


}

