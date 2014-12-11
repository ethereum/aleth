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
 * @brief Endpoint for handling network connections. Connections are passed
 * to registerConnection functions of each registered service. NetEndpoint 
 * will ensure that connection and message objects are retained when they're 
 * in use.
 *
 * NetEndpoint is responsible for the life of a connection. This enables 
 * service-based connections to ensure operations and maintain state 
 * information in a reliable manner. This also facilitates (one day...) 
 * robust and graceful shutdown of connections for any 
 * start/stop/deallocation/exception scenario.
 *
 * @todo add create-connection method, then remove get_io_service()
 */
class NetEndpoint: public Worker, public std::enable_shared_from_this<NetEndpoint>
{
public:
	NetEndpoint(boost::asio::ip::tcp::endpoint _ep);
	~NetEndpoint();

	/// Adds a service to services map (serviceId:service). New connections are registered with services prior to starting I/O.
	void registerService(NetServiceFace* _s) { m_services[_s->serviceId()] = _s; }
	
	/// Start accepting connections and running worker thread.
	void start();
	
	/// Stop worker thread and accepting connections.
	void stop();

	/// Called by external connections to make shared-use of io service.
	boost::asio::io_service& get_io_service() { return m_io; }
	
protected:
	std::map<NetMsgServiceType, NetServiceFace*> m_services;		///< Services map (see registerService)
	std::vector<std::shared_ptr<NetConnection>> m_connections;	///< List of connections.
	
private:
	/// Runs IO Service.
	void doWork();
	
	/// Initially called when started and called continuously from asio block after new connection is accepted.
	void acceptConnection();
	
	boost::asio::io_service m_io;								///< IO service run by doWork().
	boost::asio::ip::tcp::endpoint m_endpoint;	///< Endpoint where connections are being accepted.
	boost::asio::ip::tcp::acceptor m_acceptor;	///< Connection acceptor.						
};

}

