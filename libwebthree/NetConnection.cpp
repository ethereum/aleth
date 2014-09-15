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

NetConnection::NetConnection(boost::asio::io_service& _io_service, boost::asio::ip::tcp::endpoint _ep): m_socket(_io_service), m_endpoint(_ep), m_stopped(true), m_originateConnection(false)
{
}

NetConnection::NetConnection(boost::asio::io_service& _io_service, boost::asio::ip::tcp::endpoint _ep, NetMsgServiceType _svc, messageHandler* _svcMsgHandler, messageHandler* _dataMsgHandler): NetConnection(_io_service, _ep, messageHandlers({make_pair(_svc,(_svcMsgHandler) ? make_shared<messageHandler>(*_svcMsgHandler) : nullptr)}), messageHandlers({make_pair(_svc,(_dataMsgHandler) ? make_shared<messageHandler>(*_dataMsgHandler) : nullptr)}))
{
}

NetConnection::NetConnection(boost::asio::io_service& _io_service, boost::asio::ip::tcp::endpoint _ep, messageHandlers _svcMsgHandlers, messageHandlers _dataMsgHandlers): m_socket(_io_service), m_endpoint(_ep), m_stopped(true), m_serviceMsgHandlers(_svcMsgHandlers), m_dataMsgHandlers(_dataMsgHandlers), m_originateConnection(true)
{
}

NetConnection::~NetConnection()
{
	shutdown(false);
}

boost::asio::ip::tcp::socket& NetConnection::socket() {
	return m_socket;
}

void NetConnection::start()
{
	bool no = false;
	if (m_started.compare_exchange_strong(no, true))
	{
		if (m_originateConnection)
		{
			auto self(shared_from_this());
			m_socket.async_connect(m_endpoint, [self, this](boost::system::error_code const& _ec)
			{
				if (_ec)
					closeSocket();
				else
					handshake();
			});
		}
		else
			handshake();
	}
}

//void NetConnection::send(NetMsgServiceType _svc, NetMsgSequence _seq, NetMsgType _type, RLP const& _msg)
//{
//	while (!m_stopped && !m_socket.is_open())
//		usleep(200);
//	
//	shared_ptr<NetMsg> msg(new NetMsg(_svc, _seq, _type, _msg));
//	boost::asio::async_write(m_socket, boost::asio::buffer(msg->m_messageBytes), [msg](boost::system::error_code _ec, size_t _len){
//		if (_ec)
//				clog(RPCNote) << "NetConnection::send error";
//			else
//				clog(RPCNote) << "NetConnection::send";
//	});
//}

void NetConnection::send(NetMsg& _msg)
{
	if (m_stopped)
		return;
	
	boost::asio::async_write(m_socket, boost::asio::buffer(_msg.payload()), [](boost::system::error_code _ec, size_t _len){
		if (_ec)
				clog(RPCNote) << "NetConnection::send error";
			else
				clog(RPCNote) << "NetConnection::send";
	});
}

bool NetConnection::checkPacket(bytesConstRef _msg) const
{
	if (_msg.size() < 4)
		return false;
	uint32_t len = ((_msg[0] * 256 + _msg[1]) * 256 + _msg[2]) * 256 + _msg[3];
	if (_msg.size() != len + 4)
		return false;
	RLP r(_msg.cropped(4));
	if (r.actualSize() != len)
		return false;
	return true;
}

