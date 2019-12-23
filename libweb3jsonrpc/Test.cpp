// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2016-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#include "Test.h"
#include <jsonrpccpp/common/errors.h>
#include <jsonrpccpp/common/exception.h>
#include <libdevcore/CommonJS.h>
#include <libethcore/Exceptions.h>
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
string importResultToErrorMessage(ImportResult _res)
{
    switch (_res)
    {
    case ImportResult::AlreadyInChain:
        return "AlreadyInChain";
    case ImportResult::AlreadyKnown:
        return "AlreadyKnown";
    case ImportResult::BadChain:
        return "BadChain";
    case ImportResult::FutureTimeKnown:
        return "FutureTimeKnown";
    case ImportResult::FutureTimeUnknown:
        return "FutureTimeUnknown";
    case ImportResult::Malformed:
        return "Malformed";
    case ImportResult::OverbidGasPrice:
        return "OverbidGasPrice";
    case ImportResult::Success:
        return "Success";
    case ImportResult::UnknownParent:
        return "UnknownParent";
    case ImportResult::ZeroSignature:
        return "ZeroSignature";
    default:
        return "ImportResult unhandled case";
    }
}

string exceptionToErrorMessage(boost::exception_ptr _e)
{
    string ret;
    try
    {
        boost::rethrow_exception(_e);
    }
    catch (ExtraDataTooBig const&)
    {
        ret = "ExtraData too big.";
    }
    catch (InvalidDifficulty const&)
    {
        ret = "Invalid Difficulty.";
    }
    catch (InvalidGasLimit const&)
    {
        ret = "Invalid Block GasLimit.";
    }
    catch (BlockGasLimitReached const&)
    {
        ret = "Block GasLimit reached.";
    }
    catch (TooMuchGasUsed const&)
    {
        ret = "Too much gas used.";
    }
    catch (InvalidNumber const&)
    {
        ret = "Invalid number.";
    }
    catch (InvalidLogBloom const&)
    {
        ret = "Invalid log bloom.";
    }
    catch (InvalidTimestamp const&)
    {
        ret = "Invalid timestamp.";
    }
    catch (InvalidBlockNonce const&)
    {
        ret = "Invalid block nonce.";
    }
    catch (UnknownParent const&)
    {
        ret = "Unknown parent.";
    }
    catch (InvalidUnclesHash const&)
    {
        ret = "Invalid uncles hash.";
    }
    catch (InvalidTransactionsRoot const&)
    {
        ret = "Invalid transactions root.";
    }
    catch (InvalidStateRoot const&)
    {
        ret = "Invalid state root.";
    }
    catch (InvalidGasUsed const&)
    {
        ret = "Invalid gas used.";
    }
    catch (InvalidReceiptsStateRoot const&)
    {
        ret = "Invalid receipts state root.";
    }
    catch (InvalidParentHash const&)
    {
        ret = "Invalid parent hash.";
    }
    catch (...)
    {
        ret = "Unknown error.";
    }
    return ret;
}


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
        return true;
    }
    catch (std::exception const& ex)
    {
        cwarn << ex.what();
        throw JsonRpcException(Errors::ERROR_RPC_INTERNAL_ERROR, ex.what());
    }
}

bool Test::test_mineBlocks(int _number)
{
    if (!asClientTest(m_eth).mineBlocks(_number))  // Synchronous
        throw JsonRpcException("Mining failed.");
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
    catch (ImportBlockFailed const& e)
    {
        cwarn << diagnostic_information(e);

        string detailedError;
        if (auto nested = boost::get_error_info<errinfo_nestedException>(e))
            detailedError = exceptionToErrorMessage(*nested);
        else if (auto importResult = boost::get_error_info<errinfo_importResult>(e))
            detailedError = importResultToErrorMessage(*importResult);
        else
            detailedError = "No nested info, no import result info detected on the exception";

        throw JsonRpcException("Block import failed: " + detailedError);
    }
    catch (std::exception const& e)
    {
        cwarn << e.what();
        throw JsonRpcException(Errors::ERROR_RPC_INTERNAL_ERROR);
    }
}
