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

void AccountManager::streamAccountHelp(ostream& _out)
{
	_out
		<< "    account list  List all keys available in wallet." << endl
		<< "    account new	Create a new key and add it to the wallet." << endl
		<< "    account update [<uuid>|<address> , ... ]  Decrypt and re-encrypt given keys." << endl
		<< "    account import [<uuid>|<file>|<secret-hex>]	Import keys from given source and place in wallet." << endl;
}

void AccountManager::streamWalletHelp(ostream& _out)
{
	_out
		<< "    wallet import <file>	Import a presale wallet." << endl;
}

bool AccountManager::execute(int argc, char** argv)
{
	if (string(argv[1]) == "wallet")
	{
		if (3 < argc && string(argv[2]) == "import")
		{
			if (!openWallet())
				return false;
			string file = argv[3];
			string name = "presale wallet";
			string pw;
			try
			{
				KeyPair k = m_keyManager->presaleSecret(
					contentsString(file),
					[&](bool){ return (pw = getPassword("Enter the passphrase for the presale key: "));}
				);
				m_keyManager->import(k.secret(), name, pw, "Same passphrase as used for presale key");
				cout << "  Address: {" << k.address().hex() << "}" << endl;
			}
			catch (Exception const& _e)
			{
				if (auto err = boost::get_error_info<errinfo_comment>(_e))
					cout << "  Decryption failed: " << *err << endl;
				else
					cout << "  Decryption failed: Unknown reason." << endl;
				return false;
			}
		}
		else
			streamWalletHelp(cout);
		return true;
	}
	else if (string(argv[1]) == "account")
	{
		if (argc < 3 || string(argv[2]) == "list")
		{
			openWallet();
			if (m_keyManager->store().keys().empty())
				cout << "No keys found." << endl;
			else
			{
				vector<u128> bare;
				AddressHash got;
				int k = 0;
				for (auto const& u: m_keyManager->store().keys())
				{
					if (Address a = m_keyManager->address(u))
					{
						got.insert(a);
						cout << "Account #" << k << ": {" << a.hex() << "}" << endl;
						k++;
					}
					else
						bare.push_back(u);
				}
				for (auto const& a: m_keyManager->accounts())
					if (!got.count(a))
					{
						cout << "Account #" << k << ": {" << a.hex() << "}" << " (Brain)" << endl;
						k++;
					}
				for (auto const& u: bare)
				{
					cout << "Account #" << k << ": " << toUUID(u) << " (Bare)" << endl;
					k++;
				}
			}
		}
		else if (2 < argc && string(argv[2]) == "new")
		{
			openWallet();
			string name;
			string lock;
			string lockHint;
			lock = createPassword("Enter a passphrase with which to secure this account:");
			auto k = makeKey();
			h128 u = m_keyManager->import(k.secret(), name, lock, lockHint);
			cout << "Created key " << toUUID(u) << endl;
			cout << "  ICAP: " << ICAP(k.address()).encoded() << endl;
			cout << "  Address: {" << k.address().hex() << "}" << endl;
		}
		else if (3 < argc && string(argv[2]) == "import")
		{
			openWallet();
			h128 u = m_keyManager->store().importKey(argv[3]);
			if (!u)
			{
				cerr << "Error: reading key file failed" << endl;
				return false;
			}
			string pw;
			bytesSec s = m_keyManager->store().secret(u, [&](){ return (pw = getPassword("Enter the passphrase for the key: ")); });
			if (s.empty())
			{
				cerr << "Error: couldn't decode key or invalid secret size." << endl;
				return false;
			}
			else
			{
				string lockHint;
				string name;
				m_keyManager->importExisting(u, name, pw, lockHint);
				auto a = m_keyManager->address(u);
				cout << "Imported key " << toUUID(u) << endl;
				cout << "  ICAP: " << ICAP(a).encoded() << endl;
				cout << "  Address: {" << a.hex() << "}" << endl;
			}
		}
		else if (3 < argc && string(argv[2]) == "update")
		{
			openWallet();
			for (int k = 3; k < argc; k++)
			{
				string i = argv[k];
				h128 u = fromUUID(i);
				if (isHex(i) || u != h128())
				{
					string newP = createPassword("Enter the new passphrase for the account " + i);
					auto oldP = [&](){ return getPassword("Enter the current passphrase for the account " + i + ": "); };
					bool recoded = false;
					if (isHex(i))
					{
						recoded = m_keyManager->store().recode(
							Address(i),
							newP,
							oldP,
							dev::KDF::Scrypt
						);
					}
					else if (u != h128())
					{
						recoded = m_keyManager->store().recode(
							u,
							newP,
							oldP,
							dev::KDF::Scrypt
						);
					}
					if (recoded)
						cerr << "Re-encoded " << i << endl;
					else
						cerr << "Couldn't re-encode " << i << "; key does not exist, corrupt or incorrect passphrase supplied." << endl;
				}
				else
					cerr << "Couldn't re-encode " << i << "; does not represent an address or uuid." << endl;
			}
		}
		else
			streamAccountHelp(cout);
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

KeyPair AccountManager::makeKey() const
{
	bool icap = true;
	KeyPair k(Secret::random());
	while (icap && k.address()[0])
		k = KeyPair(Secret(sha3(k.secret().ref())));
	return k;
}

bool AccountManager::openWallet()
{
	if (!m_keyManager)
	{
		m_keyManager.reset(new KeyManager());
		if (m_keyManager->exists())
		{
			if (m_keyManager->load(std::string()) || m_keyManager->load(getPassword("Please enter your MASTER passphrase: ")))
				return true;
			else
			{
				cerr << "Couldn't open wallet. Please check passphrase." << endl;
				return false;
			}
		}
		else
		{
			cerr << "Couldn't open wallet. Does it exist?" << endl;
			return false;
		}
	}
	return true;
}
