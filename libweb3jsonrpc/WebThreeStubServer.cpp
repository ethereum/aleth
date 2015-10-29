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
/** @file WebThreeStubServer.cpp
 * @authors:
 *   Gav Wood <i@gavwood.com>
 *   Marek Kotewicz <marek@ethdev.com>
 * @date 2014
 */

#include "WebThreeStubServer.h"
// Make sure boost/asio.hpp is included before windows.h.
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <libdevcore/FileSystem.h>
#include <libdevcore/CommonJS.h>
#include <libethcore/KeyManager.h>
#include <libethcore/ICAP.h>
#include <libethereum/Executive.h>
#include <libethereum/Block.h>
#include <libwebthree/WebThree.h>
#include "JsonHelper.h"
using namespace std;
using namespace dev;
using namespace dev::eth;
namespace fs = boost::filesystem;

WebThreeStubServer::WebThreeStubServer(WebThreeDirect& _web3, shared_ptr<AccountHolder> const& _ethAccounts, KeyManager& _keyMan, dev::eth::TrivialGasPricer& _gp):
	WebThreeStubServerBase(_ethAccounts),
	m_web3(_web3),
	m_keyMan(_keyMan),
	m_gp(_gp)
{}

std::string WebThreeStubServer::newSession(SessionPermissions const& _p)
{
	std::string s = toBase64(h64::random().ref());
	m_sessions[s] = _p;
	return s;
}

bool WebThreeStubServer::eth_notePassword(string const& _password)
{
	m_keyMan.notePassword(_password);
	return true;
}

Json::Value WebThreeStubServer::eth_syncing()
{
	dev::eth::SyncStatus sync = m_web3.ethereum()->syncStatus();
	if (sync.state == SyncState::Idle || !m_web3.ethereum()->isMajorSyncing())
		return Json::Value(false);

	Json::Value info(Json::objectValue);
	info["startingBlock"] = sync.startBlockNumber;
	info["highestBlock"] = sync.highestBlockNumber;
	info["currentBlock"] = sync.currentBlockNumber;
	return info;
}
