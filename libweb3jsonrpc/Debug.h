#pragma once
#include "DebugFace.h"
#include <libethereum/StandardTrace.h>

namespace dev
{

namespace eth
{
class Block;
class Client;
class Executive;
class State;

StandardTrace::DebugOptions debugOptions(Json::Value const& _json);
}

namespace rpc
{

class SessionManager;

class Debug: public DebugFace
{
public:
	explicit Debug(eth::Client const& _eth);

	virtual RPCModules implementedModules() const override
	{
		return RPCModules{RPCModule{"debug", "1.0"}};
	}

    virtual Json::Value debug_accountRange(std::string const& _blockHashOrNumber, int _txIndex,
        std::string const& _addressHash, int _maxResults) override;
    virtual Json::Value debug_traceTransaction(std::string const& _txHash, Json::Value const& _json) override;
	virtual Json::Value debug_traceCall(Json::Value const& _call, std::string const& _blockNumber, Json::Value const& _options) override;
	virtual Json::Value debug_traceBlockByNumber(int _blockNumber, Json::Value const& _json) override;
	virtual Json::Value debug_traceBlockByHash(std::string const& _blockHash, Json::Value const& _json) override;
	virtual Json::Value debug_storageRangeAt(std::string const& _blockHashOrNumber, int _txIndex, std::string const& _address, std::string const& _begin, int _maxResults) override;
	virtual std::string debug_preimage(std::string const& _hashedKey) override;
	virtual Json::Value debug_traceBlock(std::string const& _blockRlp, Json::Value const& _json);

private:

	eth::Client const& m_eth;
	h256 blockHash(std::string const& _blockHashOrNumber) const;
    eth::State stateAt(std::string const& _blockHashOrNumber, int _txIndex) const;
    Json::Value traceTransaction(dev::eth::Executive& _e, dev::eth::Transaction const& _t, Json::Value const& _json);
	Json::Value traceBlock(dev::eth::Block const& _block, Json::Value const& _json);
};

}
}
