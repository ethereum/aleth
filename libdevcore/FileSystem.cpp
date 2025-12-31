// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#include "FileSystem.h"
#include "Common.h"
#include "Log.h"

#if defined(_WIN32)
#include <shlobj.h>
#else
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#endif
#include <boost/filesystem.hpp>
using namespace std;
using namespace dev;

namespace fs = boost::filesystem;

static_assert(BOOST_VERSION >= 106400, "Wrong boost headers version");

// Should be written to only once during startup
static fs::path s_ethereumDatadir;
static fs::path s_ethereumIpcPath;

void dev::setDataDir(fs::path const& _dataDir)
{
	s_ethereumDatadir = _dataDir;
}

void dev::setIpcPath(fs::path const& _ipcDir)
{
	s_ethereumIpcPath = _ipcDir;
}

fs::path dev::getIpcPath()
{
	// Strip "geth.ipc" suffix if provided.
	if (s_ethereumIpcPath.filename() == "geth.ipc")
		return s_ethereumIpcPath.parent_path();
	else
		return s_ethereumIpcPath;
}

fs::path dev::getDataDir(string _prefix)
{
	if (_prefix.empty())
		_prefix = "ethereum";
	if (_prefix == "ethereum" && !s_ethereumDatadir.empty())
		return s_ethereumDatadir;
	return getDefaultDataDir(_prefix);
}

fs::path dev::getDefaultDataDir(string _prefix)
{
	if (_prefix.empty())
		_prefix = "ethereum";

#if defined(_WIN32)
	_prefix[0] = toupper(_prefix[0]);
	char path[1024] = "";
	if (SHGetSpecialFolderPathA(NULL, path, CSIDL_APPDATA, true))
		return fs::path(path) / _prefix;
	else
	{
	#ifndef _MSC_VER // todo?
		cwarn << "getDataDir(): SHGetSpecialFolderPathA() failed.";
	#endif
		BOOST_THROW_EXCEPTION(std::runtime_error("getDataDir() - SHGetSpecialFolderPathA() failed."));
	}
#else
	fs::path dataDirPath;
	char const* homeDir = getenv("HOME");
	if (!homeDir || strlen(homeDir) == 0)
	{
		struct passwd* pwd = getpwuid(getuid());
		if (pwd)
			homeDir = pwd->pw_dir;
	}
	
	if (!homeDir || strlen(homeDir) == 0)
		dataDirPath = fs::path("/");
	else
		dataDirPath = fs::path(homeDir);
	
	return dataDirPath / ("." + _prefix);
#endif
}

fs::path dev::appendToFilename(fs::path const& _orig, string const& _suffix)
{
	if (_orig.filename() == fs::path(".") || _orig.filename() == fs::path(".."))
		return _orig / fs::path(_suffix);
	else
		return _orig.parent_path() / fs::path( _orig.filename().string() + _suffix);
}