void NetConnection::handshake(size_t _rlpLen)
{
	if (!_rlpLen) // todo: refactor to handshake()/doHandshake(size_t)
	{
		// if m_stopped is true, set to false, returns true, continues
		// if m_stopped is false, returns false, returns
		bool yes = true;
		if (!m_stopped.compare_exchange_strong(yes, false))
			return;
	}
	
	// read header
	if (m_recvdBytes == 0)
	{
		clog(RPCNote) << "NetConnection::handshake started";
		
		// write version packet
		shared_ptr<NetMsg> version(new NetMsg((NetMsgServiceType)0, 0, (NetMsgType)0, RLP("version")));
	
		// as a special case, message is manually written as
		// read/write isn't setup until connection is verified
		auto self(shared_from_this());
		boost::asio::async_write(m_socket, boost::asio::buffer(version->payload()), [this, self, version](boost::system::error_code _ec, size_t _len)
		{
			if (_ec)
				return closeSocket();
			
			m_recvdBytes += _len;
			// read length header
			m_recvBuffer.resize(4);
			boost::asio::async_read(m_socket, boost::asio::buffer(m_recvBuffer), [this, self](boost::system::error_code _ec, size_t _len)
			{
				if (_ec)
					return closeSocket();
				assert(_len == 4);
				
				m_recvdBytes += _len;
				size_t rlpLen = fromBigEndian<uint32_t>(bytesConstRef(m_recvBuffer.data(), 4));
				if (rlpLen > 16*1024)
					return; // throw MessageTooLarge();
				if (rlpLen < 3)
					return; // throw MessageTooSmall();
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
			return closeSocket();
		
		if (_len && m_recvdBytes >= _rlpLen)
		{
			m_recvdBytes += _len;
			
			try
			{
				NetMsg msg(bytesConstRef(m_recvBuffer.data() + 4, _rlpLen));
				// todo: verify version
			}
			catch (...)
			{
				shutdown();
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
	if (m_stopped && !_rlpLen)
		return closeSocket();
	
	auto self(shared_from_this());
	m_socket.async_read_some(boost::asio::buffer(boost::asio::buffer(m_recvBuffer) + m_recvdBytes), [this, self, _rlpLen](boost::system::error_code _ec, size_t _len)
	{
		if (_ec)
		{
			shutdown();
			return;
		}
		
		m_recvdBytes += _len;
		
		size_t rlpLen = _rlpLen;
		if (!rlpLen && m_recvdBytes < 4)
		{
			doRead();
			return;
		}
		else if (!rlpLen)
		{
			clog(RPCNote) << "NetConnection::doRead header";
			rlpLen = fromBigEndian<uint32_t>(bytesConstRef(m_recvBuffer.data(), 4));
			if (rlpLen > 16*1024)
			{
				clog(RPCNote) << "NetConnection::doRead MessageTooLarge";
				return; // throw MessageTooLarge();
			}
			if (rlpLen < 3)
			{
				clog(RPCNote) << "NetConnection::doRead MessageTooSmall";
				return; // throw MessageTooSmall();
			}
			
			if (2*(rlpLen+4) > m_recvBuffer.size())
				m_recvBuffer.resize(2*(rlpLen+4));
		}

		if (m_recvdBytes >= rlpLen)
		{
			try
			{
				clog(RPCNote) << "NetConnection::doRead message";
				NetMsg msg(bytesConstRef(m_recvBuffer.data(), rlpLen));
				if (msg.service())
				{
					messageHandlers &hs = msg.type() ? m_dataMsgHandlers : m_serviceMsgHandlers;

					// If handler is found, get pointer and call
					auto hit = hs.find(msg.service());
					if (hit!=hs.end())
					{
						auto h = *hit->second.get();
						if (h!=nullptr)
							h(msg.type(), RLP(msg.payload()));
					}
					else
						throw MessageServiceInvalid();
				}
				// else, control messages (ignored right now)
			}
			catch (...)
			{
				// bad message
				shutdown();
				return;
			}

			m_recvdBytes = m_recvdBytes - rlpLen - 4;
			if (m_recvdBytes)
				memmove(m_recvBuffer.data(), m_recvBuffer.data() + rlpLen + 4,m_recvdBytes);
		}

		doRead(rlpLen);
	});
}

bool NetConnection::connectionOpen() const
{
	return m_stopped ? false : m_socket.is_open();
}

void NetConnection::shutdown(bool _wait)
{
	bool no = false;
	if (m_started.compare_exchange_strong(no, true))
		m_stopped = true;
	else
		return;
	
	// when m_stopped is true, doRead will eventually close socket
	if (_wait)
		while (m_socket.is_open())
			usleep(200);
	
	m_recvBuffer.resize(0);
	m_started = false;
}

void NetConnection::closeAndShutdown()
{
	closeSocket();
	shutdown(false);
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

