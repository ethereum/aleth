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

/// @file
/// EVM Execution tool.

#include <libdevcore/CommonIO.h>
#include <libdevcore/SHA3.h>
#include <libethashseal/Ethash.h>
#include <libethashseal/GenesisInfo.h>
#include <libethcore/SealEngine.h>
#include <libethereum/Block.h>
#include <libethereum/ChainParams.h>
#include <libethereum/Executive.h>
#include <libethereum/LastBlockHashesFace.h>
#include <libevm/VM.h>
#include <libevm/VMFactory.h>

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

#include <ctime>
#include <fstream>
#include <iostream>

using namespace std;
using namespace dev;
using namespace eth;
namespace po = boost::program_options;

namespace
{
unsigned const c_lineWidth = 160;

int64_t maxBlockGasLimit()
{
    static int64_t limit =
        ChainParams(genesisInfo(Network::MainNetwork)).maxGasLimit.convert_to<int64_t>();
    return limit;
}

void version()
{
    cout << "ethvm version " << dev::Version << "\n";
    cout << "By Gav Wood, 2015.\n";
    cout << "Build: " << DEV_QUOTED(ETH_BUILD_PLATFORM) << "/" << DEV_QUOTED(ETH_BUILD_TYPE)
         << "\n";
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

enum class Mode
{
    Trace,
    Statistics,
    OutputOnly,

    /// Test mode -- output information needed for test verification and
    /// benchmarking. The execution is not introspected not to degrade
    /// performance.
    Test
};
}

class LastBlockHashes : public eth::LastBlockHashesFace
{
public:
    h256s precedingHashes(h256 const& /* _mostRecentHash */) const override
    {
        return h256s(256, h256());
    }
    void clear() override {}
};

int main(int argc, char** argv)
{
    setDefaultOrCLocale();
    string inputFile;
    Mode mode = Mode::Statistics;
    State state(0);
    Address sender = Address(69);
    Address origin = Address(69);
    u256 value = 0;
    u256 gas = maxBlockGasLimit();
    u256 gasPrice = 0;
    bool styledJson = true;
    StandardTrace st;
    Network networkName = Network::MainNetworkTest;
    BlockHeader blockHeader;  // fake block to be executed in
    blockHeader.setGasLimit(maxBlockGasLimit());
    blockHeader.setTimestamp(0);
    bytes data;
    bytes code;

    Ethash::init();
    NoProof::init();

    setupLogging(5);

    po::options_description transactionOptions("Transaction options", c_lineWidth);
    string const gasLimitDescription =
        "<n> Block gas limit (default: " + to_string(maxBlockGasLimit()) + ").";
    auto addTransactionOption = transactionOptions.add_options();
    addTransactionOption(
        "value", po::value<u256>(), "<n> Transaction should transfer the <n> wei (default: 0).");
    addTransactionOption("gas", po::value<u256>(),
        "<n> Transaction should be given <n> gas (default: block gas limit).");
    addTransactionOption("gas-limit", po::value<u256>(), gasLimitDescription.c_str());
    addTransactionOption(
        "gas-price", po::value<u256>(), "<n> Transaction's gas price' should be <n> (default: 0).");
    addTransactionOption("sender", po::value<Address>(),
        "<a> Transaction sender should be <a> (default: 0000...0069).");
    addTransactionOption("origin", po::value<Address>(),
        "<a> Transaction origin should be <a> (default: 0000...0069).");
    addTransactionOption("input", po::value<string>(), "<d> Transaction code should be <d>");
    addTransactionOption("code", po::value<string>(),
        "<d> Contract code <d>. Makes transaction a call to this contract");

    po::options_description networkOptions("Network options", c_lineWidth);
    networkOptions.add_options()("network", po::value<string>(),
        "Main|Ropsten|Homestead|Frontier|Byzantium|Constantinople\n");

    po::options_description optionsForTrace("Options for trace", c_lineWidth);
    auto addTraceOption = optionsForTrace.add_options();
    addTraceOption("flat", "Minimal whitespace in the JSON.");
    addTraceOption("mnemonics", "Show instruction mnemonics in the trace (non-standard).\n");

    po::options_description generalOptions("General options", c_lineWidth);
    auto addGeneralOption = generalOptions.add_options();
    addGeneralOption("version,v", "Show the version and exit.");
    addGeneralOption("help,h", "Show this help message and exit.");
    addGeneralOption("author", po::value<Address>(), "<a> Set author");
    addGeneralOption("difficulty", po::value<u256>(), "<n> Set difficulty");
    addGeneralOption("number",
        po::value<int64_t>()->default_value(0)->value_name("<number>")->notifier([&](int64_t _n) {
            blockHeader.setNumber(_n);
        }),
        "<n> Set number");
    addGeneralOption("timestamp",
        po::value<int64_t>()
            ->default_value(0)
            ->value_name("<timestamp>")
            ->notifier([&](int64_t _t) { blockHeader.setTimestamp(_t); }),
        "<n> Set timestamp");

    po::options_description allowedOptions(
        "Usage ethvm <options> [trace|stats|output|test] (<file>|-)");
    allowedOptions.add(vmProgramOptions(c_lineWidth))
        .add(networkOptions)
        .add(optionsForTrace)
        .add(generalOptions)
        .add(transactionOptions);
    po::parsed_options parsed =
        po::command_line_parser(argc, argv).options(allowedOptions).allow_unregistered().run();
    vector<string> unrecognisedOptions =
        collect_unrecognized(parsed.options, po::include_positional);
    po::variables_map vm;
    po::store(parsed, vm);
    po::notify(vm);

    // handling mode and input file options separately, as they don't have option name
    for (auto const& arg : unrecognisedOptions)
    {
        if (arg == "stats")
            mode = Mode::Statistics;
        else if (arg == "output")
            mode = Mode::OutputOnly;
        else if (arg == "trace")
            mode = Mode::Trace;
        else if (arg == "test")
            mode = Mode::Test;
        else if (inputFile.empty())
            inputFile = arg;  // Assign input file name only once.
        else
        {
            cerr << "Unknown argument: " << arg << '\n';
            return -1;
        }
    }
    if (vm.count("help"))
    {
        cout << allowedOptions;
        return 0;
    }
    if (vm.count("version"))
    {
        version();
    }
    if (vm.count("mnemonics"))
        st.setShowMnemonics();
    if (vm.count("flat"))
        styledJson = false;
    if (vm.count("sender"))
        sender = vm["sender"].as<Address>();
    if (vm.count("origin"))
        origin = vm["origin"].as<Address>();
    if (vm.count("gas"))
        gas = vm["gas"].as<u256>();
    if (vm.count("gas-price"))
        gasPrice = vm["gas-price"].as<u256>();
    if (vm.count("author"))
        blockHeader.setAuthor(vm["author"].as<Address>());
    if (vm.count("difficulty"))
        blockHeader.setDifficulty(vm["difficulty"].as<u256>());
    if (vm.count("gas-limit"))
        blockHeader.setGasLimit((vm["gas-limit"].as<u256>()).convert_to<int64_t>());
    if (vm.count("value"))
        value = vm["value"].as<u256>();
    if (vm.count("network"))
    {
        string network = vm["network"].as<string>();
        if (network == "Constantinople")
            networkName = Network::ConstantinopleTest;
        else if (network == "Byzantium")
            networkName = Network::ByzantiumTest;
        else if (network == "Frontier")
            networkName = Network::FrontierTest;
        else if (network == "Ropsten")
            networkName = Network::Ropsten;
        else if (network == "Homestead")
            networkName = Network::HomesteadTest;
        else if (network == "Main")
            networkName = Network::MainNetwork;
        else
        {
            cerr << "Unknown network type: " << network << "\n";
            return -1;
        }
    }
    if (vm.count("input"))
        data = fromHex(vm["input"].as<string>());
    if (vm.count("code"))
        code = fromHex(vm["code"].as<string>());


    // Read code from input file.
    if (!inputFile.empty())
    {
        if (!code.empty())
            cerr << "--code argument overwritten by input file " << inputFile << '\n';

        if (inputFile == "-")
            for (int i = cin.get(); i != -1; i = cin.get())
                code.push_back(static_cast<byte>(i));
        else
            code = contents(inputFile);

        try  // Try decoding from hex.
        {
            std::string strCode{reinterpret_cast<char const*>(code.data()), code.size()};
            strCode.erase(strCode.find_last_not_of(" \t\n\r") + 1);  // Right trim.
            code = fromHex(strCode, WhenError::Throw);
        }
        catch (BadHexCharacter const&)
        {
        }  // Ignore decoding errors.
    }

    Transaction t;
    Address contractDestination("1122334455667788991011121314151617181920");
    if (!code.empty())
    {
        // Deploy the code on some fake account to be called later.
        Account account(0, 0);
        account.setCode(bytes{code});
        std::unordered_map<Address, Account> map;
        map[contractDestination] = account;
        state.populateFrom(map);
        t = Transaction(value, gasPrice, gas, contractDestination, data, 0);
    }
    else
        // If not code provided construct "create" transaction out of the input
        // data.
        t = Transaction(value, gasPrice, gas, data, 0);

    state.addBalance(sender, value);

    unique_ptr<SealEngineFace> se(ChainParams(genesisInfo(networkName)).createSealEngine());
    LastBlockHashes lastBlockHashes;
    EnvInfo const envInfo(blockHeader, lastBlockHashes, 0);
    Executive executive(state, envInfo, *se);
    ExecutionResult res;
    executive.setResultRecipient(res);
    t.forceSender(sender);

    unordered_map<byte, pair<unsigned, bigint>> counts;
    unsigned total = 0;
    bigint memTotal;

    executive.initialize(t);
    if (!code.empty())
        executive.call(contractDestination, sender, value, gasPrice, &data, gas);
    else
        executive.create(sender, value, gasPrice, gas, &data, origin);

    OnOpFunc onOp;
    if (mode == Mode::Statistics)
    {
        onOp = [&](uint64_t, uint64_t, Instruction inst, bigint m, bigint gasCost, bigint,
                   VMFace const*, ExtVMFace const*) {
            byte b = static_cast<byte>(inst);
            counts[b].first++;
            counts[b].second += gasCost;
            total++;
            if (m > memTotal)
                memTotal = m;
        };
    }
    else if (mode == Mode::Trace)
        onOp = st;

    Timer timer;
    executive.go(onOp);
    double execTime = timer.elapsed();
    executive.finalize();
    bytes output = std::move(res.output);

    if (mode == Mode::Statistics)
    {
        cout << "Gas used: " << res.gasUsed << " (+"
             << t.baseGasRequired(se->evmSchedule(envInfo.number())) << " for transaction, -"
             << res.gasRefunded << " refunded)\n";
        cout << "Output: " << toHex(output) << "\n";
        LogEntries logs = executive.logs();
        cout << logs.size() << " logs" << (logs.empty() ? "." : ":") << "\n";
        for (LogEntry const& l : logs)
        {
            cout << "  " << l.address.hex() << ": " << toHex(t.data()) << "\n";
            for (h256 const& topic : l.topics)
                cout << "    " << topic.hex() << "\n";
        }

        cout << total << " operations in " << execTime << " seconds.\n";
        cout << "Maximum memory usage: " << memTotal * 32 << " bytes\n";
        cout << "Expensive operations:\n";
        for (auto const inst : {Instruction::SSTORE, Instruction::SLOAD, Instruction::CALL,
                 Instruction::CREATE, Instruction::CALLCODE, Instruction::DELEGATECALL,
                 Instruction::MSTORE8, Instruction::MSTORE, Instruction::MLOAD, Instruction::SHA3})
        {
            auto const& count = counts[static_cast<byte>(inst)];
            if (count.first != 0)
                cout << "  " << instructionInfo(inst).name << " x " << count.first << " ("
                     << count.second << " gas)\n";
        }
    }
    else if (mode == Mode::Trace)
        cout << st.json(styledJson);
    else if (mode == Mode::OutputOnly)
        cout << toHex(output) << '\n';
    else if (mode == Mode::Test)
    {
        // Output information needed for test verification and benchmarking
        // in YAML-like dictionaly format.
        auto exception = res.excepted != TransactionException::None;
        cout << "output: '" << toHex(output) << "'\n";
        cout << "exception: " << boolalpha << exception << '\n';
        cout << "gas used: " << res.gasUsed << '\n';
        cout << "gas/sec: " << scientific << setprecision(3) << uint64_t(res.gasUsed) / execTime
             << '\n';
        cout << "exec time: " << fixed << setprecision(6) << execTime << '\n';
    }
    return 0;
}
