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
/** CommandLineInterface.cpp
 * @author Gav Wood <i@gavwood.com>
 * @author Marek Kotewicz <marek@ethdev.com>
 * @date 2015
 */

#include <boost/algorithm/string.hpp>
#include <libabi/Abi.h>
#include <libdevcore/CommonIO.h>
#include "CommandLineInterface.h"

using namespace std;
using namespace dev;
using namespace dev::abi;

void help()
{
	cout
	<< "Usage abi enc <method_name> (<arg1>, (<arg2>, ... ))" << endl
	<< "      abi enc -a <abi.json> <method_name> (<arg1>, (<arg2>, ... ))" << endl
	<< "      abi dec -a <abi.json> [ <signature> | <unique_method_name> ]" << endl
	<< "Options:" << endl
	<< "    -a,--abi-file <filename>  Specify the JSON ABI file." << endl
	<< "    -h,--help  Print this help message and exit." << endl
	<< "    -V,--version  Show the version and exit." << endl
	<< "Input options:" << endl
	<< "    -f,--format-prefix  Require all input formats to be prefixed e.g. 0x for hex, . for decimal, @ for binary." << endl
	<< "    -F,--no-format-prefix  Require no input format to be prefixed." << endl
	<< "    -t,--typing  Require all arguments to be typed e.g. b32: (bytes32), u64: (uint64), b[]: (byte[]), i: (int256)." << endl
	<< "    -T,--no-typing  Require no arguments to be typed." << endl
	<< "Output options:" << endl
	<< "    -i,--index <n>  Output only the nth (counting from 0) return value." << endl
	<< "    -d,--decimal  All data should be displayed as decimal." << endl
	<< "    -x,--hex  Display all data as hex." << endl
	<< "    -b,--binary  Display all data as binary." << endl
	<< "    -p,--prefix  Prefix by a base identifier." << endl;
}

void version()
{
	cout << "abi version " << dev::Version << endl;
}

bool CommandLineInterface::parseArguments(int argc, char **argv)
{	
	for (int i = 1; i < argc; ++i)
	{
		string arg = argv[i];
		if (arg == "-h" || arg == "--help")
		{
			help();
			return false;
		}
		else if (arg == "enc" && i == 1)
			mode = Mode::Encode;
		else if (arg == "dec" && i == 1)
			mode = Mode::Decode;
		else if ((arg == "-a" || arg == "--abi") && argc > i)
			abiFile = argv[++i];
		else if ((arg == "-i" || arg == "--index") && argc > i)
			outputIndex = atoi(argv[++i]);
		else if (arg == "-p" || arg == "--prefix")
			prefs.prefix = true;
		else if (arg == "-f" || arg == "--format-prefix")
			formatPrefix = Tristate::True;
		else if (arg == "-F" || arg == "--no-format-prefix")
			formatPrefix = Tristate::False;
		else if (arg == "-t" || arg == "--typing")
			typePrefix = Tristate::True;
		else if (arg == "-T" || arg == "--no-typing")
			typePrefix = Tristate::False;
		else if (arg == "-x" || arg == "--hex")
			prefs.e = Encoding::Hex;
		else if (arg == "-d" || arg == "--decimal" || arg == "--dec")
			prefs.e = Encoding::Decimal;
		else if (arg == "-b" || arg == "--binary" || arg == "--bin")
			prefs.e = Encoding::Binary;
		else if (arg == "-v" || arg == "--verbose")
			verbose = true;
		else if (arg == "-V" || arg == "--version")
		{
			version();
			return false;
		}
		else if (method.empty())
			method = arg;
		else if (mode == Mode::Encode)
		{
			auto u = fromUser(arg, formatPrefix, typePrefix);
			args.push_back(get<1>(u));
			params.push_back(make_pair(get<0>(u), get<2>(u)));
		}
		else if (mode == Mode::Decode)
			incoming += arg;
	}
	return true;
}

bool CommandLineInterface::processInput()
{
	if (abiFile.empty())
	{
		cerr << "No file path specified." << endl;
		return false;
	}
	abiData = contentsString(abiFile);
	if (abiData.empty())
	{
		cerr << "Specified file is empty." << endl;
		return false;
	}
	return true;
}

bool CommandLineInterface::actOnInput()
{
	if (mode == Mode::Encode)
	{
		ABIMethod m;
		if (abiData.empty())
			m = ABIMethod(method, args);
		else
		{
			ABI abi(abiData);
			if (verbose)
				cerr << "ABI:" << endl << abi;
			try {
				m = abi.method(method, args);
			}
			catch (...)
			{
				cerr << "Unknown method in ABI." << endl;
				return false;
			}
		}
		try {
			userOutput(cout, m.encode(params), encoding);
		}
		catch (ExpectedAdditionalParameter const&)
		{
			cerr << "Expected additional parameter in input." << endl;
			return false;
		}
		catch (ExpectedOpen const&)
		{
			cerr << "Expected open-bracket '[' in input." << endl;
			return false;
		}
		catch (ExpectedClose const&)
		{
			cerr << "Expected close-bracket ']' in input." << endl;
			return false;
		}
	}
	else if (mode == Mode::Decode)
	{
		if (abiData.empty())
		{
			cerr << "Please specify an ABI file." << endl;
			return false;
		}
		else
		{
			ABI abi(abiData);
			ABIMethod m;
			if (verbose)
				cerr << "ABI:" << endl << abi;
			try {
				m = abi.method(method, args);
			}
			catch(...)
			{
				cerr << "Unknown method in ABI." << endl;
				return false;
			}
			string encoded;
			if (incoming == "--" || incoming.empty())
				for (int i = cin.get(); i != -1; i = cin.get())
					encoded.push_back((char)i);
			else
			{
				encoded = contentsString(incoming);
			}
			cout << m.decode(fromHex(boost::trim_copy(encoded)), outputIndex, prefs) << endl;
		}
	}
	return true;
}


