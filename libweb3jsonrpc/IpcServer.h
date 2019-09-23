// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
#pragma once

#if _WIN32
#include "WinPipeServer.h"
#else
#include "UnixSocketServer.h"
#endif

namespace dev
{
#if _WIN32
using IpcServer = WindowsPipeServer;
#else
using IpcServer = UnixDomainSocketServer;
#endif
}  // namespace dev
