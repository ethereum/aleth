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
