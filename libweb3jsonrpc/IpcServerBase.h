// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#pragma once

#include <atomic>
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
	std::atomic<bool> m_running{false};
	std::string m_path;
	std::unordered_set<S> m_sockets;
	std::mutex x_sockets;
	std::thread m_listeningThread; //TODO use asio for parallel request processing
};
} // namespace dev
