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
#include <liblll/Compiler.h>
#include <json_spirit/JsonSpiritHeaders.h>
#include <libethcore/Common.h>
using namespace std;
using namespace dev;
using namespace dev::eth;
namespace js = json_spirit;

#pragma GCC diagnostic ignored "-Wunused-variable"

const h256 Account::c_contractConceptionCodeHash;

AccountMap dev::eth::jsonToAccountMap(std::string const& _json, AccountMaskMap* o_mask)
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
/*	if (o.count("genesis"))
	{
		cdebug << json_spirit::write_string(o["genesis"], true);
		o = o["genesis"].get_obj();
		for (auto i: o)
			cdebug << i.first;
	}*/
	for (auto account: o.count("alloc") ? o["alloc"].get_obj() : o)
	{
		Address a(fromHex(account.first));
		auto o = account.second.get_obj();
		u256 balance = 0;

		bool haveBalance = (o.count("wei") || o.count("finney") || o.count("balance"));
		if (o.count("wei"))
			balance = u256Safe(o["wei"].get_str());
		else if (o.count("finney"))
			balance = u256Safe(o["finney"].get_str()) * finney;
		else if (o.count("balance"))
			balance = u256Safe(o["balance"].get_str());

		bool haveNonce = o.count("nonce") > 0;
		u256 nonce = haveNonce ? u256Safe(o["nonce"].get_str()) : 0;

		bool haveCode = o.count("code");
		if (haveCode)
		{
			ret[a] = Account(nonce, balance, Account::ContractConception);
			if (o["code"].type() == json_spirit::str_type)
			{
				if (o["code"].get_str().find("0x") != 0)
					ret[a].setCode(compileLLL(o["code"].get_str(), false));
				else
					ret[a].setCode(fromHex(o["code"].get_str().substr(2)));
			}
			else
				cerr << "Error importing code of account " << a << "! Code field needs to be a string";
		}
		else
			ret[a] = Account(nonce, balance, Account::NormalCreation);

		bool haveStorage = o.count("storage");
		if (haveStorage)
			for (pair<string, js::mValue> const& j: o["storage"].get_obj())
				ret[a].setStorage(u256(j.first), u256(j.second.get_str()));

		if (o_mask)
			(*o_mask)[a] = AccountMask(haveBalance, haveNonce, haveCode, haveStorage);
	}

	return ret;
}
