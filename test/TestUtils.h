
#pragma once

#include <string>

namespace dev
{
namespace test
{

bool getCommandLineOption(std::string const& _name);
std::string getCommandLineArgument(std::string const& _name, bool _require = false);
std::string loadFile(std::string const& _path);
std::string loadTestFile(std::string const& _filename);

}
}
