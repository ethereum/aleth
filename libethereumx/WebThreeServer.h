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
/** @file WebThreeServer.h
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <boost/utility.hpp>

#include <libethereum/CommonNet.h>

namespace dev
{
namespace eth
{
class WebThreeConnection;
class WebThreeDirect;
class WebThreeMessage;
	
enum WebThreeServiceType
{
	Null, /* reserved */
	WebThreeService = 0x01,
	EthereumService,
	SwarmService,
	WhisperService
};

/**
 * @brief Server for interfacing between Ethereum systems (client, whisper, swarm).
 *
 * @todo: remove WebThreeDirect reference (responders can capture)
 * @todo: parameter for initializing acceptor by default
 * @todo: replace m_responders with something direct
 * @todo: handle null channel(for connection-specific messaging)
 * @todo: rename service sequence to notify sequence
 * @todo: document header, sequences, notification channels (0x00 packets are for notifying channels; header size is 4 instead of 3)
 * @todo: refactor for reflective interface instead of assigning WebThreeDirect (or make abstract)
 *
 */
class WebThreeServer
{
	friend class WebThreeMessage; // server verifies sequence
	
public:
	typedef std::function<RLP const&(RLP const&)> serviceResponder;
	
	/// Constructor. After this, everything should be set up to go.
	WebThreeServer(WebThreeDirect *_w3, bi::tcp::endpoint _ep = bi::tcp::endpoint(bi::address::from_string("127.0.0.1"), 30310)): m_w3(_w3), m_endpoint(_ep) {}
	
	/// Destructor.
	~WebThreeServer() { stopServer(); }
	
	/// Registers and starts a RPC service which listens for requests of the given _serviceId and passes the requests
	/// payload to _responder.
	/// @todo exception if responder already registered
	void registerService(WebThreeServiceType _serviceId, std::function<RLP const&(RLP const&)> _responder) { m_responders.insert(make_pair(_serviceId,_responder)); }
	
private:
	/// Start server.
	void startServer();
	
	/// Responds to requests and continuously dispatches socket acceptor until shutdown. Disptaches two processes; one which responds to requests and the second which runs io_service.
	void runServer();
	
	/// Stops server. Server sets m_shutdown to true and then joins network and responder threads.
	void stopServer();
	
	/// @returns if RLP message size is valid and matches length from 4-byte header
	bool checkPacket(bytesConstRef _msg) const;

	WebThreeDirect* m_w3;												///< WebThree instance passed to registered service handlers.
	bi::tcp::endpoint m_endpoint;										///< Default endpoint is 127.0.0.1:30310

	boost::asio::ip::tcp::acceptor *m_acceptor;						///< Socket acceptor for incoming client connections.
	
	std::map<WebThreeServiceType,serviceResponder> m_responders;		///< Services' responder methods.
	std::vector<std::shared_ptr<WebThreeConnection>> m_sessions;			///< Connected sessions.
	std::mutex x_sessions;											///< m_sessions mutex.
	
	std::atomic<uint32_t> m_pendingRequests;							///< @todo increment from session
	
	/// Setting true causes responder-process to set a deadline timer. Acceptor and read loops halt when deadline timer occurs.
	std::atomic<bool> m_shutdown;
};

/**
 * @brief Client for interfacing with a WebThree service via local RPC session.
 *
 * @todo Random sequence initialization. Track and validate service sequence.
 */
class WebThreeClient
{
	// constructor of egress requests call nextSequence() /* todo: constructor of ingress notification messages call setServiceSequence() */
	friend class WebThreeRequest;
	
public:
	WebThreeClient(WebThreeServiceType _type): m_serviceType(_type) {}
	~WebThreeClient();

	bytes request(int _type, RLP const& _request);
	
private:
	void send(WebThreeMessage* _msg);
	uint16_t nextSequence() { return m_clientSequence++; }
	
	WebThreeServiceType m_serviceType;
	std::atomic<uint16_t> m_clientSequence;
};

class WebThreeConnection
{
	friend class WebThreeServer;
	friend class WebThreeResponse;
	
public:
	WebThreeConnection(WebThreeServer::serviceResponder* _s, boost::asio::ip::tcp::socket *_sock): m_responder(_s), m_socket(_sock) { start(); }
	WebThreeConnection() { /* sleep until disconnect */ delete m_socket; }

private:
	void send(WebThreeMessage& _msg);
	
