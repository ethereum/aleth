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
/** @file WinPipeServer.cpp
* @authors:
*   Arkadiy Paronyan <arkadiy@ethdev.com>
* @date 2015
*/
#if defined(_WIN32)

#include "WinPipeServer.h"
#include <windows.h>
#include <libdevcore/Guards.h>

using namespace std;
using namespace jsonrpc;
using namespace dev;

int const c_bufferSize = 1024;

WindowsPipeServer::WindowsPipeServer(string const& _appId):
	IpcServerBase("\\\\.\\pipe\\" + _appId + ".ipc")
{
}

void WindowsPipeServer::CloseConnection(HANDLE _socket)
{
	::CloseHandle(_socket);
}

size_t WindowsPipeServer::Write(HANDLE _connection, std::string const& _data)
{
	DWORD written = 0;
	::WriteFile(_connection, _data.data(), _data.size(), &written , nullptr);
	return written;
}

size_t WindowsPipeServer::Read(HANDLE _connection, void* _data, size_t _size)
{
	DWORD read;
	::ReadFile(_connection, _data, _size, &read, nullptr);
	return read;
}

void WindowsPipeServer::Listen()
{
	while (m_running)
	{
		HANDLE socket = CreateNamedPipe(
			m_path.c_str(),
			PIPE_ACCESS_DUPLEX,
			PIPE_READMODE_BYTE |
			PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES,
			c_bufferSize,
			c_bufferSize,
			0,
			nullptr);

		DEV_GUARDED(x_sockets)
			m_sockets.insert(socket);

		if (ConnectNamedPipe(socket, nullptr) != 0)
		{
			std::thread handler([this, socket](){ GenerateResponse(socket); });
			handler.detach();
		}
		else
		{
			DEV_GUARDED(x_sockets)
				m_sockets.erase(socket);
		}
	}
}

#endif