// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
#pragma once

#include "IpcServerBase.h"

#include <sys/un.h>
#include <atomic>

namespace dev
{
class UnixDomainSocketServer : public IpcServerBase<int>
{
public:
    explicit UnixDomainSocketServer(std::string const& _appId);
    ~UnixDomainSocketServer() override;
    bool StartListening() override;
    bool StopListening() override;

protected:
    void Listen() override;
    void CloseConnection(int _socket) override;
    size_t Write(int _connection, std::string const& _data) override;
    size_t Read(int _connection, void* _data, size_t _size) override;

    sockaddr_un m_address;
    std::atomic<int> m_socket{0};
};

}  // namespace dev
