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
#include <cstdlib>
#include <sys/socket.h>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <libdevcore/Guards.h>
#include <libdevcore/FileSystem.h>

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

int const c_pathMaxSize = sizeof(sockaddr_un::sun_path)/sizeof(sockaddr_un::sun_path[0]);

string ipcSocketPath()
{
#ifdef __APPLE__
	// A bit hacky, but backwards compatible: If we did not change the default data dir,
	// put the socket in ~/Library/Ethereum, otherwise in the set data dir.
	string path = getIpcPath();
	if (path == getDefaultDataDir())
		return getenv("HOME") + string("/Library/Ethereum");
	else
		return path;
#else
	return getIpcPath();
#endif
}

UnixDomainSocketServer::UnixDomainSocketServer(string const& _appId):
	IpcServerBase(string(getIpcPath() + "/" + _appId + ".ipc").substr(0, c_pathMaxSize))
{
}

UnixDomainSocketServer::~UnixDomainSocketServer()
{
	StopListening();
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
		strncpy(m_address.sun_path, m_path.c_str(), c_pathMaxSize);
		::bind(m_socket, reinterpret_cast<sockaddr*>(&m_address), sizeof(sockaddr_un));
		listen(m_socket, 128);
	}
	return IpcServerBase::StartListening();
}

bool UnixDomainSocketServer::StopListening()
{
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
		int connection = accept(m_socket, (sockaddr*) &(m_address), &addressLen);
		if (connection > 0)
		{
			DEV_GUARDED(x_sockets)
				m_sockets.insert(connection);

			std::thread handler([this, connection](){ GenerateResponse(connection); });
			handler.detach();
		}
	}
}

void UnixDomainSocketServer::CloseConnection(int _socket)
{
	close(_socket);
}


size_t UnixDomainSocketServer::Write(int _connection, string const& _data)
{
	int r = send(_connection, _data.data(), _data.size(), MSG_NOSIGNAL);
	if (r > 0)
		return r;
	return 0;
}

size_t UnixDomainSocketServer::Read(int _connection, void* _data, size_t _size)
{
	int r = read(_connection, _data, _size);
	if (r > 0)
		return r;
	return 0;
}

#endif
