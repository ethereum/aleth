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
/** @file
 * Class for handling testeth custom options
 */

#pragma once
#include <test/tools/libtestutils/Common.h>
#include <test/tools/libtesteth/JsonSpiritHeaders.h>
#include <libdevcore/Exceptions.h>

namespace dev
{
namespace test
{

enum class Verbosity
{
	Full,
	NiceReport,
	None
};

class Options
{
public:
	struct InvalidOption: public Exception
	{
		InvalidOption(std::string _message = std::string()): Exception(_message) {}
	};

	bool vmtrace = false;	///< Create EVM execution tracer
	bool filltests = false; ///< Create JSON test files from execution results
	bool fillchain = false; ///< Fill tests as a blockchain tests if possible
	bool stats = false;		///< Execution time and stats for state tests
	std::string statsOutFile; ///< Stats output file. "out" for standard output
	bool exectimelog = false; ///< Print execution time for each test suite
	std::string rCurrentTestSuite; ///< Remember test suite before boost overwrite (for random tests)
	bool checkstate = false;///< Throw error when checking test states
	bool statediff = false;///< Fill full post state in General tests
	bool fulloutput = false;///< Replace large output to just it's length
	bool createRandomTest = false; ///< Generate random test
	std::string testpath;	///< Custom test folder path
	Verbosity logVerbosity = Verbosity::NiceReport;

	/// Test selection
	/// @{
	bool singleTest = false;
	std::string singleTestFile;
	std::string singleTestName;
	std::string singleTestNet;
	int trDataIndex;	///< GeneralState data
	int trGasIndex;		///< GeneralState gas
	int trValueIndex;	///< GeneralState value
	bool performance = false;
	bool nonetwork = false;///< For libp2p
	bool quadratic = false;///< Time consuming tests
	bool memory = false;
	bool inputLimits = false;
	bool bigData = false;
	bool wallet = false;
	/// @}

	// Checks that all networks in the vector are valid. Exits otherwise.
	static void checkAllowedNetwork(std::vector<std::string> const& _networks);

	/// Get reference to options
	/// The first time used, options are parsed with argc, argv
	static Options const& get(int argc = 0, char** argv = 0);

private:
	Options(int argc = 0, char** argv = 0);
	Options(Options const&) = delete;
};

} //namespace test
} //namespace dev
