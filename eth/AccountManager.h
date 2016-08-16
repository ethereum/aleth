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
/** @file AccountManager.h
 * @author Yann <yann@ethdev.com>
 * @date 2016
 */
#pragma once
#include <libethcore/KeyManager.h>

/**
 * Add account management functionnalities to CLI.
 * account list
 * account new
 * account update
 * account import
 * wallet import (import presale wallet)
 */
class AccountManager
{
public:
	/// uses @a argc, @a argv provided by the CLI and executes implemented options.
	bool execute(int argc, char** argv);
	/// stream account help section
	void static streamAccountHelp(std::ostream& _out);
	/// stream wallet help section
	void static streamWalletHelp(std::ostream& _out);

private:
	/// ask end user to create a password.
	std::string createPassword(std::string const& _prompt) const;
	/// creates a ramdom secret/address pair. It uses ICAP.
	dev::KeyPair makeKey() const;
	/// instanciate KeyManager and open the wallet.
	bool openWallet();

	std::unique_ptr<dev::eth::KeyManager> m_keyManager;
};


