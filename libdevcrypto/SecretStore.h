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
/** @file SecretStore.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <functional>
#include <mutex>
#include <libdevcore/FixedHash.h>
#include <libdevcore/FileSystem.h>
#include "Common.h"

namespace dev
{

enum class KDF {
	PBKDF2_SHA256,
	Scrypt,
};

/**
 * Manages encrypted keys stored in a certain directory on disk. The keys are read into memory
 * and changes to the keys are automatically synced to the directory.
 * Each file stores exactly one key in a specific JSON format whose file name is derived from the
 * UUID of the key.
 * @note that most of the functions here affect the filesystem and throw exceptions on failure,
 * and they also throw exceptions upon rare malfunction in the cryptographic functions.
 */
class SecretStore
{
public:
	struct EncryptedKey
	{
		std::string encryptedKey;
		std::string filename;
		Address address;
	};

	/// Construct a new SecretStore but don't read any keys yet.
	/// Call setPath in
	SecretStore() = default;

	/// Construct a new SecretStore and read all keys in the given directory.
	SecretStore(std::string const& _path);

	/// Set a path for finding secrets.
	void setPath(std::string const& _path);

	/// @returns the secret key stored by the given @a _uuid.
	/// @param _pass function that returns the password for the key.
	/// @param _useCache if true, allow previously decrypted keys to be returned directly.
	bytesSec secret(h128 const& _uuid, std::function<std::string()> const& _pass, bool _useCache = true) const;
	/// @returns the secret key stored by the given @a _uuid.
	/// @param _pass function that returns the password for the key.
	static bytesSec secret(std::string const& _content, std::string const& _pass);
	/// @returns the secret key stored by the given @a _address.
	/// @param _pass function that returns the password for the key.
	bytesSec secret(Address const& _address, std::function<std::string()> const& _pass) const;
	/// Imports the (encrypted) key stored in the file @a _file and copies it to the managed directory.
	h128 importKey(std::string const& _file) { auto ret = readKey(_file, false); if (ret) save(); return ret; }
	/// Imports the (encrypted) key contained in the json formatted @a _content and stores it in
	/// the managed directory.
	h128 importKeyContent(std::string const& _content) { auto ret = readKeyContent(_content, std::string()); if (ret) save(); return ret; }
	/// Imports the decrypted key given by @a _s and stores it, encrypted with
	/// (a key derived from) the password @a _pass.
	h128 importSecret(bytesSec const& _s, std::string const& _pass);
	h128 importSecret(bytesConstRef _s, std::string const& _pass);
	/// Decrypts and re-encrypts the key identified by @a _uuid.
	bool recode(h128 const& _uuid, std::string const& _newPass, std::function<std::string()> const& _pass, KDF _kdf = KDF::Scrypt);
	/// Decrypts and re-encrypts the key identified by @a _address.
	bool recode(Address const& _address, std::string const& _newPass, std::function<std::string()> const& _pass, KDF _kdf = KDF::Scrypt);
	/// Removes the key specified by @a _uuid from both memory and disk.
	void kill(h128 const& _uuid);

	/// Returns the uuids of all stored keys.
	std::vector<h128> keys() const { return keysOf(m_keys); }

	/// @returns true iff we have the given key stored.
	bool contains(h128 const& _k) const { return m_keys.count(_k); }

	/// Clears all cached decrypted keys. The passwords have to be supplied in order to retrieve
	/// secrets again after calling this function.
	void clearCache() const;

	/// Import the key from the file @a _file, but do not copy it to the managed directory yet.
	/// @param _takeFileOwnership if true, deletes the file if it is not the canonical file for the
	/// key (derived from its uuid).
	h128 readKey(std::string const& _file, bool _takeFileOwnership);
	/// Import the key contained in the json-encoded @a _content, but do not store it in the
	/// managed directory.
	/// @param _file if given, assume this file contains @a _content and delete it later, if it is
	/// not the canonical file for the key (derived from the uuid).
	h128 readKeyContent(std::string const& _content, std::string const& _file = std::string());

	/// Store all keys in the directory @a _keysPath.
	void save(std::string const& _keysPath);
	/// Store all keys in the managed directory.
	void save() { save(m_path); }
	/// @returns true if the current file @arg _uuid contains an empty address. m_keys will be updated with the given @arg _address.
	bool noteAddress(h128 const& _uuid, Address const& _address);
	/// @returns the address of the given key or the zero address if it is unknown.
	Address address(h128 const& _uuid) const { return m_keys.at(_uuid).address; }

	/// @returns the default path for the managed directory.
	static std::string defaultPath() { return getDataDir("web3") + "/keys"; }

private:
	/// Loads all keys in the given directory.
	void load(std::string const& _keysPath);
	void load() { load(m_path); }
	/// Encrypts @a _v with a key derived from @a _pass or the empty string on error.
	static std::string encrypt(bytesConstRef _v, std::string const& _pass, KDF _kdf = KDF::Scrypt);
	/// Decrypts @a _v with a key derived from @a _pass or the empty byte array on error.
	static bytesSec decrypt(std::string const& _v, std::string const& _pass);
	/// @returns the key given the @a _address.
	std::pair<h128 const, EncryptedKey> const* key(Address const& _address) const;
	std::pair<h128 const, EncryptedKey>* key(Address const& _address);
	/// Stores decrypted keys by uuid.
	mutable std::unordered_map<h128, bytesSec> m_cached;
	/// Stores encrypted keys together with the file they were loaded from by uuid.
	std::unordered_map<h128, EncryptedKey> m_keys;

	std::string m_path;
};

}

