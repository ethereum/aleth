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
#include <libweb3jsonrpc/Debug.h>
#include <test/tools/libtesteth/Options.h>
#include <test/tools/fuzzTesting/fuzzHelper.h>

using namespace std;
using namespace dev::test;
using namespace dev::eth;

void printHelp()
{
	cout << "Usage: \n";
	cout << std::left;
	cout << "\nSetting test suite\n";
	cout << setw(30) <<	"-t <TestSuite>" << setw(25) << "Execute test operations\n";
	cout << setw(30) << "-t <TestSuite>/<TestCase>\n";
	cout << setw(30) << "--testpath <PathToTheTestRepo>\n";

	cout << "\nDebugging\n";
	cout << setw(30) << "-d <index>" << setw(25) << "Set the transaction data array index when running GeneralStateTests\n";
	cout << setw(30) << "-g <index>" << setw(25) << "Set the transaction gas array index when running GeneralStateTests\n";
	cout << setw(30) << "-v <index>" << setw(25) << "Set the transaction value array index when running GeneralStateTests\n";
	cout << setw(30) << "--singletest <TestName>" << setw(25) << "Run on a single test\n";
	cout << setw(30) << "--singletest <TestFile> <TestName>\n";
	cout << setw(30) << "--verbosity <level>" << setw(25) << "Set logs verbosity. 0 - silent, 1 - only errors, 2 - informative, >2 - detailed\n";
	cout << setw(30) << "--vm <interpreter|jit|smart>" << setw(25) << "Set VM type for VMTests suite\n";
	cout << setw(30) << "--vmtrace" << setw(25) << "Enable VM trace for the test. (Require build with VMTRACE=1)\n";
	cout << setw(30) << "--jsontrace <Options>" << setw(25) << "Enable VM trace to stdout in json format. Argument is a json config: '{ \"disableStorage\" : false, \"disableMemory\" : false, \"disableStack\" : false, \"fullStorage\" : true }'\n";
	cout << setw(30) << "--stats <OutFile>" << setw(25) << "Output debug stats to the file\n";
	cout << setw(30) << "--exectimelog" << setw(25) << "Output execution time for each test suite\n";
	cout << setw(30) << "--statediff" << setw(25) << "Trace state difference for state tests\n";

	cout << "\nAdditional Tests\n";
	cout << setw(30) << "--all" << setw(25) << "Enable all tests\n";

	cout << "\nTest Generation\n";
	cout << setw(30) << "--filltests" << setw(25) << "Run test fillers\n";
	cout << setw(30) << "--fillchain" << setw(25) << "When filling the state tests, fill tests as blockchain instead\n";
	cout << setw(30) << "--randomcode <MaxOpcodeNum>" << setw(25) << "Generate smart random EVM code\n";
	cout << setw(30) << "--createRandomTest" << setw(25) << "Create random test and output it to the console\n";
	cout << setw(30) << "--createRandomTest <PathToOptions.json>" << setw(25) << "Use following options file for random code generation\n";
	cout << setw(30) << "--seed <uint>" << setw(25) << "Define a seed for random test\n";
	cout << setw(30) << "--options <PathTo.json>" << setw(25) << "Use following options file for random code generation\n";
	//cout << setw(30) << "--fulloutput" << setw(25) << "Disable address compression in the output field\n";

	cout << setw(30) << "--help" << setw(25) << "Display list of command arguments\n";
	cout << setw(30) << "--version" << setw(25) << "Display build information\n";
}

void printVersion()
{
	cout << prepareVersionString() << "\n";
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
		else if (arg == "--version")
		{
			printVersion();
			exit(0);
		}
		else if (arg == "--vm")
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
				cerr << "Unknown VM kind: " << vmKind << "\n";
		}
		else if (arg == "--jit") // TODO: Remove deprecated option "--jit"
			VMFactory::setKind(VMKind::JIT);
		else if (arg == "--vmtrace")
		{
#if ETH_VMTRACE
			vmtrace = true;
			g_logVerbosity = 13;
#else
			cerr << "--vmtrace option requires a build with cmake -DVMTRACE=1\n";
			exit(1);
#endif
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
		else if (arg == "--all")
			all = true;
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
		else if (arg == "--options")
		{
			throwIfNoArgumentFollows();
			boost::filesystem::path file(std::string{argv[++i]});
			if (boost::filesystem::exists(file))
				randomCodeOptionsPath = file;
			else
			{
				std::cerr << "Options file not found! Default options at: tests/src/randomCodeOptions.json\n";
				exit(0);
			}
		}
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
				cerr << "Argument for the option is invalid! (use range: 1...1000)\n";
				exit(1);
			}
			test::RandomCodeOptions options;
			cout << test::RandomCode::get().generate(maxCodes, options) << "\n";
			exit(0);
		}
		else if (arg == "--createRandomTest")
		{
			createRandomTest = true;
			if (i + 1 < argc) // two params
			{
				auto options = std::string{argv[++i]};
				if (options[0] == '-') // not param, another option
					i--;
				else
				{
					boost::filesystem::path file(options);
					if (boost::filesystem::exists(file))
						randomCodeOptionsPath = file;
					else
						BOOST_THROW_EXCEPTION(InvalidOption("Options file not found! Default options at: tests/src/randomCodeOptions.json\n"));
				}
			}
		}
		else if (arg == "--seed")
		{
			throwIfNoArgumentFollows();
			u256 input = toInt(argv[++i]);
			if (input > std::numeric_limits<uint64_t>::max())
				BOOST_WARN("Seed is > u64. Using u64_max instead.");
			randomTestSeed = static_cast<uint64_t>(min<u256>(std::numeric_limits<uint64_t>::max(), input));
		}
		else if (seenSeparator)
		{
			cerr << "Unknown option: " + arg << "\n";
			exit(1);
		}
	}

	//check restrickted options
	if (createRandomTest)
	{
		if (trValueIndex >= 0 || trGasIndex >= 0 || trDataIndex >= 0 || nonetwork || singleTest
			|| all || stats || filltests || fillchain)
		{
			cerr << "--createRandomTest cannot be used with any of the options: " <<
					"trValueIndex, trGasIndex, trDataIndex, nonetwork, singleTest, all, " <<
					"stats, filltests, fillchain \n";
			exit(1);
		}
	}
	else
	{
		if (randomTestSeed.is_initialized())
			BOOST_THROW_EXCEPTION(InvalidOption("--seed <uint> could be used only with --createRandomTest \n"));
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

