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
/** @file main.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 * Ethereum client.
 */

#include <thread>
#include <fstream>
#include <iostream>
#include <libdevcore/FileSystem.h>
#include <libdevcore/Log.h>
#include <libethcore/KeyManager.h>
#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>
#include "BuildInfo.h"
#include "KeyAux.h"
using namespace std;
using namespace dev;
using namespace dev::eth;

namespace po = boost::program_options;

void help()
{
	cout
		<< "Usage ethkey [OPTIONS]" << endl
		<< "Options:" << endl << endl;
	KeyCLI::streamHelp(cout);
	cout
		<< "General Options:" << endl
		<< "    -v,--verbosity <0 - 9>  Set the log verbosity from 0 to 9 (default: 8)." << endl
		<< "    -V,--version  Show the version and exit." << endl
		<< "    -h,--help  Show this help message and exit." << endl
		;
	exit(0);
}

void version()
{
	cout << "ethkey version " << dev::Version << endl;
	cout << "Build: " << DEV_QUOTED(ETH_BUILD_PLATFORM) << "/" << DEV_QUOTED(ETH_BUILD_TYPE) << endl;
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
	g_logVerbosity = 0;
	po::options_description generalOptions("General Options");
	generalOptions.add_options()
		("verbosity,v", po::value<string>(), "<0 - 9>  Set the log verbosity from 0 to 9 (default: 8).")
		("version,V", "Show the version and exit.")
		("help,h",  "Show this help message and exit.")
	;
	int ac = 1;
	for (int i = 1; i < argc; ++i)
	{
		string arg = argv[i];
		if (m.interpretOption(i, argc, argv)) {}
		else if (((arg == "-v" || arg == "--verbosity") && i + 1 < argc) || arg == "-h" || arg == "--help" || arg == "-V" || arg == "--version") {
			argv[ac] = argv[i];
			ac++;
			if (arg == "-v" || arg == "--verbosity") {
				i++;
				argv[ac] = argv[i];
				ac++;
			}
			continue;
		}
		else
		{
			cerr << "Invalid argument: " << arg << endl;
			exit(-1);
		}
	}
	po::variables_map vm;
	po::store(po::parse_command_line(ac, argv, generalOptions), vm);
	po::notify(vm);
	if (vm.count("help")) {
		cout
				<< "Usage ethkey [OPTIONS]" << endl
				<< "Options:" << endl << endl;
		KeyCLI::streamHelp(cout);
		cout << generalOptions;
		exit(0);
	}
	if (vm.count("version"))
		version();
	if (vm.count("verbosity"))
		g_logVerbosity = atoi(vm["verbosity"].as<string>().c_str());

	m.execute();

	return 0;
}
