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
/** @file Ethereum.cpp
 * @author Gav Wood <i@gavwood.com>
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 */

#include "Ethereum.h"

#include <libdevcore/Log.h>
#include <libethereum/Client.h>
#include "EthereumSession.h"
using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace p2p;
namespace bi = boost::asio::ip;
namespace ba = boost::asio;

Ethereum::Ethereum():
	m_endpoint(bi::address::from_string("127.0.0.1"), 30310),
	m_acceptor(m_ioService),
	m_socket(m_ioService)
{
	m_shutdown = false;
	m_pendingRequests = 0;
	ensureReady();
}

void Ethereum::ensureReady()
{
	while (!clientConnectionOpen() && !isServer())
		try
		{
			startRPCServer();
		}
		catch (DatabaseAlreadyOpen)
		{
			connectToRPCServer();
		}
}

Ethereum::~Ethereum()
{
	// signal server to stop accepting connections and closes acceptor
	m_shutdown = true;

	// gracefully disconnect peers (blocking)
	lock_guard<mutex> ls(x_sessions);
	for (auto s: m_sessions)
		if (auto p = s.lock().get())
			p->disconnect();

	// With clients stopped, it's now safe to stop asio polling thread
	if (m_serverThread.joinable())
		m_serverThread.join();
	
	// Mutex client in case client connection went away and interface is transitioning from client->server
	lock_guard<mutex> lc(x_client);
	if (m_client.get())
		m_client.reset();

	if (m_socket.is_open())
	{
		// Close client connection. Ensure pending requests complete before finishing.
		while (m_pendingRequests)
			usleep(100);
		m_socket.close();
	}
}

bool Ethereum::clientConnectionOpen() const
{
	if (m_shutdown)
		return false;

	return m_socket.is_open();
}

void Ethereum::connectToRPCServer()
{
	if (clientConnectionOpen() || isServer())
		return;
	
	try
	{
		boost::system::error_code ec;
		{
			// ensure connection occurs only once
			lock_guard<mutex> l(x_socket);
			if (!m_socket.is_open())
				m_socket.connect(m_endpoint, ec);
			else
				return;
		}
		
		if (ec)
			clog(NetConnect) << "Failed connecting to RPC Server (" << ec.message() << ")";
		else
		{
			// Send version request packet, read response, verify and return
			clog(NetConnect) << "Sending version request to RPC Server";
			RLP r(sendRequest(RequestVersion, RLP(RLPEmptyList)));
			if (!r.isList())
			{
				// TODO fixme: check/disconnect invalid version
			}
			clog(NetConnect) << "Successful connection to RPC Server";
		}

	}
	catch (exception const& e)
	{
		m_socket.close();
		clog(NetConnect) << "Bad RPC Server host or I/O exception (" << e.what() << ")";
	}
}

void Ethereum::startRPCServer()
{
	if (m_shutdown || isServer())
		return;
	
	{
		lock_guard<mutex> l(x_client);
		m_client = unique_ptr<Client>(new Client("+ethereum+"));
	}
	clog(NetNote) << "Blockchain opened. Starting RPC Server.";
	
	m_serverThread = std::thread([&]()
	{
		m_acceptor = boost::asio::ip::tcp::acceptor(m_ioService);
		m_acceptor.open(m_endpoint.protocol());
		m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		m_acceptor.bind(m_endpoint);
		m_acceptor.listen();
		clog(NetNote) << "Listening for RPC connections.";
		
		runServer();
		
		while (!m_shutdown)
			m_ioService.poll();
		m_acceptor.close();
	});
}

void Ethereum::runServer()
{
	if (m_shutdown)
		return;
	
	ba::ip::tcp::socket *sock = new ba::ip::tcp::socket(m_ioService);
	m_acceptor.async_accept(*sock, [=](boost::system::error_code _ec)
	{
		clog(NetConnect) << "Accepting ethx connection";
		
		{
			// Ensure graceful shutdown by checking m_shutdown within sessions lock.
			// (destructor disconnects peers after shutdown is set)
			lock_guard<mutex> l(x_sessions);
			if (!_ec && !m_shutdown)
			{
				auto p = make_shared<EthereumSession>(this, sock);
				m_sessions.push_back(p);
				p->start();
			}
		}
		
		if (_ec.value() != 1)
			runServer();
		else
			// TODO fixme: handle _ec (restart or exception/crash if appropriate)
			clog(NetNote) << "RPC Server going away.";
	});
}

bool Ethereum::checkPacket(bytesConstRef _msg) const
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

