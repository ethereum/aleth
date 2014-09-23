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

#include "NetService.h"
#include "NetConnection.h"
#include "NetMsg.h"
#include "NetCommon.h"

namespace dev
{

class NetMsg;

/**
 * @brief NetProtocol objects are instantiated directly or by NetService in order to handle network messages and session state.
 */
class NetProtocol
{
public:
	NetProtocol(NetConnection* _conn): m_connection(_conn) {};
	NetProtocol(NetConnection* _conn, NetServiceFace* _service): m_connection(_conn) {};
	
	virtual void receiveMessage(NetMsg _msg) = 0;
	
	NetMsgSequence nextDataSequence() { return m_localDataSequence++; }
	NetConnection* connection() { return m_connection; }
	
protected:
	NetMsgSequence nextSessionSequence() { return m_localSessionSequence++; }
	
	// Session sequences are used for wire-level
	std::atomic<NetMsgSequence> m_localSessionSequence;
	std::atomic<NetMsgSequence> m_localDataSequence;

	std::atomic<NetMsgSequence> m_remoteSessionSequence;
	std::atomic<NetMsgSequence> m_remoteDataSequence;
	
	NetConnection* m_connection;
};

}

