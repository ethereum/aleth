
#pragma once

#include <functional>
#include <string>
#include <boost/filesystem.hpp>
#include <json/json.h>
#include <libethereum/BlockChain.h>
#include <libethereum/InterfaceStub.h>

namespace dev
{
namespace test
{

bytes toBytes(std::string const& _str);


class ShortLivingDirectory
{
public:
	ShortLivingDirectory(std::string const& _path) : m_path(_path) { boost::filesystem::create_directories(m_path); }
	~ShortLivingDirectory() { boost::filesystem::remove_all(m_path); }
	
private:
	std::string m_path;
};

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
