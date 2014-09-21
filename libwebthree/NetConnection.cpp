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
/** @file NetConnection.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include <memory>
#include "NetConnection.h"
#include "NetMsg.h"

using namespace std;
using namespace dev;

NetConnection::NetConnection(boost::asio::io_service& _io_service, boost::asio::ip::tcp::endpoint _ep): m_socket(_io_service), m_endpoint(_ep), m_stopped(true), m_originateConnection(false), m_socketError()
{
	m_stopped = true;
	m_started = false;
}

NetConnection::NetConnection(boost::asio::io_service& _io_service, boost::asio::ip::tcp::endpoint _ep, NetMsgServiceType _svc, messageHandler* _svcMsgHandler, messageHandler* _dataMsgHandler): NetConnection(_io_service, _ep, messageHandlers({make_pair(_svc,(_svcMsgHandler) ? make_shared<messageHandler>(*_svcMsgHandler) : nullptr)}), messageHandlers({make_pair(_svc,(_dataMsgHandler) ? make_shared<messageHandler>(*_dataMsgHandler) : nullptr)}))
{
	m_stopped = true;
	m_started = false;
}

NetConnection::NetConnection(boost::asio::io_service& _io_service, boost::asio::ip::tcp::endpoint _ep, messageHandlers _svcMsgHandlers, messageHandlers _dataMsgHandlers): m_socket(_io_service), m_endpoint(_ep), m_serviceMsgHandlers(_svcMsgHandlers), m_dataMsgHandlers(_dataMsgHandlers), m_originateConnection(true), m_stopped(true), m_socketError(), m_started(false)
{
	m_stopped = true;
	m_started = false;
}

NetConnection::~NetConnection()
{
	shutdown(true);
}

boost::asio::ip::tcp::socket* NetConnection::socket() {
	return &m_socket;
}

void NetConnection::start()
{
	assert(m_started.load() == false);
	bool no = false;
	if (!m_started.compare_exchange_strong(no, true))
		return;
	
	if (!m_originateConnection)
		handshake();
	else
	{
		clog(RPCConnect) << (void*)this << (m_originateConnection ? "[egress]" : "[ingress]") << "start()";
		
		auto self(shared_from_this());
		m_socket.async_connect(m_endpoint, [self, this](boost::system::error_code const& _ec)
		{
			if (_ec)
				// todo: retry times 3 w/250ms sleep
				shutdownWithError(_ec); // Connection failed
			else
				handshake();
		});
	}
}

void NetConnection::send(NetMsg const& _msg)
{
	if (m_stopped)
		return;
	
	auto self(shared_from_this());
	boost::asio::async_write(m_socket, boost::asio::buffer(_msg.payload()), [self](boost::system::error_code _ec, size_t _len)
	{
		if (_ec)
			return self->shutdownWithError(_ec); // send write failed
		
		assert(_len);
		clog(RPCNote) << "NetConnection::send";
	});
}

void NetConnection::doWrite()
{
	// implement write queue w/send()
}

//bool NetConnection::checkPayloadLength(bytesConstRef _msg) const
//{
//	if (_msg.size() < 4)
//		return false;
//	uint32_t len = ((_msg[0] * 256 + _msg[1]) * 256 + _msg[2]) * 256 + _msg[3];
//	if (_msg.size() != len + 4)
//		return false;
//	RLP r(_msg.cropped(4));
//	if (r.actualSize() != len)
//		return false;
//	return true;
//}

void NetConnection::handshake(size_t _rlpLen)
{
	// read header
	if (m_recvdBytes == 0)
	{
		clog(RPCConnect) << (void*)this << (m_originateConnection ? "[egress]" : "[ingress]") << "handshake() started";
		
		// write version packet
		RLPStream s;
		s << "version";
		shared_ptr<NetMsg> version(new NetMsg((NetMsgServiceType)0, 0, (NetMsgType)0, RLP(s.out())));
	
		// as a special case, message is manually written as
		// read/write isn't setup until connection is verified
		auto self(shared_from_this());
		boost::asio::async_write(m_socket, boost::asio::buffer(version->payload()), [this, self, version](boost::system::error_code _ec, size_t _len)
		{
			if (_ec)
				return shutdownWithError(_ec); // handshake write failed

			assert(_len);
			
			// read length header
			m_recvBuffer.resize(4);
			boost::asio::async_read(m_socket, boost::asio::buffer(m_recvBuffer), [this, self](boost::system::error_code _ec, size_t _len)
			{
				if (_ec)
					return shutdownWithError(_ec); // handshake read-header failed
				if (_len == 0)
					return shutdownWithError(boost::asio::error::connection_reset); // todo: reread
				
				assert(_len == 4);
				
				m_recvdBytes += _len;
				size_t rlpLen = fromBigEndian<uint32_t>(bytesConstRef(m_recvBuffer.data(), 4));
				if (rlpLen > 64*1024)
					return shutdownWithError(boost::asio::error::connection_reset); // throw MessageTooLarge();
				if (rlpLen < 3)
					return shutdownWithError(boost::asio::error::connection_reset); // throw MessageTooSmall();
				m_recvBuffer.resize(2 * (rlpLen + 4));
				assert(rlpLen > 0);
				assert(rlpLen < 14);
				handshake(rlpLen);
			});
		});
		
		return;
	}
	
	assert(_rlpLen > 0);
	auto self(shared_from_this());
	m_socket.async_read_some(boost::asio::buffer(boost::asio::buffer(m_recvBuffer) + m_recvdBytes), [this, self, _rlpLen](boost::system::error_code _ec, size_t _len)
	{
		if (_ec)
			return shutdownWithError(_ec); // handshake read-data failed
		
		m_recvdBytes += _len;
		if (m_recvdBytes >= _rlpLen)
		{
			try
			{
				NetMsg msg(bytesConstRef(m_recvBuffer.data(), _rlpLen + 4));
				if (RLP(msg.rlp()).toString() == "version")
				{
					// It's possible for handshake to finish following shutdown
					lock_guard<mutex> l(x_socketError);
					if (m_started)
						m_stopped = false;
					else
						return;
					
					clog(RPCNote) << (m_originateConnection ? "[egress]" : "[ingress]") << "Version verified!";
				}
				else
					clog(RPCNote) << (m_originateConnection ? "[egress]" : "[ingress]") << "Version Failed! ";
			}
			catch (std::exception const& _e)
			{
				shutdownWithError(boost::asio::error::connection_reset); // handshake failed negotiation
				return;
			}
			
			m_recvdBytes = m_recvdBytes - _rlpLen - 4;
			if (m_recvdBytes)
				memmove(m_recvBuffer.data(), m_recvBuffer.data() + (_rlpLen + 4 - 1), m_recvdBytes);
			
			doRead();
			return;
		}

		assert(_rlpLen > 0);
		assert(_rlpLen < 14);
		handshake(_rlpLen);
	});
}

void NetConnection::doRead(size_t _rlpLen)
{
	// Previously, shutdown wouldn't occur if _rlpLen was > 0, however with streaming protocols it's possible remote closes connection before writing a full response. Instead shutdown will defer until writes are complete (see send()).
	if (m_stopped)
		// if m_stopped is true it is certain shutdown already occurred
		return closeSocket();
	
	if (!_rlpLen && m_recvdBytes > 3)
	{
		// Read buffer if there's enough data for header
		
		clog(RPCNote) << "NetConnection::doRead length header";
		_rlpLen = fromBigEndian<uint32_t>(bytesConstRef(m_recvBuffer.data(), 4));
		if (_rlpLen > 16*1024*1024)
			// should be 16kB; larger messages to-be-chunk/streamed
			return shutdownWithError(boost::asio::error::connection_reset);
		if (_rlpLen < 3)
			return shutdownWithError(boost::asio::error::connection_reset);
		
		if (4+2*_rlpLen > m_recvBuffer.size())
			m_recvBuffer.resize(4+2*_rlpLen);
	}
	
	if (_rlpLen && m_recvdBytes >= _rlpLen)
	{
		// Handle message if data recvd > payload size
		
		try
		{
			clog(RPCNote) << "NetConnection::doRead message";
			NetMsg msg(bytesConstRef(m_recvBuffer.data(), _rlpLen));
			if (msg.service())
			{
				messageHandlers &hs = msg.type() ? m_dataMsgHandlers : m_serviceMsgHandlers;
				
				// If handler is found, get pointer and call
				auto hit = hs.find(msg.service());
				if (hit!=hs.end())
				{
					auto h = *hit->second.get();
					if (h!=nullptr)
						h(msg);
				}
				else
					throw MessageServiceInvalid();
			}
			// else, control messages (ignored right now)
		}
		catch (std::exception const& _e)
		{
			// bad message
			shutdownWithError(boost::asio::error::connection_reset); // MessageInvalid
			return;
		}
		
		m_recvdBytes = m_recvdBytes - _rlpLen - 4;
		if (m_recvdBytes)
			memmove(m_recvBuffer.data(), m_recvBuffer.data() + _rlpLen + 4,m_recvdBytes);
		
		doRead();
	}
	else
	{
		// otherwise read more data
		
		auto self(shared_from_this());
		m_socket.async_read_some(boost::asio::buffer(boost::asio::buffer(m_recvBuffer) + m_recvdBytes), [this, self, _rlpLen](boost::system::error_code _ec, size_t _len)
		{
			if (_ec)
				return shutdownWithError(_ec); // read header failed
			
			assert(_len);
			
			m_recvdBytes += _len;
			doRead(_rlpLen);
		});
	}
}

bool NetConnection::connectionError() const
{
	// Note: use of mutex here has issues
	return m_socketError!=0;
}

bool NetConnection::connectionOpen() const
{
	return m_stopped ? false : m_socket.is_open();
}

void NetConnection::shutdownWithError(boost::system::error_code _ec)
{
	// If !started and already stopped, shutdown has already occured. (EOF or Operation canceled)
	if (!m_started && m_stopped)
		return;
	
	assert(_ec);
	{
		lock_guard<mutex> l(x_socketError);
		if (m_socketError != boost::system::error_code())
			return;
		m_socketError = _ec;
	}
	
	clog(RPCWarn) << (void*)this << "Socket shutdown due to error: " << _ec.message();
	closeSocket();
	shutdown(false);
}

void NetConnection::shutdown(bool _wait)
{
	{
		lock_guard<mutex> l(x_socketError);
		bool yes = true;
		if (!m_started.compare_exchange_strong(yes, false))
			return;
		
		// if socket never left stopped-state (pre-handshake)
		if (m_stopped)
			return closeSocket();
		else
			m_stopped = true;
		
		// immediately close reads
		// TODO: (if no error) wait for pending writes
		closeSocket();
	}
	
	// when m_stopped is true, doRead will close socket
	if (_wait)
		while (m_socket.is_open() && !connectionError())
			usleep(200);
	
	m_recvBuffer.resize(0);
}

void NetConnection::closeSocket()
{
	if (m_socket.is_open())
	{
		boost::system::error_code ec;
		m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		m_socket.close();
	}
}

