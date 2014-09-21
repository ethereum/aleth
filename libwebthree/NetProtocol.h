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
/** @file NetProtocol.h
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 */

#pragma once

#include "NetCommon.h"

namespace dev
{
	
class NetMsg;
class NetConnection;
	
template <class T> // pass derived NetService class
class NetProtocol
{
public:
	NetProtocol(T* _service, NetConnection* _conn): m_service(_service), m_connection(_conn) {};
	
	virtual void connectionCreated() {};
	// todo: check headers
	virtual void receiveMessage(NetMsg _msg) = 0;
	virtual bool sendMessage(NetMsg _msg) { return false; };
	
	NetMsgSequence nextDataSequence() { return m_localDataSequence++; }
	
protected:
	NetMsgSequence nextServiceSequence() { return m_localServiceSequence++; }
	std::atomic<NetMsgSequence> m_localServiceSequence;
	std::atomic<NetMsgSequence> m_localDataSequence;

	std::atomic<NetMsgSequence> m_remoteServiceSequence;
	std::atomic<NetMsgSequence> m_remoteDataSequence;
	
	T* m_service;
	NetConnection* m_connection;
};

}

