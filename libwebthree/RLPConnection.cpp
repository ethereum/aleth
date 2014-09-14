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
/** @file RLPConnection.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include <memory>
#include "RLPConnection.h"
#include "RLPMessage.h"

using namespace std;
using namespace dev;

RLPConnection::RLPConnection(boost::asio::io_service& io_service): m_socket(io_service), m_stopped(true)
{
}

RLPConnection::RLPConnection(boost::asio::io_service& io_service, boost::asio::ip::tcp::endpoint _ep): m_socket(io_service)
{
	auto self(shared_from_this());
	m_socket.async_connect(_ep, [self, this, _ep](boost::system::error_code const& ec)
	{
		if (ec)
		{
			clog(RPCConnect) << "RLPConnection async_connect error";
			shutdown();
		}
		else
			handshake();
	});
}

void RLPConnection::setMessageHandler(RLPMessageServiceType _serviceId, messageHandler _responder)
{
	
}

void RLPConnection::send(RLPMessageServiceType _svc, RLPMessageSequence _seq, RLPMessageType _type, RLP const& _msg)
{
	while (!m_stopped && !m_socket.is_open())
		usleep(200);
	
	shared_ptr<RLPMessage> msg(new RLPMessage(_svc, _seq, _type, _msg));
	boost::asio::async_write(m_socket, boost::asio::buffer(msg->m_messageBytes), [msg](boost::system::error_code _ec, size_t _len){
		if (_ec)
				clog(RPCNote) << "RLPConnection::send error";
			else
				clog(RPCNote) << "RLPConnection::send";
	});
}

void RLPConnection::send(RLPMessage& _msg)
{
	boost::asio::async_write(m_socket, boost::asio::buffer(_msg.payload()), [](boost::system::error_code _ec, size_t _len){
		if (_ec)
				clog(RPCNote) << "RLPConnection::send error";
			else
				clog(RPCNote) << "RLPConnection::send";
	});
}

bool RLPConnection::checkPacket(bytesConstRef _msg) const
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

void RLPConnection::handshake(size_t _rlpLen)
{
	{
		lock_guard<mutex> l(x_stopped);
		if (m_stopped)
			m_stopped = false;
		else
			return;
	}
	
	clog(RPCNote) << "RLPConnection::handshake";
	
	// read header
	if (m_recvdBytes == 0)
	{
		// write version packet
		shared_ptr<RLPMessage> version(new RLPMessage((RLPMessageServiceType)0, 0, (RLPMessageType)0, RLP("version")));
	
		// as a special case, message is manually written as
		// read/write isn't setup until connection is verified
		auto self(shared_from_this());
		boost::asio::async_write(m_socket, boost::asio::buffer(version->payload()), [this, self, version](boost::system::error_code _ec, size_t _len)
		{
			if (_ec)
			{
				shutdown();
				return;
			}
			
			m_recvdBytes += _len;
			// read length header
			m_recvBuffer.resize(4);
			boost::asio::async_read(m_socket, boost::asio::buffer(m_recvBuffer), [this, self](boost::system::error_code _ec, size_t _len)
			{
				if (_ec)
				{
					shutdown();
					return;
				}
				
				m_recvdBytes += _len;
				size_t rlpLen = fromBigEndian<uint32_t>(bytesConstRef(m_recvBuffer.data(), 4));
				if (rlpLen > 16*1024)
					return; // throw MessageTooLarge();
				if (rlpLen < 3)
					return; // throw MessageTooSmall();
				m_recvBuffer.resize(2 * (rlpLen + 4));
				handshake(rlpLen);
			});
		});
	}
	
	auto self(shared_from_this());
	m_socket.async_read_some(boost::asio::buffer(boost::asio::buffer(m_recvBuffer) + m_recvdBytes), [this, self, _rlpLen](boost::system::error_code _ec, size_t _len)
	{
		if (_ec)
		{
			shutdown();
			return;
		}
		
		m_recvdBytes += _len;
		if (m_recvdBytes >= _rlpLen)
		{
			try
			{
				RLPMessage msg(bytesConstRef(m_recvBuffer.data(), _rlpLen));
			}
			catch (...)
			{
				// bad message
				shutdown();
				return;
			}
			
			m_recvdBytes = m_recvdBytes - _rlpLen - 4;
			if (m_recvdBytes)
				memmove(m_recvBuffer.data(), m_recvBuffer.data() + _rlpLen + 4,m_recvdBytes);
			
			doRead();
			return;
		}

		handshake(_rlpLen);
	});
}

void RLPConnection::doRead(size_t _rlpLen)
{
	if (m_stopped)
	{
		m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
		m_socket.close();
		return;
	}
	
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
			clog(RPCNote) << "RLPConnection::doRead header";
			rlpLen = fromBigEndian<uint32_t>(bytesConstRef(m_recvBuffer.data(), 4));
			if (rlpLen > 16*1024)
			{
				clog(RPCNote) << "RLPConnection::doRead MessageTooLarge";
				return; // throw MessageTooLarge();
			}
			if (rlpLen < 3)
			{
				clog(RPCNote) << "RLPConnection::doRead MessageTooSmall";
				return; // throw MessageTooSmall();
			}
			if (2*(rlpLen+4) > m_recvBuffer.size())
				m_recvBuffer.resize(2*(rlpLen+4));
		}

		if (m_recvdBytes >= rlpLen)
		{
			try
			{
				clog(RPCNote) << "RLPConnection::doRead message";
				RLPMessage msg(bytesConstRef(m_recvBuffer.data(), rlpLen));
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

bool RLPConnection::connectionOpen() const
{
	return m_stopped ? false : m_socket.is_open();
}

void RLPConnection::shutdown(bool _wait)
{
	m_stopped = true;
	
	if (_wait)
		while (m_socket.is_open())
			usleep(200);
}

