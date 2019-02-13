// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include <thread>
#include <fstream>
#include <iostream>
#include <regex>
#include <signal.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>

#include <libdevcore/DBFactory.h>
#include <libdevcore/FileSystem.h>
#include <libdevcore/LoggingProgramOptions.h>
#include <libdevcrypto/LibSnark.h>
#include <libethashseal/Ethash.h>
#include <libethashseal/GenesisInfo.h>
#include <libethcore/Common.h>
#include <libethcore/KeyManager.h>
#include <libethereum/SnapshotImporter.h>
#include <libethereum/SnapshotStorage.h>
#include <libevm/VMFactory.h>
#include <libwebthree/WebThree.h>

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
#include "AccountManager.h"

#include <aleth/buildinfo.h>

using namespace std;
using namespace dev;
using namespace dev::p2p;
using namespace dev::eth;
namespace po = boost::program_options;
namespace fs = boost::filesystem;

namespace
{

std::atomic<bool> g_silence = {false};
constexpr const char* c_networkConfigFileName = "network.rlp";

void version()
{
    const auto* buildinfo = aleth_get_buildinfo();
    cout << "aleth " << buildinfo->project_version << "\n";
    cout << "eth network protocol version: " << dev::eth::c_protocolVersion << "\n";
    cout << "Client database version: " << dev::eth::c_databaseVersion << "\n";
    cout << "Build: " << buildinfo->system_name << "/" << buildinfo->build_type << "\n";
}

void importPresale(KeyManager& _km, string const& _file, function<string()> _pass)
{
    KeyPair k = _km.presaleSecret(contentsString(_file), [&](bool){ return _pass(); });
    _km.import(k.secret(), "Presale wallet" + _file + " (insecure)");
}

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
        if (io_mining != ~(unsigned)0 && io_mining && asEthash(*_c->sealEngine()).isMining() &&
            _c->blockChain().details().number - _start == io_mining)
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
}

