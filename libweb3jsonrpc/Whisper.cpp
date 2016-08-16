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
/** @file Whisper.cpp
 * @authors:
 *   Gav Wood <i@gavwood.com>
 *   Marek Kotewicz <marek@ethdev.com>
 * @date 2015
 */

#include <jsonrpccpp/common/errors.h>
#include <jsonrpccpp/common/exception.h>
#include <libdevcore/CommonJS.h>
#include <libethcore/CommonJS.h>
#include <libwhisper/Interface.h>
#include <libwebthree/WebThree.h>
#include "Whisper.h"
#include "JsonHelper.h"

using namespace std;
using namespace jsonrpc;
using namespace dev;
using namespace dev::rpc;


Whisper::Whisper(WebThreeDirect& _web3, std::vector<dev::KeyPair> const& _accounts): m_web3(_web3)
{
	setIdentities(_accounts);
}

void Whisper::setIdentities(std::vector<dev::KeyPair> const& _ids)
{
	m_ids.clear();
	for (auto i: _ids)
		m_ids[i.pub()] = i.secret();
}

shh::Interface* Whisper::shh() const
{
	return m_web3.whisper().get();
}

bool Whisper::shh_post(Json::Value const& _json)
{
	try
	{
		shh::Message m = shh::toMessage(_json);
		Secret from;
		if (m.from() && m_ids.count(m.from()))
		{
			cwarn << "Silently signing message from identity" << m.from() << ": User validation hook goes here.";
			// TODO: insert validification hook here.
			from = m_ids[m.from()];
		}
		
		shh()->inject(toSealed(_json, m, from));
		return true;
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

std::string Whisper::shh_newIdentity()
{
	KeyPair kp = KeyPair::create();
	m_ids[kp.pub()] = kp.secret();
	return toJS(kp.pub());
}

bool Whisper::shh_hasIdentity(std::string const& _identity)
{
	try
	{
		return m_ids.count(jsToPublic(_identity)) > 0;
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

std::string Whisper::shh_newGroup(std::string const& _id, std::string const& _who)
{
	(void)_id;
	(void)_who;
	return "";
}

std::string Whisper::shh_addToGroup(std::string const& _group, std::string const& _who)
{
	(void)_group;
	(void)_who;
	return "";
}

std::string Whisper::shh_newFilter(Json::Value const& _json)
{
	try
	{
		pair<shh::Topics, Public> w = shh::toWatch(_json);
		auto ret = shh()->installWatch(w.first);
		m_watches.insert(make_pair(ret, w.second));
		return toJS(ret);
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

bool Whisper::shh_uninstallFilter(std::string const& _filterId)
{
	try
	{
		shh()->uninstallWatch(jsToInt(_filterId));
		return true;
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Whisper::shh_getFilterChanges(std::string const& _filterId)
{
	try
	{
		Json::Value ret(Json::arrayValue);
		
		int id = jsToInt(_filterId);
		auto pub = m_watches[id];
		if (!pub || m_ids.count(pub))
			for (h256 const& h: shh()->checkWatch(id))
			{
				auto e = shh()->envelope(h);
				shh::Message m;
				if (pub)
				{
					cwarn << "Silently decrypting message from identity" << pub << ": User validation hook goes here.";
					m = e.open(shh()->fullTopics(id), m_ids[pub]);
				}
				else
					m = e.open(shh()->fullTopics(id));
				if (!m)
					continue;
				ret.append(toJson(h, e, m));
			}
		
		return ret;
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Whisper::shh_getMessages(std::string const& _filterId)
{
	try
	{
		Json::Value ret(Json::arrayValue);
		
		int id = jsToInt(_filterId);
		auto pub = m_watches[id];
		if (!pub || m_ids.count(pub))
			for (h256 const& h: shh()->watchMessages(id))
			{
				auto e = shh()->envelope(h);
				shh::Message m;
				if (pub)
				{
					cwarn << "Silently decrypting message from identity" << pub << ": User validation hook goes here.";
					m = e.open(shh()->fullTopics(id), m_ids[pub]);
				}
				else
					m = e.open(shh()->fullTopics(id));
				if (!m)
					continue;
				ret.append(toJson(h, e, m));
			}
		return ret;
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}
