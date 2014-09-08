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
/** @file EthereumSession.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "EthereumSession.h"

#include <memory>
#include <libdevcore/Log.h>
#include <libdevcore/RLP.h>
#include "Ethereum.h"

using namespace std;
using namespace dev::eth;
namespace bi = boost::asio::ip;

EthereumSession::EthereumSession(Ethereum* _s, ba::ip::tcp::socket* _socket):
	m_ethereum(_s),
	m_socket(std::move(*_socket))
{
	m_disconnectAt = chrono::steady_clock::time_point::max();
	m_recvBuffer.resize(65536);
}

void EthereumSession::start()
{
	clog(NetNote) << "EthereumSession::start()";
	auto self(shared_from_this());
	m_socket.async_read_some(boost::asio::buffer(m_recvBuffer), [this, self](boost::system::error_code _ec, std::size_t _len)
	{
		handleRead(_ec, _len);
		// TODO: disconnect if first packet wasn't version (version won't be set) or version isn't compatible
		doReadAndRespond();
	});
}

bool EthereumSession::connectionOpen() const
{
	lock_guard<std::mutex> l(x_disconnectAt);
	if (m_disconnectAt == chrono::steady_clock::time_point::max())
		return m_socket.is_open();
	else
		return false;
}

void EthereumSession::doReadAndRespond()
{
	clog(NetNote) << "EthereumSession::doReadAndRespond()";
	if (!connectionOpen())
		return;
	
	auto self(shared_from_this());
	m_socket.async_read_some(boost::asio::buffer(m_recvBuffer), [this, self](boost::system::error_code _ec, std::size_t _len)
	{
		handleRead(_ec, _len);
		
		// disconnect if appropriate, else continue network
		// writes() are performed synchronously within handleRead() to ensure received-requests are responded to
		{
			lock_guard<std::mutex> l(x_disconnectAt);
			if (m_disconnectAt != chrono::steady_clock::time_point::max())
			{
				m_socket.shutdown(ba::ip::tcp::socket::shutdown_both);
				m_socket.close();
				return;
			}
		}

		if (connectionOpen())
			doReadAndRespond();
	});
}

void EthereumSession::handleRead(const boost::system::error_code& _ec, size_t _len)
{
	clog(NetNote) << "EthereumSession::handleRead()";
	if (!connectionOpen())
		return;

	// If error is end of file, ignore
	if (_ec && _ec.category() != boost::asio::error::get_misc_category() && _ec.value() != boost::asio::error::eof)
	{
		cwarn << "Error reading: " << _ec.message();
		disconnect();
	}
	else if (_ec && _len == 0)
	{
		cwarn << "Socket read error: " << _ec.message();
		disconnect();
	}
	else
	{
		try
		{
			m_received.resize(m_received.size() + _len);
			memcpy(m_received.data() + m_received.size() - _len, m_recvBuffer.data(), _len);
			while (m_received.size() > 4)
			{
				uint32_t len = fromBigEndian<uint32_t>(bytesConstRef(m_received.data(), 4));
				uint32_t tlen = len + 4;
				if (m_received.size() < tlen)
					break;

				// complete packet has been received
				auto data = bytesConstRef(m_received.data(), tlen);
				if (!m_ethereum->checkPacket(data))
				{
					cerr << "Received " << len << ": " << toHex(bytesConstRef(m_received.data() + 4, len)) << endl;
					cwarn << "INVALID MESSAGE RECEIVED";
					disconnect();
					return;
				}
				else
				{
					RLP r(data.cropped(4));
					if (!interpretRequest(r))
					{
						disconnect();
						return;
					}
				}
				memmove(m_received.data(), m_received.data() + tlen, m_received.size() - tlen);
				m_received.resize(m_received.size() - tlen);
			}
		}
		catch (Exception const& _e)
		{
			clogS(NetWarn) << "ERROR: " << _e.description();
			disconnect();
		}
		catch (std::exception const& _e)
		{
			clogS(NetWarn) << "ERROR: " << _e.what();
			disconnect();
		}
	}
}

bool EthereumSession::interpretRequest(RLP const& _r)
{
	clogS(NetRight) << _r;
	
	if (!_r.isList())
		return false;
	
	bool exception = false;
	bytes response = RLPEmptyList;
	try
	{
		switch (_r[0].toInt<unsigned>())
		{
			case RequestVersion:
				break;
				
			case RequestFlushTransactions:
			{
				m_ethereum->flushTransactions();
				break;
			}
				
			case RequestPeerCount:
			{
				RLPStream s;
				s.appendList(1);
				s.append(m_ethereum->peerCount());
				response = s.out();
				break;
			}
				
			case ConnectToPeer:
			{
				std::string host = _r[0].toString();
				unsigned short port = (short)_r[1].toInt();
				
				break;
			}

			default:
				return false;
		}
	}
	catch (Exception const& _e)
	{
		// TODO fixme: Exceptions received from calling Client function must be relayed to client
		exception = true;
	}
	
	if (exception)
		sendResponse(ResponseException, &RLPEmptyList);
	else
		sendResponse(ResponseSuccess, &response);
	
	return true;
}

void EthereumSession::disconnect()
{
	{
		lock_guard<std::mutex> l(x_disconnectAt);
		if (m_disconnectAt == chrono::steady_clock::time_point::max())
			m_disconnectAt = std::chrono::steady_clock::now();
	}
	
	while (connectionOpen())
		usleep(100);
}

void EthereumSession::sendResponse(RPCResponsePacketType _type, bytesConstRef _rlpmsg)
{
	RLPStream s;
	s.appendRaw(bytes(4, 0)); // pad for packet-length prefix
	s.appendList(2);
	s << (byte)_type;
	s.appendRaw(_rlpmsg);
	
	bytes bytesout;
	s.swapOut(bytesout);
	uint32_t len = (uint32_t)bytesout.size() - 4;
	bytesout[0] = (len >> 24) & 0xff;
	bytesout[1] = (len >> 16) & 0xff;
	bytesout[2] = (len >> 8) & 0xff;
	bytesout[3] = len & 0xff;
	
	if (!m_ethereum->checkPacket(&bytesout))
	{
		cwarn << "INVALID MESSAGE CREATED";
		throw RPCInvalidMessage();
	}

	// writes() are performed synchronously within handleRead() to ensure all received-requests are responded to
	boost::system::error_code _ec;
	ba::write(m_socket, ba::buffer(bytesout), _ec);
	if (_ec)
		throw RPCSocketError();
	
	clogS(NetNote) << "[s]=>c type: " << _type << "RLP length: " << len << "packet length: " << bytesout.size() << "prefix: " << (uint32_t)bytesout[0] << (uint32_t)bytesout[1] << (uint32_t)bytesout[2] << (uint32_t)bytesout[3];
}

