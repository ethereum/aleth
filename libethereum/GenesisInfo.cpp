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
/** @file GenesisInfo.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include <libdevcrypto/Common.h>
#include <json_spirit/JsonSpiritHeaders.h>
#include "GenesisInfo.h"

using namespace dev;
using namespace eth;

KeyPair const dev::eth::FluidityTreasure(Secret("a15e7af8ea0a717182d3608e6cdb2bff97ccaad3b6befc8787abe4c937796579"));

std::string const dev::eth::c_genesisInfoFluidity =
R"ETHEREUM(
{
	"sealEngine": "BasicAuthority",
	"params": {
		"accountStartNonce": "0x",
		"maximumExtraDataSize": "0x1000000",
		"blockReward": "0x",
		"registrar": "",
		"networkID" : "0x45"
	},
	"genesis": {
		"author": "0x0000000000000000000000000000000000000000",
		"timestamp": "0x00",
		"parentHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
		"extraData": "0x",
		"gasLimit": "0x1000000000000"
	},
	"accounts": {
		"0000000000000000000000000000000000000001": { "wei": "1", "precompiled": { "name": "ecrecover", "linear": { "base": 3000, "word": 0 } } },
		"0000000000000000000000000000000000000002": { "wei": "1", "precompiled": { "name": "sha256", "linear": { "base": 60, "word": 12 } } },
		"0000000000000000000000000000000000000003": { "wei": "1", "precompiled": { "name": "ripemd160", "linear": { "base": 600, "word": 120 } } },
		"0000000000000000000000000000000000000004": { "wei": "1", "precompiled": { "name": "identity", "linear": { "base": 15, "word": 3 } } },
		"00c9b024c2efc853ecabb8be2fb1d16ce8174ab1": { "wei": "1606938044258990275541962092341162602522202993782792835301376" }
	}
}
)ETHEREUM";

std::string const dev::eth::c_genesisInfoTestBasicAuthority =
R"E(
{
	"sealEngine": "BasicAuthority",
	"params": {
		"accountStartNonce": "0x00",
		"maximumExtraDataSize": "0x20",
		"minGasLimit": "0x1388",
		"maxGasLimit": "0x7fffffffffffffff",
		"gasLimitBoundDivisor": "0x0400",
		"minimumDifficulty": "0x020000",
		"difficultyBoundDivisor": "0x0800",
		"durationLimit": "0x0d",
		"blockReward": "0x4563918244F40000",
		"registrar" : "0xc6d9d2cd449a754c494264e1809c50e34d64562b",
		"networkID" : "0x1"
	},
	"genesis": {
		"nonce": "0x0000000000000042",
		"difficulty": "0x400000000",
		"mixHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
		"author": "0x0000000000000000000000000000000000000000",
		"timestamp": "0x00",
		"parentHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
		"extraData": "0x11bbe8db4e347b4e8c937c1c8370e4b5ed33adb3db69cbdb7a38e1e50b1b82fa",
		"gasLimit": "0x2fefd8"
	},
	"accounts": {
		"0000000000000000000000000000000000000001": { "wei": "1", "precompiled": { "name": "ecrecover", "linear": { "base": 3000, "word": 0 } } },
		"0000000000000000000000000000000000000002": { "wei": "1", "precompiled": { "name": "sha256", "linear": { "base": 60, "word": 12 } } },
		"0000000000000000000000000000000000000003": { "wei": "1", "precompiled": { "name": "ripemd160", "linear": { "base": 600, "word": 120 } } },
		"0000000000000000000000000000000000000004": { "wei": "1", "precompiled": { "name": "identity", "linear": { "base": 15, "word": 3 } } },
		"00c9b024c2efc853ecabb8be2fb1d16ce8174ab1": { "wei": "1606938044258990275541962092341162602522202993782792835301376" }
	}
}
)E";

static std::string const c_childDaos =
R"E(
{
"accounts": [
	{
		"address" : "0x304a554a310c7e546dfe434669c62820b7d83490",
		"extraBalanceAccount" : "0x914d1b8b43e92723e64fd0a06f5bdb8dd9b10c79"
	},
	{
		"address" : "0xfe24cdd8648121a43a7c86d289be4dd2951ed49f",
		"extraBalanceAccount" : "0x17802f43a0137c506ba92291391a8a8f207f487d"
	},
	{
		"address" : "0xb136707642a4ea12fb4bae820f03d2562ebff487",
		"extraBalanceAccount" : "0xdbe9b615a3ae8709af8b93336ce9b477e4ac0940"
	},
	{
		"address" : "0xf14c14075d6c4ed84b86798af0956deef67365b5",
		"extraBalanceAccount" : "0xca544e5c4687d109611d0f8f928b53a25af72448"
	},
	{
		"address" : "0xaeeb8ff27288bdabc0fa5ebb731b6f409507516c",
		"extraBalanceAccount" : "0xcbb9d3703e651b0d496cdefb8b92c25aeb2171f7"
	},
	{
		"address" : "0xaccc230e8a6e5be9160b8cdf2864dd2a001c28b6",
		"extraBalanceAccount" : "0x2b3455ec7fedf16e646268bf88846bd7a2319bb2"
	},
	{
		"address" : "0x4613f3bca5c44ea06337a9e439fbc6d42e501d0a",
		"extraBalanceAccount" : "0xd343b217de44030afaa275f54d31a9317c7f441e"
	},
	{
		"address" : "0x84ef4b2357079cd7a7c69fd7a37cd0609a679106",
		"extraBalanceAccount" : "0xda2fef9e4a3230988ff17df2165440f37e8b1708"
	},
	{
		"address" : "0xf4c64518ea10f995918a454158c6b61407ea345c",
		"extraBalanceAccount" : "0x7602b46df5390e432ef1c307d4f2c9ff6d65cc97"
	},
	{
		"address" : "0xbb9bc244d798123fde783fcc1c72d3bb8c189413",
		"extraBalanceAccount" : "0x807640a13483f8ac783c557fcdf27be11ea4ac7a"
	}]
})E";

dev::Addresses dev::eth::childDaos()
{
	Addresses daos;
	json_spirit::mValue val;
	json_spirit::read_string(c_childDaos, val);
	json_spirit::mObject obj = val.get_obj();
	for(auto const& items: obj)
	{
		json_spirit::mArray array = items.second.get_array();
		for (auto account: array)
		{
			daos.push_back(Address(account.get_obj()["address"].get_str()));
			daos.push_back(Address(account.get_obj()["extraBalanceAccount"].get_str()));
		}
	}
	return daos;
}

