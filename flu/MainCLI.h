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
/** @file MainCLI.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */

#pragma once

#include <string>
#include <libp2p/NodeTable.h>
#include <libp2p/Network.h>
#include <libethcore/KeyManager.h>
#include <libethereum/ChainParams.h>
#include <libwebthree/SystemManager.h>

namespace dev
{

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

struct RPCPrivate;
class WebThreeDirect;
namespace eth { class TrivialGasPricer; }

class MainCLI: public CLI, public dev::SystemManager
{
public:
	enum class Mode
	{
		Dumb,
		Benchmark,
		Console,
		Attach,
		Execute
	};

	MainCLI(Mode _mode = Mode::Dumb);

	bool interpretOption(int& i, int argc, char** argv);
	void execute();

	static void streamHelp(std::ostream& _out);

	static void staticExitHandler(int) { s_this->m_shouldExit = true; }

private:
	void exit() override { staticExitHandler(0); }
	void setup();
	void setupKeyManager();
	void startRPC(WebThreeDirect& _web3, eth::TrivialGasPricer& _gasPricer);

	Mode m_mode;

	std::string m_remoteURL = "http://localhost:8545";
	std::string m_remoteSessionKey;

	std::string m_dbPath = getDataDir("fluidity");
	std::string m_masterPassword;

	Addresses m_authorities;
	std::string m_chainName;

	bool m_startSealing = false;

	/// Networking params.
	std::string m_clientName;
	std::string m_listenIP;
	unsigned short m_listenPort = 40404;
	std::string m_publicIP;

	/// Execution params (Mode::Execute).
	strings m_toExecute;

	bool m_ipc = false;
	int m_jsonRPCPort = -1;
	std::string m_rpcCorsDomain = "";

	eth::KeyManager m_keyManager;
	p2p::NetworkPreferences m_netPrefs;
	std::string m_paramsJson;
	eth::ChainParams m_chainParams;
	std::unordered_map<std::string, bytes> m_sealEngineParams;
	std::map<p2p::NodeID, std::pair<p2p::NodeIPEndpoint, bool>> m_preferredNodes;

	std::atomic<bool> m_shouldExit = {false};
	std::string m_logBuffer;

	std::shared_ptr<RPCPrivate> m_rpcPrivate;

	static MainCLI* s_this;
};

}
