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
#include "ValidationSchemes.h"
#include <libdevcore/JsonUtils.h>
#include <string>

using namespace std;
namespace js = json_spirit;

namespace dev
{
namespace eth
{
namespace validation
{
string const c_sealEngine = "sealEngine";
string const c_params = "params";
string const c_genesis = "genesis";
string const c_accounts = "accounts";
string const c_balance = "balance";
string const c_wei = "wei";
string const c_finney = "finney";
string const c_author = "author";
string const c_coinbase = "coinbase";
string const c_nonce = "nonce";
string const c_gasLimit = "gasLimit";
string const c_timestamp = "timestamp";
string const c_difficulty = "difficulty";
string const c_extraData = "extraData";
string const c_mixHash = "mixHash";
string const c_parentHash = "parentHash";
string const c_precompiled = "precompiled";
string const c_code = "code";
string const c_storage = "storage";
string const c_gasUsed = "gasUsed";
string const c_codeFromFile = "codeFromFile";  ///< A file containg a code as bytes.
string const c_shouldnotexist = "shouldnotexist";

string const c_minGasLimit = "minGasLimit";
string const c_maxGasLimit = "maxGasLimit";
string const c_gasLimitBoundDivisor = "gasLimitBoundDivisor";
string const c_homesteadForkBlock = "homesteadForkBlock";
string const c_daoHardforkBlock = "daoHardforkBlock";
string const c_EIP150ForkBlock = "EIP150ForkBlock";
string const c_EIP158ForkBlock = "EIP158ForkBlock";
string const c_byzantiumForkBlock = "byzantiumForkBlock";
string const c_eWASMForkBlock = "eWASMForkBlock";
string const c_constantinopleForkBlock = "constantinopleForkBlock";
string const c_constantinopleFixForkBlock = "constantinopleFixForkBlock";
string const c_experimentalForkBlock = "experimentalForkBlock";
string const c_accountStartNonce = "accountStartNonce";
string const c_maximumExtraDataSize = "maximumExtraDataSize";
string const c_tieBreakingGas = "tieBreakingGas";
string const c_blockReward = "blockReward";
string const c_difficultyBoundDivisor = "difficultyBoundDivisor";
string const c_minimumDifficulty = "minimumDifficulty";
string const c_durationLimit = "durationLimit";
string const c_chainID = "chainID";
string const c_networkID = "networkID";
string const c_allowFutureBlocks = "allowFutureBlocks";

void validateConfigJson(js::mObject const& _obj)
{
    requireJsonFields(_obj, "ChainParams::loadConfig",
        {{c_sealEngine, {{js::str_type}, JsonFieldPresence::Required}},
            {c_params, {{js::obj_type}, JsonFieldPresence::Required}},
            {c_genesis, {{js::obj_type}, JsonFieldPresence::Required}},
            {c_accounts, {{js::obj_type}, JsonFieldPresence::Required}}});

    requireJsonFields(_obj.at(c_genesis).get_obj(), "ChainParams::loadConfig::genesis",
        {{c_author, {{js::str_type}, JsonFieldPresence::Required}},
            {c_nonce, {{js::str_type}, JsonFieldPresence::Required}},
            {c_gasLimit, {{js::str_type}, JsonFieldPresence::Required}},
            {c_timestamp, {{js::str_type}, JsonFieldPresence::Required}},
            {c_difficulty, {{js::str_type}, JsonFieldPresence::Required}},
            {c_extraData, {{js::str_type}, JsonFieldPresence::Required}},
            {c_mixHash, {{js::str_type}, JsonFieldPresence::Required}},
            {c_parentHash, {{js::str_type}, JsonFieldPresence::Optional}}});

    js::mObject const& accounts = _obj.at(c_accounts).get_obj();
    for (auto const& acc : accounts)
        validateAccountObj(acc.second.get_obj());
}

void validateAccountMaskObj(js::mObject const& _obj)
{
    // A map object with possibly defined fields
    requireJsonFields(_obj, "validateAccountMaskObj",
        {{c_storage, {{js::obj_type}, JsonFieldPresence::Optional}},
            {c_balance, {{js::str_type}, JsonFieldPresence::Optional}},
            {c_nonce, {{js::str_type}, JsonFieldPresence::Optional}},
            {c_code, {{js::str_type}, JsonFieldPresence::Optional}},
            {c_precompiled, {{js::obj_type}, JsonFieldPresence::Optional}},
            {c_shouldnotexist, {{js::str_type}, JsonFieldPresence::Optional}},
            {c_wei, {{js::str_type}, JsonFieldPresence::Optional}}});
}

void validateAccountObj(js::mObject const& _obj)
{
    if (_obj.count(c_precompiled))
    {
        // A precompiled contract
        requireJsonFields(_obj, "validateAccountObj",
            {{c_precompiled, {{js::obj_type}, JsonFieldPresence::Required}},
                {c_wei, {{js::str_type}, JsonFieldPresence::Optional}},
                {c_balance, {{js::str_type}, JsonFieldPresence::Optional}}});
    }
    else
    {
        // Extendable account description. Check typo errors and fields type.
        requireJsonFields(_obj, "validateAccountObj",
            {{c_code, {{js::str_type}, JsonFieldPresence::Optional}},
                {c_nonce, {{js::str_type}, JsonFieldPresence::Optional}},
                {c_storage, {{js::obj_type}, JsonFieldPresence::Optional}},
                {c_balance, {{js::str_type}, JsonFieldPresence::Optional}},
                {c_wei, {{js::str_type}, JsonFieldPresence::Optional}},
                {c_codeFromFile, {{js::str_type}, JsonFieldPresence::Optional}}});

        // At least one field must be set
        if (_obj.empty())
        {
            string comment =
                "Error in validateAccountObj: At least one field must be set (code, nonce, "
                "storage, balance, wei, codeFromFile)!";
            BOOST_THROW_EXCEPTION(MissingField() << errinfo_comment(comment));
        }

        // c_code, c_codeFromFile could not coexist
        if (_obj.count(c_code) && _obj.count(c_codeFromFile))
        {
            string comment =
                "Error in validateAccountObj: field 'code' contradicts field 'codeFromFile'!";
            BOOST_THROW_EXCEPTION(UnknownField() << errinfo_comment(comment));
        }
    }

    // c_wei, c_balance could not coexist
    if (_obj.count(c_wei) && _obj.count(c_balance))
    {
        string comment = "Error in validateAccountObj: field 'balance' contradicts field 'wei'!";
        BOOST_THROW_EXCEPTION(UnknownField() << errinfo_comment(comment));
    }
}
}
}
}
