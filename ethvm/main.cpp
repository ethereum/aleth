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
 * EVM Execution tool.
 */
#include <fstream>
#include <iostream>
#include <ctime>
#include <boost/algorithm/string.hpp>
#include <libdevcore/CommonIO.h>
#include <libdevcore/RLP.h>
#include <libdevcore/SHA3.h>
#include <libethcore/SealEngine.h>
#include <libethereum/Block.h>
#include <libethereum/Executive.h>
#include <libethereum/ChainParams.h>
#include <libethashseal/GenesisInfo.h>
#include <libethashseal/Ethash.h>
#include <libevm/VM.h>
#include <libevm/VMFactory.h>
using namespace std;
using namespace dev;
using namespace eth;

static const int64_t MaxBlockGasLimit = ChainParams(genesisInfo(Network::MainNetwork)).u256Param("maxGasLimit").convert_to<int64_t>();

void help()
{
	cout
		<< "Usage ethvm <options> [trace|stats|output|test] (<file>|-)" << endl
		<< "Transaction options:" << endl
		<< "    --value <n>  Transaction should transfer the <n> wei (default: 0)." << endl
		<< "    --gas <n>    Transaction should be given <n> gas (default: block gas limit)." << endl
		<< "    --gas-limit <n>  Block gas limit (default: " << MaxBlockGasLimit << ")." << endl
		<< "    --gas-price <n>  Transaction's gas price' should be <n> (default: 0)." << endl
		<< "    --sender <a>  Transaction sender should be <a> (default: 0000...0069)." << endl
		<< "    --origin <a>  Transaction origin should be <a> (default: 0000...0069)." << endl
		<< "    --input <d>   Transaction code should be <d>" << endl
		<< "    --code <d>    Contract code <d>. Makes transaction a call to this contract" << endl
#if ETH_EVMJIT
		<< endl
		<< "VM options:" << endl
		<< "    --vm <vm-kind>  Select VM. Options are: interpreter, jit, smart. (default: interpreter)" << endl
#endif // ETH_EVMJIT
		<< "Network options:" << endl
		<< "    --network Main|Ropsten|Homestead|Frontier" << endl
		<< endl
		<< "Options for trace:" << endl
		<< "    --flat  Minimal whitespace in the JSON." << endl
		<< "    --mnemonics  Show instruction mnemonics in the trace (non-standard)." << endl
		<< endl
		<< "General options:" << endl
		<< "    -V,--version  Show the version and exit." << endl
		<< "    -h,--help  Show this help message and exit." << endl;
	exit(0);
}

