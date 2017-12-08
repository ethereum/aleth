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
/**
 * @file main.cpp
 * @author Gav Wood <i@gavwood.com>
 * @author Tasha Carl <tasha@carl.pro> - I here by place all my contributions in this file under MIT licence, as specified by http://opensource.org/licenses/MIT.
 * @date 2014
 * Ethereum client.
 */

#include <thread>
#include <fstream>
#include <iostream>
#include <signal.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim_all.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>

#include <libdevcore/FileSystem.h>
#include <libethashseal/EthashAux.h>
#include <libevm/VM.h>
#include <libevm/VMFactory.h>
#include <libethcore/KeyManager.h>
#include <libethereum/Defaults.h>
#include <libethereum/SnapshotImporter.h>
#include <libethereum/SnapshotStorage.h>
#include <libethashseal/EthashClient.h>
#include <libethashseal/GenesisInfo.h>
#include <libwebthree/WebThree.h>

#include <libdevcrypto/LibSnark.h>

#include <libweb3jsonrpc/AccountHolder.h>
#include <libweb3jsonrpc/Eth.h>
#include <libweb3jsonrpc/ModularServer.h>
#include <libweb3jsonrpc/IpcServer.h>
#include <libweb3jsonrpc/Net.h>
#include <libweb3jsonrpc/Web3.h>
#include <libweb3jsonrpc/AdminNet.h>
#include <libweb3jsonrpc/AdminEth.h>
#include <libweb3jsonrpc/Personal.h>
#include <libweb3jsonrpc/Debug.h>
#include <libweb3jsonrpc/Test.h>

#include "MinerAux.h"
#include "BuildInfo.h"
#include "AccountManager.h"

using namespace std;
using namespace dev;
using namespace dev::p2p;
using namespace dev::eth;
using namespace boost::algorithm;
namespace po = boost::program_options;
namespace fs = boost::filesystem;

static std::atomic<bool> g_silence = {false};

string ethCredits(bool _interactive = false)
{
	std::ostringstream cout;
	if (_interactive)
		cout << "Type 'exit' to quit\n\n";
	return credits() + cout.str();
}

