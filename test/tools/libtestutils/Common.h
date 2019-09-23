// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include <string>
#include <json/json.h>
#include <libdevcore/Log.h>

#include <boost/filesystem.hpp>

namespace dev
{
namespace test
{

boost::filesystem::path getTestPath();
int randomNumber(int _min = 1, int _max = INT_MAX);
unsigned short randomPortNumber(unsigned short _min = 1024, unsigned short _max = USHRT_MAX);
Json::Value loadJsonFromFile(boost::filesystem::path const& _path);
boost::filesystem::path toTestFilePath(std::string const& _filename);
boost::filesystem::path getRandomPath();

}

}
