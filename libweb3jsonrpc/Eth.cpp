// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#include "Eth.h"
#include "AccountHolder.h"
#include <jsonrpccpp/common/exception.h>
#include <libdevcore/CommonData.h>
#include <libethashseal/Ethash.h>
#include <libethcore/CommonJS.h>
#include <libethereum/Client.h>
#include <libweb3jsonrpc/JsonHelper.h>
#include <libwebthree/WebThree.h>
#include <csignal>

using namespace std;
using namespace jsonrpc;
using namespace dev;
using namespace eth;
using namespace shh;
using namespace dev::rpc;

Eth::Eth(eth::Interface& _eth, eth::AccountHolder& _ethAccounts):
	m_eth(_eth),
	m_ethAccounts(_ethAccounts)
{
}

string Eth::eth_protocolVersion()
{
	return toJS(eth::c_protocolVersion);
}

string Eth::eth_coinbase()
{
	return toJS(client()->author());
}

string Eth::eth_hashrate()
{
    try
    {
        return toJS(getEthash().hashrate());
    }
    catch (InvalidSealEngine const&)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
    catch (std::exception const&)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INTERNAL_ERROR));
    }
}

bool Eth::eth_mining()
{
    try
    {
        return getEthash().isMining();
    }
    catch (InvalidSealEngine const&)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
    catch (std::exception const&)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INTERNAL_ERROR));
    }
}

string Eth::eth_gasPrice()
{
	return toJS(client()->gasBidPrice());
}

Json::Value Eth::eth_accounts()
{
	return toJson(m_ethAccounts.allAccounts());
}

string Eth::eth_blockNumber()
{
	return toJS(client()->number());
}