void version()
{
	cout << "eth version " << dev::Version << "\n";
	cout << "eth network protocol version: " << dev::eth::c_protocolVersion << "\n";
	cout << "Client database version: " << dev::eth::c_databaseVersion << "\n";
	cout << "Build: " << DEV_QUOTED(ETH_BUILD_PLATFORM) << "/" << DEV_QUOTED(ETH_BUILD_TYPE) << "\n";
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

void importPresale(KeyManager& _km, string const& _file, function<string()> _pass)
{
	KeyPair k = _km.presaleSecret(contentsString(_file), [&](bool){ return _pass(); });
	_km.import(k.secret(), "Presale wallet" + _file + " (insecure)");
}

Address c_config = Address("ccdeac59d35627b7de09332e819d5159e7bb7250");
string pretty(h160 _a, dev::eth::State const& _st)
{
	string ns;
	h256 n;
	if (h160 nameReg = (u160)_st.storage(c_config, 0))
		n = _st.storage(nameReg, (u160)(_a));
	if (n)
	{
		std::string s((char const*)n.data(), 32);
		if (s.find_first_of('\0') != string::npos)
			s.resize(s.find_first_of('\0'));
		ns = " " + s;
	}
	return ns;
}

inline bool isPrime(unsigned _number)
{
	if (((!(_number & 1)) && _number != 2 ) || (_number < 2) || (_number % 3 == 0 && _number != 3))
		return false;
	for(unsigned k = 1; 36 * k * k - 12 * k < _number; ++k)
		if ((_number % (6 * k + 1) == 0) || (_number % (6 * k - 1) == 0))
			return false;
	return true;
}

enum class NodeMode
{
	PeerServer,
	Full
};

enum class OperationMode
{
	Node,
	Import,
	ImportSnapshot,
	Export
};

enum class Format
{
	Binary,
	Hex,
	Human
};

void stopSealingAfterXBlocks(eth::Client* _c, unsigned _start, unsigned& io_mining)
{
	try
	{
		if (io_mining != ~(unsigned)0 && io_mining && asEthashClient(_c)->isMining() && _c->blockChain().details().number - _start == io_mining)
		{
			_c->stopSealing();
			io_mining = ~(unsigned)0;
		}
	}
	catch (InvalidSealEngine&)
	{
	}

	this_thread::sleep_for(chrono::milliseconds(100));
}

class ExitHandler
{
public:
	static void exitHandler(int) { s_shouldExit = true; }
	bool shouldExit() const { return s_shouldExit; }

private:
	static bool s_shouldExit;
};

bool ExitHandler::s_shouldExit = false;

int main(int argc, char** argv)
{
	setDefaultOrCLocale();

	// Init secp256k1 context by calling one of the functions.
	toPublic({});

	// Init defaults
	Defaults::get();
	Ethash::init();
	NoProof::init();

	g_logVerbosity = 1;

	/// Operating mode.
	OperationMode mode = OperationMode::Node;
//	unsigned prime = 0;
//	bool yesIReallyKnowWhatImDoing = false;
	strings scripts;

	/// File name for import/export.
	string filename;
	bool safeImport = false;

	/// Hashes/numbers for export range.
	string exportFrom = "1";
	string exportTo = "latest";
	Format exportFormat = Format::Binary;

	/// General params for Node operation
	NodeMode nodeMode = NodeMode::Full;

	bool ipc = true;

	string jsonAdmin;
	ChainParams chainParams;
	u256 gasFloor = Invalid256;
	string privateChain;

	bool upnp = true;
	WithExisting withExisting = WithExisting::Trust;

	/// Networking params.
	string clientName;
	string listenIP;
	unsigned short listenPort = 30303;
	string publicIP;
	string remoteHost;
	unsigned short remotePort = 30303;

	unsigned peers = 11;
	unsigned peerStretch = 7;
	std::map<NodeID, pair<NodeIPEndpoint,bool>> preferredNodes;
	bool bootstrap = true;
	bool disableDiscovery = false;
	bool pinning = false;
	bool enableDiscovery = false;
	bool noPinning = false;
	static const unsigned NoNetworkID = (unsigned)-1;
	unsigned networkID = NoNetworkID;

	/// Mining params
	unsigned mining = 0;
	Address author;
	strings presaleImports;
	bytes extraData;

	/// Transaction params
//	TransactionPriority priority = TransactionPriority::Medium;
//	double etherPrice = 30.679;
//	double blockFees = 15.0;
	u256 askPrice = DefaultGasPrice;
	u256 bidPrice = DefaultGasPrice;
	bool alwaysConfirm = true;

	/// Wallet password stuff
	string masterPassword;
	bool masterSet = false;

	/// Whisper
	bool useWhisper = false;
	bool testingMode = false;

	fs::path configFile = getDataDir() / fs::path("config.rlp");
	bytes b = contents(configFile);

	strings passwordsToNote;
	Secrets toImport;
	if (b.size())
	{
		try
		{
			RLP config(b);
			author = config[1].toHash<Address>();
		}
		catch (...) {}
	}

	if (argc > 1 && (string(argv[1]) == "wallet" || string(argv[1]) == "account"))
	{
		AccountManager accountm;
		return !accountm.execute(argc, argv);
	}


	MinerCLI m(MinerCLI::OperationMode::None);

	bool listenSet = false;
	bool chainConfigIsSet = false;
	string configJSON;
	string genesisJSON;
	po::options_description clientDefaultMode("Client mode (default)");
	clientDefaultMode.add_options()
		("format", po::value<string>(), "<binary/hex/human> Set format.")
		("script", po::value<string>(), "<script> Add script.")
		("mainnet", "Use the main network protocol.")
		("ropsten", "Use the Ropsten testnet.")
		("testnet", "Use the Ropsten testnet.")
		("gas-floor", po::value<string>(), "Set gasFloor")
		("private", po::value<string>(), "<name> Use a private chain.")
		("test", "Testing mode: Disable PoW and provide test rpc interface.")
		("config", po::value<string>(), "<file> Configure specialised blockchain using given JSON information.")
		("mode,o", po::value<string>(), "<full/peer>  Start a full node or a peer node (default: full).\n")
		("ipc", "Enable IPC server (default: on).")
		("ipcpath", po::value<string>(), "Set .ipc socket path (default: data directory)")
		("no-ipc", "Disable IPC server.")
		("admin", po::value<string>(), "<password>  Specify admin session key for JSON-RPC (default: auto-generated and printed at start-up).")
		("kill,K", "Kill the blockchain first.")
		("kill-blockchain", "Kill the blockchain first.")
		("rebuild,R", "Rebuild the blockchain from the existing database.")
		("rescue", "Attempt to rescue a corrupt database.\n")
		("import-presale", po::value<string>(), "<file>  Import a pre-sale key; you'll need to specify the password to this key.")
		("import-secret,s", po::value<string>(), "<secret>  Import a secret key into the key store.")
		("import-session-secret,S", po::value<string>(), "<secret>  Import a secret session into the key store.")
		("master", po::value<string>(), "<password>  Give the master password for the key store. Use --master \"\" to show a prompt.")
		("password", po::value<string>(), "<password>  Give a password for a private key.\n")
		("extra-data", po::value<string>(), "Set extra data")
		("genesis", po::value<string>(), "Set genesisJSON")
		("genesis-json", po::value<string>(), "Set genesisJSON")
		("json-admin", po::value<string>(), "Set jsonAdmin");
	po::options_description clientTransacting("Client transactions");
	clientTransacting.add_options()
		("ask", po::value<string>(), "See description under.")
		("bid", po::value<string>(), "See description under.")
		("unsafe-transactions", "Allow all transactions to proceed without verification. EXTREMELY UNSAFE.");
	po::options_description clientMining("Client mining");
	clientMining.add_options()
		("address,a", po::value<string>(), "<addr>  Set the author (mining payout) address to given address (default: auto).")
		("author", po::value<string>(), "<addr>  Set the author (mining payout) address to given address (default: auto).")
		("mining,m", po::value<string>(), "<on/off/number>  Enable mining, optionally for a specified number of blocks (default: off).")
		("cpu,C", "When mining, use the CPU.")
		("mining-threads,t", "<n>  Limit number of CPU/GPU miners to n (default: use everything available on selected platform).");
	po::options_description clientNetworking("Client networking");
	clientNetworking.add_options()
		("client-name", po::value<string>(), "<name>  Add a name to your client's version string (default: blank).")
		("bootstrap,b",  "Connect to the default Ethereum peer servers (default unless --no-discovery used).")
		("no-bootstrap",  "Do not connect to the default Ethereum peer servers (default only when --no-discovery is used).")
		("peers,x", po::value<int>(), "<number>  Attempt to connect to a given number of peers (default: 11).")
		("peer-stretch", po::value<int>(), "<number>  Give the accepted connection multiplier (default: 7).")
		("public-ip", po::value<string>(), "<ip>  Force advertised public IP to the given IP (default: auto).")
		("public", po::value<string>(), "<ip>  Force advertised public IP to the given IP (default: auto).")
		("listen-ip", po::value<string>(), "<ip>(:<port>)  Listen on the given IP for incoming connections (default: 0.0.0.0).")
		("listen", po::value<short>(), "<port>  Listen on the given port for incoming connections (default: 30303).")
		("listen-port", po::value<short>(), "<port>  Listen on the given port for incoming connections (default: 30303).")
		("remote,r", po::value<string>(), "<host>(:<port>)  Connect to the given remote host (default: none).")
		("port", po::value<short>(), "<port>  Connect to the given remote port (default: 30303).")
		("network-id", po::value<string>(), "<n>  Only connect to other hosts with this network id.")
		("upnp", po::value<string>(), "<on/off>  Use UPnP for NAT (default: on).")
		("peerset", po::value<string>(), "<list>  Space delimited list of peers; element format: type:publickey@ipAddress[:port].\n        Types:\n        default      Attempt connection when no other peers are available and pinning is disabled.\n        required	    Keep connected at all times.\n")
		// TODO:
		//		<< "	--trust-peers <filename>  Space delimited list of publickeys." << endl
		("no-discovery",  "Disable node discovery, implies --no-bootstrap.")
		("pin",  "Only accept or connect to trusted peers.")
		("hermit",  "Equivalent to --no-discovery --pin.")
		("sociable",  "Force discovery and no pinning.\n");
	po::options_description importExportMode("Import/export mode");
	importExportMode.add_options()
		("from", po::value<string>(), "<n>  Export only from block n; n may be a decimal, a '0x' prefixed hash, or 'latest'.")
		("to", po::value<string>(), "<n>  Export only to block n (inclusive); n may be a decimal, a '0x' prefixed hash, or 'latest'.")
		("only", po::value<string>(), "<n>  Equivalent to --export-from n --export-to n.")
		("dont-check", "Prevent checking some block aspects. Faster importing, but to apply only when the data is known to be valid.\n")
		("import-snapshot", po::value<string>(), "<path>  Import blockchain and state data from the Parity Warp Sync snapshot.");
	po::options_description generalOptions("General Options");
	generalOptions.add_options()
		("db-path,d", po::value<string>(), "See description under.")
		("datadir", po::value<string>(), "See description under.")
		("path", po::value<string>(), "See description under.")
#if ETH_EVMJIT
		("vm", "<vm-kind>  Select VM; options are: interpreter, jit or smart (default: interpreter)")
#endif // ETH_EVMJIT
		("verbosity,v", po::value<int>(), "<0 - 9>  Set the log verbosity from 0 to 9 (default: 8).")
		("version,V",  "Show the version and exit.")
		("help,h",  "Show this help message and exit.\n");
	po::options_description experimentalProofOfConcept("Experimental / Proof of Concept");
	experimentalProofOfConcept.add_options()
		("shh", "Enable Whisper.\n");
	po::options_description allowedOptions("Allowed options");
	allowedOptions.add(clientDefaultMode).add(clientTransacting).add(clientMining).add(clientNetworking).add(importExportMode).add(generalOptions);
	po::variables_map vm;
	vector<string> unrecognisedOptions;
	try
	{
			po::parsed_options parsed = po::command_line_parser(argc, argv).options(allowedOptions).allow_unregistered().run();
			unrecognisedOptions = collect_unrecognized(parsed.options, po::include_positional);
			po::store(parsed, vm);
			po::notify(vm);
	}
	catch (po::error const& e)
	{
			cerr << e.what();
			return -1;
	}
	for (size_t i = 0; i < unrecognisedOptions.size(); ++i)
	{
		string arg = unrecognisedOptions[i];
		if (m.interpretOption(i, unrecognisedOptions))
		{
		}
		else if ((arg == "-I" || arg == "--import" || arg == "import") && i + 1 < unrecognisedOptions.size())
		{
			mode = OperationMode::Import;
			filename = unrecognisedOptions[++i];
		}
		else if ((arg == "-E" || arg == "--export" || arg == "export") && i + 1 < unrecognisedOptions.size())
		{
			mode = OperationMode::Export;
			filename = unrecognisedOptions[++i];
		}
		else
		{
			cerr << "Invalid argument: " << arg << "\n";
			exit(-1);
		}
	}
#if ETH_EVMJIT
	if (vm.count("vm"))
	{
		string vmKind = vm["vm"].as<string>();
		if (vmKind == "interpreter")
			VMFactory::setKind(VMKind::Interpreter);
		else if (vmKind == "jit")
			VMFactory::setKind(VMKind::JIT);
		else if (vmKind == "smart")
			VMFactory::setKind(VMKind::Smart);
		else
		{
			cerr << "Unknown VM kind: " << vmKind << "\n";
			return -1;
		}
	}
#endif
	if (vm.count(std::string("import-snapshot")))
	{
		mode = OperationMode::ImportSnapshot;
		filename = vm[std::string("import-snapshot")].as<string>();
	}
	if (vm.count("shh"))
		useWhisper = true;
	if (vm.count("version"))
		version();
	if (vm.count("test"))
	{
		testingMode = true;
		enableDiscovery = false;
		disableDiscovery = true;
		noPinning = true;
		bootstrap = false;
	}
	if (vm.count("verbosity"))
		g_logVerbosity = vm["verbosity"].as<int>();
	if (vm.count("peers"))
		peers = vm["peers"].as<int>();
	if (vm.count("peer-stretch"))
		peerStretch = vm["peer-stretch"].as<int>();
	if (vm.count("peerset"))
	{
		string peerset = vm["peerset"].as<string>();
		if (peerset.empty())
		{
			cerr << "--peerset argument must not be empty";
			return -1;
		}

		vector<string> each;
		boost::split(each, peerset, boost::is_any_of("\t "));
		for (auto const& p: each)
		{
			string type;
			string pubk;
			string hostIP;
			unsigned short port = c_defaultListenPort;

			// type:key@ip[:port]
			vector<string> typeAndKeyAtHostAndPort;
			boost::split(typeAndKeyAtHostAndPort, p, boost::is_any_of(":"));
			if (typeAndKeyAtHostAndPort.size() < 2 || typeAndKeyAtHostAndPort.size() > 3)
				continue;

			type = typeAndKeyAtHostAndPort[0];
			if (typeAndKeyAtHostAndPort.size() == 3)
				port = (uint16_t)atoi(typeAndKeyAtHostAndPort[2].c_str());

			vector<string> keyAndHost;
			boost::split(keyAndHost, typeAndKeyAtHostAndPort[1], boost::is_any_of("@"));
			if (keyAndHost.size() != 2)
				continue;
			pubk = keyAndHost[0];
			if (pubk.size() != 128)
				continue;
			hostIP = keyAndHost[1];

			// todo: use Network::resolveHost()
			if (hostIP.size() < 4 /* g.it */)
				continue;

			bool required = type == "required";
			if (!required && type != "default")
				continue;

			Public publicKey(fromHex(pubk));
			try
			{
				preferredNodes[publicKey] = make_pair(NodeIPEndpoint(bi::address::from_string(hostIP), port, port), required);
			}
			catch (...)
			{
				cerr << "Unrecognized peerset: " << peerset << "\n";
				return -1;
			}
		}
	}
	if (vm.count("mode"))
	{
		string m = vm["mode"].as<string>();
		if (m == "full")
			nodeMode = NodeMode::Full;
		else if (m == "peer")
			nodeMode = NodeMode::PeerServer;
		else
		{
			cerr << "Unknown mode: " << m << "\n";
			return -1;
		}
	}
	if (vm.count("import-presale"))
		presaleImports.push_back(vm["import-presale"].as<string>());
	if (vm.count("json-admin"))
		jsonAdmin = vm["json-admin"].as<string>();
	if (vm.count("ipc"))
		ipc = true;
	if (vm.count("no-ipc"))
		ipc = false;
	if (vm.count("mining"))
	{
		string m = vm["mining"].as<string>();
		if (isTrue(m))
			mining = ~(unsigned)0;
		else if (isFalse(m))
			mining = 0;
		else
			try
			{
				mining = stoi(m);
			}
			catch (...) {
				cerr << "Unknown --mining option: " << m << "\n";
				return -1;
			}
	}
	if (vm.count("bootstrap"))
		bootstrap = true;
	if (vm.count("no-bootstrap"))
		bootstrap = false;
	if (vm.count("no-discovery"))
	{
		disableDiscovery = true;
		bootstrap = false;
	}
	if (vm.count("pin"))
		pinning = true;
	if (vm.count("hermit"))
		pinning = disableDiscovery = true;
	if (vm.count("sociable"))
		noPinning = enableDiscovery = true;
	if (vm.count("unsafe-transactions"))
		alwaysConfirm = false;
	if (vm.count("path"))
		setDataDir(vm["path"].as<string>());
	if (vm.count("db-path"))
		setDataDir(vm["db-path"].as<string>());
	if (vm.count("datadir"))
		setDataDir(vm["datadir"].as<string>());
	if (vm.count("ipcpath"))
		setIpcPath(vm["ipcpath"].as<string>());
	if (vm.count("genesis"))
	{
		try
		{
			genesisJSON = contentsString(vm["genesis"].as<string>());
		}
		catch (...)
		{
			cerr << "Bad --genesis option: " << vm["genesis"].as<string>() << "\n";
			return -1;
		}
	}
	if (vm.count("genesis-json"))
	{
		try
		{
			genesisJSON = contentsString(vm["genesis-json"].as<string>());
		}
		catch (...)
		{
			cerr << "Bad --genesis-json option: " << vm["genesis-json"].as<string>() << "\n";
			return -1;
		}
	}
	if (vm.count("config"))
	{
		try
		{
			configJSON = contentsString(vm["config"].as<string>());
		}
		catch (...)
		{
			cerr << "Bad --config option: " << vm["config"].as<string>() << "\n";
			return -1;
		}
	}
	if (vm.count("extra-data"))
	{
		try
		{
			extraData = fromHex(vm["extra-data"].as<string>());
		}
		catch (...)
		{
			cerr << "Bad " << "--extra-data" << " option: " << vm["extra-data"].as<string>() << "\n";
			return -1;
		}
	}
	if (vm.count("gas-floor"))
		gasFloor = u256(vm["gas-floor"].as<string>());
	if (vm.count("mainnet"))
	{
		chainParams = ChainParams(genesisInfo(eth::Network::MainNetwork), genesisStateRoot(eth::Network::MainNetwork));
		chainConfigIsSet = true;
	}
	if (vm.count("ropsten") || vm.count("testnet"))
	{
		chainParams = ChainParams(genesisInfo(eth::Network::Ropsten), genesisStateRoot(eth::Network::Ropsten));
		chainConfigIsSet = true;
	}
	if (vm.count("ask"))
	{
		try
		{
			askPrice = u256(vm["ask"].as<string>());
		}
		catch (...)
		{
			cerr << "Bad --ask option: " << vm["ask"].as<string>() << "\n";
			return -1;
		}
	}
	if (vm.count("bid"))
	{
		try
		{
			bidPrice = u256(vm["bid"].as<string>());
		}
		catch (...)
		{
			cerr << "Bad --bid option: " << vm["bid"].as<string>() << "\n";
			return -1;
		}
	}
	if (vm.count("listen-ip"))
	{
		listenIP = vm["listen-ip"].as<string>();
		listenSet = true;
	}
	if (vm.count("listen")) {
		listenPort = vm["listen"].as<short>();
		listenSet = true;
	}
	if (vm.count("listen-port")) {
		listenPort = vm["listen-port"].as<short>();
		listenSet = true;
	}
	if (vm.count("public-ip")) {
		publicIP = vm["public-ip"].as<string>();
	}
	if (vm.count("public")) {
		publicIP = vm["public"].as<string>();
	}
	if (vm.count("remote"))
	{
		string host = vm["remote"].as<string>();
		string::size_type found = host.find_first_of(':');
		if (found != std::string::npos)
		{
			remoteHost = host.substr(0, found);
			remotePort = (short)atoi(host.substr(found + 1, host.length()).c_str());
		}
		else
			remoteHost = host;
	}
	if (vm.count("port"))
	{
		remotePort = vm["port"].as<short>();
	}
	if (vm.count("password"))
		passwordsToNote.push_back(vm["password"].as<string>());
	if (vm.count("master"))
	{
		masterPassword = vm["master"].as<string>();
		masterSet = true;
	}
	if (vm.count("dont-check"))
		safeImport = true;
	if (vm.count("script"))
		scripts.push_back(vm["script"].as<string>());
	if (vm.count("format"))
	{
		string m = vm["format"].as<string>();
		if (m == "binary")
			exportFormat = Format::Binary;
		else if (m == "hex")
			exportFormat = Format::Hex;
		else if (m == "human")
			exportFormat = Format::Human;
		else
		{
			cerr << "Bad " << "--format" << " option: " << m << "\n";
			return -1;
		}
	}
	if (vm.count("to"))
		exportTo = vm["to"].as<string>();
	if (vm.count("from"))
		exportFrom = vm["from"].as<string>();
	if (vm.count("only"))
		exportTo = exportFrom = vm["only"].as<string>();
	if (vm.count("upnp"))
	{
		string m = vm["upnp"].as<string>();
		if (isTrue(m))
			upnp = true;
		else if (isFalse(m))
			upnp = false;
		else
		{
			cerr << "Bad " << "--upnp" << " option: " << m << "\n";
			return -1;
		}
	}
	if (vm.count("network-id"))
		try
		{
			networkID = stol(vm["network-id"].as<string>());
		}
		catch (...)
		{
			cerr << "Bad " << "--network-id" << " option: " << vm["network-id"].as<string>() << "\n";
			return -1;
		}
	if (vm.count("private"))
		try
		{
			privateChain = vm["private"].as<string>();
		}
		catch (...)
		{
			cerr << "Bad " << "--private" << " option: " << vm["private"].as<string>() << "\n";
			return -1;
		}
	if (vm.count("independent"))
		try
		{
			privateChain = vm["independent"].as<string>();
			noPinning = enableDiscovery = true;
		}
		catch (...)
		{
			cerr << "Bad " << "--independent" << " option: " << vm["independent"].as<string>() << "\n";
			return -1;
		}
	if (vm.count("kill") || vm.count("kill-blockchain"))
		withExisting = WithExisting::Kill;
	if (vm.count("rebuild"))
		withExisting = WithExisting::Verify;
	if (vm.count("rescue"))
		withExisting = WithExisting::Rescue;
	if (vm.count("client-name"))
		clientName = vm["client-name"].as<string>();
	if (vm.count("address"))
		try
		{
			author = h160(fromHex(vm["address"].as<string>(), WhenError::Throw));
		}
		catch (BadHexCharacter&)
		{
			cerr << "Bad hex in " << "--adress" << " option: " << vm["address"].as<string>() << "\n";
			return -1;
		}
		catch (...)
		{
			cerr << "Bad " << "--adress" << " option: " << vm["address"].as<string>() << "\n";
			return -1;
		}
	if (vm.count("author"))
		try
		{
			author = h160(fromHex(vm["author"].as<string>(), WhenError::Throw));
		}
		catch (BadHexCharacter&)
		{
			cerr << "Bad hex in " << "--author" << " option: " << vm["author"].as<string>() << "\n";
			return -1;
		}
		catch (...)
		{
			cerr << "Bad " << "--author" << " option: " << vm["author"].as<string>() << "\n";
			return -1;
		}
	if ((vm.count("import-secret")))
	{
		Secret s(fromHex(vm["import-secret"].as<string>()));
		toImport.emplace_back(s);
	}
	if (vm.count("import-session-secret"))
	{
		Secret s(fromHex(vm["import-session-secret"].as<string>()));
		toImport.emplace_back(s);
	}
	if (vm.count("help"))
	{
		cout << "\n\n\n\n\n";
		cout
				<< "Usage eth [OPTIONS]\n"
				<< "Options:\n\n"
				<< "Wallet usage:\n";
		AccountManager::streamAccountHelp(cout);
		AccountManager::streamWalletHelp(cout);
		cout << clientDefaultMode;
		cout << clientTransacting;
		cout << "  --ask                  <wei>  Set the minimum ask gas price under which no transaction will be mined\n                         (default" << toString(DefaultGasPrice) << ").\n";
		cout << "  --bid                  <wei>  Set the bid gas price to pay for transactions\n                         (default " << toString(DefaultGasPrice) << ").\n";
		cout << clientMining << clientNetworking;
		MinerCLI::streamHelp(cout);
		cout << importExportMode << generalOptions << "  -d[--db-path,--path,--datadir] <path>  Load database from path\n                          (default: " << getDataDir() << ").\n";
		cout << experimentalProofOfConcept;
		exit(0);
	}


	if (!configJSON.empty())
	{
		try
		{
			chainParams = chainParams.loadConfig(configJSON);
			chainConfigIsSet = true;
		}
		catch (...)
		{
			cerr << "provided configuration is not well formatted\n";
			cerr << "sample: \n" << genesisInfo(eth::Network::MainNetworkTest) << "\n";
			return 0;
		}
	}


	if (!genesisJSON.empty())
	{
		try
		{
			chainParams = chainParams.loadGenesis(genesisJSON);
			chainConfigIsSet = true;
		}
		catch (...)
		{
			cerr << "provided genesis block description is not well formatted\n";
			string genesisSample =
					R"E(
			{
				"nonce": "0x0000000000000042",
				"difficulty": "0x400000000",
				"mixHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
				"author": "0x0000000000000000000000000000000000000000",
				"timestamp": "0x00",
				"parentHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
				"extraData": "0x11bbe8db4e347b4e8c937c1c8370e4b5ed33adb3db69cbdb7a38e1e50b1b82fa",
				"gasLimit": "0x1388"
			}
			)E";
			cerr << "sample: \n" << genesisSample << "\n";
			return 0;
		}
	}


	if (!privateChain.empty())
	{
		chainParams.extraData = sha3(privateChain).asBytes();
		chainParams.difficulty = chainParams.minimumDifficulty;
		chainParams.gasLimit = u256(1) << 32;
	}
	// TODO: Open some other API path
//	if (gasFloor != Invalid256)
//		c_gasFloorTarget = gasFloor;

	if (!chainConfigIsSet)
		// default to mainnet if not already set with any of `--mainnet`, `--testnet`, `--genesis`, `--config`
		chainParams = ChainParams(genesisInfo(eth::Network::MainNetwork), genesisStateRoot(eth::Network::MainNetwork));

	if (g_logVerbosity > 0)
		cout << EthGrayBold "cpp-ethereum, a C++ Ethereum client" EthReset << "\n";

	m.execute();

	fs::path secretsPath;
	if (testingMode)
		secretsPath = boost::filesystem::path(getDataDir()) / "keystore";
	else
		secretsPath = SecretStore::defaultPath();
	KeyManager keyManager(KeyManager::defaultPath(), secretsPath);
	for (auto const& s: passwordsToNote)
		keyManager.notePassword(s);

	// the first value is deprecated (never used)
	writeFile(configFile, rlpList(author, author));

	if (!clientName.empty())
		clientName += "/";

	string logbuf;
	std::string additional;

	auto getPassword = [&](string const& prompt) {
		bool s = g_silence;
		g_silence = true;
		cout << "\n";
		string ret = dev::getPassword(prompt);
		g_silence = s;
		return ret;
	};
	auto getResponse = [&](string const& prompt, unordered_set<string> const& acceptable) {
		bool s = g_silence;
		g_silence = true;
		cout << "\n";
		string ret;
		while (true)
		{
			cout << prompt;
			getline(cin, ret);
			if (acceptable.count(ret))
				break;
			cout << "Invalid response: " << ret << "\n";
		}
		g_silence = s;
		return ret;
	};
	auto getAccountPassword = [&](Address const& a){
		return getPassword("Enter password for address " + keyManager.accountName(a) + " (" + a.abridged() + "; hint:" + keyManager.passwordHint(a) + "): ");
	};

	auto netPrefs = publicIP.empty() ? NetworkPreferences(listenIP, listenPort, upnp) : NetworkPreferences(publicIP, listenIP ,listenPort, upnp);
	netPrefs.discovery = (privateChain.empty() && !disableDiscovery) || enableDiscovery;
	netPrefs.pin = (pinning || !privateChain.empty()) && !noPinning;

	auto nodesState = contents(getDataDir() / fs::path("network.rlp"));
	auto caps = useWhisper ? set<string>{"eth", "shh"} : set<string>{"eth"};

	if (testingMode)
	{
		chainParams.sealEngineName = "NoProof";
		chainParams.allowFutureBlocks = true;
	}

	dev::WebThreeDirect web3(
		WebThreeDirect::composeClientVersion("eth"),
		getDataDir(),
		chainParams,
		withExisting,
		nodeMode == NodeMode::Full ? caps : set<string>(),
		netPrefs,
		&nodesState,
		testingMode
	);

	if (!extraData.empty())
		web3.ethereum()->setExtraData(extraData);

	auto toNumber = [&](string const& s) -> unsigned {
		if (s == "latest")
			return web3.ethereum()->number();
		if (s.size() == 64 || (s.size() == 66 && s.substr(0, 2) == "0x"))
			return web3.ethereum()->blockChain().number(h256(s));
		try
		{
			return stol(s);
		}
		catch (...)
		{
			cerr << "Bad block number/hash option: " << s << "\n";
			exit(-1);
		}
	};

	if (mode == OperationMode::Export)
	{
		ofstream fout(filename, std::ofstream::binary);
		ostream& out = (filename.empty() || filename == "--") ? cout : fout;

		unsigned last = toNumber(exportTo);
		for (unsigned i = toNumber(exportFrom); i <= last; ++i)
		{
			bytes block = web3.ethereum()->blockChain().block(web3.ethereum()->blockChain().numberHash(i));
			switch (exportFormat)
			{
				case Format::Binary: out.write((char const*)block.data(), block.size()); break;
				case Format::Hex: out << toHex(block) << "\n"; break;
				case Format::Human: out << RLP(block) << "\n"; break;
				default:;
			}
		}
		return 0;
	}

	if (mode == OperationMode::Import)
	{
		ifstream fin(filename, std::ifstream::binary);
		istream& in = (filename.empty() || filename == "--") ? cin : fin;
		unsigned alreadyHave = 0;
		unsigned good = 0;
		unsigned futureTime = 0;
		unsigned unknownParent = 0;
		unsigned bad = 0;
		chrono::steady_clock::time_point t = chrono::steady_clock::now();
		double last = 0;
		unsigned lastImported = 0;
		unsigned imported = 0;
		while (in.peek() != -1)
		{
			bytes block(8);
			in.read((char*)block.data(), 8);
			block.resize(RLP(block, RLP::LaissezFaire).actualSize());
			in.read((char*)block.data() + 8, block.size() - 8);

			switch (web3.ethereum()->queueBlock(block, safeImport))
			{
				case ImportResult::Success: good++; break;
				case ImportResult::AlreadyKnown: alreadyHave++; break;
				case ImportResult::UnknownParent: unknownParent++; break;
				case ImportResult::FutureTimeUnknown: unknownParent++; futureTime++; break;
				case ImportResult::FutureTimeKnown: futureTime++; break;
				default: bad++; break;
			}

			// sync chain with queue
			tuple<ImportRoute, bool, unsigned> r = web3.ethereum()->syncQueue(10);
			imported += get<2>(r);

			double e = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - t).count() / 1000.0;
			if ((unsigned)e >= last + 10)
			{
				auto i = imported - lastImported;
				auto d = e - last;
				cout << i << " more imported at " << (round(i * 10 / d) / 10) << " blocks/s. " << imported << " imported in " << e << " seconds at " << (round(imported * 10 / e) / 10) << " blocks/s (#" << web3.ethereum()->number() << ")" << "\n";
				last = (unsigned)e;
				lastImported = imported;
			}
		}

		bool moreToImport = true;
		while (moreToImport)
		{
			this_thread::sleep_for(chrono::seconds(1));
			tie(ignore, moreToImport, ignore) = web3.ethereum()->syncQueue(100000);
		}
		double e = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - t).count() / 1000.0;
		cout << imported << " imported in " << e << " seconds at " << (round(imported * 10 / e) / 10) << " blocks/s (#" << web3.ethereum()->number() << ")\n";
		return 0;
	}

	try
	{
		if (keyManager.exists())
		{
			if (!keyManager.load(masterPassword) && masterSet)
			{
				while (true)
				{
					masterPassword = getPassword("Please enter your MASTER password: ");
					if (keyManager.load(masterPassword))
						break;
					cout << "The password you entered is incorrect. If you have forgotten your password, and you wish to start afresh, manually remove the file: " << (getDataDir("ethereum") / fs::path("keys.info")).string() << "\n";
				}
			}
		}
		else
		{
			if (masterSet)
				keyManager.create(masterPassword);
			else
				keyManager.create(std::string());
		}
	}
	catch(...)
	{
		cerr << "Error initializing key manager: " << boost::current_exception_diagnostic_information() << "\n";
		return -1;
	}

	for (auto const& presale: presaleImports)
		importPresale(keyManager, presale, [&](){ return getPassword("Enter your wallet password for " + presale + ": "); });

	for (auto const& s: toImport)
	{
		keyManager.import(s, "Imported key (UNSAFE)");
	}

	cout << ethCredits();

	if (mode == OperationMode::ImportSnapshot)
	{
		try
		{
			auto stateImporter = web3.ethereum()->createStateImporter();
			auto blockChainImporter = web3.ethereum()->createBlockChainImporter();
			SnapshotImporter importer(*stateImporter, *blockChainImporter);

			auto snapshotStorage(createSnapshotStorage(filename));
			importer.import(*snapshotStorage);
			// continue with regular sync from the snapshot block
		}
		catch (...)
		{
			cerr << "Error during importing the snapshot: " << boost::current_exception_diagnostic_information() << endl;
			return -1;
		}
	}


	web3.setIdealPeerCount(peers);
	web3.setPeerStretch(peerStretch);
