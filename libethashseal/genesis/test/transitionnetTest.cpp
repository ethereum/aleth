// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "../../GenesisInfo.h"

static std::string const c_genesisInfoTest = std::string() +
R"E(
{
	"sealEngine": "Ethash",
	"params": {
		"accountStartNonce": "0x00",
		"homesteadForkBlock": "0x05",
		"daoHardforkBlock": "0x08",
		"EIP150ForkBlock": "0x0a",
		"EIP158ForkBlock": "0x0e",
		"byzantiumForkBlock": "0x10",
		"constantinopleForkBlock": "0x14",
		"networkID" : "0x01",
		"chainID": "0x01",
		"maximumExtraDataSize": "0x20",
		"tieBreakingGas": false,
		"minGasLimit": "0x1388",
		"maxGasLimit": "0x7fffffffffffffff",
		"gasLimitBoundDivisor": "0x0400",
		"minimumDifficulty": "0x020000",
		"difficultyBoundDivisor": "0x0800",
		"durationLimit": "0x0d",
		"blockReward": "0x4563918244F40000",
		"allowFutureBlocks" : true
	},
	"genesis": {
		"nonce": "0x0000000000000042",
		"difficulty": "0x020000",
		"mixHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
		"author": "0x0000000000000000000000000000000000000000",
		"timestamp": "0x00",
		"parentHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
		"extraData": "0x11bbe8db4e347b4e8c937c1c8370e4b5ed33adb3db69cbdb7a38e1e50b1b82fa",
		"gasLimit": "0x1388"
	},
	"accounts": {
		"0000000000000000000000000000000000000001": { "precompiled": { "name": "ecrecover", "linear": { "base": 3000, "word": 0 } } },
		"0000000000000000000000000000000000000002": { "precompiled": { "name": "sha256", "linear": { "base": 60, "word": 12 } } },
		"0000000000000000000000000000000000000003": { "precompiled": { "name": "ripemd160", "linear": { "base": 600, "word": 120 } } },
		"0000000000000000000000000000000000000004": { "precompiled": { "name": "identity", "linear": { "base": 15, "word": 3 } } },
		"0000000000000000000000000000000000000005": { "precompiled": { "name": "modexp", "startingBlock": "0x10" } },
		"0000000000000000000000000000000000000006": { "precompiled": { "name": "alt_bn128_G1_add", "startingBlock": "0x10", "linear": { "base": 500, "word": 0 } } },
		"0000000000000000000000000000000000000007": { "precompiled": { "name": "alt_bn128_G1_mul", "startingBlock": "0x10", "linear": { "base": 40000, "word": 0 } } },
		"0000000000000000000000000000000000000008": { "precompiled": { "name": "alt_bn128_pairing_product", "startingBlock": "0x10" } }
	}
}
)E";
