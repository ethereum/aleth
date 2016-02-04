#pragma once
#include "PersonalFace.h"

namespace dev
{
	
namespace eth
{
class KeyManager;
class AccountHolder;
}

namespace rpc
{

class Personal: public dev::rpc::PersonalFace
{
public:
	Personal(dev::eth::KeyManager& _keyManager, dev::eth::AccountHolder& _accountHolder);
	virtual std::pair<std::string, std::string> implementedModule() const override
	{
		return std::make_pair(std::string("personal"), std::string("1.0"));
	}
	virtual std::string personal_newAccount(std::string const& _password) override;
	virtual bool personal_unlockAccount(std::string const& _address, std::string const& _password, int _duration) override;

private:
	dev::eth::KeyManager& m_keyManager;
	dev::eth::AccountHolder& m_accountHolder;
};

}
}