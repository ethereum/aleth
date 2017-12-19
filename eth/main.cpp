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

namespace
{

std::atomic<bool> g_silence = {false};
unsigned const c_lineWidth = 160;

string ethCredits(bool _interactive = false)
{
	std::ostringstream cout;
	if (_interactive)
		cout
			<< "Type 'exit' to quit\n\n";
	return credits() + cout.str();
}

void version()
{
	cout << "eth version " << dev::Version << "\n";
	cout << "eth network protocol version: " << dev::eth::c_protocolVersion << "\n";
	cout << "Client database version: " << dev::eth::c_databaseVersion << "\n";
	cout << "Build: " << DEV_QUOTED(ETH_BUILD_PLATFORM) << "/" << DEV_QUOTED(ETH_BUILD_TYPE) << "\n";
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

}

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
	string privateChain;

	bool upnp = true;
	WithExisting withExisting = WithExisting::Trust;

	/// Networking params.
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
	
	po::options_description clientDefaultMode("Client mode (default)", c_lineWidth);
	clientDefaultMode.add_options()
		("mainnet", "Use the main network protocol.")
		("ropsten", "Use the Ropsten testnet.")
		("private", po::value<string>()->value_name("<name>"), "Use a private chain.")
		("test", "Testing mode: Disable PoW and provide test rpc interface.")
		("config", po::value<string>()->value_name("<file>"), "Configure specialised blockchain using given JSON information.\n")
		("genesis", po::value<string>()->value_name("<file>"), "Set genesis JSON file.")
		("mode,o", po::value<string>()->value_name("<full/peer>"), "Start a full node or a peer node (default: full).\n")
		("ipc", "Enable IPC server (default: on).")
		("ipcpath", po::value<string>()->value_name("<path>"), "Set .ipc socket path (default: data directory)")
		("no-ipc", "Disable IPC server.")
		("admin", po::value<string>()->value_name("<password>"), "Specify admin session key for JSON-RPC (default: auto-generated and printed at start-up).")
		("kill,K", "Kill the blockchain first.")
		("rebuild,R", "Rebuild the blockchain from the existing database.")
		("rescue", "Attempt to rescue a corrupt database.\n")
		("import-presale", po::value<string>()->value_name("<file>"), "Import a pre-sale key; you'll need to specify the password to this key.")
		("import-secret,s", po::value<string>()->value_name("<secret>"), "Import a secret key into the key store.")
		("import-session-secret,S", po::value<string>()->value_name("<secret>"), "Import a secret session into the key store.")
		("master", po::value<string>()->value_name("<password>"), "Give the master password for the key store. Use --master \"\" to show a prompt.")
		("password", po::value<string>()->value_name("<password>"), "Give a password for a private key.\n");
	
	po::options_description clientTransacting("Client transacting", c_lineWidth);
	clientTransacting.add_options()
		("ask", po::value<u256>()->value_name("<wei>"), ("Set the minimum ask gas price under which no transaction will be mined\n(default " + toString(DefaultGasPrice) + ").").c_str())
		("bid", po::value<u256>()->value_name("<wei>"), ("Set the bid gas price to pay for transactions\n(default " + toString(DefaultGasPrice) + ").").c_str())
		("unsafe-transactions", "Allow all transactions to proceed without verification. EXTREMELY UNSAFE.\n");
	
	po::options_description clientMining("Client mining", c_lineWidth);
	clientMining.add_options()
		("address,a", po::value<Address>()->value_name("<addr>"), "Set the author (mining payout) address to given address (default: auto).")
		("mining,m", po::value<string>()->value_name("<on/off/number>"), "Enable mining, optionally for a specified number of blocks (default: off).")
		("extra-data", po::value<string>(), "Set extra data for the sealed blocks.\n");

	po::options_description clientNetworking("Client networking", c_lineWidth);
	clientNetworking.add_options()
		("bootstrap,b",  "Connect to the default Ethereum peer servers (default unless --no-discovery used).")
		("no-bootstrap",  "Do not connect to the default Ethereum peer servers (default only when --no-discovery is used).")
		("peers,x", po::value<int>()->value_name("<number>"), "Attempt to connect to a given number of peers (default: 11).")
		("peer-stretch", po::value<int>()->value_name("<number>"), "Give the accepted connection multiplier (default: 7).")
		("public-ip", po::value<string>()->value_name("<ip>"), "Force advertised public IP to the given IP (default: auto).")
		("listen-ip", po::value<string>()->value_name("<ip>(:<port>)"), "Listen on the given IP for incoming connections (default: 0.0.0.0).")
		("listen", po::value<unsigned short>()->value_name("<port>"), "Listen on the given port for incoming connections (default: 30303).")
		("remote,r", po::value<string>()->value_name("<host>(:<port>)"), "Connect to the given remote host (default: none).")
		("port", po::value<short>()->value_name("<port>"), "Connect to the given remote port (default: 30303).")
		("network-id", po::value<unsigned>()->value_name("<n>"), "Only connect to other hosts with this network id.")
		("upnp", po::value<string>()->value_name("<on/off>"), "Use UPnP for NAT (default: on).")
		("peerset", po::value<string>()->value_name("<list>"), "Space delimited list of peers; element format: type:publickey@ipAddress[:port].\n        Types:\n        default     Attempt connection when no other peers are available and pinning is disabled.\n        required	    Keep connected at all times.\n")
		("no-discovery",  "Disable node discovery, implies --no-bootstrap.")
		("pin",  "Only accept or connect to trusted peers.")
		("hermit",  "Equivalent to --no-discovery --pin.")
		("sociable",  "Force discovery and no pinning.\n");
	
