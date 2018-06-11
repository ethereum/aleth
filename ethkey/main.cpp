/*
    This file is part of cpp-ethereum.

    cpp-ethereum is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    cpp-ethereum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "KeyAux.h"

#include <libdevcore/FileSystem.h>
#include <libdevcore/LoggingProgramOptions.h>
#include <libethcore/KeyManager.h>

#include <aleth-buildinfo.h>

#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>

#include <fstream>
#include <iostream>
#include <thread>

using namespace std;
using namespace dev;
using namespace dev::eth;

namespace po = boost::program_options;

void version()
{
    auto const* const buildinfo = aleth_get_buildinfo();
    cout << "ethkey " << buildinfo->project_version << "\nBuild: " << buildinfo->system_name << "/"
         << buildinfo->build_type << endl;
    exit(0);
}

/*
The equivalent of setlocale(LC_ALL, “C”) is called before any user code is run.
If the user has an invalid environment setting then it is possible for the call
to set locale to fail, so there are only two possible actions, the first is to
throw a runtime exception and cause the program to quit (default behaviour),
or the second is to modify the environment to something sensible (least
surprising behaviour).

The follow code produces the least surprising behaviour. It will use the user
specified default locale if it is valid, and if not then it will modify the
environment the process is running in to use a sensible default. This also means
that users do not need to install language packs for their OS.
*/
void setDefaultOrCLocale()
{
#if __unix__
    if (!std::setlocale(LC_ALL, ""))
    {
        setenv("LC_ALL", "C", 1);
    }
#endif
}

int main(int argc, char** argv)
{
    setDefaultOrCLocale();
    KeyCLI m(KeyCLI::OperationMode::ListBare);

    LoggingOptions loggingOptions;
    po::options_description loggingProgramOptions(createLoggingProgramOptions(
        po::options_description::m_default_line_length, loggingOptions));

    po::options_description generalOptions("General Options");
    auto addOption = generalOptions.add_options();
    addOption("version,V", "Show the version and exit.");
    addOption("help,h", "Show this help message and exit.");

    po::options_description allowedOptions("Allowed options");
    allowedOptions.add(loggingProgramOptions).add(generalOptions);

    po::variables_map vm;
    vector<string> unrecognisedOptions;
    try
    {
        po::parsed_options parsed =
            po::command_line_parser(argc, argv).options(allowedOptions).allow_unregistered().run();
        unrecognisedOptions = collect_unrecognized(parsed.options, po::include_positional);
        po::store(parsed, vm);
        po::notify(vm);
    }
    catch (po::error const& e)
    {
        cerr << e.what();
        return -1;
    }

    for (size_t i = 0; i < unrecognisedOptions.size(); ++i)
        if (!m.interpretOption(i, unrecognisedOptions))
        {
            cerr << "Invalid argument: " << unrecognisedOptions[i] << endl;
            return -1;
        }

    if (vm.count("help"))
    {
        cout
            << "Usage ethkey [OPTIONS]" << endl
            << "Options:" << endl << endl;
        KeyCLI::streamHelp(cout);
        cout << allowedOptions;
        return 0;
    }
    if (vm.count("version"))
        version();

    setupLogging(loggingOptions);

    m.execute();

    return 0;
}