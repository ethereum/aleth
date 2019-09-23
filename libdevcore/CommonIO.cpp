// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "CommonIO.h"
#include <libdevcore/FileSystem.h>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <stdio.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <termios.h>
#endif
#include "Exceptions.h"
#include <boost/filesystem.hpp>
using namespace std;
using namespace dev;

namespace fs = boost::filesystem;

namespace dev
{
namespace
{
void createDirectoryIfNotExistent(boost::filesystem::path const& _path)
{
    if (!fs::exists(_path))
    {
        fs::create_directories(_path);
        DEV_IGNORE_EXCEPTIONS(fs::permissions(_path, fs::owner_all));
    }
}

}  // namespace

string memDump(bytes const& _bytes, unsigned _width, bool _html)
{
    stringstream ret;
    if (_html)
        ret << "<pre style=\"font-family: Monospace,Lucida Console,Courier,Courier New,sans-serif; font-size: small\">";
    for (unsigned i = 0; i < _bytes.size(); i += _width)
    {
        ret << hex << setw(4) << setfill('0') << i << " ";
        for (unsigned j = i; j < i + _width; ++j)
            if (j < _bytes.size())
                if (_bytes[j] >= 32 && _bytes[j] < 127)
                    if ((char)_bytes[j] == '<' && _html)
                        ret << "&lt;";
                    else if ((char)_bytes[j] == '&' && _html)
                        ret << "&amp;";
                    else
                        ret << (char)_bytes[j];
                else
                    ret << '?';
            else
                ret << ' ';
        ret << " ";
        for (unsigned j = i; j < i + _width && j < _bytes.size(); ++j)
            ret << setfill('0') << setw(2) << hex << (unsigned)_bytes[j] << " ";
        ret << "\n";
    }
    if (_html)
        ret << "</pre>";
    return ret.str();
}

template <typename _T>
inline _T contentsGeneric(boost::filesystem::path const& _file)
{
    _T ret;
    size_t const c_elementSize = sizeof(typename _T::value_type);
    boost::filesystem::ifstream is(_file, std::ifstream::binary);
    if (!is)
        return ret;

    // get length of file:
    is.seekg(0, is.end);
    streamoff length = is.tellg();
    if (length == 0)
        return ret; // do not read empty file (MSVC does not like it)
    is.seekg(0, is.beg);

    ret.resize((length + c_elementSize - 1) / c_elementSize);
    is.read(const_cast<char*>(reinterpret_cast<char const*>(ret.data())), length);
    return ret;
}

bytes contents(boost::filesystem::path const& _file)
{
    return contentsGeneric<bytes>(_file);
}

bytesSec contentsSec(boost::filesystem::path const& _file)
{
    bytes b = contentsGeneric<bytes>(_file);
    bytesSec ret(b);
    bytesRef(&b).cleanse();
    return ret;
}

string contentsString(boost::filesystem::path const& _file)
{
    return contentsGeneric<string>(_file);
}

void writeFile(boost::filesystem::path const& _file, bytesConstRef _data, bool _writeDeleteRename)
{
    if (_writeDeleteRename)
    {
        fs::path tempPath = appendToFilename(_file, "-%%%%%%"); // XXX should not convert to string for this
        writeFile(tempPath, _data, false);
        // will delete _file if it exists
        fs::rename(tempPath, _file);
    }
    else
    {
        createDirectoryIfNotExistent(_file.parent_path());

        boost::filesystem::ofstream s(_file, ios::trunc | ios::binary);
        s.write(reinterpret_cast<char const*>(_data.data()), _data.size());
        if (!s)
            BOOST_THROW_EXCEPTION(FileError() << errinfo_comment("Could not write to file: " + _file.string()));
        DEV_IGNORE_EXCEPTIONS(fs::permissions(_file, fs::owner_read | fs::owner_write));
    }
}

void copyDirectory(boost::filesystem::path const& _srcDir, boost::filesystem::path const& _dstDir)
{
    createDirectoryIfNotExistent(_dstDir);

    for (fs::directory_iterator file(_srcDir); file != fs::directory_iterator(); ++file)
        fs::copy_file(file->path(), _dstDir / file->path().filename());
}

std::string getPassword(std::string const& _prompt)
{
#if defined(_WIN32)
    cout << _prompt << flush;
    // Get current Console input flags
    HANDLE hStdin;
    DWORD fdwSaveOldMode;
    if ((hStdin = GetStdHandle(STD_INPUT_HANDLE)) == INVALID_HANDLE_VALUE)
        BOOST_THROW_EXCEPTION(
            ExternalFunctionFailure() << errinfo_externalFunction("GetStdHandle"));
    if (!GetConsoleMode(hStdin, &fdwSaveOldMode))
        BOOST_THROW_EXCEPTION(
            ExternalFunctionFailure() << errinfo_externalFunction("GetConsoleMode"));
    // Set console flags to no echo
    if (!SetConsoleMode(hStdin, fdwSaveOldMode & (~ENABLE_ECHO_INPUT)))
        BOOST_THROW_EXCEPTION(
            ExternalFunctionFailure() << errinfo_externalFunction("SetConsoleMode"));
    // Read the string
    std::string ret;
    std::getline(cin, ret);
    // Restore old input mode
    if (!SetConsoleMode(hStdin, fdwSaveOldMode))
        BOOST_THROW_EXCEPTION(
            ExternalFunctionFailure() << errinfo_externalFunction("SetConsoleMode"));
    return ret;
#else
    struct termios oflags;
    struct termios nflags;
    char password[256];

    // disable echo in the terminal
    tcgetattr(fileno(stdin), &oflags);
    nflags = oflags;
    nflags.c_lflag &= ~ECHO;
    nflags.c_lflag |= ECHONL;

    if (tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0)
        BOOST_THROW_EXCEPTION(ExternalFunctionFailure() << errinfo_externalFunction("tcsetattr"));

    printf("%s", _prompt.c_str());
    if (!fgets(password, sizeof(password), stdin))
        BOOST_THROW_EXCEPTION(ExternalFunctionFailure() << errinfo_externalFunction("fgets"));
    password[strlen(password) - 1] = 0;

    // restore terminal
    if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0)
        BOOST_THROW_EXCEPTION(ExternalFunctionFailure() << errinfo_externalFunction("tcsetattr"));


    return password;
#endif
}

}  // namespace dev