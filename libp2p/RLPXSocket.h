// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include "Common.h"

namespace dev
{
namespace p2p
{

/**
 * @brief Shared pointer wrapper for ASIO TCP socket.
 *
 * Thread Safety
 * Distinct Objects: Safe.
 * Shared objects: Unsafe.
 * * an instance method must not be called concurrently
 */
class RLPXSocket: public std::enable_shared_from_this<RLPXSocket>
{
public:
    explicit RLPXSocket(bi::tcp::socket _socket) : m_socket(std::move(_socket)) {}
    ~RLPXSocket() { close(); }
	
	bool isConnected() const { return m_socket.is_open(); }
	void close() { try { boost::system::error_code ec; m_socket.shutdown(bi::tcp::socket::shutdown_both, ec); if (m_socket.is_open()) m_socket.close(); } catch (...){} }
	bi::tcp::endpoint remoteEndpoint() { boost::system::error_code ec; return m_socket.remote_endpoint(ec); }
	bi::tcp::socket& ref() { return m_socket; }

protected:
	bi::tcp::socket m_socket;
};

}
}
