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
class DatabasePaths
{
public:
    DatabasePaths(boost::filesystem::path const& _rootPath, h256 const& _genesisHash) noexcept;
    boost::filesystem::path const& rootPath() const noexcept;
    boost::filesystem::path const& chainPath() const noexcept;
    boost::filesystem::path const& blocksPath() const noexcept;
    boost::filesystem::path const& statePath() const noexcept;
    boost::filesystem::path const& extrasPath() const noexcept;
    boost::filesystem::path const& extrasTemporaryPath() const noexcept;
    boost::filesystem::path const& extrasMinorVersionPath() const noexcept;

private:
    boost::filesystem::path m_rootPath;
    boost::filesystem::path m_chainPath;
    boost::filesystem::path m_blocksPath;
    boost::filesystem::path m_statePath;
    boost::filesystem::path m_extrasPath;
    boost::filesystem::path m_extrasTemporaryPath;
    boost::filesystem::path m_extrasMinorVersionPath;
};
}  // namespace eth
}  // namespace dev