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
#include <chrono>
#include <fstream>
#include <iostream>
#include <libdevcore/FileSystem.h>
#include <libdevcore/Log.h>
#include <libethcore/KeyManager.h>
#include "BuildInfo.h"
#include "KeyAux.h"
using namespace std;
using namespace dev;
using namespace dev::eth;

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

int main(int argc, char** argv)
{
	KeyCLI m(KeyCLI::OperationMode::ListBare);
	g_logVerbosity = 0;

	for (int i = 1; i < argc; ++i)
	{
		string arg = argv[i];
		if (m.interpretOption(i, argc, argv)) {}
		else if ((arg == "-v" || arg == "--verbosity") && i + 1 < argc)
			g_logVerbosity = atoi(argv[++i]);
		else if (arg == "-h" || arg == "--help")
			help();
		else if (arg == "-V" || arg == "--version")
			version();
		else
		{
			cerr << "Invalid argument: " << arg << endl;
			exit(-1);
		}
	}

	m.execute();

	return 0;
}

