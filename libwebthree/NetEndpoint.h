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
/** @file NetEndpoint.h
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 */

#pragma once

#include <boost/asio.hpp>
#include <libdevcore/Worker.h>
#include "NetService.h"
#include "NetCommon.h"

namespace dev
{
	
class NetConnection;
	
/**
 * @brief Endpoint for handling RLP-encapsulated network messages; messages are passed to NetServiceType::receivedMessage() based on the NetMsgServiceType of the message. NetEndpoint will ensure that connection and message objects are retained when they're passed to service.
 */
class NetEndpoint: public std::enable_shared_from_this<NetEndpoint>, public Worker
{
public:
	NetEndpoint(boost::asio::ip::tcp::endpoint _ep);
	~NetEndpoint();

	std::shared_ptr<NetServiceType> registerService(NetServiceType* _s) { auto ret = std::shared_ptr<NetServiceType>(_s); m_services[_s->service()] = ret; return ret; }
	
	void start();
	void stop();
	
	// todo: connect(NetHost);

protected:
	std::map<NetMsgServiceType, std::shared_ptr<NetServiceType>> m_services;
	std::vector<std::shared_ptr<NetConnection> > m_connections;
	
private:
	/// Run IO Service
	void doWork();
	
	/// Initially called when started and called recursively via asio block when new connection is accepted.
	void acceptConnection();
	
	boost::asio::io_service m_io;
	boost::asio::ip::tcp::endpoint m_endpoint;
	boost::asio::ip::tcp::acceptor m_acceptor;
	std::thread m_ioThread;
};

}

