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
/** @file NetHandler.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <thread>
#include <libdevcore/Worker.h>
#include "NetConnection.h"
#include "NetCommon.h"

namespace dev
{
	
class NetConnection;
	
/**
 * @brief Interface for handling Network service connections.
 * @todo stop/start and constructing with disconnected accpetor.
 */
class NetHandler: public std::enable_shared_from_this<NetHandler>, public Worker
{
public:
	NetHandler(boost::asio::ip::tcp::endpoint _ep);
	~NetHandler();

//	virtual static std::string name() const = 0;
//	virtual void receivedServiceMessage(NetMsg _msg) = 0;
//	virtual void receivedDataMessage(NetMsg _msg) = 0;
//	virtual void connectionReady(std::weak_ptr<NetConnection> _conn) = 0;
	
private:
	/// Run IO Service
	void doWork();
	
	/// Initially called when started and called recursively via asio block when new connection is accepted.
	void acceptConnection();
	
//	/// Get next sequence for service messages.
//	NetMsgSequence nextServiceSequence();
	
	/// Get next sequence for data messages.
	NetMsgSequence nextDataSequence();

//	/// Sequence usage is utilized by message implementations (for now)
//	todo: integrate w/connection (or map to peer)
//	std::atomic<NetMsgSequence> m_localServiceSequence;
	std::atomic<NetMsgSequence> m_localDataSequence;
//	std::atomic<NetMsgSequence> m_remoteServiceSequence;
//	std::atomic<NetMsgSequence> m_remoteDataSequence;
	
	boost::asio::io_service m_io;
	boost::asio::ip::tcp::endpoint m_endpoint;
	boost::asio::ip::tcp::acceptor m_acceptor;
	std::thread m_ioThread;
	
	std::vector<std::shared_ptr<NetConnection> > m_connections;

	std::atomic<bool> m_stopped;
	std::mutex x_stopped;
};
	
}

