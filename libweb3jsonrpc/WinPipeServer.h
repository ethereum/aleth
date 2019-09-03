/*
This file is part of aleth.

aleth is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

aleth is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with aleth.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include "IpcServerBase.h"

#include <windows.h>

namespace dev
{
class WindowsPipeServer : public IpcServerBase<HANDLE>
{
public:
    WindowsPipeServer(std::string const& _appId);

protected:
    void Listen() override;
    void CloseConnection(HANDLE _socket) override;
    size_t Write(HANDLE _connection, std::string const& _data) override;
    size_t Read(HANDLE _connection, void* _data, size_t _size) override;
};

}  // namespace dev
