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

#include <libdevcore/FileSystem.h>
#include <libethashseal/EthashAux.h>
#include <libevm/VM.h>
#include <libevm/VMFactory.h>
#include <libethcore/KeyManager.h>
#include <libethcore/ICAP.h>
#include <libethereum/Defaults.h>
#include <libethereum/BlockChainSync.h>
#include <libethashseal/EthashClient.h>
#include <libethashseal/GenesisInfo.h>
#include <libwebthree/WebThree.h>

#include <libweb3jsonrpc/AccountHolder.h>
#include <libweb3jsonrpc/Eth.h>
#include <libweb3jsonrpc/SafeHttpServer.h>
#include <jsonrpccpp/client/connectors/httpclient.h>
#include <libweb3jsonrpc/ModularServer.h>
#include <libweb3jsonrpc/IpcServer.h>
#include <libweb3jsonrpc/LevelDB.h>
#include <libweb3jsonrpc/Whisper.h>
#include <libweb3jsonrpc/Net.h>
#include <libweb3jsonrpc/Web3.h>
#include <libweb3jsonrpc/AdminNet.h>
#include <libweb3jsonrpc/AdminEth.h>
#include <libweb3jsonrpc/AdminUtils.h>
#include <libweb3jsonrpc/Personal.h>
#include <libweb3jsonrpc/Debug.h>
#include <libweb3jsonrpc/Test.h>

#include <ethminer/MinerAux.h>
#include "BuildInfo.h"
#include "AccountManager.h"

using namespace std;
using namespace dev;
using namespace dev::p2p;
using namespace dev::eth;
using namespace boost::algorithm;

static std::atomic<bool> g_silence = {false};

