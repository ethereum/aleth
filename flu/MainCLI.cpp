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
#include <libfluidity/FluidityClient.h>
#include <libwebthree/WebThree.h>
#include <libweb3jsonrpc/WebThreeStubServer.h>
#include <libweb3jsonrpc/AccountHolder.h>
#include <libjsconsole/JSLocalConsole.h>
using namespace std;
using namespace dev;
using namespace p2p;
using namespace eth;

MainCLI* MainCLI::s_this = nullptr;

MainCLI::MainCLI(Mode _mode):
	m_mode(_mode),
	m_keyManager(getDataDir("fluidity") + "/keys.info"),
	m_chainParams(eth::Network::Fluidity)
{
	s_this = this;
}

bool MainCLI::interpretOption(int& i, int argc, char** argv)
{
	string arg = argv[i];
	if (arg == "console")
		m_mode = Mode::Console;
	else if (arg == "--path" && i + 1 < argc)
		m_dbPath = argv[++i];
	else if (arg == "--client-name" && i + 1 < argc)
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
	else if (arg == "--start-mining")
		m_startSealing = true;
	else
	{
		p2p::NodeSpec ns(arg);
		if (!ns.nodeIPEndpoint())
			return false;
		m_preferredNodes[ns.id()] = make_pair(ns.nodeIPEndpoint(), true);
	}
	return true;
}

void MainCLI::execute()
{
	setup();
	setupKeyManager();
	switch (m_mode)
	{
	case Mode::Console:
	case Mode::Dumb:
	{
		auto nodesState = contents(m_dbPath + "/network.rlp");
		dev::WebThreeDirect web3(
			WebThreeDirect::composeClientVersion("flu", m_clientName),
			m_dbPath,
			m_chainParams,
			WithExisting::Trust,
			set<string>{"flu"},
			m_netPrefs,
			&nodesState);

		signal(SIGABRT, &MainCLI::staticExitHandler);
		signal(SIGTERM, &MainCLI::staticExitHandler);
		signal(SIGINT, &MainCLI::staticExitHandler);

		// TODO: remove all gas pricing components.
		auto gasPricer = make_shared<eth::TrivialGasPricer>(0, 0);
		web3.ethereum()->setGasPricer(gasPricer);

		// Set up fluidity authorities & signing key.
		web3.ethereum()->setSealOption("authorities", rlp(m_authorities));
		for (auto i: m_authorities)
			if (Secret s = m_keyManager.secret(i))
			{
				web3.ethereum()->setSealOption("authority", rlp(s.makeInsecure()));
				web3.ethereum()->setAuthor(i);
			}

		web3.startNetwork();
		if (contents(m_dbPath + "/network.rlp").empty())
			writeFile(m_dbPath + "/network.rlp", web3.saveNetwork());
		cout << "Node ID: " << web3.enode() << endl;

		for (auto const& p: m_preferredNodes)
			if (p.second.second)
				web3.requirePeer(p.first, p.second.first);

		if (m_startSealing)
			web3.ethereum()->startSealing();

		if (m_mode == Mode::Console)
		{
			JSLocalConsole console;
			shared_ptr<dev::WebThreeStubServer> rpcServer = make_shared<dev::WebThreeStubServer>(
				*console.connector(),
				web3,
				make_shared<SimpleAccountHolder>(
					[&](){ return web3.ethereum(); },
					[](Address){ return string(); },
					m_keyManager),
				vector<KeyPair>(),
				m_keyManager,
				*gasPricer,
				this
			);
			string sessionKey = rpcServer->newSession(SessionPermissions{{Privilege::Admin}});
			console.eval("web3.admin.setSessionKey('" + sessionKey + "')");
			while (!m_shouldExit)
				console.readAndEval();
			rpcServer->StopListening();
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
		m_keyManager.import(Secret::random(), "Default");
}

void MainCLI::setup()
{
	m_chainParams = ChainParams(eth::Network::Fluidity);
	m_netPrefs = m_publicIP.empty() ? NetworkPreferences(m_listenIP, m_listenPort, false) : NetworkPreferences(m_publicIP, m_listenIP, m_listenPort, false);
	m_netPrefs.discovery = false;
	m_netPrefs.pin = true;
}

void MainCLI::streamHelp(ostream& _out)
{
	_out
		<< "Main operation modes:" << endl
		<< "    daemon  Daemonify." << endl
		<< "    console  Launch interactive console." << endl
		;
}
