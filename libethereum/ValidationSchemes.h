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

#pragma once

#include <json_spirit/JsonSpiritHeaders.h>
#include <string>

namespace dev
{
namespace eth
{
namespace validation
{
extern std::string const c_sealEngine;
extern std::string const c_params;
extern std::string const c_genesis;
extern std::string const c_accounts;
extern std::string const c_balance;
extern std::string const c_wei;
extern std::string const c_finney;
extern std::string const c_author;
extern std::string const c_coinbase;
extern std::string const c_nonce;
extern std::string const c_gasLimit;
extern std::string const c_timestamp;
extern std::string const c_difficulty;
extern std::string const c_extraData;
extern std::string const c_mixHash;
extern std::string const c_parentHash;
extern std::string const c_precompiled;
extern std::string const c_storage;
extern std::string const c_code;
extern std::string const c_gasUsed;
extern std::string const c_codeFromFile;
extern std::string const c_shouldnotexist;

extern std::string const c_minGasLimit;
extern std::string const c_maxGasLimit;
extern std::string const c_gasLimitBoundDivisor;
extern std::string const c_homesteadForkBlock;
extern std::string const c_daoHardforkBlock;
extern std::string const c_EIP150ForkBlock;
extern std::string const c_EIP158ForkBlock;
extern std::string const c_byzantiumForkBlock;
extern std::string const c_eWASMForkBlock;
extern std::string const c_constantinopleForkBlock;
extern std::string const c_constantinopleFixForkBlock;
extern std::string const c_experimentalForkBlock;
extern std::string const c_accountStartNonce;
extern std::string const c_maximumExtraDataSize;
extern std::string const c_tieBreakingGas;
extern std::string const c_blockReward;
extern std::string const c_difficultyBoundDivisor;
extern std::string const c_minimumDifficulty;
extern std::string const c_durationLimit;
extern std::string const c_chainID;
extern std::string const c_networkID;
extern std::string const c_allowFutureBlocks;

// Validate config.json that contains chain params and genesis state
void validateConfigJson(json_spirit::mObject const& _obj);

// Validate account json object
void validateAccountObj(json_spirit::mObject const& _o);

// TODO move AccountMaskObj to libtesteth (it is used only in test logic)
// Validate accountMask json object. Mask indicates which fields are set
void validateAccountMaskObj(json_spirit::mObject const& _o);
}
}
}
