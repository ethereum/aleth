#include "Debug.h"
#include "JsonHelper.h"
#include <jsonrpccpp/common/exception.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/CommonJS.h>
#include <libethcore/CommonJS.h>
#include <libethereum/Client.h>
#include <libethereum/Executive.h>
#include <libethereum/StandardTrace.h>
using namespace std;
using namespace dev;
using namespace dev::rpc;
using namespace dev::eth;

Debug::Debug(eth::Client const& _eth):
    m_eth(_eth)
{}

StandardTrace::DebugOptions dev::eth::debugOptions(Json::Value const& _json)
{
    StandardTrace::DebugOptions op;
    if (!_json.isObject() || _json.empty())
        return op;
    if (!_json["disableStorage"].empty())
        op.disableStorage = _json["disableStorage"].asBool();
    if (!_json["disableMemory"].empty())
        op.disableMemory = _json["disableMemory"].asBool();
    if (!_json["disableStack"].empty())
        op.disableStack =_json["disableStack"].asBool();
    if (!_json["fullStorage"].empty())
        op.fullStorage = _json["fullStorage"].asBool();
    return op;
}

h256 Debug::blockHash(string const& _blockNumberOrHash) const
{
    if (isHash<h256>(_blockNumberOrHash))
        return h256(_blockNumberOrHash.substr(_blockNumberOrHash.size() - 64, 64));
    try
    {
        return m_eth.blockChain().numberHash(stoul(_blockNumberOrHash));
    }
    catch (...)
    {
        throw jsonrpc::JsonRpcException("Invalid argument");
    }
}

State Debug::stateAt(std::string const& _blockHashOrNumber, int _txIndex) const
{
    if (_txIndex < 0)
        throw jsonrpc::JsonRpcException("Negative index");

    Block block = m_eth.block(blockHash(_blockHashOrNumber));
    auto const txCount = block.pending().size();

    State state(State::Null);
    if (static_cast<size_t>(_txIndex) < txCount)
        createIntermediateState(state, block, _txIndex, m_eth.blockChain());
    else if (static_cast<size_t>(_txIndex) == txCount)
        // the final state of block (after applying rewards)
        state = block.state();
    else
        throw jsonrpc::JsonRpcException("Transaction index " + toString(_txIndex) + " out of range (" + toString(txCount) + ") for block " + _blockHashOrNumber);

    return state;
}

Json::Value Debug::traceTransaction(Executive& _e, Transaction const& _t, Json::Value const& _json)
{
    Json::Value traceJson{Json::arrayValue};
    StandardTrace st{traceJson};
    st.setShowMnemonics();
    st.setOptions(debugOptions(_json));
    _e.initialize(_t);
    if (!_e.execute())
        _e.go(st.onOp());
    _e.finalize();
    return traceJson;
}

Json::Value Debug::traceBlock(Block const& _block, Json::Value const& _json)
{
    State s(_block.state());
    s.setRoot(_block.stateRootBeforeTx(0));

    Json::Value traces(Json::arrayValue);
    for (unsigned k = 0; k < _block.pending().size(); k++)
    {
        Transaction t = _block.pending()[k];

        u256 const gasUsed = k ? _block.receipt(k - 1).cumulativeGasUsed() : 0;
        auto const& bc = m_eth.blockChain();
        EnvInfo envInfo(_block.info(), bc.lastBlockHashes(), gasUsed, bc.chainID());
        Executive e(s, envInfo, *bc.sealEngine());

        eth::ExecutionResult er;
        e.setResultRecipient(er);
        traces.append(traceTransaction(e, t, _json));
    }
    return traces;
}

Json::Value Debug::debug_traceTransaction(string const& _txHash, Json::Value const& _json)
{
    Json::Value ret;
    try
    {
        LocalisedTransaction t = m_eth.localisedTransaction(h256(_txHash));
        Block block = m_eth.block(t.blockHash());
        State s(State::Null);
        eth::ExecutionResult er;
        Executive e(s, block, t.transactionIndex(), m_eth.blockChain());
        e.setResultRecipient(er);
        Json::Value trace = traceTransaction(e, t, _json);
        ret["gas"] = toJS(t.gas());
        ret["return"] = toHexPrefixed(er.output);
        ret["structLogs"] = trace;
    }
    catch(Exception const& _e)
    {
        cwarn << diagnostic_information(_e);
    }
    return ret;
}

Json::Value Debug::debug_traceBlock(string const& _blockRLP, Json::Value const& _json)
{
    bytes bytes = fromHex(_blockRLP);
    BlockHeader blockHeader(bytes);
    return debug_traceBlockByHash(blockHeader.hash().hex(), _json);
}

