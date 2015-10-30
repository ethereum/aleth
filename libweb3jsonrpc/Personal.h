#pragma once
#include "PersonalFace.h"

namespace dev
{
	
namespace eth
{
class KeyManager;
}

namespace rpc
{

class Personal: public dev::rpc::PersonalFace
{
public:
	Personal(dev::eth::KeyManager& _keyManager);
	virtual std::string personal_newAccount(std::string const& _password) override;
	virtual bool personal_unlockAccount(std::string const& _address, std::string const& _password, int _duration) override;

private:
	dev::eth::KeyManager& m_keyManager;
};

}
}