void help()
{
	cout
		<< "Usage eth [OPTIONS]" << endl
		<< "Options:" << endl << endl
		<< "Wallet usage:" << endl;
	AccountManager::streamAccountHelp(cout);
	AccountManager::streamWalletHelp(cout);
	cout
		<< endl;
	cout
		<< "Client mode (default):" << endl
		<< "    --mainnet  Use the main network protocol." << endl
		<< "    --ropsten  Use the Ropsten testnet." << endl
		<< "    --private <name>  Use a private chain." << endl
		<< "    --test  Testing mode: Disable PoW and provide test rpc interface." << endl
		<< "    --config <file>  Configure specialised blockchain using given JSON information." << endl
		<< "    --oppose-dao-fork  Ignore DAO hard fork (default is to participate)." << endl
		<< endl
		<< "    -o,--mode <full/peer>  Start a full node or a peer node (default: full)." << endl
		<< endl
		<< "    -j,--json-rpc  Enable JSON-RPC server (default: off)." << endl
		<< "    --ipc  Enable IPC server (default: on)." << endl
		<< "    --ipcpath Set .ipc socket path (default: data directory)" << endl
		<< "    --admin-via-http  Expose admin interface via http - UNSAFE! (default: off)." << endl
		<< "    --no-ipc  Disable IPC server." << endl
		<< "    --json-rpc-port <n>  Specify JSON-RPC server port (implies '-j', default: " << SensibleHttpPort << ")." << endl
		<< "    --rpccorsdomain <domain>  Domain on which to send Access-Control-Allow-Origin header." << endl
		<< "    --admin <password>  Specify admin session key for JSON-RPC (default: auto-generated and printed at start-up)." << endl
		<< "    -K,--kill  Kill the blockchain first." << endl
		<< "    -R,--rebuild  Rebuild the blockchain from the existing database." << endl
		<< "    --rescue  Attempt to rescue a corrupt database." << endl
		<< endl
		<< "    --import-presale <file>  Import a pre-sale key; you'll need to specify the password to this key." << endl
		<< "    -s,--import-secret <secret>  Import a secret key into the key store." << endl
		<< "    --master <password>  Give the master password for the key store. Use --master \"\" to show a prompt." << endl
		<< "    --password <password>  Give a password for a private key." << endl
		<< endl
		<< "Client transacting:" << endl
		/*<< "    -B,--block-fees <n>  Set the block fee profit in the reference unit, e.g. ¢ (default: 15)." << endl
		<< "    -e,--ether-price <n>  Set the ether price in the reference unit, e.g. ¢ (default: 30.679)." << endl
		<< "    -P,--priority <0 - 100>  Set the default priority percentage (%) of a transaction (default: 50)." << endl*/
		<< "    --ask <wei>  Set the minimum ask gas price under which no transaction will be mined (default " << toString(DefaultGasPrice) << " )." << endl
		<< "    --bid <wei>  Set the bid gas price to pay for transactions (default " << toString(DefaultGasPrice) << " )." << endl
		<< "    --unsafe-transactions  Allow all transactions to proceed without verification. EXTREMELY UNSAFE."
		<< endl
		<< "Client mining:" << endl
		<< "    -a,--address <addr>  Set the author (mining payout) address to given address (default: auto)." << endl
		<< "    -m,--mining <on/off/number>  Enable mining, optionally for a specified number of blocks (default: off)." << endl
		<< "    -f,--force-mining  Mine even when there are no transactions to mine (default: off)." << endl
		<< "    -C,--cpu  When mining, use the CPU." << endl
		<< "    -t, --mining-threads <n>  Limit number of CPU/GPU miners to n (default: use everything available on selected platform)." << endl
		<< endl
		<< "Client networking:" << endl
		<< "    --client-name <name>  Add a name to your client's version string (default: blank)." << endl
		<< "    --bootstrap  Connect to the default Ethereum peer servers (default unless --no-discovery used)." << endl
		<< "    --no-bootstrap  Do not connect to the default Ethereum peer servers (default only when --no-discovery is used)." << endl
		<< "    -x,--peers <number>  Attempt to connect to a given number of peers (default: 11)." << endl
		<< "    --peer-stretch <number>  Give the accepted connection multiplier (default: 7)." << endl

		<< "    --public-ip <ip>  Force advertised public IP to the given IP (default: auto)." << endl
		<< "    --listen-ip <ip>(:<port>)  Listen on the given IP for incoming connections (default: 0.0.0.0)." << endl
		<< "    --listen <port>  Listen on the given port for incoming connections (default: 30303)." << endl
		<< "    -r,--remote <host>(:<port>)  Connect to the given remote host (default: none)." << endl
		<< "    --port <port>  Connect to the given remote port (default: 30303)." << endl
		<< "    --network-id <n>  Only connect to other hosts with this network id." << endl
		<< "    --upnp <on/off>  Use UPnP for NAT (default: on)." << endl

		<< "    --peerset <list>  Space delimited list of peers; element format: type:publickey@ipAddress[:port]." << endl
		<< "        Types:" << endl
		<< "        default		Attempt connection when no other peers are available and pinning is disabled." << endl
		<< "        required		Keep connected at all times." << endl
// TODO:
//		<< "	--trust-peers <filename>  Space delimited list of publickeys." << endl

		<< "    --no-discovery  Disable node discovery, implies --no-bootstrap." << endl
		<< "    --pin  Only accept or connect to trusted peers." << endl
		<< "    --hermit  Equivalent to --no-discovery --pin." << endl
		<< "    --sociable  Force discovery and no pinning." << endl
		<< endl;
	MinerCLI::streamHelp(cout);
	cout
		<< "Import/export modes:" << endl
		<< "    --from <n>  Export only from block n; n may be a decimal, a '0x' prefixed hash, or 'latest'." << endl
		<< "    --to <n>  Export only to block n (inclusive); n may be a decimal, a '0x' prefixed hash, or 'latest'." << endl
		<< "    --only <n>  Equivalent to --export-from n --export-to n." << endl
		<< "    --dont-check  Prevent checking some block aspects. Faster importing, but to apply only when the data is known to be valid." << endl
		<< endl
		<< "General Options:" << endl
		<< "    -d,--db-path,--datadir <path>  Load database from path (default: " << getDataDir() << ")." << endl
#if ETH_EVMJIT
		<< "    --vm <vm-kind>  Select VM; options are: interpreter, jit or smart (default: interpreter)." << endl
#endif // ETH_EVMJIT
		<< "    -v,--verbosity <0 - 9>  Set the log verbosity from 0 to 9 (default: 8)." << endl
		<< "    -V,--version  Show the version and exit." << endl
		<< "    -h,--help  Show this help message and exit." << endl
		<< endl
		<< "Experimental / Proof of Concept:" << endl
		<< "    --shh  Enable Whisper." << endl
		<< endl
		;
		exit(0);
}