Json::Value Debug::debug_traceBlockByHash(string const& _blockHash, Json::Value const& _json)
{
    Json::Value ret;
    Block block = m_eth.block(h256(_blockHash));
    ret["structLogs"] = traceBlock(block, _json);
    return ret;
}

Json::Value Debug::debug_traceBlockByNumber(int _blockNumber, Json::Value const& _json)
{
    Json::Value ret;
    Block block = m_eth.block(blockHash(std::to_string(_blockNumber)));
    ret["structLogs"] = traceBlock(block, _json);
    return ret;
}

Json::Value Debug::debug_accountRange(
    string const& _blockHashOrNumber, int _txIndex, string const& _addressHash, int _maxResults)
{
    Json::Value ret(Json::objectValue);

    if (_maxResults <= 0)
        throw jsonrpc::JsonRpcException("Nonpositive maxResults");

    try
    {
        State const state = stateAt(_blockHashOrNumber, _txIndex);

        auto const addressMap = state.addresses(h256(_addressHash), _maxResults);

        Json::Value addressList(Json::objectValue);
        for (auto const& record : addressMap.first)
            addressList[toHexPrefixed(record.first)] = toHexPrefixed(record.second);

        ret["addressMap"] = addressList;
        ret["nextKey"] = toHexPrefixed(addressMap.second);
    }
    catch (Exception const& _e)
    {
        cwarn << diagnostic_information(_e);
        throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_RPC_INVALID_PARAMS);
    }

    return ret;
}

Json::Value Debug::debug_storageRangeAt(string const& _blockHashOrNumber, int _txIndex, string const& _address, string const& _begin, int _maxResults)
{
    Json::Value ret(Json::objectValue);
    ret["complete"] = true;
    ret["storage"] = Json::Value(Json::objectValue);

    if (_maxResults <= 0)
        throw jsonrpc::JsonRpcException("Nonpositive maxResults");

    try
    {
        State const state = stateAt(_blockHashOrNumber, _txIndex);

        map<h256, pair<u256, u256>> const storage(state.storage(Address(_address)));

        // begin is inclusive
        auto itBegin = storage.lower_bound(h256fromHex(_begin));
        for (auto it = itBegin; it != storage.end(); ++it)
        {
            if (ret["storage"].size() == static_cast<unsigned>(_maxResults))
            {
                ret["nextKey"] = toCompactHexPrefixed(it->first, 1);
                break;
            }

            Json::Value keyValue(Json::objectValue);
            std::string hashedKey = toCompactHexPrefixed(it->first, 1);
            keyValue["key"] = toCompactHexPrefixed(it->second.first, 1);
            keyValue["value"] = toCompactHexPrefixed(it->second.second, 1);

            ret["storage"][hashedKey] = keyValue;
        }
    }
    catch (Exception const& _e)
    {
        cwarn << diagnostic_information(_e);
        throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_RPC_INVALID_PARAMS);
    }

    return ret;
}

std::string Debug::debug_preimage(std::string const& _hashedKey)
{
    h256 const hashedKey(h256fromHex(_hashedKey));
    bytes const key = m_eth.stateDB().lookupAux(hashedKey);

    return key.empty() ? std::string() : toHexPrefixed(key);
}

Json::Value Debug::debug_traceCall(Json::Value const& _call, std::string const& _blockNumber, Json::Value const& _options)
{
    Json::Value ret;
    try
    {
        Block temp = m_eth.blockByNumber(jsToBlockNumber(_blockNumber));
        TransactionSkeleton ts = toTransactionSkeleton(_call);
        if (!ts.from) {
            ts.from = Address();
        }
        u256 nonce = temp.transactionsFrom(ts.from);
        u256 gas = ts.gas == Invalid256 ? m_eth.gasLimitRemaining() : ts.gas;
        u256 gasPrice = ts.gasPrice == Invalid256 ? m_eth.gasBidPrice() : ts.gasPrice;
        temp.mutableState().addBalance(ts.from, gas * gasPrice + ts.value);
        Transaction transaction(ts.value, gasPrice, gas, ts.to, ts.data, nonce);
        transaction.forceSender(ts.from);
        eth::ExecutionResult er;
        Executive e(temp, m_eth.blockChain().lastBlockHashes());
        e.setResultRecipient(er);
        Json::Value trace = traceTransaction(e, transaction, _options);
        ret["gas"] = toJS(transaction.gas());
        ret["return"] = toHexPrefixed(er.output);
        ret["structLogs"] = trace;
    }
    catch(Exception const& _e)
    {
        cwarn << diagnostic_information(_e);
    }
    return ret;
}
