#include <libethcore/KeyManager.h>
#include <libweb3jsonrpc/AccountHolder.h>
#include <libethcore/CommonJS.h>
#include <libweb3jsonrpc/JsonHelper.h>
#include "Personal.h"

using namespace std;
using namespace dev;
using namespace dev::rpc;
using namespace dev::eth;

Personal::Personal(KeyManager& _keyManager, AccountHolder& _accountHolder):
	m_keyManager(_keyManager),
	m_accountHolder(_accountHolder)
{
}

std::string Personal::personal_newAccount(std::string const& _password)
{
	KeyPair p = KeyManager::newKeyPair(KeyManager::NewKeyType::NoVanity);
	m_keyManager.import(p.secret(), std::string(), _password, std::string());
	return toJS(p.address());
}

bool Personal::personal_unlockAccount(std::string const& _address, std::string const& _password, int _duration)
{
	return m_accountHolder.unlockAccount(Address(fromHex(_address, WhenError::Throw)), _password, _duration);
}

Json::Value Personal::personal_listAccounts()
{
	return toJson(m_keyManager.accounts());
}
