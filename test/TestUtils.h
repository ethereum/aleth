
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

struct LoadTestFileFixture
{
	LoadTestFileFixture();
	
	Json::Value m_json;
};

struct BlockChainFixture: public LoadTestFileFixture
{
	BlockChainFixture() {}
	void enumerateBlockchains(std::function<void(dev::eth::BlockChain&, Json::Value const&)> callback);
};

struct InterfaceStubFixture: public BlockChainFixture
{
	InterfaceStubFixture() {}
	void enumerateInterfaces(std::function<void(dev::eth::InterfaceStub&, Json::Value const&)> callback);
};

}
}
