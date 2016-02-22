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
/** @file MainCLI.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#include "MainCLI.h"
#include <libethereum/GenesisInfo.h>
#include <libwebthree/WebThree.h>
#if ETH_JSCONSOLE || !ETH_TRUE
#include <libjsconsole/JSLocalConsole.h>
#include <libjsconsole/JSRemoteConsole.h>
#endif
#if ETH_READLINE || !ETH_TRUE
#include <readline/readline.h>
#include <readline/history.h>
#endif
#include <json_spirit/JsonSpiritHeaders.h>
#if ETH_JSONRPC || !ETH_TRUE
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
#include <libweb3jsonrpc/SessionManager.h>
#include <libweb3jsonrpc/AdminNet.h>
#include <libweb3jsonrpc/AdminEth.h>
#include <libweb3jsonrpc/AdminUtils.h>
#include <libweb3jsonrpc/Personal.h>
#endif
#include <libjsconsole/JSLocalConsole.h>
using namespace std;
using namespace dev;
using namespace p2p;
using namespace eth;
namespace js = json_spirit;

MainCLI* MainCLI::s_this = nullptr;

MainCLI::MainCLI(Mode _mode):
	m_mode(_mode),
	m_keyManager(getDataDir("fluidity") + "/keys.info", getDataDir("fluidity") + "/keys"),
	m_paramsJson(c_genesisInfoFluidity)
{
	s_this = this;
}

bool MainCLI::interpretOption(int& i, int argc, char** argv)
{
	string arg = argv[i];
	if (arg == "console")
		m_mode = Mode::Console;
	else if (arg == "benchmark")
		m_mode = Mode::Benchmark;
	else if (arg == "--path" && i + 1 < argc)
	{
		m_dbPath = argv[++i];
		m_keyManager.setKeysFile(m_dbPath + "/keys.info");
		m_keyManager.setSecretsPath(m_dbPath + "/keys");
	}
	else if (arg == "--db-path" && i + 1 < argc)
		m_dbPath = argv[++i];
	else if (arg == "--keys-file" && i + 1 < argc)
		m_keyManager.setKeysFile(argv[++i]);
	else if (arg == "--keys-path" && i + 1 < argc)
		m_keyManager.setSecretsPath(argv[++i]);
	else if (arg == "--config" && i + 1 < argc)
		m_paramsJson = asString(contents(argv[++i]));
	else if (arg == "--client-name" && i + 1 < argc)
		m_clientName = argv[++i];
	else if (arg == "--public-ip" && i + 1 < argc)
		m_publicIP = argv[++i];
	else if (arg == "--listen-ip" && i + 1 < argc)
		m_listenIP = argv[++i];
	else if (arg == "--listen-port" && i + 1 < argc)
		m_listenPort = (short)atoi(argv[++i]);
	else if (arg == "--chain-name" && i + 1 < argc)
		m_chainName = argv[++i];
	else if (arg == "--master" && i + 1 < argc)
		m_masterPassword = argv[++i];
	else if (arg == "--authority" && i + 1 < argc)
		m_authorities.push_back(Address(argv[++i]));
	else if (arg == "--start-sealing")
		m_startSealing = true;
#if ETH_JSONRPC || !ETH_TRUE
	else if (arg == "--ipc")
		m_ipc = true;
	else if (arg == "--jsonrpc" || arg == "--allow-attach")
		m_jsonRPCPort = 8545;
	else if (arg == "--jsonrpc-port" && i + 1 < argc)
		m_jsonRPCPort = atoi(argv[++i]);
	else if (arg == "--jsonrpc-cors-domain" && i + 1 < argc)
		m_rpcCorsDomain = argv[++i];
#endif
	else if (arg == "--url" && i + 1 < argc)
		m_remoteURL = argv[++i];
	else if (arg == "--session-key" && i + 1 < argc)
		m_remoteSessionKey = argv[++i];
	else if (arg == "--node" && i + 1 < argc)
	{
		p2p::NodeSpec ns(argv[++i]);
		if (!ns.nodeIPEndpoint())
			return false;
		m_preferredNodes[ns.id()] = make_pair(ns.nodeIPEndpoint(), true);
	}
	else if (m_mode == Mode::Execute)
		m_toExecute.push_back(arg);
	return true;
}

void MainCLI::execute()
{
	switch (m_mode)
	{
	case Mode::Attach:
	case Mode::Execute:
	{
#if ETH_JSCONSOLE || !ETH_TRUE
		JSRemoteConsole console(m_remoteURL);
		if (m_mode == Mode::Attach)
		{
			string givenURL = contentsString(m_dbPath + "/session.url");
			if (!givenURL.empty())
				m_remoteURL = givenURL;
			if (m_remoteSessionKey.empty())
				m_remoteSessionKey = contentsString(m_dbPath + "/session.key");
			if (!m_remoteSessionKey.empty())
				console.eval("web3.admin.setSessionKey('" + m_remoteSessionKey + "')");
			while (true)
				console.readAndEval();
		}
		else if (m_mode == Mode::Execute)
		{
			if (m_toExecute.empty())
			{
				string e;
				while (cin)
				{
					string s;
					getline(cin, s);
					e += s;
				}
				console.eval(e);
			}
			else
				for (auto const& i: m_toExecute)
				{
					string c = contentsString(i);
					console.eval(c.empty() ? i : c);
				}
		}
#endif
		break;
	}
	case Mode::Console:
	case Mode::Benchmark:
	case Mode::Dumb:
	{
		setup();
		setupKeyManager();

		auto nodesState = contents(m_dbPath + "/network.rlp");
		dev::WebThreeDirect web3(
			WebThreeDirect::composeClientVersion("flu", m_clientName),
			m_dbPath,
			m_chainParams,
			WithExisting::Trust,
			set<string>{"eth"},
			m_netPrefs,
			&nodesState,
			TransactionQueue::Limits{10240, 10240});

		signal(SIGABRT, &MainCLI::staticExitHandler);
		signal(SIGTERM, &MainCLI::staticExitHandler);
		signal(SIGINT, &MainCLI::staticExitHandler);

		// TODO: remove all gas pricing components.
		auto gasPricer = make_shared<eth::TrivialGasPricer>(0, 0);
		web3.ethereum()->setGasPricer(gasPricer);

		// Set up fluidity authorities & signing key.
		for (auto const& i: m_sealEngineParams)
			web3.ethereum()->setSealOption(i.first, i.second);
		if (m_sealEngineParams.count("authorities"))
			m_authorities = RLP(m_sealEngineParams["authorities"]).convert<Addresses>(RLP::LaissezFaire);
		else
		{
			if (m_authorities.empty())
				m_authorities = m_keyManager.accounts();
			web3.ethereum()->setSealOption("authorities", rlp(m_authorities));
		}
		if (!m_sealEngineParams.count("authority"))
			for (auto i: m_authorities)
				if (Secret s = m_keyManager.secret(i))
				{
					web3.ethereum()->setSealOption("authority", rlp(s.makeInsecure()));
					web3.ethereum()->setAuthor(i);
					break;
				}

		web3.startNetwork();
		if (contents(m_dbPath + "/network.rlp").empty())
			writeFile(m_dbPath + "/network.rlp", web3.saveNetwork());
		cnote << "Node ID: " << web3.enode();

		for (auto const& p: m_preferredNodes)
			if (p.second.second)
			{
				cdebug << "Requiring peer:" << p.first << p.second.first;
				web3.requirePeer(p.first, p.second.first);
			}

		if (m_startSealing)
			web3.ethereum()->startSealing();

		startRPC(web3, *gasPricer);

		if (m_mode == Mode::Console)
		{
			SimpleAccountHolder accountHolder([&](){ return web3.ethereum(); }, [](Address){ return string(); }, m_keyManager);
			rpc::SessionManager sessionManager;
			string sessionKey = sessionManager.newSession(rpc::SessionPermissions{{rpc::Privilege::Admin}});

			auto ethFace = new rpc::Eth(*web3.ethereum(), accountHolder);
			auto adminEthFace = new rpc::AdminEth(*web3.ethereum(), *gasPricer.get(), m_keyManager, sessionManager);
			auto adminNetFace = new rpc::AdminNet(web3, sessionManager);
			auto adminUtilsFace = new rpc::AdminUtils(sessionManager, this);

			ModularServer<
				rpc::EthFace, rpc::DBFace, rpc::WhisperFace,
				rpc::NetFace, rpc::Web3Face, rpc::PersonalFace,
				rpc::AdminEthFace, rpc::AdminNetFace, rpc::AdminUtilsFace
			> rpcServer(
				ethFace, new rpc::LevelDB(), new rpc::Whisper(web3, {}),
				new rpc::Net(web3), new rpc::Web3(web3.clientVersion()), new rpc::Personal(m_keyManager, accountHolder),
				adminEthFace, adminNetFace, adminUtilsFace
			);

			JSLocalConsole console;
			rpcServer.addConnector(console.createConnector());
			rpcServer.StartListening();

			console.eval("web3.admin.setSessionKey('" + sessionKey + "')");
			while (!m_shouldExit)
				console.readAndEval();
			rpcServer.StopListening();
		}
		else if (m_mode == Mode::Benchmark)
		{
			unsigned const c_pitch = 500;
			unsigned const c_delayMS = 100;
			bool const delayAnyway = false;
			Timer t;
			unsigned lastSecond = 0;
			unsigned count = 0;
			auto limits = web3.ethereum()->transactionQueueLimits();
			TransactionSkeleton ts;
			ts.to = ts.from = FluidityTreasure.address();
			ts.gas = 1000000;
			ts.gasPrice = 0;
			ts.nonce = web3.ethereum()->countAt(ts.from);
			ts.value = u256(1) << 30;
			while (!m_shouldExit)
			{
				auto status = web3.ethereum()->transactionQueueStatus();
				if (status.current + c_pitch > limits.current || status.future + c_pitch > limits.future || delayAnyway)
				{
					// buffer overrun - sleep for a bit.
//					cdebug << "Buffer overrun. current:" << status.current << " future:" << status.future << " unverified:" << status.unverified;
					this_thread::sleep_for(chrono::milliseconds(c_delayMS));
				}
				else
				{
					Timer t;
					vector<bytes> txs;
					for (unsigned i = 0; i < c_pitch; ++i)
					{
						Transaction tx(ts, FluidityTreasure.secret());
						txs.push_back(tx.rlp());
						ts.nonce++;
					}
//					cdebug << "Encoded" << c_pitch << "transactions in" << (t.elapsed() * 1000) << "ms";
//					t.restart();
					for (bytes const& tx: txs)
						web3.ethereum()->injectTransaction(tx);
					cdebug << "Submitted" << c_pitch << "transactions in" << (t.elapsed() * 1000) << "ms";
					count += c_pitch;
				}

				if (lastSecond < (unsigned)t.elapsed())
				{
					cdebug << "Transactions submitted:" << count << "; tx/sec:" << (count / t.elapsed());
					lastSecond++;
				}
			}

		}
		else if (m_mode == Mode::Dumb)
			while (!m_shouldExit)
				this_thread::sleep_for(chrono::milliseconds(300));
		auto netData = web3.saveNetwork();
		if (!netData.empty())
			writeFile(m_dbPath + "/network.rlp", netData);
		break;
	}
	}
}

namespace dev
{
struct RPCPrivate
{
#if ETH_JSONRPC || !ETH_TRUE
	unique_ptr<ModularServer<
		rpc::EthFace, rpc::DBFace, rpc::WhisperFace,
		rpc::NetFace, rpc::Web3Face, rpc::PersonalFace,
		rpc::AdminEthFace, rpc::AdminNetFace, rpc::AdminUtilsFace
	>> jsonrpcServer;
	unique_ptr<rpc::SessionManager> sessionManager;
	unique_ptr<SimpleAccountHolder> accountHolder;
#endif
};
}

void MainCLI::startRPC(WebThreeDirect& _web3, TrivialGasPricer& _gasPricer)
{
#if ETH_JSONRPC || !ETH_TRUE
	m_rpcPrivate = make_shared<RPCPrivate>();

	if (m_jsonRPCPort > -1 || m_ipc)
	{
		m_rpcPrivate->sessionManager.reset(new rpc::SessionManager());
		m_rpcPrivate->accountHolder.reset(new SimpleAccountHolder([&](){ return _web3.ethereum(); }, [](Address){ return string(); }, m_keyManager));
		auto ethFace = new rpc::Eth(*_web3.ethereum(), *m_rpcPrivate->accountHolder);
		auto adminEthFace = new rpc::AdminEth(*_web3.ethereum(), _gasPricer, m_keyManager, *m_rpcPrivate->sessionManager);
		m_rpcPrivate->jsonrpcServer.reset(new ModularServer<
			rpc::EthFace, rpc::DBFace, rpc::WhisperFace,
			rpc::NetFace, rpc::Web3Face, rpc::PersonalFace,
			rpc::AdminEthFace, rpc::AdminNetFace, rpc::AdminUtilsFace
		>(
			ethFace, new rpc::LevelDB(), new rpc::Whisper(_web3, {}),
			new rpc::Net(_web3), new rpc::Web3(_web3.clientVersion()), new rpc::Personal(m_keyManager, *m_rpcPrivate->accountHolder),
			adminEthFace, new rpc::AdminNet(_web3, *m_rpcPrivate->sessionManager), new rpc::AdminUtils(*m_rpcPrivate->sessionManager)
		));
		if (m_jsonRPCPort > -1)
		{
			auto httpConnector = new SafeHttpServer(m_jsonRPCPort, "", "", SensibleHttpThreads);
			httpConnector->setAllowedOrigin(m_rpcCorsDomain);
			m_rpcPrivate->jsonrpcServer->addConnector(httpConnector);
			httpConnector->StartListening();
		}
		if (m_ipc)
		{
			auto ipcConnector = new IpcServer("flu");
			m_rpcPrivate->jsonrpcServer->addConnector(ipcConnector);
			ipcConnector->StartListening();
		}

		string jsonAdmin = m_rpcPrivate->sessionManager->newSession(rpc::SessionPermissions{{rpc::Privilege::Admin}});

		cnote << "JSONRPC Admin Session Key: " << jsonAdmin;
		writeFile(m_dbPath + "/session.key", jsonAdmin);
		writeFile(m_dbPath + "/session.url", "http://localhost:" + toString(m_jsonRPCPort));
	}
#endif
}

void MainCLI::setupKeyManager()
{
	if (m_keyManager.exists())
	{
		if (!m_keyManager.load(m_masterPassword))
			::exit(0);
	}
	else
		m_keyManager.create(m_masterPassword);

	if (m_keyManager.accounts().empty())
	{
		m_keyManager.import(Secret::random(), "Default");
		cnote << "Created key: 0x" << m_keyManager.accounts().front().hex();
	}
}

static bytes toRLP(js::mValue const& _v)
{
	if (_v.type() == js::str_type)
	{
		string s = _v.get_str();
		if (s.substr(0, 2) == "0x")
			return rlp(fromHex(s));
		else
			return rlp(asBytes(s));
	}
	else if (_v.type() == js::int_type)
		return rlp(_v.get_int64());
	else if (_v.type() == js::bool_type)
		return rlp(_v.get_bool());
	else if (_v.type() == js::array_type)
	{
		js::mArray a = _v.get_array();
		RLPStream s(a.size());
		for (js::mValue const& i: a)
			s.appendRaw(toRLP(i));
		return s.out();
	}
	else if (_v.type() == js::obj_type)
	{
		js::mObject a = _v.get_obj();
		RLPStream s(a.size());
		for (pair<string, js::mValue> const& i: a)
			s.appendList(2).append(i.first).appendRaw(toRLP(i.second));
		return s.out();
	}
	else
		return rlp("");
}

void MainCLI::setup()
{
	m_netPrefs = m_publicIP.empty() ? NetworkPreferences(m_listenIP, m_listenPort, false) : NetworkPreferences(m_publicIP, m_listenIP, m_listenPort, false);
	m_netPrefs.discovery = false;
	m_netPrefs.pin = true;
	m_chainParams = ChainParams(m_paramsJson);
	if (!m_chainName.empty())
		m_chainParams.extraData = sha3(m_chainName).asBytes();

	js::mValue val;
	js::read_string(m_paramsJson, val);
	js::mObject o = val.get_obj();
	if (o.count("options"))
	{
		js::mObject params = o["options"].get_obj();
		for (pair<string, js::mValue> const& p: params)
			m_sealEngineParams[p.first] = toRLP(p.second);
	}
	if (o.count("network"))
	{
		js::mObject params = o["network"].get_obj();
		if (params.count("nodes"))
		{
			js::mArray nodes = params["nodes"].get_array();
			for (js::mValue const& i: nodes)
			{
				p2p::NodeSpec ns(i.get_str());
				if (!ns.nodeIPEndpoint())
					continue;
				m_preferredNodes[ns.id()] = make_pair(ns.nodeIPEndpoint(), true);
			}
		}
	}
}

void MainCLI::streamHelp(ostream& _out)
{
	_out
		<< "Main operation modes:" << endl
		<< "    benchmark  Don't sleep - just launch as many transactions as possible." << endl
		<< "    console  Launch interactive console." << endl
		<< "    attach  Attach to existing flu process with interactive console." << endl
		<< "    execute  Execute a JS script." << endl
		;
	// TODO: all the other options.
}
