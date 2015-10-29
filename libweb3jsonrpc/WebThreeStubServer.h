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
/** @file WebThreeStubServer.h
 * @authors:
 *   Gav Wood <i@gavwood.com>
 *   Marek Kotewicz <marek@ethdev.com>
 * @date 2014
 */

#pragma once

#include <libdevcore/db.h>
#include "WebThreeStubServerBase.h"
#include "SessionManager.h"

namespace dev
{

class WebThreeDirect;
namespace eth
{
class KeyManager;
class TrivialGasPricer;
class BlockChain;
class BlockQueue;
}

/**
 * @brief JSON-RPC api implementation for WebThreeDirect
 */
class WebThreeStubServer: public dev::WebThreeStubServerBase
{
public:
	WebThreeStubServer(dev::WebThreeDirect& _web3, std::shared_ptr<dev::eth::AccountHolder> const& _ethAccounts, dev::eth::KeyManager& _keyMan, dev::eth::TrivialGasPricer& _gp);

	std::string newSession(SessionPermissions const& _p);
	void addSession(std::string const& _session, SessionPermissions const& _p) { m_sessions[_session] = _p; }

	virtual void setMiningBenefactorChanger(std::function<void(Address const&)> const& _f) { m_setMiningBenefactor = _f; }

private:

	virtual bool eth_notePassword(std::string const& _password) override;
	virtual Json::Value eth_syncing() override;

private:


	dev::WebThreeDirect& m_web3;
	dev::eth::KeyManager& m_keyMan;
	dev::eth::TrivialGasPricer& m_gp;

	std::function<void(Address const&)> m_setMiningBenefactor;
	std::unordered_map<std::string, SessionPermissions> m_sessions;
};

}
