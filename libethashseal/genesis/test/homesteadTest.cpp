// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2016-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "../../GenesisInfo.h"

static std::string const c_genesisInfoHomesteadTest = std::string() +
R"E(
{
	"sealEngine": "Ethash",
	"params": {
		"accountStartNonce": "0x00",
		"maximumExtraDataSize": "0x20",
		"homesteadForkBlock": "0x00",
		"networkID" : "0x01",
		"chainID": "0x01",
		"minGasLimit": "0x1388",
		"maxGasLimit": "7fffffffffffffff",
		"tieBreakingGas": false,
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
	}
}
)E";
