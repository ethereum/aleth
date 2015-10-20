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

MainCLI::MainCLI(Mode _mode):
	m_mode(_mode)
{
}

bool MainCLI::interpretOption(int& i, int argc, char** argv)
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

void MainCLI::execute()
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

void MainCLI::setupKeyManager()
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

void MainCLI::setup()
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

void MainCLI::streamHelp(ostream& _out)
{
	_out
		<< "Main operation modes:" << endl
		<< "    daemon  Daemonify." << endl
		<< "    console  Launch interactive console." << endl
		;
}