void version()
{
	cout << "ethvm version " << dev::Version << endl;
	cout << "By Gav Wood, 2015." << endl;
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

int main(int argc, char** argv)
{
	setDefaultOrCLocale();
	string inputFile;
	Mode mode = Mode::Statistics;
	VMKind vmKind = VMKind::Interpreter;
	State state(0);
	Address sender = Address(69);
	Address origin = Address(69);
	u256 value = 0;
	u256 gas = MaxBlockGasLimit;
	u256 gasPrice = 0;
	bool styledJson = true;
	StandardTrace st;
	EnvInfo envInfo;
	Network networkName = Network::MainNetwork;
	envInfo.setGasLimit(MaxBlockGasLimit);
	bytes data;
	bytes code;

	Ethash::init();
	NoProof::init();

	for (int i = 1; i < argc; ++i)
	{
		string arg = argv[i];
		if (arg == "-h" || arg == "--help")
			help();
		else if (arg == "-V" || arg == "--version")
			version();
		else if (arg == "--vm" && i + 1 < argc)
		{
			string vmKindStr = argv[++i];
			if (vmKindStr == "interpreter")
				vmKind = VMKind::Interpreter;
#if ETH_EVMJIT
			else if (vmKindStr == "jit")
				vmKind = VMKind::JIT;
			else if (vmKindStr == "smart")
				vmKind = VMKind::Smart;
#endif
			else
			{
				cerr << "Unknown/unsupported VM kind: " << vmKindStr << endl;
				return -1;
			}
		}
		else if (arg == "--mnemonics")
			st.setShowMnemonics();
		else if (arg == "--flat")
			styledJson = false;
		else if (arg == "--sender" && i + 1 < argc)
			sender = Address(argv[++i]);
		else if (arg == "--origin" && i + 1 < argc)
			origin = Address(argv[++i]);
		else if (arg == "--gas" && i + 1 < argc)
			gas = u256(argv[++i]);
		else if (arg == "--gas-price" && i + 1 < argc)
			gasPrice = u256(argv[++i]);
		else if (arg == "--author" && i + 1 < argc)
			envInfo.setAuthor(Address(argv[++i]));
		else if (arg == "--number" && i + 1 < argc)
			envInfo.setNumber(u256(argv[++i]));
		else if (arg == "--difficulty" && i + 1 < argc)
			envInfo.setDifficulty(u256(argv[++i]));
		else if (arg == "--timestamp" && i + 1 < argc)
			envInfo.setTimestamp(u256(argv[++i]));
		else if (arg == "--gas-limit" && i + 1 < argc)
			envInfo.setGasLimit(u256(argv[++i]).convert_to<int64_t>());
		else if (arg == "--value" && i + 1 < argc)
			value = u256(argv[++i]);
		else if (arg == "--network" && i + 1 < argc)
		{
			string network = argv[++i];
			if (network == "Frontier")
				networkName = Network::FrontierTest;
			else if (network == "Ropsten")
				networkName = Network::Ropsten;
			else if (network == "Homestead")
				networkName = Network::HomesteadTest;
			else if (network == "Main")
				networkName = Network::MainNetwork;
			else
			{
				cerr << "Unknown network type: " << network << endl;
				return -1;
			}
		}
		else if (arg == "stats")
			mode = Mode::Statistics;
		else if (arg == "output")
			mode = Mode::OutputOnly;
		else if (arg == "trace")
			mode = Mode::Trace;
		else if (arg == "test")
			mode = Mode::Test;
		else if (arg == "--input" && i + 1 < argc)
			data = fromHex(argv[++i]);
		else if (arg == "--code" && i + 1 < argc)
			code = fromHex(argv[++i]);
		else if (inputFile.empty())
			inputFile = arg;  // Assign input file name only once.
		else
		{
			cerr << "Unknown argument: " << arg << '\n';
			return -1;
		}
	}

	VMFactory::setKind(vmKind);


	// Read code from input file.
	if (!inputFile.empty())
	{
		if (!code.empty())
			cerr << "--code argument overwritten by input file "
				 << inputFile << '\n';

		if (inputFile == "-")
			for (int i = cin.get(); i != -1; i = cin.get())
				code.push_back((char)i);
		else
			code = contents(inputFile);

		try  // Try decoding from hex.
		{
			std::string strCode{reinterpret_cast<char const*>(code.data()), code.size()};
			strCode.erase(strCode.find_last_not_of(" \t\n\r") + 1);  // Right trim.
			code = fromHex(strCode, WhenError::Throw);
		}
		catch (BadHexCharacter const&) {}  // Ignore decoding errors.
	}

	Transaction t;
	Address contractDestination("1122334455667788991011121314151617181920");
	if (!code.empty())
	{
		// Deploy the code on some fake account to be called later.
		Account account(0, 0);
		account.setNewCode(bytes{code});
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
	Executive executive(state, envInfo, *se);
	ExecutionResult res;
	executive.setResultRecipient(res);
	t.forceSender(sender);

	unordered_map<byte, pair<unsigned, bigint>> counts;
	unsigned total = 0;
	bigint memTotal;
	auto onOp = [&](uint64_t step, uint64_t PC, Instruction inst, bigint m, bigint gasCost, bigint gas, VM* vm, ExtVMFace const* extVM) {
		if (mode == Mode::Statistics)
		{
			counts[(byte)inst].first++;
			counts[(byte)inst].second += gasCost;
			total++;
			if (m > 0)
				memTotal = m;
		}
		else if (mode == Mode::Trace)
			st(step, PC, inst, m, gasCost, gas, vm, extVM);
	};

	executive.initialize(t);
	if (!code.empty())
		executive.call(contractDestination, sender, value, gasPrice, &data, gas);
	else
		executive.create(sender, value, gasPrice, gas, &data, origin);

	Timer timer;
	if ((mode == Mode::Statistics || mode == Mode::Trace) && vmKind == VMKind::Interpreter)
		// If we use onOp, the factory falls back to "interpreter"
		executive.go(onOp);
	else
		executive.go();
	double execTime = timer.elapsed();
	executive.finalize();
	bytes output = std::move(res.output);

	if (mode == Mode::Statistics)
	{
		cout << "Gas used: " << res.gasUsed << " (+" << t.baseGasRequired(se->evmSchedule(envInfo)) << " for transaction, -" << res.gasRefunded << " refunded)" << endl;
		cout << "Output: " << toHex(output) << endl;
		LogEntries logs = executive.logs();
		cout << logs.size() << " logs" << (logs.empty() ? "." : ":") << endl;
		for (LogEntry const& l: logs)
		{
			cout << "  " << l.address.hex() << ": " << toHex(t.data()) << endl;
			for (h256 const& t: l.topics)
				cout << "    " << t.hex() << endl;
		}

		cout << total << " operations in " << execTime << " seconds." << endl;
		cout << "Maximum memory usage: " << memTotal * 32 << " bytes" << endl;
		cout << "Expensive operations:" << endl;
		for (auto const& c: {Instruction::SSTORE, Instruction::SLOAD, Instruction::CALL, Instruction::CREATE, Instruction::CALLCODE, Instruction::DELEGATECALL, Instruction::MSTORE8, Instruction::MSTORE, Instruction::MLOAD, Instruction::SHA3})
			if (!!counts[(byte)c].first)
				cout << "  " << instructionInfo(c).name << " x " << counts[(byte)c].first << " (" << counts[(byte)c].second << " gas)" << endl;
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
		cout << "gas/sec: " << scientific << setprecision(3) << uint64_t(res.gasUsed)/execTime << '\n';
		cout << "exec time: " << fixed << setprecision(6) << execTime << '\n';
	}

	return 0;
}
