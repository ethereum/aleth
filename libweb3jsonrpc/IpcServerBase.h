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
/** @file IpcServerBase.h
 * @authors:
 *   Arkadiy Paronyan <arkadiy@ethdev.com>
 * @date 2015
 */

#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <unordered_set>
#include <jsonrpccpp/server/abstractserverconnector.h>

namespace dev
{
template <class S> class IpcServerBase: public jsonrpc::AbstractServerConnector
{
public:
	IpcServerBase(std::string const& _path);
	virtual bool StartListening();
	virtual bool StopListening();
	virtual bool SendResponse(std::string const& _response, void* _addInfo = nullptr);

protected:
	virtual void Listen() = 0;
	virtual void CloseConnection(S _socket) = 0;
	virtual size_t Write(S _connection, std::string const& _data) = 0;
	virtual size_t Read(S _connection, void* _data, size_t _size) = 0;
	void GenerateResponse(S _connection);

protected:
	bool m_running = false;
	std::string m_path;
	std::unordered_set<S> m_sockets;
	std::mutex x_sockets;
	std::thread m_listeningThread; //TODO use asio for parallel request processing
};
} // namespace dev
