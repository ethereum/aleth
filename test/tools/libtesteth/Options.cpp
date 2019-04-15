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

#include <libdevcore/DBFactory.h>
#include <libevm/VMFactory.h>
#include <libweb3jsonrpc/Debug.h>
#include <test/tools/fuzzTesting/fuzzHelper.h>
#include <test/tools/libtesteth/Options.h>
#include <boost/iostreams/device/null.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/program_options.hpp>

using namespace std;
using namespace dev::test;
using namespace dev::db;
using namespace dev::eth;

namespace
{
void printHelp()
{
    cout << "Usage: \n";
    cout << std::left;
    cout << "\nSetting test suite\n";
    cout << setw(30) << "-t <TestSuite>" << setw(25) << "Execute test operations\n";
    cout << setw(30) << "-t <TestSuite>/<TestCase>\n";
    cout << setw(30) << "--testpath <PathToTheTestRepo>\n";

    cout << "\nDebugging\n";
    cout << setw(30) << "-d <index>" << setw(25) << "Set the transaction data array index when running GeneralStateTests\n";
    cout << setw(30) << "-g <index>" << setw(25) << "Set the transaction gas array index when running GeneralStateTests\n";
    cout << setw(30) << "-v <index>" << setw(25) << "Set the transaction value array index when running GeneralStateTests\n";
    cout << setw(30) << "--singletest <TestName>" << setw(25) << "Run on a single test\n";
    cout << setw(30) << "--singletest <TestFile> <TestName>\n";
    cout << setw(30) << "--singlenet <networkId>" << setw(25) << "Run tests for a specific network (Frontier|Homestead|EIP150|EIP158|Byzantium|Constantinople|ConstantinopleFix)\n";
    cout << setw(30) << "--verbosity <level>" << setw(25) << "Set logs verbosity. 0 - silent, 1 - only errors, 2 - informative, >2 - detailed\n";
    cout << setw(30) << "--vm <name|path> (=legacy)" << setw(25) << "Set VM type for VMTests suite. Available options are: interpreter, legacy.\n";
    cout << setw(30) << "--evmc <option>=<value>" << setw(25) << "EVMC option\n";
    cout << setw(30) << "--vmtrace" << setw(25) << "Enable VM trace for the test\n";
    cout << setw(30) << "--jsontrace <Options>" << setw(25) << "Enable VM trace to stdout in json format. Argument is a json config: '{ \"disableStorage\" : false, \"disableMemory\" : false, \"disableStack\" : false, \"fullStorage\" : true }'\n";
    cout << setw(30) << "--stats <OutFile>" << setw(25) << "Output debug stats to the file\n";
    cout << setw(30) << "--exectimelog" << setw(25) << "Output execution time for each test suite\n";
    cout << setw(30) << "--statediff" << setw(25) << "Trace state difference for state tests\n";

    cout << "\nAdditional Tests\n";
    cout << setw(30) << "--all" << setw(25) << "Enable all tests\n";

    cout << "\nTest Generation\n";
    cout << setw(30) << "--filltests" << setw(25) << "Run test fillers\n";
    cout << setw(30) << "--fillchain" << setw(25) << "When filling the state tests, fill tests as blockchain instead\n";
    cout << setw(30) << "--showhash" << setw(25) << "Show filler hash debug information\n";
    cout << setw(30) << "--randomcode <MaxOpcodeNum>" << setw(25) << "Generate smart random EVM code\n";
    cout << setw(30) << "--createRandomTest" << setw(25) << "Create random test and output it to the console\n";
    cout << setw(30) << "--createRandomTest <PathToOptions.json>" << setw(25) << "Use following options file for random code generation\n";
    cout << setw(30) << "--seed <uint>" << setw(25) << "Define a seed for random test\n";
    cout << setw(30) << "--options <PathTo.json>" << setw(25) << "Use following options file for random code generation\n";
    //cout << setw(30) << "--fulloutput" << setw(25) << "Disable address compression in the output field\n";
    cout << setw(30) << "--db <name> (=memorydb)" << setw(25) << "Use the supplied database for the block and state databases. Valid options: leveldb, rocksdb, memorydb\n";
    cout << setw(30) << "--help" << setw(25) << "Display list of command arguments\n";
    cout << setw(30) << "--version" << setw(25) << "Display build information\n";
}

void printVersion()
{
    cout << prepareVersionString() << "\n";
}
}

void Options::setVerbosity(int _level)
{
    static boost::iostreams::stream<boost::iostreams::null_sink> nullOstream(
        (boost::iostreams::null_sink()));
    dev::LoggingOptions logOptions;
    verbosity = _level;
    logOptions.verbosity = verbosity;
    if (_level <= 0)
    {
        logOptions.verbosity = VerbositySilent;
        std::cout.rdbuf(nullOstream.rdbuf());
        std::cerr.rdbuf(nullOstream.rdbuf());
    }
    else if (_level == 1 || _level == 2)
        logOptions.verbosity = VerbositySilent;
    dev::setupLogging(logOptions);
}

