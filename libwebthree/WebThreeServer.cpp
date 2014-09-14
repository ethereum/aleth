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
/** @file WebThreeServer.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 * 
 * @todo Implement RPC log channels
 */

#include "WebThreeServer.h"
#include "RLPMessage.h"
#include "RLPConnection.h"

using namespace std;
using namespace dev;
namespace ba = boost::asio;
namespace bi = boost::asio::ip;

WebThreeServer::WebThreeServer(bi::tcp::endpoint _ep): m_endpoint(_ep), m_io(), m_acceptor(m_io, m_endpoint)
{
	m_stopped = true;
	startServer();
}

WebThreeServer::~WebThreeServer()
{
	stopServer();
}

void WebThreeServer::setMessageHandler(WebThreeServiceType _serviceId, messageHandler _responder)
{
	lock_guard<mutex> l(x_responders);
	if (!m_responders.count(_serviceId))
		m_responders.insert(make_pair(_serviceId,make_shared<messageHandler>(_responder)));
}

void WebThreeServer::startServer()
{
	{
		lock_guard<mutex> l(x_stopped);
		if (!m_stopped)
			return;
		m_stopped = false;
	}

	m_ioThread = std::thread([&]()
	{
		// if server is restarting acceptor must be reopened
		if (!m_acceptor.is_open())
		{
			m_acceptor.open(m_endpoint.protocol());
			m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
			m_acceptor.bind(m_endpoint);
			m_acceptor.listen();
		}

		doAccept();
		
		m_io.run();
		m_acceptor.close();
	});
}

void WebThreeServer::refreshHandlers()
{
	if (m_stopped)
		return;
	
	for (auto r: m_responders)
	{
		
	}
}

void WebThreeServer::stopServer()
{
	{
		lock_guard<mutex> l(x_stopped);
		if (m_stopped)
			return;
		m_stopped = true;
	
		lock_guard<mutex> ls(x_connections);
		for (auto s: m_connections)
			if (auto p = s.get())
				p->shutdown();
	}
	
	// stop IO service, causing ioThread to end
	m_io.stop();
	
	// With clients stopped, it's safe to stop asio polling thread
	if (m_ioThread.joinable())
		m_ioThread.join();
}

void WebThreeServer::doAccept()
{
	shared_ptr<RLPConnection> conn(new RLPConnection(m_io));
	m_acceptor.async_accept(conn->m_socket, [this, conn](boost::system::error_code _ec)
	{
		clog(RPCConnect) << "WebThreeServer::doAccept new connection";
		
		if (!_ec)
		{
			{
				lock_guard<mutex> l(x_connections);
				if (!m_stopped)
					m_connections.push_back(conn);
			}
			conn->handshake();
		}

		// TODO fixme: handle _ec (restart or exception/crash if appropriate)
		if (_ec.value() != 1)
			doAccept();
		else
			clog(RPCNote) << "WebThreeServer::doAccept going awawy";
	});
}




