#include <libethcore/KeyManager.h>
#include <libethcore/CommonJS.h>
#include "Personal.h"

using namespace std;
using namespace dev;
using namespace dev::rpc;
using namespace dev::eth;

Personal::Personal(dev::eth::KeyManager& _keyManager): m_keyManager(_keyManager) {}

std::string Personal::personal_newAccount(std::string const& _password)
{
	KeyPair p = KeyManager::newKeyPair(KeyManager::NewKeyType::DirectICAP);
	m_keyManager.import(p.secret(), std::string(), _password, std::string());
	return toJS(p.address());
}

bool Personal::personal_unlockAccount(std::string const& _address, std::string const& _password, int _duration)
{
	(void)_duration;
	Address address(fromHex(_address));

	if (!m_keyManager.hasAccount(address))
		return false;

	m_keyManager.notePassword(_password);

	try
	{
		m_keyManager.secret(address);
	}
	catch (PasswordUnknown const&)
	{
		return false;
	}

	return true;
}
