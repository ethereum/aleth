/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file AccountManager.cpp
 * @author Yann <yann@ethdev.com>
 * @date 2016
 */

#include <libdevcore/SHA3.h>
#include <libdevcore/FileSystem.h>
#include <libethcore/ICAP.h>
#include <libethcore/KeyManager.h>
#include "AccountManager.h"
using namespace std;
using namespace dev;
using namespace dev::eth;

bool AccountManager::execute(int argc, char** argv)
{
	if ((string)argv[1] == "wallet")
	{
		if (4 < argc && (string)argv[2] == "import")
		{
			keyManager();
			string file =  argv[3];
			string name = argv[4];
			std::string pw;
			KeyPair k = m_keyManager->presaleSecret(contentsString(file), [&](bool){ return (pw = getPassword("Enter the passphrase for the presale key: ")); });
			m_keyManager->import(k.secret(), name, pw, "Same passphrase as used for presale key");
		}
		else
			cout
				<< "	wallet import <file> <name>	Import a presale wallet into a key with the given name." << endl;

		m_keyManager.reset();
		return true;
	}
	else if ((string)argv[1] == "account")
	{
		if (2 < argc && (string)argv[2] == "list")
		{
			keyManager();
			if (m_keyManager->store().keys().empty())
				cout << "No keys found." << endl;
			else
			{
				vector<u128> bare;
				AddressHash got;
				for (auto const& u: m_keyManager->store().keys())
					if (Address a = m_keyManager->address(u))
					{
						got.insert(a);
						cout << toUUID(u) << " " << a.abridged();
						string s = ICAP(a).encoded();
						cout << " " << s << string(35 - s.size(), ' ');
						cout << " " << m_keyManager->accountName(a) << endl;
					}
					else
						bare.push_back(u);
				for (auto const& a: m_keyManager->accounts())
					if (!got.count(a))
					{
						cout << "               (Brain)               " << a.abridged();
						cout << " " << ICAP(a).encoded() << " ";
						cout << " " << m_keyManager->accountName(a) << endl;
					}
				for (auto const& u: bare)
					cout << toUUID(u) << " (Bare)" << endl;
			}
		}
		else if (3 < argc && (string)argv[2] == "new")
		{
			keyManager();
			string name = argv[3];
			string lock;
			string lockHint;
			tie(lock, lockHint) = createPassword("Enter a passphrase with which to secure this account (or nothing to use the master passphrase): ", lock, lockHint);
			auto k = makeKey();
			bool usesMaster = lock.empty();
			h128 u = usesMaster ? m_keyManager->import(k.secret(), name) : m_keyManager->import(k.secret(), name, lock, lockHint);
			cout << "Created key " << toUUID(u) << endl;
			cout << "  Name: " << name << endl;
			if (usesMaster)
				cout << "  Uses master passphrase." << endl;
			else
				cout << "  Password hint: " << lockHint << endl;
			cout << "  ICAP: " << ICAP(k.address()).encoded() << endl;
			cout << "  Raw hex: " << k.address().hex() << endl;
		}
		else if (3 < argc && (string)argv[2] == "import")
		{
			keyManager();
			for (int k = 3; k < argc; k++)
			{
				string input = argv[k];
				h128 u;
				bytesSec b;
				b.writable() = fromHex(input);
				if (b.size() != 32)
				{
					std::string s = contentsString(input);
					b.writable() = fromHex(s);
					if (b.size() != 32)
						u = m_keyManager->store().importKey(input);
				}
				if (!u && b.size() == 32)
					u = m_keyManager->store().importSecret(b, createPassword("Enter a passphrase with which to secure account " + toAddress(Secret(b)).abridged() + ": "));
				if (!u)
				{
					cerr << "Cannot import " << input << " not a file or secret." << endl;
					continue;
				}
				cout << "Successfully imported " << input << " as " << toUUID(u);
			}
		}
		else if (3 < argc && (string)argv[2] == "update")
		{
			keyManager();
			for (int k = 3; k < argc; k++)
			{
				string i = argv[k];
				if (h128 u = fromUUID(i))
					if (m_keyManager->store().recode(u, createPassword("Enter the new passphrase for key " + toUUID(u)), [&](){ return getPassword("Enter the current passphrase for key " + toUUID(u) + ": "); }, kdf()))
						cerr << "Re-encoded " << toUUID(u) << endl;
					else
						cerr << "Couldn't re-encode " << toUUID(u) << "; key corrupt or incorrect passphrase supplied." << endl;
				else
					cerr << "Couldn't re-encode " << i << "; not found." << endl;
			}
		}
		else
		{
			cout
				<< "	account list  List all keys available in wallet." << endl
				<< "	account new <name>  Create a new key with given name and add it in the wallet." << endl
				<< "	account update [ <uuid>|<file> , ... ]  Decrypt and re-encrypt given keys." << endl
				<< "	account import [ <file>|<secret-hex> , ... ] Import keys from given sources." << endl;
		}
		m_keyManager.reset();
		return true;
	}
	else
		return false;
}

string AccountManager::createPassword(string const& _prompt) const
{
	string ret;
	while (true)
	{
		ret = getPassword(_prompt);
		string confirm = getPassword("Please confirm the passphrase by entering it again: ");
		if (ret == confirm)
			break;
		cout << "Passwords were different. Try again." << endl;
	}
	return ret;
}

pair<string, string> AccountManager::createPassword(std::string const& _prompt, std::string const& _pass, std::string const& _hint) const
{
	string pass = _pass;
	if (pass.empty())
		while (true)
		{
			pass = getPassword(_prompt);
			string confirm = getPassword("Please confirm the passphrase by entering it again: ");
			if (pass == confirm)
				break;
			cout << "Passwords were different. Try again." << endl;
		}
	string hint = _hint;
	if (hint.empty() && !pass.empty() && !m_keyManager->haveHint(pass))
	{
		cout << "Enter a hint to help you remember this passphrase: " << flush;
		getline(cin, hint);
	}
	return make_pair(pass, hint);
}

KeyPair AccountManager::makeKey() const
{
	bool icap = true;
	KeyPair k(Secret::random());
	while (icap && k.address()[0])
		k = KeyPair(Secret(sha3(k.secret().ref())));
	return k;
}

KeyManager& AccountManager::keyManager()
{
	if (!m_keyManager)
	{
		m_keyManager.reset(new KeyManager());
		if (m_keyManager->exists())
			openWallet();
		else
			cerr << "Couldn't open wallet. Does it exist?" << endl;
	}
	return *m_keyManager;
}

void AccountManager::openWallet()
{
	string password;
	while (true)
	{
		if (m_keyManager->load(password))
			break;
		if (!password.empty())
		{
			cout << "Password invalid. Try again." << endl;
			password.clear();
		}
		password = getPassword("Please enter your MASTER passphrase: ");
	}
}