string ethCredits(bool _interactive = false)
{
	std::ostringstream cout;
	if (_interactive)
		cout
			<< "Type 'exit' to quit" << endl << endl;
	return credits() + cout.str();
}

void version()
{
	cout << "eth version " << dev::Version << endl;
	cout << "eth network protocol version: " << dev::eth::c_protocolVersion << endl;
	cout << "Client database version: " << dev::eth::c_databaseVersion << endl;
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

class ExitHandler: public rpc::SystemManager
{
public:
	void exit() { exitHandler(0); }
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

#if ETH_DEBUG
	g_logVerbosity = 4;
#else
	g_logVerbosity = 1;
#endif

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
	bool interactive = false;

	int jsonRPCURL = -1;
	bool adminViaHttp = false;
	bool ipc = true;
	std::string rpcCorsDomain = "";

	string jsonAdmin;
	ChainParams chainParams(genesisInfo(eth::Network::MainNetwork), genesisStateRoot(eth::Network::MainNetwork));
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

	string configFile = getDataDir() + "/config.rlp";
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
	string configJSON;
	string genesisJSON;
	for (int i = 1; i < argc; ++i)
	{
		string arg = argv[i];
		if (m.interpretOption(i, argc, argv))
		{
		}
		else if (arg == "--listen-ip" && i + 1 < argc)
		{
			listenIP = argv[++i];
			listenSet = true;
		}
		else if ((arg == "--listen" || arg == "--listen-port") && i + 1 < argc)
		{
			listenPort = (short)atoi(argv[++i]);
			listenSet = true;
		}
		else if ((arg == "--public-ip" || arg == "--public") && i + 1 < argc)
		{
			publicIP = argv[++i];
		}
		else if ((arg == "-r" || arg == "--remote") && i + 1 < argc)
		{
			string host = argv[++i];
			string::size_type found = host.find_first_of(':');
			if (found != std::string::npos)
			{
				remoteHost = host.substr(0, found);
				remotePort = (short)atoi(host.substr(found + 1, host.length()).c_str());
			}
			else
				remoteHost = host;
		}
		else if (arg == "--port" && i + 1 < argc)
		{
			remotePort = (short)atoi(argv[++i]);
		}
		else if (arg == "--password" && i + 1 < argc)
			passwordsToNote.push_back(argv[++i]);
		else if (arg == "--master" && i + 1 < argc)
		{
			masterPassword = argv[++i];
			masterSet = true;
		}
		else if ((arg == "-I" || arg == "--import" || arg == "import") && i + 1 < argc)
		{
			mode = OperationMode::Import;
			filename = argv[++i];
		}
		else if (arg == "--dont-check")
			safeImport = true;
		else if ((arg == "-E" || arg == "--export" || arg == "export") && i + 1 < argc)
		{
			mode = OperationMode::Export;
			filename = argv[++i];
		}
		else if (arg == "--script" && i + 1 < argc)
			scripts.push_back(argv[++i]);
		else if (arg == "--format" && i + 1 < argc)
		{
			string m = argv[++i];
			if (m == "binary")
				exportFormat = Format::Binary;
			else if (m == "hex")
				exportFormat = Format::Hex;
			else if (m == "human")
				exportFormat = Format::Human;
			else
			{
				cerr << "Bad " << arg << " option: " << m << endl;
				return -1;
			}
		}
		else if (arg == "--to" && i + 1 < argc)
			exportTo = argv[++i];
		else if (arg == "--from" && i + 1 < argc)
			exportFrom = argv[++i];
		else if (arg == "--only" && i + 1 < argc)
			exportTo = exportFrom = argv[++i];
		else if (arg == "--upnp" && i + 1 < argc)
		{
			string m = argv[++i];
			if (isTrue(m))
				upnp = true;
			else if (isFalse(m))
				upnp = false;
			else
			{
				cerr << "Bad " << arg << " option: " << m << endl;
				return -1;
			}
		}
		else if (arg == "--network-id" && i + 1 < argc)
			try {
				networkID = stol(argv[++i]);
			}
			catch (...)
			{
				cerr << "Bad " << arg << " option: " << argv[i] << endl;
				return -1;
			}
		else if (arg == "--private" && i + 1 < argc)
			try {
				privateChain = argv[++i];
			}
			catch (...)
			{
				cerr << "Bad " << arg << " option: " << argv[i] << endl;
				return -1;
			}
		else if (arg == "--independent" && i + 1 < argc)
			try {
				privateChain = argv[++i];
				noPinning = enableDiscovery = true;
			}
			catch (...)
			{
				cerr << "Bad " << arg << " option: " << argv[i] << endl;
				return -1;
			}
		else if (arg == "-K" || arg == "--kill-blockchain" || arg == "--kill")
			withExisting = WithExisting::Kill;
		else if (arg == "-R" || arg == "--rebuild")
			withExisting = WithExisting::Verify;
		else if (arg == "-R" || arg == "--rescue")
			withExisting = WithExisting::Rescue;
		else if (arg == "--client-name" && i + 1 < argc)
			clientName = argv[++i];
		else if ((arg == "-a" || arg == "--address" || arg == "--author") && i + 1 < argc)
			try {
				author = h160(fromHex(argv[++i], WhenError::Throw));
			}
			catch (BadHexCharacter&)
			{
				cerr << "Bad hex in " << arg << " option: " << argv[i] << endl;
				return -1;
			}
			catch (...)
			{
				cerr << "Bad " << arg << " option: " << argv[i] << endl;
				return -1;
			}
		else if ((arg == "-s" || arg == "--import-secret") && i + 1 < argc)
		{
			Secret s(fromHex(argv[++i]));
			toImport.emplace_back(s);
		}
		else if ((arg == "-S" || arg == "--import-session-secret") && i + 1 < argc)
		{
			Secret s(fromHex(argv[++i]));
			toImport.emplace_back(s);
		}
		else if ((arg == "-d" || arg == "--path" || arg == "--db-path" || arg == "--datadir") && i + 1 < argc)
			setDataDir(argv[++i]);
		else if (arg == "--ipcpath" && i + 1 < argc )
			setIpcPath(argv[++i]);
		else if ((arg == "--genesis-json" || arg == "--genesis") && i + 1 < argc)
		{
			try
			{
				genesisJSON = contentsString(argv[++i]);
			}
			catch (...)
			{
				cerr << "Bad " << arg << " option: " << argv[i] << endl;
				return -1;
			}
		}
		else if (arg == "--config" && i + 1 < argc)
		{
			try
			{
				configJSON = contentsString(argv[++i]);
			}
			catch (...)
			{
				cerr << "Bad " << arg << " option: " << argv[i] << endl;
				return -1;
			}
		}
		else if (arg == "--extra-data" && i + 1 < argc)
		{
			try
			{
				extraData = fromHex(argv[++i]);
			}
			catch (...)
			{
				cerr << "Bad " << arg << " option: " << argv[i] << endl;
				return -1;
			}
		}
		else if (arg == "--gas-floor" && i + 1 < argc)
			gasFloor = u256(argv[++i]);
		else if (arg == "--mainnet")
			chainParams = ChainParams(genesisInfo(eth::Network::MainNetwork), genesisStateRoot(eth::Network::MainNetwork));
		else if (arg == "--ropsten" || arg == "--testnet")
			chainParams = ChainParams(genesisInfo(eth::Network::Ropsten), genesisStateRoot(eth::Network::Ropsten));
		else if (arg == "--oppose-dao-fork")
		{
			chainParams = ChainParams(genesisInfo(eth::Network::MainNetwork), genesisStateRoot(eth::Network::MainNetwork));
			chainParams.otherParams["daoHardforkBlock"] = toHex(u256(-1) - 10, HexPrefix::Add);
		}
		else if (arg == "--support-dao-fork")
		{
			// default
		}
		else if (arg == "--bob")
		{
			cout << "Asking Bob for blocks (this should work in theoreum)..." << endl;
			while (true)
			{
				u256 x(h256::random());
				u256 c;
				for (; x != 1; ++c)
				{
					x = (x & 1) == 0 ? x / 2 : 3 * x + 1;
					cout << toHex(x) << endl;
					this_thread::sleep_for(chrono::seconds(1));
				}
				cout << "Block number: " << hex << c << endl;
				exit(0);
			}
		}
/*		else if ((arg == "-B" || arg == "--block-fees") && i + 1 < argc)
		{
			try
			{
				blockFees = stof(argv[++i]);
			}
			catch (...)
			{
				cerr << "Bad " << arg << " option: " << argv[i] << endl;
				return -1;
			}
		}
		else if ((arg == "-e" || arg == "--ether-price") && i + 1 < argc)
		{
			try
			{
				etherPrice = stof(argv[++i]);
			}
			catch (...)
			{
				cerr << "Bad " << arg << " option: " << argv[i] << endl;
				return -1;
			}
		}*/
		else if (arg == "--ask" && i + 1 < argc)
		{
			try
			{
				askPrice = u256(argv[++i]);
			}
			catch (...)
			{
				cerr << "Bad " << arg << " option: " << argv[i] << endl;
				return -1;
			}
		}
		else if (arg == "--bid" && i + 1 < argc)
		{
			try
			{
				bidPrice = u256(argv[++i]);
			}
			catch (...)
			{
				cerr << "Bad " << arg << " option: " << argv[i] << endl;
				return -1;
			}
		}
/*		else if ((arg == "-P" || arg == "--priority") && i + 1 < argc)
		{
			string m = boost::to_lower_copy(string(argv[++i]));
			if (m == "lowest")
				priority = TransactionPriority::Lowest;
			else if (m == "low")
				priority = TransactionPriority::Low;
			else if (m == "medium" || m == "mid" || m == "default" || m == "normal")
				priority = TransactionPriority::Medium;
			else if (m == "high")
				priority = TransactionPriority::High;
			else if (m == "highest")
				priority = TransactionPriority::Highest;
			else
				try {
					priority = (TransactionPriority)(max(0, min(100, stoi(m))) * 8 / 100);
				}
				catch (...) {
					cerr << "Unknown " << arg << " option: " << m << endl;
					return -1;
				}
		}*/
		else if ((arg == "-m" || arg == "--mining") && i + 1 < argc)
		{
			string m = argv[++i];
			if (isTrue(m))
				mining = ~(unsigned)0;
			else if (isFalse(m))
				mining = 0;
			else
				try {
					mining = stoi(m);
				}
				catch (...) {
					cerr << "Unknown " << arg << " option: " << m << endl;
					return -1;
				}
		}
		else if (arg == "-b" || arg == "--bootstrap")
			bootstrap = true;
		else if (arg == "--no-bootstrap")
			bootstrap = false;
		else if (arg == "--no-discovery")
		{
			disableDiscovery = true;
			bootstrap = false;
		}
		else if (arg == "--pin")
			pinning = true;
		else if (arg == "--hermit")
			pinning = disableDiscovery = true;
		else if (arg == "--sociable")
			noPinning = enableDiscovery = true;
		else if (arg == "--unsafe-transactions")
			alwaysConfirm = false;
		else if (arg == "--import-presale" && i + 1 < argc)
			presaleImports.push_back(argv[++i]);
		else if (arg == "--old-interactive")
			interactive = true;

		else if ((arg == "-j" || arg == "--json-rpc"))
			jsonRPCURL = jsonRPCURL == -1 ? SensibleHttpPort : jsonRPCURL;
		else if (arg == "--admin-via-http")
			adminViaHttp = true;
		else if (arg == "--json-rpc-port" && i + 1 < argc)
			jsonRPCURL = atoi(argv[++i]);
		else if (arg == "--rpccorsdomain" && i + 1 < argc)
			rpcCorsDomain = argv[++i];
		else if (arg == "--json-admin" && i + 1 < argc)
			jsonAdmin = argv[++i];
		else if (arg == "--ipc")
			ipc = true;
		else if (arg == "--no-ipc")
			ipc = false;

		else if ((arg == "-v" || arg == "--verbosity") && i + 1 < argc)
			g_logVerbosity = atoi(argv[++i]);
		else if ((arg == "-x" || arg == "--peers") && i + 1 < argc)
			peers = atoi(argv[++i]);
		else if (arg == "--peer-stretch" && i + 1 < argc)
			peerStretch = atoi(argv[++i]);
		else if (arg == "--peerset" && i + 1 < argc)
		{
			string peerset = argv[++i];
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
					cerr << "Unrecognized peerset: " << peerset << endl;
					return -1;
				}
			}
		}
		else if ((arg == "-o" || arg == "--mode") && i + 1 < argc)
		{
			string m = argv[++i];
			if (m == "full")
				nodeMode = NodeMode::Full;
			else if (m == "peer")
				nodeMode = NodeMode::PeerServer;
			else
			{
				cerr << "Unknown mode: " << m << endl;
				return -1;
			}
		}
#if ETH_EVMJIT
		else if (arg == "--vm" && i + 1 < argc)
		{
			string vmKind = argv[++i];
			if (vmKind == "interpreter")
				VMFactory::setKind(VMKind::Interpreter);
			else if (vmKind == "jit")
				VMFactory::setKind(VMKind::JIT);
			else if (vmKind == "smart")
				VMFactory::setKind(VMKind::Smart);
			else
			{
				cerr << "Unknown VM kind: " << vmKind << endl;
				return -1;
			}
		}
#endif
		else if (arg == "--shh")
			useWhisper = true;
		else if (arg == "-h" || arg == "--help")
			help();
		else if (arg == "-V" || arg == "--version")
			version();
		else if (arg == "--test")
		{
			testingMode = true;
			enableDiscovery = false;
			disableDiscovery = true;
			noPinning = true;
			bootstrap = false;
		}
		else
		{
			cerr << "Invalid argument: " << arg << endl;
			exit(-1);
		}
	}

	if (!configJSON.empty())
	{
		try
		{
			chainParams = chainParams.loadConfig(configJSON);
		}
		catch (...)
		{
			cerr << "provided configuration is not well formatted" << endl;
			cerr << "sample: " << endl << genesisInfo(eth::Network::MainNetworkTest) << endl;
			return 0;
		}
	}


	if (!genesisJSON.empty())
	{
		try
		{
			chainParams = chainParams.loadGenesis(genesisJSON);
		}
		catch (...)
		{
			cerr << "provided genesis block description is not well formatted" << endl;
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
			cerr << "sample: " << endl << genesisSample << endl;
			return 0;
		}
	}


	if (!privateChain.empty())
	{
		chainParams.extraData = sha3(privateChain).asBytes();
		chainParams.difficulty = chainParams.u256Param("minimumDifficulty");
		chainParams.gasLimit = u256(1) << 32;
	}
	// TODO: Open some other API path
