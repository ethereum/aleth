// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "DatabasePaths.h"
#include "libdevcore/CommonIO.h"
#include "libdevcore/DBFactory.h"
#include "libethcore/Exceptions.h"

namespace dev
{
namespace eth
{
namespace database_paths
{
namespace fs = boost::filesystem;

fs::path g_rootPath;
fs::path g_chainPath;
fs::path g_blocksDbPath;
fs::path g_stateDbPath;
fs::path g_extrasDbPath;
fs::path g_extrasDbTempPath;
fs::path g_extrasDbMinorVersionPath;

bool databasePathsSet()
{
    return !g_rootPath.empty();
}

void setDatabasePaths(fs::path const& _rootPath, h256 const& _genesisHash)
{
    // Allow empty hashes since they are required by tests
    
    g_rootPath = _rootPath.empty() ? db::databasePath() : _rootPath;
    g_chainPath = g_rootPath / fs::path(toHex(_genesisHash.ref().cropped(0, 4)));
    g_stateDbPath = g_chainPath / fs::path("state");
    g_blocksDbPath = g_chainPath / fs::path("blocks");

    auto const extrasRootPath = g_chainPath / fs::path(toString(c_databaseVersion));
    g_extrasDbPath = extrasRootPath / fs::path("extras");
    g_extrasDbTempPath = extrasRootPath / fs::path("extras.old");
    g_extrasDbMinorVersionPath = g_extrasDbPath / fs::path("minor");
}

fs::path rootPath()
{
    return g_rootPath;
}

fs::path chainPath()
{
    return g_chainPath;
}

fs::path stateDatabasePath()
{
    return g_stateDbPath;
}

fs::path blocksDatabasePath()
{
    return g_blocksDbPath;
}

fs::path extrasDatabasePath()
{
    return g_extrasDbPath;
}

fs::path extrasDatabaseTemporaryPath()
{
    return g_extrasDbTempPath;
}

fs::path extrasDatabaseMinorVersionPath()
{
    return g_extrasDbMinorVersionPath;
}
}  // namespace database_paths
}  // namespace eth
}  // namespace dev