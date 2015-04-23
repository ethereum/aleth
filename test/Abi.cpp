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
/** @file Abi.cpp
 * @author Marek Kotewicz <marek@ethdev.com>
 * @date 2015
 */

#include <boost/test/unit_test.hpp>
#include <libabi/CommandLineInterface.h>
#include <libdevcore/TransientDirectory.h>
#include "TestUtils.h"

using namespace std;
using namespace dev;
using namespace dev::test;
using namespace dev::abi;

// issues:
// - when encoding methods with number of input params > 0, we need to add additional command line param
// - formatting negative ints is not correct (see encode_negative_int test_case)
// - usage of --format-prefix is not clear or do not work

BOOST_FIXTURE_TEST_SUITE(Abi, CoutFixture)

BOOST_AUTO_TEST_CASE(help)
{
	CommandLineInterface cli;
	const char* argv[2] = {"abi", "--help"};
	
	BOOST_CHECK_EQUAL(cli.parseArguments(2, (char**)argv), false);
	BOOST_CHECK_EQUAL(getOutput().length() > 0, true);
}

BOOST_AUTO_TEST_CASE(version)
{
	CommandLineInterface cli;
	const char* argv[2] = {"abi", "--version"};
	
	BOOST_CHECK_EQUAL(cli.parseArguments(2, (char**)argv), false);
	BOOST_CHECK_EQUAL(getOutput().length() > 0, true);
}

BOOST_AUTO_TEST_CASE(no_path)
{
	CommandLineInterface cli;
	const char* argv[5] = {"abi", "enc", "-a", "", "f"};
	
	BOOST_CHECK_EQUAL(cli.parseArguments(5, (char**)argv), true);
	BOOST_CHECK_EQUAL(cli.processInput(), false);
}

BOOST_AUTO_TEST_CASE(empty_file_or_not_existing_file)
{
	CommandLineInterface cli;
	const char* argv[5] = {"abi", "enc", "-a", "test899796", "f"};
	
	BOOST_CHECK_EQUAL(cli.parseArguments(5, (char**)argv), true);
	BOOST_CHECK_EQUAL(cli.processInput(), false);
}

BOOST_AUTO_TEST_CASE(simple_encode0)
{
	char const* interface = R"([
	{
		"name": "f",
		"constant": false,
		"type": "function",
		"inputs": [],
		"outputs": []
	}
	])";
	TransientFile f(interface);
	CommandLineInterface cli;
	const char* argv[5] = {"abi", "enc", "-a", f.path().c_str(), "f"};
	
	BOOST_CHECK_EQUAL(cli.parseArguments(5, (char**)argv), true);
	BOOST_CHECK_EQUAL(cli.processInput(), true);
	BOOST_CHECK_EQUAL(cli.actOnInput(), true);
	string output = getOutput();
	BOOST_CHECK_EQUAL(output, "26121ff0");
}

BOOST_AUTO_TEST_CASE(simple_encode1)
{
	char const* interface = R"([
	{
		"name": "f",
		"constant": false,
		"type": "function",
		"inputs": [],
		"outputs": []
	},
	{
		"name": "g",
		"constant": false,
		"type": "function",
		"inputs": [],
		"outputs": []
	}
	])";
	TransientFile f(interface);
	CommandLineInterface cli;
	const char* argv[5] = {"abi", "enc", "-a", f.path().c_str(), "g"};
	
	BOOST_CHECK_EQUAL(cli.parseArguments(5, (char**)argv), true);
	BOOST_CHECK_EQUAL(cli.processInput(), true);
	BOOST_CHECK_EQUAL(cli.actOnInput(), true);
	string output = getOutput();
	BOOST_CHECK_EQUAL(output, "e2179b8e");
}

BOOST_AUTO_TEST_CASE(simple_encode2)
{
	char const* interface = R"([
	{
		"name": "f",
		"constant": false,
		"type": "function",
		"inputs": [
		{
			"name": "a",
			"type": "uint256"
		}
		],
		"outputs": []
	},
	{
		"name": "g",
		"constant": false,
		"type": "function",
		"inputs": [],
		"outputs": []
	}
	])";
	TransientFile f(interface);
	CommandLineInterface cli;
	// TODO: 1 extra param required!?
	const char* argv[7] = {"abi", "enc", "-a", f.path().c_str(), "f", "0x12", ""};
	
	BOOST_CHECK_EQUAL(cli.parseArguments(7, (char**)argv), true);
	BOOST_CHECK_EQUAL(cli.processInput(), true);
	BOOST_CHECK_EQUAL(cli.actOnInput(), true);
	string output = getOutput();
	BOOST_CHECK_EQUAL(output, "b3de648b0000000000000000000000000000000000000000000000000000000000000012");
}

