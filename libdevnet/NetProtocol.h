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
 * @brief NetProtocol objects are instantiated directly or by NetService in 
 * order to handle network messages and session state. NetProtocol by default will
 * ignore the service parameter, as the endpoint will always pass a service; this is subject to be removed in future implementation where endpoint will check if protocol hasService().
 *
 * @todo separate service-dependant protocols from indepenent protocols (ex: rpc client doesn't need service/endpoint)
 * @todo Implement (virtual) receiveMessage directly to verify sequencing; pass message to pure virtual interpretMessage.
 */
class NetProtocol
{
public:
	NetProtocol(NetConnection* _conn): m_connection(_conn) {};
	NetProtocol(NetConnection* _conn, NetServiceFace* _service): m_connection(_conn) {};
	
	/// Handlers for messages received by connections.
	virtual void receiveMessage(NetMsg const& _msg) = 0;
	
	/// Atomically increment sequences for outgoing messages.
	NetMsgSequence nextDataSequence() { return m_localDataSequence++; }
	
	/// Called from service implementations for broadcasts or connection management.
	NetConnection* connection() const { return m_connection; }
	
protected:
	NetMsgSequence nextSessionSequence() { return m_localSessionSequence++; }
	
	std::atomic<NetMsgSequence> m_localSessionSequence;	///< Session sequences are used for service messages
	std::atomic<NetMsgSequence> m_localDataSequence;		///< Data sequences are used for data messages

	// std::atomic<NetMsgSequence> m_remoteSessionSequence;
	// std::atomic<NetMsgSequence> m_remoteDataSequence;
	
	NetConnection* m_connection;							///< Corresponding connection.
};

/**
 * @brief NetServiceProtocol utilizes additional service parameter to provide the
 * getService() accessor for an implementation to access a shared service.
 */
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
 * @brief Template for implement RPC client/server protocol. Currently 
 * implements client interface which generates a sequence for every 
 * request and then waits for a response with a matching sequence. 
 * Although the user-request is blocking, the network protocol itself is 
 * asynchronous so that requests can be made concurrently.
 *
 * @TODO Pass flag and/or create template for server interface (reflects sequence id).
 * @todo handle/encapsulate exceptions
 */
template <class T>
class NetRPCClientProtocol: public NetProtocol
{
public:
	/// Constructor. Sets data message handler of connection to receiveMessage.
	NetRPCClientProtocol(NetConnection* _conn);
	
	/// Constructor. Sets data message handler of connection to receiveMessage.
	NetRPCClientProtocol(NetConnection* _conn, NetServiceFace* _service): NetRPCClientProtocol(_conn) {};

	static NetMsgServiceType serviceId() { return T::serviceId(); };
	
	/// Messages received by RPC client are checked against promises list via sequence id; if present, corresponding promise value will be set to msg.
	void receiveMessage(NetMsg const& _msg)
	{
		clog(RPCNote) << "[" << serviceId() << "] receiveMessage";
		if (auto p = m_promises[_msg.sequence()])
			if (_msg.type() == Success)
				p->set_value(std::make_shared<NetMsg>(_msg));
//			else if (_msg.type() == 2) // exception
//			else // malformed
	}

	/// Calls performRequest(NetMsgType, RLPStream&) with stream of empty list.
	bytes performRequest(NetMsgType _type);
	
	/// Sends request type and blocks until response is received or timeout.
	bytes performRequest(NetMsgType _type, RLPStream& _s);

protected:
	typedef std::promise<std::shared_ptr<NetMsg>> promiseResponse;										///< NetMsg promise (added to m_promises and used by receiveMessage)
	typedef std::future<std::shared_ptr<NetMsg>> futureResponse;										///< NetMsg future (used to wait for value set by receiveMessage)
	std::map<NetMsgSequence,promiseResponse*> m_promises;										///< List of promised (expected) responses from server.
	std::mutex x_promises;										///< Mutex concurrent access to m_promises.
};

template <class T>
NetRPCClientProtocol<T>::NetRPCClientProtocol(NetConnection* _conn):
	NetProtocol(_conn)
{
	_conn->setDataMessageHandler(serviceId(), [=](NetMsg const& _msg)
	{
		receiveMessage(_msg);
	});
}
	
template <class T>
bytes NetRPCClientProtocol<T>::performRequest(NetMsgType _type, RLPStream& _s)
{
	promiseResponse p;
	futureResponse f = p.get_future();
	
	NetMsg msg(serviceId(), nextDataSequence(), _type, RLP(_s.out()));
	{
		std::lock_guard<std::mutex> l(x_promises);
		m_promises.insert(make_pair(msg.sequence(),&p));
	}
	connection()->send(msg);
	
	auto s = f.wait_until(std::chrono::steady_clock::now() + std::chrono::seconds(connection()->connectionOpen() ? 2 : 10));
	
	{
		std::lock_guard<std::mutex> l(x_promises);
		m_promises.erase(msg.sequence());
	}
	
	if (s != std::future_status::ready)
		throw RPCRequestTimeout();
	
	return std::move(f.get()->rlp());
}

template <class T>
bytes NetRPCClientProtocol<T>::performRequest(NetMsgType _type	)
{
	RLPStream s(0);
	return performRequest(_type, s);
}

}

