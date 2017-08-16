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
#include <test/tools/libtesteth/Options.h>
#include <test/tools/fuzzTesting/fuzzHelper.h>

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
	cout << setw(30) << "--testpath <PathToTheTestRepo>" << std::endl;

	cout << std::endl << "Debugging" << std::endl;
	cout << setw(30) << "-d <index>" << setw(25) << "Set the transaction data array index when running GeneralStateTests" << std::endl;
	cout << setw(30) << "-g <index>" << setw(25) << "Set the transaction gas array index when running GeneralStateTests" << std::endl;
	cout << setw(30) << "-v <index>" << setw(25) << "Set the transaction value array index when running GeneralStateTests" << std::endl;
	cout << setw(30) << "--singletest <TestName>" << setw(25) << "Run on a single test" << std::endl;
	cout << setw(30) << "--singletest <TestFile> <TestName>" << std::endl;
	cout << setw(30) << "--verbosity <level>" << setw(25) << "Set logs verbosity. 0 - silent, 1 - only errors, 2 - informative, >2 - detailed" << std::endl;
	cout << setw(30) << "--vm <interpreter|jit|smart>" << setw(25) << "Set VM type for VMTests suite" << std::endl;
	cout << setw(30) << "--vmtrace" << setw(25) << "Enable VM trace for the test. (Require build with VMTRACE=1)" << std::endl;
	cout << setw(30) << "--jsontrace <Options>" << setw(25) << "Enable VM trace to stdout in json format. Argument is a json config: '{ \"disableStorage\" : false, \"disableMemory\" : false, \"disableStack\" : false, \"fullStorage\" : true }'" << std::endl;
	cout << setw(30) << "--stats <OutFile>" << setw(25) << "Output debug stats to the file" << std::endl;
	cout << setw(30) << "--exectimelog" << setw(25) << "Output execution time for each test suite" << std::endl;
	cout << setw(30) << "--statediff" << setw(25) << "Trace state difference for state tests" << std::endl;

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
	cout << setw(30) << "--fillchain" << setw(25) << "When filling the state tests, fill tests as blockchain instead" << std::endl;
	cout << setw(30) << "--randomcode <MaxOpcodeNum>" << setw(25) << "Generate smart random EVM code" << std::endl;
	cout << setw(30) << "--createRandomTest" << setw(25) << "Create random test and output it to the console" << std::endl;
	//cout << setw(30) << "--fulloutput" << setw(25) << "Disable address compression in the output field" << std::endl;

	cout << setw(30) << "--help" << setw(25) << "Display list of command arguments" << std::endl;
}

StandardTrace::DebugOptions debugOptions(Json::Value const& _json)
{
	StandardTrace::DebugOptions op;
	if (!_json.isObject() || _json.empty())
		return op;
	if (!_json["disableStorage"].empty())
		op.disableStorage = _json["disableStorage"].asBool();
	if (!_json["disableMemory"].empty())
		op.disableMemory = _json["disableMemory"].asBool();
	if (!_json["disableStack"].empty())
		op.disableStack =_json["disableStack"].asBool();
	if (!_json["fullStorage"].empty())
		op.fullStorage = _json["fullStorage"].asBool();
	return op;
}

