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

#include <future>
#include "NetService.h"

namespace dev
{

// tood: namespacing
enum RPCResponseMsgType : NetMsgType
{
	Success = 0x01
};
	
class NetMsg;

/**
 * @brief NetProtocol objects are instantiated directly or by NetService in order to handle network messages and session state.
 * @todo separate service-dependant protocols from indepenent protocols (ex: rpc clients typically do not need a service/endpoint)
 * @todo Implement (virtual) receiveMessage directly to verify sequencing; pass message to pure virtual interpretMessage.
 */
class NetProtocol
{
public:
	NetProtocol(NetConnection* _conn): m_connection(_conn) {};
	NetProtocol(NetConnection* _conn, NetServiceFace* _service): m_connection(_conn) {};

	virtual void receiveMessage(NetMsg const& _msg) = 0;
	
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
	
template <class T>
class NetServiceProtocol: public NetProtocol
{
public:
	NetServiceProtocol(NetConnection* _conn, NetServiceFace* _service): NetProtocol(_conn), m_service(static_cast<T*>(_service)) {};
	
	virtual void receiveMessage(NetMsg const& _msg) = 0;
	
	T* getService() { return m_service; }

protected:
	T* m_service;
};


/**
 * @brief Template for implement RPC client/server protocol. Currently implements client interface which generates a sequence for every request and then waits for a response with a matching sequence. Although the user-request is blocking, the network protocol itself is asynchronous so that requests can be made concurrently.
 * @TODO Pass flag and/or create template for server interface (reflects sequence id).
 */
template <class T>
class NetRPCProtocol: public NetProtocol
{
public:
	NetRPCProtocol(NetConnection* _conn, NetServiceFace* _service): NetProtocol(_conn), m_rpcService(static_cast<T*>(_service)) {};

	void receiveMessage(NetMsg const& _msg) final
	{
		switch (_msg.type())
		{
			case Success:
				// second item is result
				if (auto p = m_promises[_msg.sequence()])
					p->set_value(std::make_shared<NetMsg>(_msg));
				break;
				
			case 2:
				// exception
				break;
		}
	}
	
	bytes performRequest(NetMsgType _type)
	{
		RLPStream s(0);
		return performRequest(_type, s);
	}
	
	bytes performRequest(NetMsgType _type, RLPStream& _s)
	{
		promiseResponse p;
		futureResponse f = p.get_future();
		
		NetMsg msg(T::serviceId(), nextDataSequence(), _type, RLP(_s.out()));
		{
			std::lock_guard<std::mutex> l(x_promises);
			m_promises.insert(make_pair(msg.sequence(),&p));
		}
		connection()->send(msg);

		auto s = f.wait_until(std::chrono::steady_clock::now() + std::chrono::seconds(2));
		
		{
			std::lock_guard<std::mutex> l(x_promises);
			m_promises.erase(msg.sequence());
		}
		
		if (s != std::future_status::ready)
			throw RPCRequestTimeout();
		
		return std::move(f.get()->rlp());
	}
	
	T* getRpcService() { return m_rpcService; }

protected:
	T* m_rpcService;
	
	typedef std::promise<std::shared_ptr<NetMsg> > promiseResponse;
	typedef std::future<std::shared_ptr<NetMsg> > futureResponse;
	std::map<NetMsgSequence,promiseResponse*> m_promises;
	std::mutex x_promises;										///< Mutex concurrent access to m_promises.
};

}

