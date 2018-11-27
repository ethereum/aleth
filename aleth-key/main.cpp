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

#include <aleth/buildinfo.h>

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
    const auto* buildinfo = aleth_get_buildinfo();
    cout << "aleth-key " << buildinfo->project_version << "\nBuild: " << buildinfo->system_name << "/"
         << buildinfo->build_type << endl;
    exit(0);
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
            << "Usage aleth-key [OPTIONS]" << endl
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