//	if (gasFloor != Invalid256)
//		c_gasFloorTarget = gasFloor;

	if (g_logVerbosity > 0)
		cout << EthGrayBold "cpp-ethereum, a C++ Ethereum client" EthReset << endl;

	m.execute();

	std::string secretsPath;
	if (testingMode)
		secretsPath = (boost::filesystem::path(getDataDir()) / "keystore").string();
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
	if (interactive)
		g_logPost = [&](std::string const& a, char const*){
			static SpinLock s_lock;
			SpinGuard l(s_lock);

			if (g_silence)
				logbuf += a + "\n";
			else
				cout << "\r           \r" << a << endl << additional << flush;

			// helpful to use OutputDebugString on windows
	#if defined(_WIN32)
			{
				OutputDebugStringA(a.data());
				OutputDebugStringA("\n");
			}
	#endif
		};

	auto getPassword = [&](string const& prompt) {
		bool s = g_silence;
		g_silence = true;
		cout << endl;
		string ret = dev::getPassword(prompt);
		g_silence = s;
		return ret;
	};
	auto getResponse = [&](string const& prompt, unordered_set<string> const& acceptable) {
		bool s = g_silence;
		g_silence = true;
		cout << endl;
		string ret;
		while (true)
		{
			cout << prompt;
			getline(cin, ret);
			if (acceptable.count(ret))
				break;
			cout << "Invalid response: " << ret << endl;
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

	auto nodesState = contents(getDataDir() + "/network.rlp");
	auto caps = useWhisper ? set<string>{"eth", "shh"} : set<string>{"eth"};

	if (testingMode)
	{
		chainParams.sealEngineName = "NoProof";
		chainParams.otherParams["allowFutureBlocks"] = "1";
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
		try {
			return stol(s);
		}
		catch (...)
		{
			cerr << "Bad block number/hash option: " << s << endl;
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
			case Format::Hex: out << toHex(block) << endl; break;
			case Format::Human: out << RLP(block) << endl; break;
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
				cout << i << " more imported at " << (round(i * 10 / d) / 10) << " blocks/s. " << imported << " imported in " << e << " seconds at " << (round(imported * 10 / e) / 10) << " blocks/s (#" << web3.ethereum()->number() << ")" << endl;
				last = (unsigned)e;
				lastImported = imported;
//				cout << web3.ethereum()->blockQueueStatus() << endl;
			}
		}

		bool moreToImport = true;
		while (moreToImport)
		{
			this_thread::sleep_for(chrono::seconds(1));
			tie(ignore, moreToImport, ignore) = web3.ethereum()->syncQueue(100000);
		}
		double e = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - t).count() / 1000.0;
		cout << imported << " imported in " << e << " seconds at " << (round(imported * 10 / e) / 10) << " blocks/s (#" << web3.ethereum()->number() << ")" << endl;
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
					cout << "The password you entered is incorrect. If you have forgotten your password, and you wish to start afresh, manually remove the file: " + getDataDir("ethereum") + "/keys.info" << endl;
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
		cerr << "Error initializing key manager: " << boost::current_exception_diagnostic_information() << endl;
		return -1;
	}

	for (auto const& presale: presaleImports)
		importPresale(keyManager, presale, [&](){ return getPassword("Enter your wallet password for " + presale + ": "); });

	for (auto const& s: toImport)
	{
		keyManager.import(s, "Imported key (UNSAFE)");
	}

	cout << ethCredits();
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
		return ICAP(_a).encoded() + " (" + toUUID(keyManager.uuid(_a)) + " - " + toHex(_a.ref().cropped(0, 4)) + ")";
	};

	if (author)
		cout << "Mining Beneficiary: " << renderFullAddress(author) << endl;

	if (bootstrap || !remoteHost.empty() || enableDiscovery || listenSet)
	{
		web3.startNetwork();
		cout << "Node ID: " << web3.enode() << endl;
	}
	else
		cout << "Networking disabled. To start, use netstart or pass --bootstrap or a remote host." << endl;

	unique_ptr<ModularServer<>> jsonrpcHttpServer;
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
				}, [&](Address const& _a) { return ICAP(_a).encoded() + " (" + _a.abridged() + ")"; }
			) + "\nEnter yes/no/always (always to this address): ", {"yes", "n", "N", "no", "NO", "always"});
			if (r == "always")
				allowedDestinations.insert(_t.to);
			return r == "yes" || r == "always";
		};

	ExitHandler exitHandler;

	if (jsonRPCURL > -1 || ipc)
	{
		using FullServer = ModularServer<
			rpc::EthFace, rpc::DBFace, rpc::WhisperFace,
			rpc::NetFace, rpc::Web3Face, rpc::PersonalFace,
			rpc::AdminEthFace, rpc::AdminNetFace, rpc::AdminUtilsFace,
			rpc::DebugFace, rpc::TestFace
		>;

		sessionManager.reset(new rpc::SessionManager());
		accountHolder.reset(new SimpleAccountHolder([&](){ return web3.ethereum(); }, getAccountPassword, keyManager, authenticator));
		auto ethFace = new rpc::Eth(*web3.ethereum(), *accountHolder.get());
		rpc::TestFace* testEth = nullptr;
		if (testingMode)
			testEth = new rpc::Test(*web3.ethereum());

		if (jsonRPCURL >= 0)
		{
			rpc::AdminEth* adminEth = nullptr;
			rpc::PersonalFace* personal = nullptr;
			rpc::AdminNet* adminNet = nullptr;
			rpc::AdminUtils* adminUtils = nullptr;
			if (adminViaHttp)
			{
				personal = new rpc::Personal(keyManager, *accountHolder, *web3.ethereum());
				adminEth = new rpc::AdminEth(*web3.ethereum(), *gasPricer.get(), keyManager, *sessionManager.get());
				adminNet = new rpc::AdminNet(web3, *sessionManager.get());
				adminUtils = new rpc::AdminUtils(*sessionManager.get());
			}

			jsonrpcHttpServer.reset(new FullServer(
				ethFace, new rpc::LevelDB(), new rpc::Whisper(web3, {}),
				new rpc::Net(web3), new rpc::Web3(web3.clientVersion()), personal,
				adminEth, adminNet, adminUtils,
				new rpc::Debug(*web3.ethereum()),
				testEth
			));
			auto httpConnector = new SafeHttpServer(jsonRPCURL, "", "", SensibleHttpThreads);
			httpConnector->setAllowedOrigin(rpcCorsDomain);
			jsonrpcHttpServer->addConnector(httpConnector);
			jsonrpcHttpServer->StartListening();
		}
		if (ipc)
		{
			jsonrpcIpcServer.reset(new FullServer(
				ethFace, new rpc::LevelDB(), new rpc::Whisper(web3, {}), new rpc::Net(web3),
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

		cout << "JSONRPC Admin Session Key: " << jsonAdmin << endl;
		writeFile(getDataDir("web3") + "/session.key", jsonAdmin);
		writeFile(getDataDir("web3") + "/session.url", "http://localhost:" + toString(jsonRPCURL));
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

	if (jsonrpcHttpServer.get())
		jsonrpcHttpServer->StopListening();
	if (jsonrpcIpcServer.get())
		jsonrpcIpcServer->StopListening();

	auto netData = web3.saveNetwork();
	if (!netData.empty())
		writeFile(getDataDir() + "/network.rlp", netData);
	return 0;
}
