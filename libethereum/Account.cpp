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
#include <libdevcore/JsonUtils.h>
#include <libethcore/ChainOperationParams.h>
#include <libethcore/Precompiled.h>

using namespace std;
using namespace dev;
using namespace dev::eth;

void Account::setCode(bytes&& _code)
{
	m_codeCache = std::move(_code);
	m_hasNewCode = true;
	m_codeHash = sha3(m_codeCache);
}

namespace js = json_spirit;

namespace
{

uint64_t toUnsigned(js::mValue const& _v)
{
	switch (_v.type())
	{
	case js::int_type: return _v.get_uint64();
	case js::str_type: return fromBigEndian<uint64_t>(fromHex(_v.get_str()));
	default: return 0;
	}
}

PrecompiledContract createPrecompiledContract(js::mObject& _precompiled)
{
	auto n = _precompiled["name"].get_str();
	try
	{
		u256 startingBlock = 0;
		if (_precompiled.count("startingBlock"))
			startingBlock = u256(_precompiled["startingBlock"].get_str());

		if (!_precompiled.count("linear"))
			return PrecompiledContract(PrecompiledRegistrar::pricer(n), PrecompiledRegistrar::executor(n), startingBlock);

		auto l = _precompiled["linear"].get_obj();
		unsigned base = toUnsigned(l["base"]);
		unsigned word = toUnsigned(l["word"]);
		return PrecompiledContract(base, word, PrecompiledRegistrar::executor(n), startingBlock);
	}
	catch (PricerNotFound const&)
	{
		cwarn << "Couldn't create a precompiled contract account. Missing a pricer called:" << n;
		throw;
	}
	catch (ExecutorNotFound const&)
	{
		// Oh dear - missing a plugin?
		cwarn << "Couldn't create a precompiled contract account. Missing an executor called:" << n;
		throw;
	}
}

}
namespace
{
	string const c_wei = "wei";
	string const c_finney = "finney";
	string const c_balance = "balance";
	string const c_nonce = "nonce";
	string const c_code = "code";
	string const c_storage = "storage";
	string const c_shouldnotexist = "shouldnotexist";
	string const c_precompiled = "precompiled";
	std::set<string> const c_knownAccountFields = {
		c_wei, c_finney, c_balance, c_nonce, c_code, c_storage, c_shouldnotexist,
		c_code, c_precompiled
	};
	void validateAccountMapObj(js::mObject const& _o)
	{
		for (auto const& field: _o)
			validateFieldNames(field.second.get_obj(), c_knownAccountFields);
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
	json_spirit::read_string_or_throw(_json, val);
	js::mObject o = val.get_obj();
	validateAccountMapObj(o);
	for (auto const& account: o)
	{
		Address a(fromHex(account.first));
		auto o = account.second.get_obj();

		bool haveBalance = (o.count(c_wei) || o.count(c_finney) || o.count(c_balance));
		bool haveNonce = o.count(c_nonce);
		bool haveCode = o.count(c_code);
		bool haveStorage = o.count(c_storage);
		bool shouldNotExists = o.count(c_shouldnotexist);

		if (haveStorage || haveCode || haveNonce || haveBalance)
		{
			u256 balance = 0;
			if (o.count(c_wei))
				balance = u256Safe(o[c_wei].get_str());
			else if (o.count(c_finney))
				balance = u256Safe(o[c_finney].get_str()) * finney;
			else if (o.count(c_balance))
				balance = u256Safe(o[c_balance].get_str());

			u256 nonce = haveNonce ? u256Safe(o[c_nonce].get_str()) : _defaultNonce;

			if (haveCode)
			{
				ret[a] = Account(nonce, balance);
				if (o[c_code].type() == json_spirit::str_type)
				{
					if (o[c_code].get_str().find("0x") != 0 && !o[c_code].get_str().empty())
						cerr << "Error importing code of account " << a << "! Code needs to be hex bytecode prefixed by \"0x\".";
					else
						ret[a].setCode(fromHex(o[c_code].get_str()));
				}
				else
					cerr << "Error importing code of account " << a << "! Code field needs to be a string";
			}
			else
				ret[a] = Account(nonce, balance);

			if (haveStorage)
				for (pair<string, js::mValue> const& j: o[c_storage].get_obj())
					ret[a].setStorage(u256(j.first), u256(j.second.get_str()));
		}

		if (o_mask)
		{
			(*o_mask)[a] = AccountMask(haveBalance, haveNonce, haveCode, haveStorage, shouldNotExists);
			if (!haveStorage && !haveCode && !haveNonce && !haveBalance && shouldNotExists) //defined only shouldNotExists field
				ret[a] = Account(0, 0);
		}

		if (o_precompiled && o.count(c_precompiled))
		{
			js::mObject p = o[c_precompiled].get_obj();
			o_precompiled->insert(make_pair(a, createPrecompiledContract(p)));
		}
	}

	return ret;
}
