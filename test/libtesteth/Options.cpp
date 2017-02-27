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

#include <libevm/VMFactory.h>
#include <test/libtesteth/Options.h>

using namespace std;
using namespace dev::test;
using namespace dev::eth;

void printHelp()
{
	cout << "Usage: " << std::endl;
	cout << std::left;
	cout << std::endl << "Setting test suite" << std::endl;
	cout << setw(30) <<	"-t <TestSuite>" << setw(25) << "Execute test operations" << std::endl;
	cout << setw(30) << "-t <TestSuite>/<TestCase>" << std::endl;

	cout << std::endl << "Debugging" << std::endl;
	cout << setw(30) << "-d <index>" << setw(25) << "Set the transaction data array index when running GeneralStateTests" << std::endl;
	cout << setw(30) << "-g <index>" << setw(25) << "Set the transaction gas array index when running GeneralStateTests" << std::endl;
	cout << setw(30) << "-v <index>" << setw(25) << "Set the transaction value array index when running GeneralStateTests" << std::endl;
	cout << setw(30) << "--singletest <TestName>" << setw(25) << "Run on a single test" << std::endl;
	cout << setw(30) << "--singletest <TestFile> <TestName>" << std::endl;
	cout << setw(30) << "--verbosity <level>" << setw(25) << "Set logs verbosity. 0 - silent, 1 - only errors, 2 - informative, >2 - detailed" << std::endl;
	cout << setw(30) << "--vm <interpreter|jit|smart>" << setw(25) << "Set VM type for VMTests suite" << std::endl;
	cout << setw(30) << "--vmtrace" << setw(25) << "Enable VM trace for the test. (Require build with VMTRACE=1)" << std::endl;
	cout << setw(30) << "--stats <OutFile>" << setw(25) << "Output debug stats to the file" << std::endl;
	cout << setw(30) << "--exectimelog" << setw(25) << "Output execution time for each test suite" << std::endl;
	cout << setw(30) << "--filltest <FileData>" << setw(25) << "Try fill tests from the given json stream" << std::endl;
	cout << setw(30) << "--checktest <FileData>" << setw(25) << "Try run tests from the given json stream" << std::endl;

	cout << std::endl << "Additional Tests" << std::endl;
	cout << setw(30) << "--performance" << setw(25) << "Enable perfomance tests" << std::endl;
	cout << setw(30) << "--quadratic" << setw(25) << "Enable quadratic complexity tests" << std::endl;
	cout << setw(30) << "--memory" << setw(25) << "Enable memory consuming tests" << std::endl;
	cout << setw(30) << "--inputLimits" << setw(25) << "Enable inputLimits tests" << std::endl;
	cout << setw(30) << "--bigdata" << setw(25) << "Enable bigdata tests" << std::endl;
	cout << setw(30) << "--wallet" << setw(25) << "Enable wallet tests" << std::endl;
	cout << setw(30) << "--all" << setw(25) << "Enable all tests" << std::endl;

	cout << std::endl << "Test Generation" << std::endl;
	cout << setw(30) << "--filltests" << setw(25) << "Run test fillers" << std::endl;
	cout << setw(30) << "--checkstate" << setw(25) << "Enable expect section state checks" << std::endl;
	cout << setw(30) << "--fillchain" << setw(25) << "When filling the state tests, fill tests as blockchain instead" << std::endl;
	cout << setw(30) << "--createRandomTest" << setw(25) << "Create random test and output it to the console" << std::endl;
	//cout << setw(30) << "--fulloutput" << setw(25) << "Disable address compression in the output field" << std::endl;

	cout << setw(30) << "--help" << setw(25) << "Display list of command arguments" << std::endl;
}

