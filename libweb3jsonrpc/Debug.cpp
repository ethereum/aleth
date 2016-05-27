#include <jsonrpccpp/common/exception.h>
#include <libdevcore/CommonJS.h>
#include <libethereum/Client.h>
#include <libethereum/Executive.h>
#include "Debug.h"
#include "JsonHelper.h"
using namespace std;
using namespace dev;
using namespace dev::rpc;
using namespace dev::eth;

Debug::Debug(eth::Client& _eth):
	m_eth(_eth)
{}

StandardTrace::DebugOptions debugOptions(Json::Value const& _json)
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

Json::Value Debug::traceTransaction(Executive& _e, Transaction const& _t, Json::Value const& _json)
{
	Json::Value trace;
	StandardTrace st;
	st.setShowMnemonics();
	st.setOptions(debugOptions(_json));
	_e.initialize(_t);
	if (!_e.execute())
		_e.go(st.onOp());
	_e.finalize();
	Json::Reader().parse(st.json(), trace);
	return trace;
}

Json::Value Debug::traceBlock(Block const& _block, Json::Value const& _json)
{
	Json::Value traces(Json::arrayValue);
	for (unsigned k = 0; k < _block.pending().size(); k++)
	{
		Transaction t = _block.pending()[k];
		State s(State::Null);
		eth::ExecutionResult er;
		Executive e(s, _block, k, m_eth.blockChain());
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
		ret["gas"] = toHex(t.gas(), HexPrefix::Add);
		ret["return"] = toHex(er.output, 2, HexPrefix::Add);
		ret["structLogs"] = trace;
	}
	catch(Exception const& _e)
	{
		cwarn << diagnostic_information(_e);
	}
	return ret;
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

Json::Value Debug::debug_storageAt(string const& _blockHashOrNumber, int _txIndex, string const& _address)
{
	Json::Value ret(Json::objectValue);

	if (_txIndex < 0)
		throw jsonrpc::JsonRpcException("Negative index");
	Block block = m_eth.block(blockHash(_blockHashOrNumber));

	unsigned i = ((unsigned)_txIndex < block.pending().size()) ? (unsigned)_txIndex : block.pending().size();
	State state = block.fromPending(i);

	for (auto const& i: state.storage(Address(_address)))
		ret[toHex(toCompactBigEndian(i.first, 1), 2, HexPrefix::Add)] = toHex(toCompactBigEndian(i.second, 1), 2, HexPrefix::Add);
	return ret;
}