BOOST_AUTO_TEST_CASE(simple_encode3)
{
	char const* interface = R"([
	{
		"name": "f",
		"constant": false,
		"type": "function",
		"inputs": [
				   {
				   "name": "a",
				   "type": "bytes32"
				   }
				   ],
		"outputs": []
	},
	{
		"name": "g",
		"constant": false,
		"type": "function",
		"inputs": [],
		"outputs": []
	}
	])";
	TransientFile f(interface);
	CommandLineInterface cli;
	// TODO: 1 extra param required!?
	const char* argv[7] = {"abi", "enc", "-a", f.path().c_str(), "f", "0x12", ""};
	
	BOOST_CHECK_EQUAL(cli.parseArguments(7, (char**)argv), true);
	BOOST_CHECK_EQUAL(cli.processInput(), true);
	BOOST_CHECK_EQUAL(cli.actOnInput(), true);
	string output = getOutput();
	BOOST_CHECK_EQUAL(output, "d7da973a1200000000000000000000000000000000000000000000000000000000000000");
}

BOOST_AUTO_TEST_CASE(simple_encode4)
{
	char const* interface = R"([
	{
		"name": "f",
		"constant": false,
		"type": "function",
		"inputs": [
				   {
				   "name": "a",
				   "type": "bytes32"
				   },
				   {
				   "name": "b",
				   "type": "uint256"
				   }
				   ],
		"outputs": []
	},
	{
		"name": "g",
		"constant": false,
		"type": "function",
		"inputs": [],
		"outputs": []
	}
	])";
	TransientFile f(interface);
	CommandLineInterface cli;
	// TODO: 1 extra param required!?
	const char* argv[8] = {"abi", "enc", "-a", f.path().c_str(), "f", "0x12", "0x11", ""};
	
	BOOST_CHECK_EQUAL(cli.parseArguments(8, (char**)argv), true);
	BOOST_CHECK_EQUAL(cli.processInput(), true);
	BOOST_CHECK_EQUAL(cli.actOnInput(), true);
	string output = getOutput();
	BOOST_CHECK_EQUAL(output, (string("bea09669") +
					  "1200000000000000000000000000000000000000000000000000000000000000" +
					  "0000000000000000000000000000000000000000000000000000000000000011"));
}

BOOST_AUTO_TEST_CASE(encode_negative_int)
{
	char const* interface = R"([
	{
		"name": "f",
		"constant": false,
		"type": "function",
		"inputs": [
				   {
				   "name": "a",
				   "type": "int256"
				   }
				   ],
		"outputs": []
	},
	{
		"name": "g",
		"constant": false,
		"type": "function",
		"inputs": [],
		"outputs": []
	}
	])";
	TransientFile f(interface);
	CommandLineInterface cli;
	// TODO: 1 extra param required!?
	const char* argv[7] = {"abi", "enc", "-a", f.path().c_str(), "f", "-1", ""};
	
	BOOST_CHECK_EQUAL(cli.parseArguments(7, (char**)argv), true);
	BOOST_CHECK_EQUAL(cli.processInput(), true);
	BOOST_CHECK_EQUAL(cli.actOnInput(), true);
	string output = getOutput();
	BOOST_CHECK_EQUAL(output, "1c008df9ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
}

BOOST_AUTO_TEST_CASE(encode_negative_int_format_decimal)
{
	char const* interface = R"([
	{
		"name": "f",
		"constant": false,
		"type": "function",
		"inputs": [
				   {
				   "name": "a",
				   "type": "int256"
				   }
				   ],
		"outputs": []
	},
	{
		"name": "g",
		"constant": false,
		"type": "function",
		"inputs": [],
		"outputs": []
	}
	])";
	TransientFile f(interface);
	CommandLineInterface cli;
	// TODO: 1 extra param required!?
	const char* argv[8] = {"abi", "-f", "enc", "-a", f.path().c_str(), "f", ".-1", ""};
	
	BOOST_CHECK_EQUAL(cli.parseArguments(8, (char**)argv), true);
	BOOST_CHECK_EQUAL(cli.processInput(), true);
	BOOST_CHECK_EQUAL(cli.actOnInput(), true);
	string output = getOutput();
	BOOST_CHECK_EQUAL(output, "1c008df9ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
}

BOOST_AUTO_TEST_CASE(encode_with_formatter)
{
	char const* interface = R"([
	{
		"name": "f",
		"constant": false,
		"type": "function",
		"inputs": [
				   {
				   "name": "a",
				   "type": "int256"
				   }
				   ],
		"outputs": []
	},
	{
		"name": "g",
		"constant": false,
		"type": "function",
		"inputs": [],
		"outputs": []
	}
	])";
	TransientFile f(interface);
	CommandLineInterface cli;
	// TODO: 1 extra param required!?
	const char* argv[8] = {"abi", "enc", "-f", "-a", f.path().c_str(), "f", "0x1", ""};
	
	BOOST_CHECK_EQUAL(cli.parseArguments(8, (char**)argv), true);
	BOOST_CHECK_EQUAL(cli.processInput(), true);
	BOOST_CHECK_EQUAL(cli.actOnInput(), true);
	string output = getOutput();
	BOOST_CHECK_EQUAL(output, "1c008df90000000000000000000000000000000000000000000000000000000000000000");
}

BOOST_AUTO_TEST_SUITE_END()
