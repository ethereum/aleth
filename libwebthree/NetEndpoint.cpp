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
/** @file NetEndpoint.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 */

#include "NetEndpoint.h"
#include "NetConnection.h"

using namespace std;
using namespace dev;

NetEndpoint::NetEndpoint(boost::asio::ip::tcp::endpoint _ep): Worker("endpoint"), m_io(), m_endpoint(_ep), m_acceptor(m_io)
{
}

NetEndpoint::~NetEndpoint()
{
	stop();
	for (auto c: m_connections)
		c.reset();
}

void NetEndpoint::start()
{
	try
	{
		m_acceptor.open(m_endpoint.protocol());
		m_acceptor.set_option(boost::asio::socket_base::reuse_address(true));
		m_acceptor.bind(m_endpoint);
		m_acceptor.listen();
	} catch(...) { }
	
	acceptConnection();
	startWorking();
}

void NetEndpoint::doWork()
{
	// TODO: cleanup connections and deregister service
	
	if (m_io.stopped())
		m_io.reset();
	m_io.poll();
}

void NetEndpoint::stop()
{
	// TODO: iterate and disconnect m_connections
	
	if (m_acceptor.is_open())
		m_acceptor.cancel();
	m_acceptor.close();
	m_io.stop();
	stopWorking();
	m_io.reset();
}

void NetEndpoint::acceptConnection()
{
	// pointer is used to prevent dealloc issues w/boost
	auto newConn = make_shared<NetConnection>(m_io);
	m_connections.push_back(newConn);
	
	auto self = shared_from_this();
	m_acceptor.async_accept(*newConn->socket(), [self, this, newConn](boost::system::error_code _ec)
	{
		if (_ec)
		{
			clog(RPCConnect) << "error accepting socket: " << _ec.message();
			return;
		}
		
		for (auto s: m_services)
			s.second->registerConnection(newConn);
		
		acceptConnection();
		newConn->start();
	});
}