	po::options_description importExportMode("Import/export modes", c_lineWidth);
	importExportMode.add_options()
		("import,I", po::value<string>()->value_name("<file>"), "Import blocks from file.")
		("export,E", po::value<string>()->value_name("<file>"), "Export blocks to file.")
		("from", po::value<string>()->value_name("<n>"), "Export only from block n; n may be a decimal, a '0x' prefixed hash, or 'latest'.")
		("to", po::value<string>()->value_name("<n>"), "Export only to block n (inclusive); n may be a decimal, a '0x' prefixed hash, or 'latest'.")
		("only", po::value<string>()->value_name("<n>"), "Equivalent to --export-from n --export-to n.")
		("format", po::value<string>()->value_name("<binary/hex/human>"), "Set export format.")
		("dont-check", "Prevent checking some block aspects. Faster importing, but to apply only when the data is known to be valid.")
		("import-snapshot", po::value<string>()->value_name("<path>"), "Import blockchain and state data from the Parity Warp Sync snapshot.\n");
	
	po::options_description generalOptions("General Options", c_lineWidth);
	generalOptions.add_options()
		("db-path,d", po::value<string>()->value_name("<path>"), ("Load database from path\n(default: " + getDataDir().string() + ").\n").c_str())
#if ETH_EVMJIT
		("vm", "<vm-kind>  Select VM; options are: interpreter, jit or smart (default: interpreter)")
#endif // ETH_EVMJIT
		("verbosity,v", po::value<int>()->value_name("<0 - 9>"), "Set the log verbosity from 0 to 9 (default: 8).")
		("version,V",  "Show the version and exit.")
		("help,h",  "Show this help message and exit.\n");
	

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
		if (!m.interpretOption(i, unrecognisedOptions))
		{
			cerr << "Invalid argument: " << unrecognisedOptions[i] << "\n";
			return -1;
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
	if (vm.count("import-snapshot"))
	{
		mode = OperationMode::ImportSnapshot;
		filename = vm["import-snapshot"].as<string>();
	}
	if (vm.count("version"))
	{
		version();
		return 0;
	}
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
	if (vm.count("admin"))
		jsonAdmin = vm["admin"].as<string>();
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
	if (vm.count("db-path"))
		setDataDir(vm["db-path"].as<string>());
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
	if (vm.count("mainnet"))
	{
		chainParams = ChainParams(genesisInfo(eth::Network::MainNetwork), genesisStateRoot(eth::Network::MainNetwork));
		chainConfigIsSet = true;
	}
	if (vm.count("ropsten"))
	{
		chainParams = ChainParams(genesisInfo(eth::Network::Ropsten), genesisStateRoot(eth::Network::Ropsten));
		chainConfigIsSet = true;
	}
	if (vm.count("ask"))
	{
		try
		{
			askPrice = vm["ask"].as<u256>();
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
			bidPrice = vm["bid"].as<u256>();
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
		listenPort = vm["listen"].as<unsigned short>();
		listenSet = true;
	}
	if (vm.count("public-ip")) {
		publicIP = vm["public-ip"].as<string>();
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
	if (vm.count("import"))
	{
		mode = OperationMode::Import;
		filename = vm["import"].as<string>();
	}
	if (vm.count("export"))
	{
		mode = OperationMode::Export;
		filename = vm["export"].as<string>();
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
			networkID = vm["network-id"].as<unsigned>();
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
	if (vm.count("kill"))
		withExisting = WithExisting::Kill;
	if (vm.count("rebuild"))
		withExisting = WithExisting::Verify;
	if (vm.count("rescue"))
		withExisting = WithExisting::Rescue;
	if (vm.count("address"))
		try
		{
			author = vm["address"].as<Address>();
		}
		catch (BadHexCharacter&)
		{
			cerr << "Bad hex in " << "--address" << " option: " << vm["address"].as<string>() << "\n";
			return -1;
		}
		catch (...)
		{
			cerr << "Bad " << "--address" << " option: " << vm["address"].as<string>() << "\n";
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
		cout
				<< "Usage eth [OPTIONS]\n"
				<< "Options:\n\n"
				<< "Wallet usage:\n";
		AccountManager::streamAccountHelp(cout);
		AccountManager::streamWalletHelp(cout);
		cout << clientDefaultMode << clientTransacting << clientMining << clientNetworking;
		MinerCLI::streamHelp(cout);
		cout << importExportMode << generalOptions;
		return 0;
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

	if (!chainConfigIsSet)
		// default to mainnet if not already set with any of `--mainnet`, `--ropsten`, `--genesis`, `--config`
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
	auto caps = set<string>{"eth"};

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
			return -1;
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
			importer.import(*snapshotStorage, web3.ethereum()->blockChain().genesisHash());
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

	if (bootstrap || !remoteHost.empty() || enableDiscovery || listenSet || !preferredNodes.empty())
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

		jsonrpcIpcServer.reset(new FullServer(
			ethFace, new rpc::Net(web3),
			new rpc::Web3(web3.clientVersion()), new rpc::Personal(keyManager, *accountHolder, *web3.ethereum()),
			new rpc::AdminEth(*web3.ethereum(), *gasPricer.get(), keyManager, *sessionManager.get()),
			new rpc::AdminNet(web3, *sessionManager.get()),
			new rpc::Debug(*web3.ethereum()),
			testEth
		));
		auto ipcConnector = new IpcServer("geth");
		jsonrpcIpcServer->addConnector(ipcConnector);
		ipcConnector->StartListening();

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