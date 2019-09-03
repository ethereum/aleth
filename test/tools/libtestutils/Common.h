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
/** @file Common.h
 * @author Marek Kotewicz <marek@ethdev.com>
 * @date 2015
 */

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
