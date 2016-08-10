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
/** @file Whisper.h
 * @authors:
 *   Gav Wood <i@gavwood.com>
 *   Marek Kotewicz <marek@ethdev.com>
 * @date 2015
 */

#pragma once
#include <libdevcrypto/Common.h>
#include "WhisperFace.h"

namespace dev
{

class WebThreeDirect;

namespace shh
{
class Interface;
}

namespace rpc
{
	
class Whisper: public WhisperFace
{
public:
	// TODO: init with whisper interface instead of webthreedirect
	Whisper(WebThreeDirect& _web3, std::vector<dev::KeyPair> const& _accounts);
	virtual RPCModules implementedModules() const override
	{
		return RPCModules{RPCModule{"shh", "1.0"}};
	}

	virtual void setIdentities(std::vector<dev::KeyPair> const& _ids);
	std::map<dev::Public, dev::Secret> const& ids() const { return m_ids; }

	virtual bool shh_post(Json::Value const& _json) override;
	virtual std::string shh_newIdentity() override;
	virtual bool shh_hasIdentity(std::string const& _identity) override;
	virtual std::string shh_newGroup(std::string const& _id, std::string const& _who) override;
	virtual std::string shh_addToGroup(std::string const& _group, std::string const& _who) override;
	virtual std::string shh_newFilter(Json::Value const& _json) override;
	virtual bool shh_uninstallFilter(std::string const& _filterId) override;
	virtual Json::Value shh_getFilterChanges(std::string const& _filterId) override;
	virtual Json::Value shh_getMessages(std::string const& _filterId) override;
	
private:
	shh::Interface* shh() const;

	WebThreeDirect& m_web3;
	std::map<dev::Public, dev::Secret> m_ids;
	std::map<unsigned, dev::Public> m_watches;
};

}
}
