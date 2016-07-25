#pragma once
#include "PersonalFace.h"

namespace dev
{
	
namespace eth
{
class KeyManager;
class AccountHolder;
class Interface;
}

namespace rpc
{

class Personal: public dev::rpc::PersonalFace
{
public:
	Personal(dev::eth::KeyManager& _keyManager, dev::eth::AccountHolder& _accountHolder, eth::Interface& _eth);
	virtual RPCModules implementedModules() const override
	{
		return RPCModules{RPCModule{"personal", "1.0"}};
	}
	virtual std::string personal_newAccount(std::string const& _password) override;
	virtual bool personal_unlockAccount(std::string const& _address, std::string const& _password, int _duration) override;
	virtual std::string personal_signAndSendTransaction(Json::Value const& _transaction, std::string const& _password) override;
	virtual std::string personal_sendTransaction(Json::Value const& _transaction, std::string const& _password) override;
	virtual Json::Value personal_listAccounts() override;

private:
	dev::eth::KeyManager& m_keyManager;
	dev::eth::AccountHolder& m_accountHolder;
	dev::eth::Interface& m_eth;
};

}
}