//	std::shared_ptr<eth::BasicGasPricer> gasPricer = make_shared<eth::BasicGasPricer>(u256(double(ether / 1000) / etherPrice), u256(blockFees * 1000));
	std::shared_ptr<eth::TrivialGasPricer> gasPricer = make_shared<eth::TrivialGasPricer>(askPrice, bidPrice);
	eth::Client* c = nodeMode == NodeMode::Full ? web3.ethereum() : nullptr;
	if (c)
	{
		c->setGasPricer(gasPricer);
		DEV_IGNORE_EXCEPTIONS(asEthashClient(c)->setShouldPrecomputeDAG(m.shouldPrecompute()));
		c->setSealer(m.minerType());
		c->setAuthor(author);
		if (networkID != NoNetworkID)
			c->setNetworkId(networkID);
	}

	auto renderFullAddress = [&](Address const& _a) -> std::string
	{
		return toUUID(keyManager.uuid(_a)) + " - " + _a.hex();
	};

	if (author)
		cout << "Mining Beneficiary: " << renderFullAddress(author) << "\n";

	if (bootstrap || !remoteHost.empty() || enableDiscovery || listenSet)
	{
		web3.startNetwork();
		cout << "Node ID: " << web3.enode() << "\n";
	}
	else
		cout << "Networking disabled. To start, use netstart or pass --bootstrap or a remote host.\n";

	unique_ptr<ModularServer<>> jsonrpcIpcServer;
	unique_ptr<rpc::SessionManager> sessionManager;
	unique_ptr<SimpleAccountHolder> accountHolder;

	AddressHash allowedDestinations;

	std::function<bool(TransactionSkeleton const&, bool)> authenticator;
	if (testingMode)
		authenticator = [](TransactionSkeleton const&, bool) -> bool { return true; };
	else
		authenticator = [&](TransactionSkeleton const& _t, bool isProxy) -> bool {
			// "unlockAccount" functionality is done in the AccountHolder.
			if (!alwaysConfirm || allowedDestinations.count(_t.to))
				return true;

			string r = getResponse(_t.userReadable(isProxy,
					[&](TransactionSkeleton const& _t) -> pair<bool, string>
					{
							h256 contractCodeHash = web3.ethereum()->postState().codeHash(_t.to);
							if (contractCodeHash == EmptySHA3)
									return std::make_pair(false, std::string());
							// TODO: actually figure out the natspec. we'll need the natspec database here though.
							return std::make_pair(true, std::string());
					}, [&](Address const& _a) { return _a.hex(); }
			) + "\nEnter yes/no/always (always to this address): ", {"yes", "n", "N", "no", "NO", "always"});
			if (r == "always")
				allowedDestinations.insert(_t.to);
			return r == "yes" || r == "always";
		};

	ExitHandler exitHandler;

	if (ipc)
	{
		using FullServer = ModularServer<
			rpc::EthFace,
			rpc::NetFace, rpc::Web3Face, rpc::PersonalFace,
			rpc::AdminEthFace, rpc::AdminNetFace,
			rpc::DebugFace, rpc::TestFace
		>;

		sessionManager.reset(new rpc::SessionManager());
		accountHolder.reset(new SimpleAccountHolder([&](){ return web3.ethereum(); }, getAccountPassword, keyManager, authenticator));
		auto ethFace = new rpc::Eth(*web3.ethereum(), *accountHolder.get());
		rpc::TestFace* testEth = nullptr;
		if (testingMode)
			testEth = new rpc::Test(*web3.ethereum());

		if (ipc)
		{
			jsonrpcIpcServer.reset(new FullServer(
					ethFace, new rpc::Whisper(web3, {}), new rpc::Net(web3),
					new rpc::Web3(web3.clientVersion()), new rpc::Personal(keyManager, *accountHolder, *web3.ethereum()),
					new rpc::AdminEth(*web3.ethereum(), *gasPricer.get(), keyManager, *sessionManager.get()),
					new rpc::AdminNet(web3, *sessionManager.get()),
					new rpc::AdminUtils(*sessionManager.get()),
					new rpc::Debug(*web3.ethereum()),
					testEth
			));
			auto ipcConnector = new IpcServer("geth");
			jsonrpcIpcServer->addConnector(ipcConnector);
			ipcConnector->StartListening();
		}

		if (jsonAdmin.empty())
			jsonAdmin = sessionManager->newSession(rpc::SessionPermissions{{rpc::Privilege::Admin}});
		else
			sessionManager->addSession(jsonAdmin, rpc::SessionPermissions{{rpc::Privilege::Admin}});

		cout << "JSONRPC Admin Session Key: " << jsonAdmin << "\n";
	}

	for (auto const& p: preferredNodes)
		if (p.second.second)
			web3.requirePeer(p.first, p.second.first);
		else
			web3.addNode(p.first, p.second.first);

	if (bootstrap && privateChain.empty())
		for (auto const& i: Host::pocHosts())
			web3.requirePeer(i.first, i.second);
	if (!remoteHost.empty())
		web3.addNode(p2p::NodeID(), remoteHost + ":" + toString(remotePort));

	signal(SIGABRT, &ExitHandler::exitHandler);
	signal(SIGTERM, &ExitHandler::exitHandler);
	signal(SIGINT, &ExitHandler::exitHandler);

	if (c)
	{
		unsigned n = c->blockChain().details().number;
		if (mining)
			c->startSealing();

		while (!exitHandler.shouldExit())
			stopSealingAfterXBlocks(c, n, mining);
	}
	else
		while (!exitHandler.shouldExit())
			this_thread::sleep_for(chrono::milliseconds(1000));

	if (jsonrpcIpcServer.get())
		jsonrpcIpcServer->StopListening();

	auto netData = web3.saveNetwork();
	if (!netData.empty())
		writeFile(getDataDir() / fs::path("network.rlp"), netData);
	return 0;
}