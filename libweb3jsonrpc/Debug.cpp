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

Json::Value Debug::debug_trace(string const& _blockNumberOrHash, int _txIndex)
{
	Json::Value ret;
	if (_txIndex < 0)
		throw jsonrpc::JsonRpcException("Negative index");
	Block block = m_eth.block(blockHash(_blockNumberOrHash));
	if ((unsigned)_txIndex < block.pending().size())
	{
		Transaction t = block.pending()[_txIndex];
		State s(State::Null);
		Executive e(s, block, _txIndex, m_eth.blockChain());
		try
		{
			StandardTrace st;
			st.setShowMnemonics();
			e.initialize(t);
			if (!e.execute())
				e.go(st.onOp());
			e.finalize();
			Json::Reader().parse(st.json(), ret);
		}
		catch(Exception const& _e)
		{
			cwarn << diagnostic_information(_e);
		}
	}
	return ret;
}

Json::Value Debug::debug_storageAt(string const& _blockNumberOrHash, int _txIndex, string const& _address)
{
	Json::Value ret(Json::objectValue);

	if (_txIndex < 0)
		throw jsonrpc::JsonRpcException("Negative index");
	Block block = m_eth.block(blockHash(_blockNumberOrHash));
	if ((unsigned)_txIndex < block.pending().size())
	{
		State state = block.fromPending(_txIndex);
		for (auto const& i: state.storage(Address(_address)))
			ret["0x" + toHex(toCompactBigEndian(i.first, 1))] = "0x" + toHex(toCompactBigEndian(i.second, 1));
	}
	return ret;
}
