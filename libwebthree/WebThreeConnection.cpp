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
/** @file WebThreeConnection.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "WebThreeConnection.h"
#include "WebThreeRequest.h"
#include "WebThreeResponse.h"

using namespace dev;

WebThreeConnection::WebThreeConnection(messageHandler* _s, boost::asio::ip::tcp::socket *_sock): m_responder(_s), m_socket(_sock)
{
	start();
}

WebThreeConnection::WebThreeConnection()
{
	/* sleep until disconnect */
	delete m_socket;
}

void WebThreeConnection::send(WebThreeMessage& _msg)
{
	
}

/// Called when message is received. Message is passed to designated messageHandler.
void WebThreeConnection::messageReceived(WebThreeMessage& _msg)
{
	
}

/// @returns if RLP message size matches length from 4-byte header and is valid. Called by payload.
bool WebThreeConnection::checkPacket(bytesConstRef _netMsg) const
{
	
}

void WebThreeConnection::start()
{
	/* register w/server(?) */
	doRead();
	handshake();
}

void WebThreeConnection::handshake()
{
	if (/* failure */ false)
		disconnect();
}

void WebThreeConnection::doRead()
{
	
}

void WebThreeConnection::write(bytes& _msg)
{
	
}

void WebThreeConnection::doWrite()
{
	
}

bool WebThreeConnection::connectionOpen() const
{
	std::lock_guard<std::mutex> l(x_disconnectAt);
	if (m_disconnectAt != std::chrono::steady_clock::time_point::max())
		return false;
	return m_socket->is_open();
}

void WebThreeConnection::disconnect()
{
	{
		std::lock_guard<std::mutex> l(x_disconnectAt);
		if (m_disconnectAt == std::chrono::steady_clock::time_point::max())
			m_disconnectAt = std::chrono::steady_clock::now();
	}
	
	while (connectionOpen())
		usleep(200);
}

