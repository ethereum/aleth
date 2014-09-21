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

NetEndpoint::NetEndpoint(boost::asio::ip::tcp::endpoint _ep): m_io(), m_endpoint(_ep), m_acceptor(m_io, m_endpoint)
{
}

NetEndpoint::~NetEndpoint()
{
	stop();
}

void NetEndpoint::start()
{
	acceptConnection();
	startWorking();
}

void NetEndpoint::doWork()
{
	if (m_io.stopped())
		m_io.reset();
	m_io.run();
}

void NetEndpoint::stop()
{
	m_acceptor.close();
	m_io.stop();
	stopWorking();
	m_io.reset();
}

void NetEndpoint::acceptConnection()
{
	// todo: instantiate connection with handler/bind [](){ checkService->m_services[c.service()]->receivedMessage(c,_msg); }
	// socket pointer is used to prevent dealloc issues w/boost
	auto newConn = make_shared<NetConnection>(m_io, m_endpoint);
	m_connections.push_back(newConn);
	
	auto self = shared_from_this();
	m_acceptor.async_accept(*newConn->socket(), [self, this, newConn](boost::system::error_code _ec)
	{
		acceptConnection();

		if (!_ec)
			newConn->start();
		else
			clog(RPCConnect) << "error accepting socket: " << _ec.message();
	});
}