Options::Options(int argc, const char** argv)
{
    {
        namespace po = boost::program_options;

        // For some reason boost is confused by -- separator. This extra parser "skips" the --.
        auto skipDoubleDash = [](const std::string& s) -> std::pair<std::string, std::string> {
            if (s == "--")
                return {"--", {}};
            return {};
        };

        auto vmOpts = vmProgramOptions();
        po::parsed_options parsed = po::command_line_parser(argc, argv)
                                        .options(vmOpts)
                                        .extra_parser(skipDoubleDash)
                                        .allow_unregistered()
                                        .run();
        po::variables_map vm;
        po::store(parsed, vm);
        po::notify(vm);
    }

    trDataIndex = -1;
    trGasIndex = -1;
    trValueIndex = -1;
    bool seenSeparator = false; // true if "--" has been seen.
    setDatabaseKind(DatabaseKind::MemoryDB); // default to MemoryDB in the interest of reduced test execution time
    for (auto i = 0; i < argc; ++i)
    {
        auto arg = std::string{argv[i]};
        auto throwIfNoArgumentFollows = [&i, &argc, &arg]()
        {
            if (i + 1 >= argc)
                BOOST_THROW_EXCEPTION(
                    InvalidOption() << errinfo_comment(arg + " option is missing an argument."));
        };
        auto throwIfAfterSeparator = [&seenSeparator, &arg]()
        {
            if (seenSeparator)
                BOOST_THROW_EXCEPTION(InvalidOption() << errinfo_comment(
                                          arg + " option appears after the separator --."));
        };
        if (arg == "--")
        {
            if (seenSeparator)
                BOOST_THROW_EXCEPTION(
                    InvalidOption() << errinfo_comment(
                        "The separator -- appears more than once in the command line."));
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
            // Skip VM options because they are handled by vmProgramOptions().
            throwIfNoArgumentFollows();
            ++i;
        }
        else if (arg == "--evmc")
        {
            // Skip VM options because they are handled by vmProgramOptions().
            throwIfNoArgumentFollows();

            // --evmc is a multitoken option so skip all tokens.
            while (i + 1 < argc && argv[i + 1][0] != '-')
                ++i;
        }
        else if (arg == "--vmtrace")
        {
            vmtrace = true;
            verbosity = VerbosityTrace;
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
        else if (arg == "--showhash")
            showhash = true;
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
            ImportTest::checkAllowedNetwork(singleTestNet);
        }
        else if (arg == "--fulloutput")
            fulloutput = true;
        else if (arg == "--verbosity")
        {
            throwIfNoArgumentFollows();
            verbosity = std::max(verbosity, atoi(argv[++i]));
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
        {
            statediff = true;
            verbosity = VerbosityTrace;
        }
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
                        BOOST_THROW_EXCEPTION(InvalidOption() << errinfo_comment(
                                                  "Options file not found! Default options at: "
                                                  "tests/src/randomCodeOptions.json\n"));
                }
            }
        }
        else if (arg == "--seed")
        {
            throwIfNoArgumentFollows();
            u256 input = toU256(argv[++i]);
            if (input > std::numeric_limits<uint64_t>::max())
                BOOST_WARN("Seed is > u64. Using u64_max instead.");
            randomTestSeed = static_cast<uint64_t>(min<u256>(std::numeric_limits<uint64_t>::max(), input));
        }
        else if (arg == "--db")
        {
            throwIfNoArgumentFollows();
            setDatabaseKindByName(argv[++i]);
        }
        else if (seenSeparator)
        {
            cerr << "Unknown option: " + arg << "\n";
            exit(1);
        }
    }

    //check restricted options
    if (createRandomTest)
    {
        if (trValueIndex >= 0 || trGasIndex >= 0 || trDataIndex >= 0 || singleTest || all ||
            stats || filltests || fillchain)
        {
            cerr << "--createRandomTest cannot be used with any of the options: " <<
                    "trValueIndex, trGasIndex, trDataIndex, singleTest, all, " <<
                    "stats, filltests, fillchain \n";
            exit(1);
        }
    }
    else
    {
        if (randomTestSeed.is_initialized())
            BOOST_THROW_EXCEPTION(
                InvalidOption() << errinfo_comment(
                    "--seed <uint> could be used only with --createRandomTest \n"));
    }

    // If no verbosity is set. use default
    setVerbosity(verbosity == -1 ? 1 : verbosity);
}

Options const& Options::get(int argc, const char** argv)
{
    static Options instance(argc, argv);
    return instance;
}