	/// Called when message is received. Message is passed to designated serviceResponder.
	void messageReceived(WebThreeMessage& _msg);
	
	/// @returns if RLP message size matches length from 4-byte header and is valid. Called by payload.
	bool checkPacket(bytesConstRef _netMsg) const;
	
	void start() { /* register w/server(?) */ doRead(); handshake(); };
	void handshake() { if (/* failure */ false) disconnect(); };
	void doRead();
	
	/// Schedules write; attempts to send immediately if no writes are pending.
	void write(bytes& _msg);
	void doWrite();

	
	// Networking:
	bool connectionOpen() const
	{
		std::lock_guard<std::mutex> l(x_disconnectAt);
		if (m_disconnectAt != std::chrono::steady_clock::time_point::max())
			return false;
		return m_socket->is_open();
	}
	
	void disconnect()
	{
		{
			std::lock_guard<std::mutex> l(x_disconnectAt);
			if (m_disconnectAt == std::chrono::steady_clock::time_point::max())
				m_disconnectAt = std::chrono::steady_clock::now();
		}
		
		while (connectionOpen())
			usleep(200);
	}
	
	WebThreeServer::serviceResponder* m_responder;
	boost::asio::ip::tcp::socket *m_socket;

	std::chrono::steady_clock::time_point m_disconnectAt;
	mutable std::mutex x_disconnectAt;
};

/**
 * @brief Used by WebThreeClient to send requests; message is immediately sent.
 * @todo implement send
 * @todo (notes) received on wire; sequence is parsed from RLP
 * @todo check RLP
 * @todo: rename sequence to serviceSequence, implement dataSequence
 */
class WebThreeMessage
{
	friend class WebThreeServer;
	friend class WebThreeConnection;
	friend class WebThreeRequest;
	friend class WebThreeResponse;
	
	// Ingress constructor
	WebThreeMessage(bytes& _packetData): messageBytes(std::move(_packetData)) {}
	// Egress constructor
	WebThreeMessage(uint16_t _seq, RLP const& _req): sequence(_seq), messageBytes(std::move(_req.data().toBytes())) {}
	~WebThreeMessage();

	/// @returns if payload size matches length from 4-byte header, size of RLP is valid, and sequenceId is valid
	bool check(bytesConstRef _netMsg);
	/// @returns RLP message.
	bytes payload(bytesConstRef _netMsg);
	
	WebThreeServiceType service;
	uint16_t sequence;			///< Message sequence. Initial value is random and chosen by client.
	int requestType;				///< Message type; omitted from header if 0.
	bytes const& messageBytes;	///< RLP Bytes of the message.
};

class WebThreeRequest;
/**
 * @brief Private class called by WebThreeRequest::respond() which is typically called from a WebThreeConnection::serviceResponder. (message is immediately sent)
 * @todo move implementation into cpp and remove cast from webthreeresponse
 * @todo check RLP
 */
class WebThreeResponse: public WebThreeMessage
{
	friend class WebThreeServer;
	friend class WebThreeRequest;
	WebThreeResponse(WebThreeConnection *_s, WebThreeRequest *_request, RLP const& _response): WebThreeMessage(((WebThreeMessage*)_request)->sequence, _response) { _s->send(*this); }
};
	
/**
 * @brief Private class used by WebThreeClient to send requests and by WebThree. (messages are sent immediately)
 * @todo move implementation into cpp and remove cast from webthreeresponse
 * @todo (notes) received on wire; sequence is parsed from RLP
 * @todo check RLP
 */
class WebThreeRequest: public WebThreeMessage
{
	friend class WebThreeClient;
	WebThreeRequest(WebThreeClient *_c, RLP const& _request): WebThreeMessage(_c->nextSequence(), _request) { _c->send(this); };
	void respond(WebThreeConnection *_s, RLP const& _response) { WebThreeResponse(_s, this, _response); } ;
};

//class WebThreeNotification: public WebThreeMessage; // @todo: implementation requires serviceSequence tracking


}
}
