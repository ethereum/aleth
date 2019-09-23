// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "IpcServerBase.h"
#include <cstdlib>
#include <cstdio>
#include <string>
#include <libdevcore/Guards.h>
#include <libdevcore/Log.h>

using namespace std;
using namespace jsonrpc;
using namespace dev;

int const c_bufferSize = 1024;

template <class S> IpcServerBase<S>::IpcServerBase(string const& _path):
    m_path(_path)
{
    clog(VerbosityInfo, "rpc") << "JSON-RPC socket path: " << _path;
}

template <class S> bool IpcServerBase<S>::StartListening()
{
    bool wasRunning = m_running.exchange(true);
    if (!wasRunning)
    {
        m_listeningThread = std::thread([this](){ Listen(); });
        return true;
    }
    return false;
}

template <class S> bool IpcServerBase<S>::StopListening()
{
    bool wasRunning = m_running.exchange(false);
    if (wasRunning)
    {
        DEV_GUARDED(x_sockets)
        {
            for (S s : m_sockets)
                CloseConnection(s);
            m_sockets.clear();
        }
        m_listeningThread.join();
        return true;
    }
    return false;
}

template <class S> bool IpcServerBase<S>::SendResponse(string const& _response, void* _addInfo)
{
    bool fullyWritten = false;
    bool errorOccured = false;
    S socket = (S)(reinterpret_cast<intptr_t>(_addInfo));
    string toSend = _response;
    do
    {
        size_t bytesWritten = Write(socket, toSend);
        if (bytesWritten == 0)
            errorOccured = true;
        else if (bytesWritten < toSend.size())
        {
            int len = toSend.size() - bytesWritten;
            toSend = toSend.substr(bytesWritten + sizeof(char), len);
        }
        else
            fullyWritten = true;
    } while (!fullyWritten && !errorOccured);
    clog(VerbosityTrace, "rpc") << _response;
    return fullyWritten && !errorOccured;
}

template <class S> void IpcServerBase<S>::GenerateResponse(S _connection)
{
    char buffer[c_bufferSize];
    string request;
    bool escape = false;
    bool inString = false;
    size_t i = 0;
    int depth = 0;
    size_t nbytes = 0;
    do
    {
        nbytes = Read(_connection, buffer, c_bufferSize);
        if (nbytes <= 0)
            break;
        request.append(buffer, nbytes);
        while (i < request.size())
        {
            char c = request[i];
            if (c == '\"' && !inString)
            {
                inString = true;
                escape = false;
            }
            else if (c == '\"' && inString && !escape)
            {
                inString = false;
                escape = false;
            }
            else if (inString && c == '\\' && !escape)
            {
                escape = true;
            }
            else if (inString)
            {
                escape = false;
            }
            else if (!inString && (c == '{' || c == '['))
            {
                depth++;
            }
            else if (!inString && (c == '}' || c == ']'))
            {
                depth--;
                if (depth == 0)
                {
                    std::string r = request.substr(0, i + 1);
                    request.erase(0, i + 1);
                    clog(VerbosityTrace, "rpc") << r;
                    OnRequest(r, reinterpret_cast<void*>((intptr_t)_connection));
                    i = 0;
                    continue;
                }
            }
            i++;
        }
    } while (true);
    DEV_GUARDED(x_sockets)
        m_sockets.erase(_connection);
}

namespace dev
{
template class IpcServerBase<int>;
template class IpcServerBase<void*>;
}

