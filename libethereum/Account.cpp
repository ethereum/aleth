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
/** @file Account.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "Account.h"
#include <json_spirit/JsonSpiritHeaders.h>
#include <libethcore/ChainOperationParams.h>
#include <libethcore/Precompiled.h>

using namespace std;
using namespace dev;
using namespace dev::eth;

void Account::setNewCode(bytes&& _code)
{
	m_codeCache = std::move(_code);
	m_hasNewCode = true;
	m_codeHash = sha3(m_codeCache);
}

namespace js = json_spirit;

uint64_t toUnsigned(js::mValue const& _v)
{
	switch (_v.type())
	{
	case js::int_type: return _v.get_uint64();
	case js::str_type: return fromBigEndian<uint64_t>(fromHex(_v.get_str()));
	default: return 0;
	}
}

AccountMap dev::eth::jsonToAccountMap(std::string const& _json, u256 const& _defaultNonce, AccountMaskMap* o_mask, PrecompiledContractMap* o_precompiled)
{
	auto u256Safe = [](std::string const& s) -> u256 {
		bigint ret(s);
		if (ret >= bigint(1) << 256)
			BOOST_THROW_EXCEPTION(ValueTooLarge() << errinfo_comment("State value is equal or greater than 2**256") );
		return (u256)ret;
	};

	std::unordered_map<Address, Account> ret;

	js::mValue val;
	json_spirit::read_string(_json, val);
	js::mObject o = val.get_obj();
	for (auto const& account: o.count("alloc") ? o["alloc"].get_obj() : o.count("accounts") ? o["accounts"].get_obj() : o)
	{
		Address a(fromHex(account.first));
		auto o = account.second.get_obj();

		bool haveBalance = (o.count("wei") || o.count("finney") || o.count("balance"));
		bool haveNonce = o.count("nonce");
		bool haveCode = o.count("code");
		bool haveStorage = o.count("storage");
		bool shouldNotExists = o.count("shouldnotexist");

		if (haveStorage || haveCode || haveNonce || haveBalance)
		{
			u256 balance = 0;
			if (o.count("wei"))
				balance = u256Safe(o["wei"].get_str());
			else if (o.count("finney"))
				balance = u256Safe(o["finney"].get_str()) * finney;
			else if (o.count("balance"))
				balance = u256Safe(o["balance"].get_str());

			u256 nonce = haveNonce ? u256Safe(o["nonce"].get_str()) : _defaultNonce;

			if (haveCode)
			{
				ret[a] = Account(nonce, balance);
				if (o["code"].type() == json_spirit::str_type)
				{
					if (o["code"].get_str().find("0x") != 0)
						cerr << "Error importing code of account " << a << "! Code needs to be hex bytecode prefixed by \"0x\".";
					else
						ret[a].setNewCode(fromHex(o["code"].get_str().substr(2)));
				}
				else
					cerr << "Error importing code of account " << a << "! Code field needs to be a string";
			}
			else
				ret[a] = Account(nonce, balance);

			if (haveStorage)
				for (pair<string, js::mValue> const& j: o["storage"].get_obj())
					ret[a].setStorage(u256(j.first), u256(j.second.get_str()));
		}

		if (o_mask)
		{
			(*o_mask)[a] = AccountMask(haveBalance, haveNonce, haveCode, haveStorage, shouldNotExists);
			if (!haveStorage && !haveCode && !haveNonce && !haveBalance && shouldNotExists) //defined only shouldNotExists field
				ret[a] = Account(0, 0);
		}

		if (o_precompiled && o.count("precompiled"))
		{
			js::mObject p = o["precompiled"].get_obj();
			auto n = p["name"].get_str();
			if (!p.count("linear"))
			{
				cwarn << "No gas cost given for precompiled contract " << n;
				throw;
			}
			try
			{
				auto l = p["linear"].get_obj();
				u256 startingBlock = 0;
				if (p.count("startingBlock"))
					startingBlock = u256(p["startingBlock"].get_str());
				unsigned base = toUnsigned(l["base"]);
				unsigned word = toUnsigned(l["word"]);
				o_precompiled->insert(make_pair(a, PrecompiledContract(base, word, PrecompiledRegistrar::executor(n), startingBlock)));
			}
			catch (ExecutorNotFound)
			{
				// Oh dear - missing a plugin?
				cwarn << "Couldn't create a precompiled contract account. Missing an executor called:" << n;
				throw;
			}
		}
	}

	return ret;
}