string Eth::eth_getBalance(string const& _address, string const& _blockNumber)
{
	try
	{
		return toJS(client()->balanceAt(jsToAddress(_address), jsToBlockNumber(_blockNumber)));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_getStorageAt(string const& _address, string const& _position, string const& _blockNumber)
{
	try
	{
		return toJS(toCompactBigEndian(client()->stateAt(jsToAddress(_address), jsToU256(_position), jsToBlockNumber(_blockNumber)), 32));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_getStorageRoot(string const& _address, string const& _blockNumber)
{
	try
	{
		return toString(client()->stateRootAt(jsToAddress(_address), jsToBlockNumber(_blockNumber)));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_pendingTransactions()
{
	//Return list of transaction that being sent by local accounts
	Transactions ours;
	for (Transaction const& pending:client()->pending())
	{
		for (Address const& account:m_ethAccounts.allAccounts())
		{
			if (pending.sender() == account)
			{
				ours.push_back(pending);
				break;
			}
		}
	}

	return toJson(ours);
}

string Eth::eth_getTransactionCount(string const& _address, string const& _blockNumber)
{
    try
    {
        return toString(client()->countAt(jsToAddress(_address), jsToBlockNumber(_blockNumber)));
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
    }
}

Json::Value Eth::eth_getBlockTransactionCountByHash(string const& _blockHash)
{
	try
	{
		h256 blockHash = jsToFixed<32>(_blockHash);
		if (!client()->isKnown(blockHash))
			return Json::Value(Json::nullValue);

		return toJS(client()->transactionCount(blockHash));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getBlockTransactionCountByNumber(string const& _blockNumber)
{
	try
	{
		BlockNumber blockNumber = jsToBlockNumber(_blockNumber);
		if (!client()->isKnown(blockNumber))
			return Json::Value(Json::nullValue);

		return toJS(client()->transactionCount(jsToBlockNumber(_blockNumber)));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getUncleCountByBlockHash(string const& _blockHash)
{
	try
	{
		h256 blockHash = jsToFixed<32>(_blockHash);
		if (!client()->isKnown(blockHash))
			return Json::Value(Json::nullValue);

		return toJS(client()->uncleCount(blockHash));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getUncleCountByBlockNumber(string const& _blockNumber)
{
	try
	{
		BlockNumber blockNumber = jsToBlockNumber(_blockNumber);
		if (!client()->isKnown(blockNumber))
			return Json::Value(Json::nullValue);

		return toJS(client()->uncleCount(blockNumber));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_getCode(string const& _address, string const& _blockNumber)
{
	try
	{
		return toJS(client()->codeAt(jsToAddress(_address), jsToBlockNumber(_blockNumber)));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

void Eth::setTransactionDefaults(TransactionSkeleton& _t)
{
	if (!_t.from)
		_t.from = m_ethAccounts.defaultTransactAccount();
}

string Eth::eth_sendTransaction(Json::Value const& _json)
{
	try
	{
		TransactionSkeleton t = toTransactionSkeleton(_json);
		setTransactionDefaults(t);
		pair<bool, Secret> ar = m_ethAccounts.authenticate(t);
		if (!ar.first)
		{
			h256 txHash = client()->submitTransaction(t, ar.second);
			return toJS(txHash);
		}
		else
		{
			m_ethAccounts.queueTransaction(t);
			h256 emptyHash;
			return toJS(emptyHash); // TODO: give back something more useful than an empty hash.
		}
	}
	catch (Exception const&)
	{
		throw JsonRpcException(exceptionToErrorMessage());
	}
}

Json::Value Eth::eth_signTransaction(Json::Value const& _json)
{
	try
	{
		TransactionSkeleton ts = toTransactionSkeleton(_json);
		setTransactionDefaults(ts);
		ts = client()->populateTransactionWithDefaults(ts);
		pair<bool, Secret> ar = m_ethAccounts.authenticate(ts);
		Transaction t(ts, ar.second);
		RLPStream s;
		t.streamRLP(s);
		return toJson(t, s.out());
	}
	catch (Exception const&)
	{
		throw JsonRpcException(exceptionToErrorMessage());
	}
}

Json::Value Eth::eth_inspectTransaction(std::string const& _rlp)
{
	try
	{
		return toJson(Transaction(jsToBytes(_rlp, OnFailed::Throw), CheckTransaction::Everything));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_sendRawTransaction(std::string const& _rlp)
{
	try
	{
		// Don't need to check the transaction signature (CheckTransaction::None) since it will
		// be checked as a part of transaction import
		Transaction t(jsToBytes(_rlp, OnFailed::Throw), CheckTransaction::None);
		return toJS(client()->importTransaction(t));
	}
	catch (Exception const&)
	{
		throw JsonRpcException(exceptionToErrorMessage());
	}
}

string Eth::eth_call(Json::Value const& _json, string const& _blockNumber)
{
	try
	{
		TransactionSkeleton t = toTransactionSkeleton(_json);
		setTransactionDefaults(t);
		ExecutionResult er = client()->call(t.from, t.value, t.to, t.data, t.gas, t.gasPrice, jsToBlockNumber(_blockNumber), FudgeFactor::Lenient);
		return toJS(er.output);
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_estimateGas(Json::Value const& _json)
{
	try
	{
		TransactionSkeleton t = toTransactionSkeleton(_json);
		setTransactionDefaults(t);
		int64_t gas = static_cast<int64_t>(t.gas);
		return toJS(client()->estimateGas(t.from, t.value, t.to, t.data, gas, t.gasPrice, PendingBlock).first);
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

bool Eth::eth_flush()
{
	client()->flushTransactions();
	return true;
}

Json::Value Eth::eth_getBlockByHash(string const& _blockHash, bool _includeTransactions)
{
	try
	{
		h256 const h = jsToFixed<32>(_blockHash);
		if (!client()->isKnown(h))
			return Json::Value(Json::nullValue);

        Json::Value ret;
        auto const blockDetails = client()->blockDetails(h);
        auto const blockHeader = client()->blockInfo(h);
        auto const uncleHashes = client()->uncleHashes(h);
        auto* sealEngine = client()->sealEngine();
        if (_includeTransactions)
            ret = toJson(
                blockHeader, blockDetails, uncleHashes, client()->transactions(h), sealEngine);
        else
            ret = toJson(
                blockHeader, blockDetails, uncleHashes, client()->transactionHashes(h), sealEngine);
        return ret;
    }
    catch (...)
    {
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getBlockByNumber(string const& _blockNumber, bool _includeTransactions)
{
	try
	{
		BlockNumber const h = jsToBlockNumber(_blockNumber);
		if (!client()->isKnown(h))
			return Json::Value(Json::nullValue);

        Json::Value ret;
        auto const blockDetails = client()->blockDetails(h);
        auto const blockHeader = client()->blockInfo(h);
        auto const uncleHashes = client()->uncleHashes(h);
        auto* sealEngine = client()->sealEngine();
        if (_includeTransactions)
            ret = toJson(
                blockHeader, blockDetails, uncleHashes, client()->transactions(h), sealEngine);
        else
            ret = toJson(
                blockHeader, blockDetails, uncleHashes, client()->transactionHashes(h), sealEngine);
        return ret;
    }
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getTransactionByHash(string const& _transactionHash)
{
	try
	{
		h256 h = jsToFixed<32>(_transactionHash);
		if (!client()->isKnownTransaction(h))
			return Json::Value(Json::nullValue);

		return toJson(client()->localisedTransaction(h));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getTransactionByBlockHashAndIndex(string const& _blockHash, string const& _transactionIndex)
{
	try
	{
		h256 bh = jsToFixed<32>(_blockHash);
		unsigned ti = jsToInt(_transactionIndex);
		if (!client()->isKnownTransaction(bh, ti))
			return Json::Value(Json::nullValue);

		return toJson(client()->localisedTransaction(bh, ti));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getTransactionByBlockNumberAndIndex(string const& _blockNumber, string const& _transactionIndex)
{
	try
	{
		BlockNumber bn = jsToBlockNumber(_blockNumber);
		h256 bh = client()->hashFromNumber(bn);
		unsigned ti = jsToInt(_transactionIndex);
		if (!client()->isKnownTransaction(bh, ti))
			return Json::Value(Json::nullValue);

		return toJson(client()->localisedTransaction(bh, ti));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getTransactionReceipt(string const& _transactionHash)
{
	try
	{
		h256 h = jsToFixed<32>(_transactionHash);
		if (!client()->isKnownTransaction(h))
			return Json::Value(Json::nullValue);

		return toJson(client()->localisedTransactionReceipt(h));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getUncleByBlockHashAndIndex(string const& _blockHash, string const& _uncleIndex)
{
	try
	{
		return toJson(client()->uncle(jsToFixed<32>(_blockHash), jsToInt(_uncleIndex)), client()->sealEngine());
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getUncleByBlockNumberAndIndex(string const& _blockNumber, string const& _uncleIndex)
{
	try
	{
		return toJson(client()->uncle(jsToBlockNumber(_blockNumber), jsToInt(_uncleIndex)), client()->sealEngine());
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_newFilter(Json::Value const& _json)
{
	try
	{
		return toJS(client()->installWatch(toLogFilter(_json, *client())));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_newFilterEx(Json::Value const& _json)
{
	try
	{
		return toJS(client()->installWatch(toLogFilter(_json)));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_newBlockFilter()
{
	h256 filter = dev::eth::ChainChangedFilter;
	return toJS(client()->installWatch(filter));
}

string Eth::eth_newPendingTransactionFilter()
{
	h256 filter = dev::eth::PendingChangedFilter;
	return toJS(client()->installWatch(filter));
}

bool Eth::eth_uninstallFilter(string const& _filterId)
{
	try
	{
		return client()->uninstallWatch(jsToInt(_filterId));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getFilterChanges(string const& _filterId)
{
	try
	{
		int id = jsToInt(_filterId);
		auto entries = client()->checkWatch(id);
//		if (entries.size())
//			cnote << "FIRING WATCH" << id << entries.size();
		return toJson(entries);
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getFilterChangesEx(string const& _filterId)
{
	try
	{
		int id = jsToInt(_filterId);
		auto entries = client()->checkWatch(id);
//		if (entries.size())
//			cnote << "FIRING WATCH" << id << entries.size();
		return toJsonByBlock(entries);
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getFilterLogs(string const& _filterId)
{
	try
	{
		return toJson(client()->logs(jsToInt(_filterId)));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getFilterLogsEx(string const& _filterId)
{
	try
	{
		return toJsonByBlock(client()->logs(jsToInt(_filterId)));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getLogs(Json::Value const& _json)
{
	try
	{
		return toJson(client()->logs(toLogFilter(_json, *client())));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getLogsEx(Json::Value const& _json)
{
	try
	{
		return toJsonByBlock(client()->logs(toLogFilter(_json)));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_getWork()
{
	try
	{
		Json::Value ret(Json::arrayValue);
        auto r = client()->getWork();
        ret.append(toJS(get<0>(r)));
		ret.append(toJS(get<1>(r)));
		ret.append(toJS(get<2>(r)));
		return ret;
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_syncing()
{
	dev::eth::SyncStatus sync = client()->syncStatus();
	if (sync.state == SyncState::Idle || !sync.majorSyncing)
		return Json::Value(false);

	Json::Value info(Json::objectValue);
	info["startingBlock"] = sync.startBlockNumber;
	info["highestBlock"] = sync.highestBlockNumber;
	info["currentBlock"] = sync.currentBlockNumber;
	return info;
}

string Eth::eth_chainId()
{
	return toJS(client()->chainId());
}

bool Eth::eth_submitWork(string const& _nonce, string const&, string const& _mixHash)
{
	try
	{
        return getEthash().submitEthashWork(
            jsToFixed<32>(_mixHash), jsToFixed<Nonce::size>(_nonce));
    }
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

bool Eth::eth_submitHashrate(string const& _hashes, string const& _id)
{
	try
    {
        getEthash().submitExternalHashrate(jsToInt<32>(_hashes), jsToFixed<32>(_id));
        return true;
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string Eth::eth_register(string const& _address)
{
	try
	{
		return toJS(m_ethAccounts.addProxyAccount(jsToAddress(_address)));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

bool Eth::eth_unregister(string const& _accountId)
{
	try
	{
		return m_ethAccounts.removeProxyAccount(jsToInt(_accountId));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

Json::Value Eth::eth_fetchQueuedTransactions(string const& _accountId)
{
	try
	{
		auto id = jsToInt(_accountId);
		Json::Value ret(Json::arrayValue);
		// TODO: throw an error on no account with given id
		for (TransactionSkeleton const& t: m_ethAccounts.queuedTransactions(id))
			ret.append(toJson(t));
		m_ethAccounts.clearQueue(id);
		return ret;
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(JsonRpcException(Errors::ERROR_RPC_INVALID_PARAMS));
	}
}

string dev::rpc::exceptionToErrorMessage()
{
	string ret;
	try
	{
		throw;
	}
	// Transaction submission exceptions
	catch (ZeroSignatureTransaction const&)
	{
		ret = "Zero signature transaction.";
	}
	catch (GasPriceTooLow const&)
	{
		ret = "Pending transaction with same nonce but higher gas price exists.";
	}
	catch (OutOfGasIntrinsic const&)
	{
		ret = "Transaction gas amount is less than the intrinsic gas amount for this transaction type.";
	}
    catch (BlockGasLimitReached const& _ex)
    {
        string errorString = "Block gas limit reached! ";
        cwarn << errorString + _ex.what();
        ret = errorString;
    }
	catch (InvalidNonce const&)
	{
		ret = "Invalid transaction nonce.";
	}
	catch (PendingTransactionAlreadyExists const&)
	{
		ret = "Same transaction already exists in the pending transaction queue.";
	}
	catch (TransactionAlreadyInChain const&)
	{
		ret = "Transaction is already in the blockchain.";
	}
	catch (NotEnoughCash const&)
	{
		ret = "Account balance is too low (balance < value + gas * gas price).";
	}
	catch (InvalidSignature const&)
	{
		ret = "Invalid transaction signature.";
	}
	// Acount holder exceptions
	catch (AccountLocked const&)
	{
		ret = "Account is locked.";
	}
	catch (UnknownAccount const&)
	{
		ret = "Unknown account.";
	}
	catch (TransactionRefused const&)
	{
		ret = "Transaction rejected by user.";
	}
	catch (...)
	{
		ret = "Invalid RPC parameters.";
	}
	return ret;
}

Ethash& Eth::getEthash()
{
    return asEthash(*client()->sealEngine());
}
