// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "Common.h"
#include <libdevcore/CommonData.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/FileSystem.h>
#include <test/tools/libtesteth/Options.h>
#include <boost/filesystem.hpp>
#include <random>

using namespace std;
using namespace dev;
using namespace dev::test;
namespace fs = boost::filesystem;

namespace
{
mt19937_64 g_randomGenerator(random_device{}());
}

boost::filesystem::path dev::test::getTestPath()
{
    if (!Options::get().testpath.empty())
        return Options::get().testpath;

    string testPath;
    const char* ptestPath = getenv("ETHEREUM_TEST_PATH");

    if (ptestPath == nullptr)
    {
        clog(VerbosityWarning, "test")
            << " could not find environment variable ETHEREUM_TEST_PATH \n";
        testPath = "../../test/jsontests";
    }
    else
        testPath = ptestPath;

    return boost::filesystem::path(testPath);
}

int dev::test::randomNumber(int _min, int _max)
{
    return std::uniform_int_distribution<int>{_min, _max}(g_randomGenerator);
}

unsigned short dev::test::randomPortNumber(unsigned short _min, unsigned short _max)
{
    return std::uniform_int_distribution<unsigned short>{_min, _max}(g_randomGenerator);
}

Json::Value dev::test::loadJsonFromFile(fs::path const& _path)
{
    Json::Reader reader;
    Json::Value result;
    string s = dev::contentsString(_path);
    if (!s.length())
        clog(VerbosityWarning, "test")
            << "Contents of " << _path.string()
            << " is empty. Have you cloned the 'tests' repo branch develop and "
               "set ETHEREUM_TEST_PATH to its path?";
    else
        clog(VerbosityWarning, "test") << "FIXTURE: loaded test from file: " << _path.string();

    reader.parse(s, result);
    return result;
}

fs::path dev::test::toTestFilePath(std::string const& _filename)
{
    return getTestPath() / fs::path(_filename + ".json");
}

fs::path dev::test::getRandomPath()
{
    std::stringstream stream;
    stream << randomNumber();
    return getDataDir("EthereumTests") / fs::path(stream.str());
}

