#pragma once
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

	virtual Json::Value debug_trace(std::string const& _blockNumberOrHash, int _txIndex) override;

private:
	eth::Client& m_eth;
	h256 blockHash(std::string const& _blockNumberOrHash) const;
};

}
}