Options::Options(int argc, char** argv)
{
	trDataIndex = -1;
	trGasIndex = -1;
	trValueIndex = -1;
	bool seenSeparator = false; // true if "--" has been seen.
	for (auto i = 0; i < argc; ++i)
	{
		auto arg = std::string{argv[i]};
		auto throwIfNoArgumentFollows = [&i, &argc, &arg]()
		{
			if (i + 1 >= argc)
				BOOST_THROW_EXCEPTION(InvalidOption(arg + " option is missing an argument."));
		};
		auto throwIfAfterSeparator = [&seenSeparator, &arg]()
		{
			if (seenSeparator)
				BOOST_THROW_EXCEPTION(InvalidOption(arg + " option appears after the separator --."));
		};
		if (arg == "--")
		{
			if (seenSeparator)
				BOOST_THROW_EXCEPTION(InvalidOption("The separator -- appears more than once in the command line."));
			seenSeparator = true;
			continue;
		}
		if (arg == "--help")
		{
			printHelp();
			exit(0);
		}
		else
		if (arg == "--vm")
		{
			throwIfNoArgumentFollows();
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
		else if (arg == "--jsontrace")
		{
			throwIfNoArgumentFollows();
			jsontrace = true;
			auto arg = std::string{argv[++i]};
			Json::Value value;
			Json::Reader().parse(arg, value);
			jsontraceOptions = debugOptions(value);
		}
		else if (arg == "--filltests")
			filltests = true;
		else if (arg == "--fillchain")
			fillchain = true;
		else if (arg == "--stats")
		{
			throwIfNoArgumentFollows();
			stats = true;
			statsOutFile = argv[++i];
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
		else if (arg == "--singletest")
		{
			throwIfNoArgumentFollows();
			singleTest = true;
			auto name1 = std::string{argv[++i]};
			if (i + 1 < argc) // two params
			{
				auto name2 = std::string{argv[++i]};
				if (name2[0] == '-') // not param, another option
				{
					singleTestName = std::move(name1);
					i--;
				}
				else
				{
					singleTestFile = std::move(name1);
					singleTestName = std::move(name2);
				}
			}
			else
				singleTestName = std::move(name1);
		}
		else if (arg == "--singlenet")
		{
			throwIfNoArgumentFollows();
			singleTestNet = std::string{argv[++i]};
			ImportTest::checkAllowedNetwork({singleTestNet});
		}
		else if (arg == "--fulloutput")
			fulloutput = true;
		else if (arg == "--verbosity")
		{
			throwIfNoArgumentFollows();
			static std::ostringstream strCout; //static string to redirect logs to
			std::string indentLevel = std::string{argv[++i]};
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

			int indentLevelInt = atoi(argv[i]);
			if (indentLevelInt > g_logVerbosity)
				g_logVerbosity = indentLevelInt;
		}
		else if (arg == "--createRandomTest")
			createRandomTest = true;
		else if (arg == "-t")
		{
			throwIfAfterSeparator();
			throwIfNoArgumentFollows();
			rCurrentTestSuite = std::string{argv[++i]};
		}
		else if (arg == "--nonetwork")
			nonetwork = true;
		else if (arg == "-d")
		{
			throwIfNoArgumentFollows();
			trDataIndex = atoi(argv[++i]);
		}
		else if (arg == "-g")
		{
			throwIfNoArgumentFollows();
			trGasIndex = atoi(argv[++i]);
		}
		else if (arg == "-v")
		{
			throwIfNoArgumentFollows();
			trValueIndex = atoi(argv[++i]);
		}
		else if (arg == "--testpath")
		{
			throwIfNoArgumentFollows();
			testpath = std::string{argv[++i]};
		}
		else if (arg == "--statediff")
			statediff = true;
		else if (arg == "--randomcode")
		{
			throwIfNoArgumentFollows();
			int maxCodes = atoi(argv[++i]);
			if (maxCodes > 1000 || maxCodes <= 0)
			{
				cerr << "Argument for the option is invalid! (use range: 1...1000)" << endl;
				exit(1);
			}
			cout << dev::test::RandomCode::generate(maxCodes) << endl;
			exit(0);
		}
		else if (arg == "--createRandomTest")
			createRandomTest = true;
		else if (seenSeparator)
		{
			cerr << "Unknown option: " + arg << endl;
			exit(1);
		}
	}

	//check restrickted options
	if (createRandomTest)
	{
		string optionlist = "trValueIndex, trGasIndex, trDataIndex, nonetwork, singleTest, performance, quadratic, memory, inputLimits, bigData, wallet, stats, filltests, fillchain";
		if (trValueIndex >= 0 || trGasIndex >= 0 || trDataIndex >= 0 || nonetwork || singleTest
			|| 	performance || quadratic || memory || inputLimits || bigData || wallet
			|| 	stats || filltests || fillchain)
		{
			cerr << "--createRandomTest could not be used with one of the options: " + optionlist << endl;
			exit(1);
		}
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

