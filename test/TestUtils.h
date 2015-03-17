
#pragma once

#include <functional>
#include <string>
#include <json/json.h>
#include <libethereum/BlockChain.h>
#include <libethereum/InterfaceStub.h>

namespace dev
{
namespace test
{

bytes toBytes(std::string const& _str);

class TestUtils
{
public:
	static dev::eth::State toState(Json::Value const& _json);
};

struct LoadTestFileFixture
{
	LoadTestFileFixture();
	
	static bool m_loaded;
	static Json::Value m_json;
};

struct BlockChainFixture: public LoadTestFileFixture
{
	BlockChainFixture() {}
	void enumerateBlockchains(std::function<void(Json::Value const&, dev::eth::BlockChain&, dev::eth::State state)> callback);
};

struct InterfaceStubFixture: public BlockChainFixture
{
	InterfaceStubFixture() {}
	void enumerateInterfaces(std::function<void(Json::Value const&, dev::eth::InterfaceStub&)> callback);
};

}
}
