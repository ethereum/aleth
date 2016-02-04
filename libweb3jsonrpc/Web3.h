#pragma once
#include "Web3Face.h"

namespace dev
{
namespace rpc
{

class Web3: public Web3Face
{
public:
	Web3(std::string _clientVersion = "C++ (ethereum-cpp)"): m_clientVersion(_clientVersion) {}
	virtual std::pair<std::string, std::string> implementedModule() const override
	{
		return std::make_pair(std::string("web3"), std::string("1.0"));
	}
	virtual std::string web3_sha3(std::string const& _param1) override;
	virtual std::string web3_clientVersion() override { return m_clientVersion; }
	
private:
	std::string m_clientVersion;
};

}
}
