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
/** @file NetConnectHandler.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "NetConnectHandler.h"
#include "NetMsg.h"

using namespace std;
using namespace dev;

NetConnectHandler::NetConnectHandler(NetMsgServiceType _type, boost::asio::ip::tcp::endpoint _ep): m_serviceType(_type), m_localDataSequence(0), m_io(), m_connection(m_io, _ep, _type, nullptr, nullptr)
{
}

NetConnectHandler::~NetConnectHandler()
{
	m_io.stop();
	if (m_ioThread.joinable())
		m_ioThread.join();
}

void NetConnectHandler::run()
{
	m_ioThread = std::thread([&]()
	{
		m_io.run();
	});
}

void NetConnectHandler::send(NetMsgType _type, RLP const& _rlp)
{
	NetMsg msg(m_serviceType, nextDataSequence(), _type, _rlp);
	m_connection.send(msg);
}

NetMsgSequence NetConnectHandler::nextDataSequence()
{
	return m_localDataSequence++;
}

