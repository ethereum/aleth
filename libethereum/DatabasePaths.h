// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include "libdevcore/FixedHash.h"
#include <boost/filesystem/path.hpp>

namespace dev
{
namespace eth
{
namespace database_paths
{
bool databasePathsSet();
void setDatabasePaths(boost::filesystem::path const& _rootPath, h256 const& _genesisHash);
boost::filesystem::path rootPath();
boost::filesystem::path chainPath();
boost::filesystem::path blocksDatabasePath();
boost::filesystem::path stateDatabasePath();
boost::filesystem::path extrasDatabasePath();
boost::filesystem::path extrasDatabaseTemporaryPath();
boost::filesystem::path extrasDatabaseMinorVersionPath();
}  // namespace database_paths
}  // namespace eth
}  // namespace dev