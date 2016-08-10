#pragma once
#include <libethereum/Executive.h>
#include "DebugFace.h"

namespace dev
{

namespace eth
{
class Client;
}

namespace rpc
{

class SessionManager;

class Debug: public DebugFace
{
public:
	explicit Debug(eth::Client& _eth);

	virtual RPCModules implementedModules() const override
	{
		return RPCModules{RPCModule{"debug", "1.0"}};
	}


	virtual Json::Value debug_traceTransaction(std::string const& _txHash, Json::Value const& _json) override;
	virtual Json::Value debug_traceBlockByNumber(int _blockNumber, Json::Value const& _json) override;
	virtual Json::Value debug_traceBlockByHash(std::string const& _blockHash, Json::Value const& _json) override;
	virtual Json::Value debug_storageAt(std::string const& _blockHashOrNumber, int _txIndex, std::string const& _address) override;
	virtual Json::Value debug_traceBlock(std::string const& _blockRlp, Json::Value const& _json);

private:

	eth::Client& m_eth;
	h256 blockHash(std::string const& _blockHashOrNumber) const;
	Json::Value traceTransaction(dev::eth::Executive& _e, dev::eth::Transaction const& _t, Json::Value const& _json);
	Json::Value traceBlock(dev::eth::Block const& _block, Json::Value const& _json);
};

}
}
