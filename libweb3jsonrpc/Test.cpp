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
/** @file Test.cpp
 * @authors:
 *   Dimitry Khokhlov <dimitry@ethdev.com>
 * @date 2016
 */

#include "Test.h"
#include <libdevcore/CommonJS.h>
#include <jsonrpccpp/common/errors.h>
#include <jsonrpccpp/common/exception.h>
#include <libethereum/ClientTest.h>
#include <libethereum/ChainParams.h>

using namespace std;
using namespace dev;
using namespace dev::rpc;
using namespace jsonrpc;

Test::Test(eth::Client& _eth): m_eth(_eth) {}

Json::Value fillJsonWithState(eth::State const& _state)
{
    Json::Value oState;
    for (auto const& a : _state.addresses())
    {
        Json::Value o;
        o["balance"] = toCompactHexPrefixed(_state.balance(a.first), 1);
        o["nonce"] = toCompactHexPrefixed(_state.getNonce(a.first), 1);

        Json::Value store;
        for (auto const& s : _state.storage(a.first))
            store[toCompactHexPrefixed(s.second.first, 1)] =
                toCompactHexPrefixed(s.second.second, 1);
        o["storage"] = store;
        o["code"] = toHexPrefixed(_state.code(a.first));
        oState[toHexPrefixed(a.first)] = o;
    }
    return oState;
}

string exportLog(eth::LogEntries const& _logs)
{
    RLPStream s;
    s.appendList(_logs.size());
    for (eth::LogEntry const& l : _logs)
        l.streamRLP(s);
    return toHexPrefixed(sha3(s.out()));
}

const string c_postStateJustHashVersion = "justhash";
const string c_postStateFullStateVersion = "fullstate";
const string c_postStateLogHashVersion = "justloghash";
string Test::test_getPostState(Json::Value const& param1)
{
    Json::Value out;
    Json::FastWriter fastWriter;
    if (param1["version"].asString() == c_postStateJustHashVersion)
        return toJS(m_eth.postState().state().rootHash());
    else if (param1["version"].asString() == c_postStateFullStateVersion)
    {
        out = fillJsonWithState(m_eth.postState().state());
        return fastWriter.write(out);
    }
    else if (param1["version"].asString() == c_postStateLogHashVersion)
    {
        if (m_eth.blockChain().receipts().receipts.size() != 0)
            return exportLog(m_eth.blockChain().receipts().receipts[0].log());
        else
            return toJS(EmptyListSHA3);
    }
    BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    return "error";
}

bool Test::test_setChainParams(Json::Value const& param1)
{
    try
    {
        Json::FastWriter fastWriter;
        std::string output = fastWriter.write(param1);
        asClientTest(m_eth).setChainParams(output);
        asClientTest(m_eth).completeSync();  // set sync state to idle for mining
    }
    catch (std::exception const&)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INTERNAL_ERROR));
    }

    return true;
}

bool Test::test_mineBlocks(int _number)
{
    try
    {
        asClientTest(m_eth).mineBlocks(_number);
    }
    catch (std::exception const&)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INTERNAL_ERROR));
    }

    return true;
}

bool Test::test_modifyTimestamp(int _timestamp)
{
    // FIXME: Fix year 2038 issue.
    try
    {
        asClientTest(m_eth).modifyTimestamp(_timestamp);
    }
    catch (std::exception const&)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INTERNAL_ERROR));
    }
    return true;
}

bool Test::test_addBlock(std::string const& _rlp)
{
    try
    {
        asClientTest(m_eth).addBlock(_rlp);
    }
    catch (std::exception const&)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INTERNAL_ERROR));
    }
    return true;
}

bool Test::test_rewindToBlock(int _number)
{
    try
    {
        m_eth.rewind(_number);
        asClientTest(m_eth).completeSync();
    }
    catch (std::exception const&)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INTERNAL_ERROR));
    }
    return true;
}