Options::Options(int argc, char** argv)
{
	trDataIndex = -1;
	trGasIndex = -1;
	trValueIndex = -1;
	for (auto i = 0; i < argc; ++i)
	{
		auto arg = std::string{argv[i]};
		if (arg == "--help")
		{
			printHelp();
			exit(0);
		}
		else
		if (arg == "--vm" && i + 1 < argc)
		{
			string vmKind = argv[++i];
			if (vmKind == "interpreter")
				VMFactory::setKind(VMKind::Interpreter);
			else if (vmKind == "jit")
				VMFactory::setKind(VMKind::JIT);
			else if (vmKind == "smart")
				VMFactory::setKind(VMKind::Smart);
			else
				cerr << "Unknown VM kind: " << vmKind << endl;
		}
		else if (arg == "--jit") // TODO: Remove deprecated option "--jit"
			VMFactory::setKind(VMKind::JIT);
		else if (arg == "--vmtrace")
		{
			vmtrace = true;
			g_logVerbosity = 13;
		}
		else if (arg == "--filltests")
			filltests = true;
		else if (arg == "--fillchain")
			fillchain = true;
		else if (arg == "--stats" && i + 1 < argc)
		{
			stats = true;
			statsOutFile = argv[i + 1];
		}
		else if (arg == "--exectimelog")
			exectimelog = true;
		else if (arg == "--performance")
			performance = true;
		else if (arg == "--quadratic")
			quadratic = true;
		else if (arg == "--memory")
			memory = true;
		else if (arg == "--inputlimits")
			inputLimits = true;
		else if (arg == "--bigdata")
			bigData = true;
		else if (arg == "--checkstate")
			checkstate = true;
		else if (arg == "--wallet")
			wallet = true;
		else if (arg == "--all")
		{
			performance = true;
			quadratic = true;
			memory = true;
			inputLimits = true;
			bigData = true;
			wallet = true;
		}
		else if (arg == "--singletest" && i + 1 < argc)
		{
			singleTest = true;
			auto name1 = std::string{argv[i + 1]};
			if (i + 2 < argc) // two params
			{
				auto name2 = std::string{argv[i + 2]};
				if (name2[0] == '-') // not param, another option
					singleTestName = std::move(name1);
				else
				{
					singleTestFile = std::move(name1);
					singleTestName = std::move(name2);
				}
			}
			else
				singleTestName = std::move(name1);
		}
		else if (arg == "--singlenet" && i + 1 < argc)
			singleTestNet = std::string{argv[i + 1]};
		else if (arg == "--fulloutput")
			fulloutput = true;
		else if (arg == "--verbosity" && i + 1 < argc)
		{
			static std::ostringstream strCout; //static string to redirect logs to
			std::string indentLevel = std::string{argv[i + 1]};
			if (indentLevel == "0")
			{
				logVerbosity = Verbosity::None;
				std::cout.rdbuf(strCout.rdbuf());
				std::cerr.rdbuf(strCout.rdbuf());
			}
			else if (indentLevel == "1")
				logVerbosity = Verbosity::NiceReport;
			else
				logVerbosity = Verbosity::Full;

			int indentLevelInt = atoi(argv[i + 1]);
			if (indentLevelInt > g_logVerbosity)
				g_logVerbosity = indentLevelInt;
		}
		else if (arg == "--createRandomTest")
			createRandomTest = true;
		else if (arg == "-t" && i + 1 < argc)
			rCurrentTestSuite = std::string{argv[i + 1]};
		else if (arg == "--checktest" || arg == "--filltest")
		{
			//read all line to the end
			for (int j = i+1; j < argc; ++j)
				rCheckTest += argv[j];
			break;
		}
		else if (arg == "--nonetwork")
			nonetwork = true;
		else if (arg == "-d" && i + 1 < argc)
			trDataIndex = atoi(argv[i + 1]);
		else if (arg == "-g" && i + 1 < argc)
			trGasIndex = atoi(argv[i + 1]);
		else if (arg == "-v" && i + 1 < argc)
			trValueIndex = atoi(argv[i + 1]);
	}

	//Default option
	if (logVerbosity == Verbosity::NiceReport)
		g_logVerbosity = -1;	//disable cnote but leave cerr and cout
}

Options const& Options::get(int argc, char** argv)
{
	static Options instance(argc, argv);
	return instance;
}