int main(int argc, char** argv)
{
    setDefaultOrCLocale();

    // Init secp256k1 context by calling one of the functions.
    toPublic({});

    // Init defaults
    Ethash::init();
    NoProof::init();

    /// Operating mode.
    OperationMode mode = OperationMode::Node;

    /// File name for import/export.
    string filename;
    bool safeImport = false;

    /// Hashes/numbers for export range.
    string exportFrom = "1";
    string exportTo = "latest";
    Format exportFormat = Format::Binary;

    bool ipc = true;

    string jsonAdmin;
    ChainParams chainParams;
    string privateChain;

    bool upnp = true;
    WithExisting withExisting = WithExisting::Trust;

    /// Networking params.
    string listenIP;
    unsigned short listenPort = dev::p2p::c_defaultListenPort;
    string publicIP;
    string remoteHost;
    unsigned short remotePort = dev::p2p::c_defaultListenPort;

    unsigned peers = 11;
    unsigned peerStretch = 7;
    std::map<p2p::NodeID, pair<NodeIPEndpoint, bool>> preferredNodes;
    bool bootstrap = true;
    bool disableDiscovery = false;
    bool enableDiscovery = false;
    bool allowLocalDiscovery = false;
    static const unsigned NoNetworkID = (unsigned)-1;
    unsigned networkID = NoNetworkID;

    /// Mining params
    unsigned mining = 0;
    Address author;
    strings presaleImports;
    bytes extraData;

    /// Transaction params
//  TransactionPriority priority = TransactionPriority::Medium;
//  double etherPrice = 30.679;
//  double blockFees = 15.0;
    u256 askPrice = 0;
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


    MinerCLI miner(MinerCLI::OperationMode::None);

    bool listenSet = false;
    bool chainConfigIsSet = false;
    fs::path configPath;
    string configJSON;

    po::options_description clientDefaultMode("CLIENT MODE (default)", c_lineWidth);
    auto addClientOption = clientDefaultMode.add_options();
    addClientOption("mainnet", "Use the main network protocol");
    addClientOption("ropsten", "Use the Ropsten testnet");
    addClientOption("private", po::value<string>()->value_name("<name>"), "Use a private chain");
    addClientOption("test", "Testing mode; disable PoW and provide test rpc interface");
    addClientOption("config", po::value<string>()->value_name("<file>"),
        "Configure specialised blockchain using given JSON information\n");
    addClientOption("ipc", "Enable IPC server (default: on)");
    addClientOption("ipcpath", po::value<string>()->value_name("<path>"),
        "Set .ipc socket path (default: data directory)");
    addClientOption("no-ipc", "Disable IPC server");
    addClientOption("admin", po::value<string>()->value_name("<password>"),
        "Specify admin session key for JSON-RPC (default: auto-generated and printed at "
        "start-up)");
    addClientOption("kill,K", "Kill the blockchain first");
    addClientOption("rebuild,R", "Rebuild the blockchain from the existing database");
    addClientOption("rescue", "Attempt to rescue a corrupt database\n");
    addClientOption("import-presale", po::value<string>()->value_name("<file>"),
        "Import a pre-sale key; you'll need to specify the password to this key");
    addClientOption("import-secret,s", po::value<string>()->value_name("<secret>"),
        "Import a secret key into the key store");
    addClientOption("import-session-secret,S", po::value<string>()->value_name("<secret>"),
        "Import a secret session into the key store");
    addClientOption("master", po::value<string>()->value_name("<password>"),
        "Give the master password for the key store; use --master \"\" to show a prompt");
    addClientOption("password", po::value<string>()->value_name("<password>"),
        "Give a password for a private key\n");

    po::options_description clientTransacting("CLIENT TRANSACTING", c_lineWidth);
    auto addTransactingOption = clientTransacting.add_options();
    addTransactingOption("ask", po::value<u256>()->value_name("<wei>"),
        ("Set the minimum ask gas price under which no transaction will be mined (default: " +
            toString(DefaultGasPrice) + ")")
            .c_str());
    addTransactingOption("bid", po::value<u256>()->value_name("<wei>"),
        ("Set the bid gas price to pay for transactions (default: " + toString(DefaultGasPrice) +
            ")")
            .c_str());
    addTransactingOption("unsafe-transactions",
        "Allow all transactions to proceed without verification; EXTREMELY UNSAFE\n");

    po::options_description clientMining("CLIENT MINING", c_lineWidth);
    auto addMininigOption = clientMining.add_options();
    addMininigOption("address,a", po::value<Address>()->value_name("<addr>"),
        "Set the author (mining payout) address (default: auto)");
    addMininigOption("mining,m", po::value<string>()->value_name("<on/off/number>"),
        "Enable mining; optionally for a specified number of blocks (default: off)");
    addMininigOption("extra-data", po::value<string>(), "Set extra data for the sealed blocks\n");

    po::options_description clientNetworking("CLIENT NETWORKING", c_lineWidth);
    auto addNetworkingOption = clientNetworking.add_options();
    addNetworkingOption("bootstrap,b",
        "Connect to the default Ethereum peer servers (default unless --no-discovery used)");
    addNetworkingOption("no-bootstrap",
        "Do not connect to the default Ethereum peer servers (default only when --no-discovery is "
        "used)");
    addNetworkingOption("peers,x", po::value<int>()->value_name("<number>"),
        "Attempt to connect to a given number of peers (default: 11)");
    addNetworkingOption("peer-stretch", po::value<int>()->value_name("<number>"),
        "Give the accepted connection multiplier (default: 7)");
    addNetworkingOption("public-ip", po::value<string>()->value_name("<ip>"),
        "Force advertised public IP to the given IP (default: auto)");
    addNetworkingOption("listen-ip", po::value<string>()->value_name("<ip>(:<port>)"),
        "Listen on the given IP for incoming connections (default: 0.0.0.0)");
    addNetworkingOption("listen", po::value<unsigned short>()->value_name("<port>"),
        "Listen on the given port for incoming connections (default: 30303)");
    addNetworkingOption("remote,r", po::value<string>()->value_name("<host>(:<port>)"),
        "Connect to the given remote host (default: none)");
    addNetworkingOption("port", po::value<short>()->value_name("<port>"),
        "Connect to the given remote port (default: 30303)");
    addNetworkingOption("network-id", po::value<unsigned>()->value_name("<n>"),
        "Only connect to other hosts with this network id");
    addNetworkingOption("allow-local-discovery", po::bool_switch(&allowLocalDiscovery),
        "Include local addresses in the discovery process. Used for testing purposes.");
#if ETH_MINIUPNPC
    addNetworkingOption(
        "upnp", po::value<string>()->value_name("<on/off>"), "Use UPnP for NAT (default: on)");
#endif

    stringstream peersetDescriptionStream;
    peersetDescriptionStream << "Comma delimited list of peers; element format: type:enode://publickey@ipAddress[:port[?discport=port]]\n"
        "        Types:\n"
        "        default     Attempt connection when no other peers are available and pinning is disabled\n"
        "        required    Keep connected at all times\n\n"
        "        Ports:\n"
        "        The first port argument is the tcp port used for direct communication among peers. If the second port\n"
        "        argument isn't supplied, the first port argument will also be the udp port used for node discovery.\n"
        "        If neither the first nor second port arguments are supplied, a default port of " << dev::p2p::c_defaultListenPort << " will be used for\n"
        "        both peer communication and node discovery.";
    string peersetDescription = peersetDescriptionStream.str();
    addNetworkingOption("peerset", po::value<string>()->value_name("<list>"), peersetDescription.c_str());
    addNetworkingOption("no-discovery", "Disable node discovery; implies --no-bootstrap");
    addNetworkingOption("pin", "Only accept or connect to trusted peers\n");

    std::string snapshotPath;
    po::options_description importExportMode("IMPORT/EXPORT MODES", c_lineWidth);
    auto addImportExportOption = importExportMode.add_options();
    addImportExportOption(
        "import,I", po::value<string>()->value_name("<file>"), "Import blocks from file");
    addImportExportOption(
        "export,E", po::value<string>()->value_name("<file>"), "Export blocks to file");
    addImportExportOption("from", po::value<string>()->value_name("<n>"),
        "Export only from block n; n may be a decimal, a '0x' prefixed hash, or 'latest'");
    addImportExportOption("to", po::value<string>()->value_name("<n>"),
        "Export only to block n (inclusive); n may be a decimal, a '0x' prefixed hash, or "
        "'latest'");
    addImportExportOption("only", po::value<string>()->value_name("<n>"),
        "Equivalent to --export-from n --export-to n");
    addImportExportOption(
        "format", po::value<string>()->value_name("<binary/hex/human>"), "Set export format");
    addImportExportOption("dont-check",
        "Prevent checking some block aspects. Faster importing, but to apply only when the data is "
        "known to be valid");
    addImportExportOption("download-snapshot",
        po::value<string>(&snapshotPath)->value_name("<path>"),
        "Download Parity Warp Sync snapshot data to the specified path");
    addImportExportOption("import-snapshot", po::value<string>()->value_name("<path>"),
        "Import blockchain and state data from the Parity Warp Sync snapshot\n");

    LoggingOptions loggingOptions;
    po::options_description loggingProgramOptions(
        createLoggingProgramOptions(c_lineWidth, loggingOptions));

    po::options_description generalOptions("GENERAL OPTIONS", c_lineWidth);
    auto addGeneralOption = generalOptions.add_options();
    addGeneralOption("data-dir,d", po::value<string>()->value_name("<path>"),
        ("Load configuration files and keystore from path (default: " + getDataDir().string() + ")").c_str());
    addGeneralOption("version,V", "Show the version and exit");
    addGeneralOption("help,h", "Show this help message and exit\n");

    po::options_description vmOptions = vmProgramOptions(c_lineWidth);
    po::options_description dbOptions = db::databaseProgramOptions(c_lineWidth);
    po::options_description minerOptions = MinerCLI::createProgramOptions(c_lineWidth);

    po::options_description allowedOptions("Allowed options");
    allowedOptions.add(clientDefaultMode)
        .add(clientTransacting)
        .add(clientMining)
        .add(minerOptions)
        .add(clientNetworking)
        .add(importExportMode)
        .add(vmOptions)
        .add(dbOptions)
        .add(loggingProgramOptions)
        .add(generalOptions);

    po::variables_map vm;

    try
    {
        po::parsed_options parsed = po::parse_command_line(argc, argv, allowedOptions);
        po::store(parsed, vm);
        po::notify(vm);
    }
    catch (po::error const& e)
    {
        cerr << e.what() << "\n";
        return AlethErrors::ArgumentProcessingFailure;
    }

    miner.interpretOptions(vm);

    if (vm.count("import-snapshot"))
    {
        mode = OperationMode::ImportSnapshot;
        filename = vm["import-snapshot"].as<string>();
    }
    if (vm.count("version"))
    {
        version();
        return AlethErrors::Success;
    }
    if (vm.count("test"))
    {
        testingMode = true;
        enableDiscovery = false;
        disableDiscovery = true;
        bootstrap = false;
    }
    if (vm.count("peers"))
        peers = vm["peers"].as<int>();
    if (vm.count("peer-stretch"))
        peerStretch = vm["peer-stretch"].as<int>();

    if (vm.count("peerset"))
    {
        string peersetStr = vm["peerset"].as<string>();
        vector<string> peersetList;
        boost::split(peersetList, peersetStr, boost::is_any_of(","));
        bool parsingError = false;
        const string peerPattern = "^(default|required):(.*)";
        regex rx(peerPattern);
        smatch match;
        for (auto const& p : peersetList)
        {
            if (regex_match(p, match, rx))
            {
                bool required = match.str(1) == "required";
                NodeSpec ns(match.str(2));
                if (ns.isValid())
                    preferredNodes[ns.id()] = make_pair(ns.nodeIPEndpoint(), required);
                else
                {
                    parsingError = true;
                    break;
                }
            }
            else
            {
                parsingError = true;
                break;
            }
        }
        if (parsingError)
        {
             cerr << "Unrecognized peerset: " << peersetStr << "\n";
             return AlethErrors::UnrecognizedPeerset;
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
                return AlethErrors::UnknownMiningOption;
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
    if (vm.count("unsafe-transactions"))
        alwaysConfirm = false;
    if (vm.count("data-dir"))
        setDataDir(vm["data-dir"].as<string>());
    if (vm.count("ipcpath"))
        setIpcPath(vm["ipcpath"].as<string>());
    if (vm.count("config"))
    {
        try
        {
            configPath = vm["config"].as<string>();
            configJSON = contentsString(configPath.string());

            if (configJSON.empty())
            {
                cerr << "Config file not found or empty (" << configPath.string() << ")\n";
                return AlethErrors::ConfigFileEmptyOrNotFound;
            }
        }
        catch (...)
        {
            cerr << "Bad --config option: " << vm["config"].as<string>() << "\n";
            return AlethErrors::BadConfigOption;
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
            return AlethErrors::BadExtraDataOption;
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
            return AlethErrors::BadAskOption;
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
            return AlethErrors::BadBidOption;
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
            return AlethErrors::BadFormatOption;
        }
    }
    if (vm.count("to"))
        exportTo = vm["to"].as<string>();
    if (vm.count("from"))
        exportFrom = vm["from"].as<string>();
    if (vm.count("only"))
        exportTo = exportFrom = vm["only"].as<string>();
#if ETH_MINIUPNPC
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
            return AlethErrors::BadUpnpOption;
        }
    }
#endif
    if (vm.count("network-id"))
        try
        {
            networkID = vm["network-id"].as<unsigned>();
        }
        catch (...)
        {
            cerr << "Bad " << "--network-id" << " option: " << vm["network-id"].as<string>() << "\n";
            return AlethErrors::BadNetworkIdOption;
        }
    if (vm.count("private"))
        try
        {
            privateChain = vm["private"].as<string>();
        }
        catch (...)
        {
            cerr << "Bad " << "--private" << " option: " << vm["private"].as<string>() << "\n";
            return AlethErrors::BadPrivateOption;
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
            return AlethErrors::BadHexValueInAddressOption;
        }
        catch (...)
        {
            cerr << "Bad " << "--address" << " option: " << vm["address"].as<string>() << "\n";
            return AlethErrors::BadAddressOption;
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
        cout << "NAME:\n"
             << "   aleth " << Version << '\n'
             << "USAGE:\n"
             << "   aleth [options]\n\n"
             << "WALLET USAGE:\n";
        AccountManager::streamAccountHelp(cout);
        AccountManager::streamWalletHelp(cout);
        cout << clientDefaultMode << clientTransacting << clientNetworking << clientMining << minerOptions;
        cout << importExportMode << dbOptions << vmOptions << loggingProgramOptions << generalOptions;
        return AlethErrors::Success;
    }

    if (!configJSON.empty())
    {
        try
        {
            chainParams = chainParams.loadConfig(configJSON, {}, configPath);
            chainConfigIsSet = true;
        }
        catch (...)
        {
            cerr << "provided configuration is not well formatted\n";
            cerr << "sample: \n" << genesisInfo(eth::Network::MainNetworkTest) << "\n";
            return AlethErrors::Success;
        }
    }

    setupLogging(loggingOptions);

    if (!privateChain.empty())
    {
        chainParams.extraData = sha3(privateChain).asBytes();
        chainParams.difficulty = chainParams.minimumDifficulty;
        chainParams.gasLimit = u256(1) << 32;
    }

    if (!chainConfigIsSet)
        // default to mainnet if not already set with any of `--mainnet`, `--ropsten`, `--genesis`, `--config`
        chainParams = ChainParams(genesisInfo(eth::Network::MainNetwork), genesisStateRoot(eth::Network::MainNetwork));

    if (loggingOptions.verbosity > 0)
        cout << EthGrayBold "aleth, a C++ Ethereum client" EthReset << "\n";

    miner.execute();

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

    auto netPrefs = publicIP.empty() ? NetworkConfig(listenIP, listenPort, upnp) : NetworkConfig(publicIP, listenIP ,listenPort, upnp);
    netPrefs.discovery = (privateChain.empty() && !disableDiscovery) || enableDiscovery;
    netPrefs.allowLocalDiscovery = allowLocalDiscovery;
    netPrefs.pin = vm.count("pin") != 0;

    auto nodesState = contents(getDataDir() / fs::path(c_networkConfigFileName));

    if (testingMode)
        chainParams.allowFutureBlocks = true;

    dev::WebThreeDirect web3(WebThreeDirect::composeClientVersion("aleth"), db::databasePath(),
        snapshotPath, chainParams, withExisting, netPrefs, &nodesState, testingMode);

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
            return AlethErrors::BadBlockNumberHashOption;
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
        return AlethErrors::Success;
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
        return AlethErrors::Success;
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
        return AlethErrors::KeyManagerInitializationFailure;
    }

    for (auto const& presale: presaleImports)
        importPresale(keyManager, presale, [&](){ return getPassword("Enter your wallet password for " + presale + ": "); });

    for (auto const& s: toImport)
    {
        keyManager.import(s, "Imported key (UNSAFE)");
    }

    cout << "aleth " << Version << "\n";

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
            return AlethErrors::SnapshotImportFailure;
        }
    }


    web3.setIdealPeerCount(peers);
    web3.setPeerStretch(peerStretch);
    std::shared_ptr<eth::TrivialGasPricer> gasPricer =
        make_shared<eth::TrivialGasPricer>(askPrice, bidPrice);
    Client& c = *(web3.ethereum());
    c.setGasPricer(gasPricer);
    c.setSealer(miner.minerType());
    c.setAuthor(author);
    if (networkID != NoNetworkID)
        c.setNetworkId(networkID);

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

    unique_ptr<rpc::SessionManager> sessionManager;
    unique_ptr<SimpleAccountHolder> accountHolder;
    unique_ptr<ModularServer<>> jsonrpcIpcServer;


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

    if (web3.isNetworkStarted())
    {
        for (auto const& p: preferredNodes)
            if (p.second.second)
                web3.requirePeer(p.first, p.second.first);
            else
                web3.addNode(p.first, p.second.first);

        if (bootstrap && privateChain.empty())
            for (auto const& i : Host::pocHosts())
                web3.requirePeer(i.first, i.second);
        if (!remoteHost.empty())
            web3.addNode(p2p::NodeID(), remoteHost + ":" + toString(remotePort));
    }

    signal(SIGABRT, &ExitHandler::exitHandler);
    signal(SIGTERM, &ExitHandler::exitHandler);
    signal(SIGINT, &ExitHandler::exitHandler);

    unsigned n = c.blockChain().details().number;
    if (mining)
        c.startSealing();

    while (!exitHandler.shouldExit())
        stopSealingAfterXBlocks(&c, n, mining);

    if (jsonrpcIpcServer.get())
        jsonrpcIpcServer->StopListening();

    if (web3.isNetworkStarted())
    {
        web3.stopNetwork();
        auto netData = web3.saveNetwork();
        if (!netData.empty())
            writeFile(getDataDir() / fs::path(c_networkConfigFileName), netData);
    }
    return AlethErrors::Success;
}
