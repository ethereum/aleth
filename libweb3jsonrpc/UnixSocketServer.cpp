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
/** @file UnixSocketServer.cpp
* @authors:
*   Arkadiy Paronyan <arkadiy@ethdev.com>
* @date 2015
*/
#if !defined(_WIN32)

#include "UnixSocketServer.h"
#include <algorithm>
#include <iostream>
#include <sys/select.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <libdevcore/Guards.h>
#include <libdevcore/FileSystem.h>
#include <boost/filesystem/path.hpp>

// "Mac OS X does not support the flag MSG_NOSIGNAL but we have an equivalent."
// See http://lists.apple.com/archives/macnetworkprog/2002/Dec/msg00091.html
#if defined(__APPLE__)
	#if !defined(MSG_NOSIGNAL)
		#define MSG_NOSIGNAL SO_NOSIGPIPE
	#endif
#endif

using namespace std;
using namespace jsonrpc;
using namespace dev;
namespace fs = boost::filesystem;

namespace
{
size_t const c_socketPathMaxLength = sizeof(sockaddr_un::sun_path) / sizeof(sockaddr_un::sun_path[0]);

fs::path getIpcPathOrDataDir()
{
	// On Unix use datadir as default IPC path.
	fs::path path = getIpcPath();
	if (path.empty())
		return getDataDir();
	return path;
}

/**
 * Waits for one of the file descriptors to become readable.
 * @retval true One file descriptor became available.
 * @retval false Most likely an error occured, or a timeout.
 */
static bool waitForReadable(std::initializer_list<int> sockets)
{
	fd_set in, out, err;
	FD_ZERO(&in);
	FD_ZERO(&out);
	FD_ZERO(&err);

	int const maxfd = *std::max_element(sockets.begin(), sockets.end());
	for (int fd : sockets)
		FD_SET(fd, &in);

	return select(maxfd + 1, &in, &out, &err, nullptr) > 0;
}

}

UnixDomainSocketServer::UnixDomainSocketServer(string const& _appId):
	IpcServerBase((getIpcPathOrDataDir() / fs::path(_appId + ".ipc")).string().substr(0, c_socketPathMaxLength)),
	m_exitChannel{-1, -1}
{
	if (pipe(m_exitChannel) < 0)
		abort(); // I don't like. Replace me with throw.

	fcntl(m_exitChannel[0], F_SETFL, fcntl(m_exitChannel[0], F_GETFL) | O_NONBLOCK);
	fcntl(m_exitChannel[1], F_SETFL, fcntl(m_exitChannel[1], F_GETFL) | O_NONBLOCK);
}

UnixDomainSocketServer::~UnixDomainSocketServer()
{
	StopListening();

	::close(m_exitChannel[0]);
	::close(m_exitChannel[1]);
}

bool UnixDomainSocketServer::StartListening()
{
	if (!m_running)
	{
		if (access(m_path.c_str(), F_OK) != -1)
			unlink(m_path.c_str());

		if (access(m_path.c_str(), F_OK) != -1)
			return false;

		m_socket = socket(PF_UNIX, SOCK_STREAM, 0);
		memset(&(m_address), 0, sizeof(sockaddr_un));
		m_address.sun_family = AF_UNIX;
#ifdef __APPLE__
		m_address.sun_len = m_path.size() + 1;
#endif
		strncpy(m_address.sun_path, m_path.c_str(), c_socketPathMaxLength);
		::bind(m_socket, reinterpret_cast<sockaddr*>(&m_address), sizeof(sockaddr_un));
		fs::permissions(m_path, fs::owner_read | fs::owner_write);
		listen(m_socket, 128);
	}
	return IpcServerBase::StartListening();
}

bool UnixDomainSocketServer::StopListening()
{
	int dummy = 0x90;
	if (::write(m_exitChannel[1], &dummy, sizeof(dummy)) < 0)
		cerr << "Failed to notify UNIX domain server loop. " << strerror(errno) << "\n";

	shutdown(m_socket, SHUT_RDWR);
	close(m_socket);
	m_socket = -1;
	if (IpcServerBase::StopListening())
	{
		unlink(m_path.c_str());
		return true;
	}
	return false;
}

void UnixDomainSocketServer::Listen()
{
	socklen_t addressLen = sizeof(m_address);
	while (m_running)
	{
		// Block until either m_socket is readable or skip tail and recheck for m_running.
		// We do this test before calling accept() in order to gracefully exit
		// this function when an external thread has set m_running to true
		// (such as a signal handler in the main thread).
		if (!waitForReadable({m_socket, m_exitChannel[0]}))
			continue;

		int connection = accept(m_socket, (sockaddr*) &(m_address), &addressLen);
		if (connection > 0)
		{
			DEV_GUARDED(x_sockets)
				m_sockets.insert(connection);

			// Handle the request in a new detached thread.
			std::thread{[this, connection]
			{
				GenerateResponse(connection);
				CloseConnection(connection);
			}}.detach();
		}
	}
}

void UnixDomainSocketServer::CloseConnection(int _socket)
{
	shutdown(_socket, SHUT_RDWR);
	close(_socket);
}


size_t UnixDomainSocketServer::Write(int _connection, string const& _data)
{
	ssize_t r = send(_connection, _data.data(), _data.size(), MSG_NOSIGNAL);
	if (r < 0)
		return 0;
	return static_cast<size_t>(r);
}

size_t UnixDomainSocketServer::Read(int _connection, void* _data, size_t _size)
{
	ssize_t r = read(_connection, _data, _size);
	if (r < 0)
		return 0;
	return static_cast<size_t>(r);
}

#endif
