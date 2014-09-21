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
/** @file NetService.h
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 */

#pragma once

#include "NetMsg.h"

namespace dev
{

class NetConnection;
class NetEndpoint;
	
struct NetServiceType
{
	virtual NetMsgServiceType service() = 0;
};

/**
 * @brief NetService class is registered with an endpoint and upon doing so will be passed messages destined for it's service id.
 *
 * NetMsgServiceType is used by Endpoint to map/match service of ingress messages; N is NetProtocol and is coupled to connections when connectionReady() is called (after handshake).
 *
 *	periodically call services to clean deallocated weak ptrs (maybe via endpoint)
 *
 */
template <NetMsgServiceType T, class N>
class NetService: public NetServiceType, public std::enable_shared_from_this<N>
{
	friend class NetEndpoint;
	
public:
	virtual NetMsgServiceType service() { return T; }

protected:
	void connectionReady(std::shared_ptr<NetConnection> _conn) { m_connState.insert(std::make_pair(_conn,N(this))); }
	
	void receivedMessage(std::shared_ptr<NetConnection> _conn, NetMsg _msg) { m_connState[_conn]->receiveMessage(_msg); }

	std::map<std::weak_ptr<NetConnection>,N> m_connState;
};

}

