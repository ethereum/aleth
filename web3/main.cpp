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
 * @author Gav Wood <g@ethcore.io>
 * @date 2015
 * Fluidity client.
 */
#include <fstream>
#include <iostream>
#include <ctime>
#include <boost/algorithm/string.hpp>
#include <libp2p/Network.h>
#include <libethcore/KeyManager.h>
#include <libethereum/Defaults.h>
#include <libethereum/Block.h>
#include <libethereum/Executive.h>
#include <libethereum/Client.h>
#include <libfluidity/FluidityClient.h>
#include <libjsconsole/JSLocalConsole.h>
#include <libjsconsole/JSRemoteConsole.h>
#include <libwebthree/WebThree.h>
#include <libweb3jsonrpc/AccountHolder.h>
#include <libweb3jsonrpc/WebThreeStubServer.h>
#include <libevm/VM.h>
#include <libevm/VMFactory.h>
using namespace std;
using namespace dev;
using namespace p2p;
using namespace eth;

class CLI
{
public:
	static bool isTrue(std::string const& _m)
	{
		return _m == "on" || _m == "yes" || _m == "true" || _m == "1";
	}

	static bool isFalse(std::string const& _m)
	{
		return _m == "off" || _m == "no" || _m == "false" || _m == "0";
	}
};

class MainCLI: public CLI
{
public:
	enum class Mode
	{
		Console
	};

	MainCLI(Mode _mode = Mode::Console):
		m_mode(_mode)
	{
	}

	// fluidity --config c peer1 peer2
	// c: { authorities: [ 0xaddress1..., 0xaddress2, ... ] }
	// creates the fluidity PoA contract initialised with those authorities

	bool interpretOption(int& i, int argc, char** argv)
	{
		string arg = argv[i];
		if (arg == "--client-name" && i + 1 < argc)
			m_clientName = argv[++i];
		else if (arg == "--public-ip" && i + 1 < argc)
			m_publicIP = argv[++i];
		else if (arg == "--listen-ip" && i + 1 < argc)
			m_listenIP = argv[++i];
		else if (arg == "--listen-port" && i + 1 < argc)
			m_listenPort = (short)atoi(argv[++i]);
		else if (arg == "--chain-name" && i + 1 < argc)
			m_privateChain = argv[++i];
		else if (arg == "--master" && i + 1 < argc)
			m_masterPassword = argv[++i];
		else if (arg == "--authority" && i + 1 < argc)
			m_authorities.push_back(Address(argv[++i]));
		else
		{
			p2p::NodeSpec ns(arg);
			if (!ns.nodeIPEndpoint())
				return false;
			m_preferredNodes[ns.id()] = make_pair(ns.nodeIPEndpoint(), true);
		}
		return true;
	}

	void execute()
	{
		setup();
		switch (m_mode)
		{
		case Mode::Console:
		{
			dev::WebThreeDirect web3(
				WebThreeDirect::composeClientVersion("web3", m_clientName),
				m_dbPath,
				WithExisting::Trust,
				set<string>{"flu"},
				m_netPrefs);

			// TODO: remove all gas pricing components.
			auto gasPricer = make_shared<eth::TrivialGasPricer>(0, 0);
			web3.ethereum()->setGasPricer(gasPricer);

			// Set up fluidity authorities & signing key.
			web3.ethereum()->setSealOption("authorities", rlp(m_authorities));
			for (auto i: m_authorities)
				if (Secret s = m_keyManager.secret(i))
					web3.ethereum()->setSealOption("authority", rlp(s.makeInsecure()));

			JSLocalConsole console;
			shared_ptr<dev::WebThreeStubServer> rpcServer = make_shared<dev::WebThreeStubServer>(*console.connector(), web3, make_shared<FixedAccountHolder>([&](){ return web3.ethereum(); }, vector<KeyPair>()), vector<KeyPair>(), m_keyManager, *gasPricer);
			string sessionKey = rpcServer->newSession(SessionPermissions{{Privilege::Admin}});
			console.eval("web3.admin.setSessionKey('" + sessionKey + "')");
			while (!Client::shouldExit())
				console.readAndEval();
			rpcServer->StopListening();
			break;
		}
		}
	}

	void setupKeyManager()
	{
		if (m_keyManager.exists())
		{
			if (!m_keyManager.load(m_masterPassword))
				exit(0);
		}
		else
			m_keyManager.create(m_masterPassword);

		if (m_keyManager.accounts().empty())
			m_keyManager.import(Secret::random(), "Default");
	}

	void setup()
	{
		if (!m_privateChain.empty())
		{
			CanonBlockChain<Fluidity>::forceGenesisExtraData(sha3(m_privateChain).asBytes());
			c_network = resetNetwork(eth::Network::Fluidity);
		}
		m_netPrefs = m_publicIP.empty() ? NetworkPreferences(m_listenIP, m_listenPort, false) : NetworkPreferences(m_publicIP, m_listenIP, m_listenPort, false);
		m_netPrefs.discovery = false;
		m_netPrefs.pin = true;
	}

	static void streamHelp(ostream& _out)
	{
		_out
			<< "Main operation modes:" << endl
			<< "    daemon  Daemonify." << endl
			<< "    console  Launch interactive console." << endl
			;
	}

private:
	Mode m_mode;

	string m_remoteURL = "http://localhost:8545";
	string m_remoteSessionKey;

	string m_dbPath = getDataDir("fluidity");
	string m_masterPassword;

	Addresses m_authorities;
	string m_privateChain;

	/// Networking params.
	string m_clientName;
	string m_listenIP;
	unsigned short m_listenPort = 40404;
	string m_publicIP;

	std::map<p2p::NodeID, pair<p2p::NodeIPEndpoint, bool>> m_preferredNodes;

	KeyManager m_keyManager;
	p2p::NetworkPreferences m_netPrefs;

	string m_logBuffer;
};

void help()
{
	cout
		<< "Usage web3 [OPTIONS]" << endl
		<< "Options:" << endl << endl;
	MainCLI::streamHelp(cout);
	cout
		<< "General Options:" << endl
		<< "    -v,--verbosity <0 - 9>  Set the log verbosity from 0 to 9 (default: 8)." << endl
		<< "    -V,--version  Show the version and exit." << endl
		<< "    -h,--help  Show this help message and exit." << endl
		;
	exit(0);
}

void version()
{
	cout << "web3 version " << dev::Version << endl;
	cout << "Build: " << DEV_QUOTED(ETH_BUILD_PLATFORM) << "/" << DEV_QUOTED(ETH_BUILD_TYPE) << endl;
	exit(0);
}

int main(int argc, char** argv)
{
	MainCLI m(MainCLI::Mode::Console);
	g_logVerbosity = 0;

	for (int i = 1; i < argc; ++i)
	{
		string arg = argv[i];
		if ((arg == "-v" || arg == "--verbosity") && i + 1 < argc)
			g_logVerbosity = atoi(argv[++i]);
		else if (arg == "-h" || arg == "--help")
			help();
		else if (arg == "-V" || arg == "--version")
			version();
		else if (m.interpretOption(i, argc, argv)) {}
		else
		{
			cerr << "Invalid argument: " << arg << endl;
			exit(-1);
		}
	}

	m.execute();

	return 0;
}
