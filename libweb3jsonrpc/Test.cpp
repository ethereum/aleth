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
#include <jsonrpccpp/common/errors.h>
#include <jsonrpccpp/common/exception.h>
#include <libdevcore/CommonJS.h>
#include <libethereum/ChainParams.h>
#include <libethereum/ClientTest.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::rpc;
using namespace jsonrpc;

Test::Test(eth::Client& _eth): m_eth(_eth) {}

namespace
{
string logEntriesToLogHash(eth::LogEntries const& _logs)
{
    RLPStream s;
    s.appendList(_logs.size());
    for (eth::LogEntry const& l : _logs)
        l.streamRLP(s);
    return toJS(sha3(s.out()));
}

h256 stringToHash(string const& _hashString)
{
    try
    {
        return h256(_hashString);
    }
    catch (BadHexCharacter const&)
    {
        throw JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS);
    }
}

string queueStatusToString(QueueStatus const& _status)
{
    switch (_status)
    {
    case QueueStatus::Bad:
        return "Bad";
    case QueueStatus::Importing:
        return "Importing";
    case QueueStatus::Ready:
        return "Ready";
    case QueueStatus::Unknown:
        return "Unknown";
    case QueueStatus::UnknownParent:
        return "UnknownParent";
    }
    return "Undefined";
}
}

string Test::test_getLogHash(string const& _txHash)
{
    try
    {
        h256 txHash = stringToHash(_txHash);
        if (m_eth.blockChain().isKnownTransaction(txHash))
        {
            LocalisedTransaction t = m_eth.localisedTransaction(txHash);
            BlockReceipts const& blockReceipts = m_eth.blockChain().receipts(t.blockHash());
            if (blockReceipts.receipts.size() != 0)
                return logEntriesToLogHash(blockReceipts.receipts[t.transactionIndex()].log());
        }
        return toJS(dev::EmptyListSHA3);
    }
    catch (std::exception const& ex)
    {
        cwarn << ex.what();
        throw JsonRpcException(Errors::ERROR_RPC_INTERNAL_ERROR, ex.what());
    }
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
    catch (std::exception const& ex)
    {
        cwarn << ex.what();
        throw JsonRpcException(Errors::ERROR_RPC_INTERNAL_ERROR, ex.what());
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

std::string Test::test_importRawBlock(string const& _blockRLP)
{
    try
    {
        ClientTest& client = asClientTest(m_eth);
        return toJS(client.importRawBlock(_blockRLP));
    }
    catch (std::exception const& ex)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INTERNAL_ERROR, ex.what()));
    }
}

std::string Test::test_getBlockStatus(std::string const& _blockHash)
{
    QueueStatus status;
    ClientTest& client = asClientTest(m_eth);
    if (_blockHash.empty())
    {
        switch (client.getLastBlockStatus())
        {
        case ClientTest::BlockStatus::Idle:
            status = QueueStatus::Ready;
            break;
        case ClientTest::BlockStatus::Mining:
            status = QueueStatus::Importing;
            break;
        case ClientTest::BlockStatus::Error:
            status = QueueStatus::Bad;
            break;
        case ClientTest::BlockStatus::Success:
            status = QueueStatus::Ready;
            break;
        default:
            status = QueueStatus::Unknown;
            break;
        }
    }
    else
        status = client.blockQueue().blockStatus(stringToHash(_blockHash));

    return queueStatusToString(status);
}