bytes Ethereum::sendRequest(RPCRequestPacketType _requestType, RLP const& _requestRLP) const
{
	m_pendingRequests++;

	RLPStream s;
	s.appendRaw(bytes(4, 0)); // pad for packet-length prefix
	s.appendList(2);
	s << _requestType;
	s << _requestRLP;
	
	bytes bytesout;
	s.swapOut(bytesout);
	uint32_t outlen = (uint32_t)bytesout.size() - 4;
	bytesout[0] = (outlen >> 24) & 0xff;
	bytesout[1] = (outlen >> 16) & 0xff;
	bytesout[2] = (outlen >> 8) & 0xff;
	bytesout[3] = outlen & 0xff;

	bytes recvBytes;
	recvBytes.resize(5); // large enough for version response
	try
	{
		// Request I/O (write request, read response)
		
		lock_guard<mutex> l(x_socket);
		
		// Write request
		ba::write(m_socket, boost::asio::buffer(bytesout));
		clogS(NetNote) << "Ethereum::sendRequest() write() type:" << _requestType << "RLP length: " << outlen << "packet length: " << bytesout.size();
		
		// Read response
		ba::mutable_buffer recv_buffer = boost::asio::buffer(recvBytes);
		size_t recvd = 0;
		
		while (recvd < recvBytes.size())
			recvd += ba::read(m_socket, boost::asio::buffer(recv_buffer));
		
		uint32_t len = fromBigEndian<uint32_t>(bytesConstRef(recvBytes.data(), 4));
		recvBytes.resize(len + 4);
		recv_buffer = boost::asio::buffer(recvBytes);
		
		clogS(NetNote) << "Ethereum::sendRequest() read(), length: " << recvBytes.size() << "prefix: " << (uint32_t)recvBytes[0] << (uint32_t)recvBytes[1] << (uint32_t)recvBytes[2] << (uint32_t)recvBytes[3];
		
		while (recvd < recvBytes.size())
			recvd += m_socket.read_some(boost::asio::buffer(recv_buffer + recvd));
		assert(recvBytes.size() == recvd);
	}
	catch (...)
	{
		clogS(NetNote) << "ethx: Server went away, reconnecting.";
		// any network or standard exception is cause for disconnect
		m_pendingRequests--;
		m_socket.close();
		const_cast<Ethereum*>(this)->ensureReady();
		throw RPCServerWentAway();
	}

	// Check packet, parse, and return RLP[1]
	auto data = bytesConstRef(recvBytes.data(), recvBytes.size());
	if (!checkPacket(data))
	{
		m_pendingRequests--;
		throw RPCInvalidResponse();
	}

	RLP r(data.cropped(4));
	if (!r.isList())
	{
		m_pendingRequests--;
		throw RPCInvalidResponse();
	}

	if (r[0].toInt<unsigned>() != ResponseSuccess)
	{
		m_pendingRequests--;
		throw RPCRemoteException();
	}
	
	m_pendingRequests--;

	return r[1].data().toBytes();
}

#pragma mark -
#pragma mark Client API

void Ethereum::flushTransactions()
{
	while (true)
		if (isServer())
			return m_client->flushTransactions();
		else
			try
			{
				sendRequest(RequestFlushTransactions, RLP(RLPEmptyList));
				return;
			}
			catch (RPCServerWentAway const&){} // try again
			catch (std::exception const& _e)
			{
				// RPCRemoteException, RPCInvalidResponse, or other
				throw _e;
			}
}

std::vector<PeerInfo> Ethereum::peers()
{
	// RequestPeers
	std::vector<PeerInfo> peers;
//	while (true)
//		if (isServer())
//			return m_client->peers();
//		else
//			try
//			{
//				RLP resp(sendRequest(RequestPeers, RLP(RLPEmptyList)));
//				for (auto r: resp[1])
//					// TODO fixme: de/serialize port, timestamp
//					peers.push_back(PeerInfo({r[0].toString(),r[1].toString(),r[2].toInt<unsigned short>(), std::chrono::steady_clock::duration(0)}));
//			}
//			catch (RPCServerWentAway const&){} // try again
//			catch (std::exception const& _e)
//			{
//				// RPCRemoteException, RPCInvalidResponse, or other
//				throw _e;
//			}
	
	return peers;
}

size_t Ethereum::peerCount() const
{
	while (true)
		if (isServer())
			return m_client->peerCount();
		else
			try
			{
				RLP r(sendRequest(RequestPeerCount, RLP(RLPEmptyList)));
				try
				{
					size_t count = r[0].toInt();
					return count;
				}
				catch (Exception const& _e)
				{
					
				}
			}
			catch (RPCServerWentAway const&){} // try again
			catch (std::exception const& _e)
			{
				// RPCRemoteException, RPCInvalidResponse, or other (BadCast)
				throw _e;
			}
}

void Ethereum::connect(std::string const& _seedHost, unsigned short _port)
{
	// RequestConnectionToPeer
//	while (true)
//		if (isServer())
//			return m_client->connect(_seedHost, _port);
//		else
//			try
//			{
//				RLPStream s(2);
//				s << _seedHost;
//				s.append(_port);
//
//				sendRequest(ConnectToPeer, RLP(s.out()));
//				return;
//			}
//			catch (RPCServerWentAway const&){} // try again
//			catch (std::exception const& _e)
//			{
//				// RPCRemoteException, RPCInvalidResponse, or other
//				throw _e;
//			}
}

void Ethereum::transact(Secret _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice)
{
	// RequestSubmitTransaction
}

bytes Ethereum::call(Secret _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice)
{
	// RequestCallTransaction
	return bytes();
}

Address Ethereum::transact(Secret _secret, u256 _endowment, bytes const& _init, u256 _gas, u256 _gasPrice)
{
	// RequestCreateContract
	return Address();
}

void Ethereum::inject(bytesConstRef _rlp)
{
	// RequestRLPInject
}

u256 Ethereum::balanceAt(Address _a, int _block) const
{
	// RequestBalanceAt
	return u256();
}

PastMessages Ethereum::messages(MessageFilter const& _filter) const
{
	// RequestMessages
}

std::map<u256, u256> Ethereum::storageAt(Address _a, int _block) const
{
	// RequestStorageAt
	return std::map<u256, u256>();
}

u256 Ethereum::countAt(Address _a, int _block) const
{
	return u256();
}

u256 Ethereum::stateAt(Address _a, u256 _l, int _block) const
{
	return u256();
}

bytes Ethereum::codeAt(Address _a, int _block) const
{
	return bytes();
}
