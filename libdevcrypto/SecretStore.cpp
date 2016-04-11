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
/** @file SecretStore.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "SecretStore.h"
#include <thread>
#include <mutex>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <libdevcore/Log.h>
#include <libdevcore/Guards.h>
#include <libdevcore/SHA3.h>
#include <libdevcore/FileSystem.h>
#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcrypto/Exceptions.h>
using namespace std;
using namespace dev;
namespace js = json_spirit;
namespace fs = boost::filesystem;

static const int c_keyFileVersion = 3;

/// Upgrade the json-format to the current version.
static js::mValue upgraded(string const& _s)
{
	js::mValue v;
	js::read_string(_s, v);
	if (v.type() != js::obj_type)
		return js::mValue();
	js::mObject ret = v.get_obj();
	unsigned version = ret.count("Version") ? stoi(ret["Version"].get_str()) : ret.count("version") ? ret["version"].get_int() : 0;
	if (version == 1)
	{
		// upgrade to version 2
		js::mObject old;
		swap(old, ret);

		ret["id"] = old["Id"];
		js::mObject c;
		c["ciphertext"] = old["Crypto"].get_obj()["CipherText"];
		c["cipher"] = "aes-128-cbc";
		{
			js::mObject cp;
			cp["iv"] = old["Crypto"].get_obj()["IV"];
			c["cipherparams"] = cp;
		}
		c["kdf"] = old["Crypto"].get_obj()["KeyHeader"].get_obj()["Kdf"];
		{
			js::mObject kp;
			kp["salt"] = old["Crypto"].get_obj()["Salt"];
			for (auto const& i: old["Crypto"].get_obj()["KeyHeader"].get_obj()["KdfParams"].get_obj())
				if (i.first != "SaltLen")
					kp[boost::to_lower_copy(i.first)] = i.second;
			c["kdfparams"] = kp;
		}
		c["sillymac"] = old["Crypto"].get_obj()["MAC"];
		c["sillymacjson"] = _s;
		ret["crypto"] = c;
		version = 2;
	}
	if (ret.count("Crypto") && !ret.count("crypto"))
	{
		ret["crypto"] = ret["Crypto"];
		ret.erase("Crypto");
	}
	if (version == 2)
	{
		ret["crypto"].get_obj()["cipher"] = "aes-128-ctr";
		ret["crypto"].get_obj()["compat"] = "2";
		version = 3;
	}
	if (version == c_keyFileVersion)
		return ret;
	return js::mValue();
}

SecretStore::SecretStore(string const& _path): m_path(_path)
{
	load();
}

void SecretStore::setPath(string const& _path)
{
	m_path = _path;
	load();
}

bytesSec SecretStore::secret(h128 const& _uuid, function<string()> const& _pass, bool _useCache) const
{
	auto rit = m_cached.find(_uuid);
	if (_useCache && rit != m_cached.end())
		return rit->second;
	auto it = m_keys.find(_uuid);
	bytesSec key;
	if (it != m_keys.end())
	{
		key = bytesSec(decrypt(it->second.encryptedKey, _pass()));
		if (!key.empty())
			m_cached[_uuid] = key;
	}
	return key;
}

bytesSec SecretStore::secret(Address const& _address, function<string()> const& _pass) const
{
	bytesSec ret;
	if (auto k = key(_address))
		ret = bytesSec(decrypt(k->second.encryptedKey, _pass()));
	return ret;
}

bytesSec SecretStore::secret(string const& _content, string const& _pass)
{
	try
	{
		js::mValue u = upgraded(_content);
		if (u.type() != js::obj_type)
			return bytesSec();
		return decrypt(js::write_string(u.get_obj()["crypto"], false), _pass);
	}
	catch (...)
	{
		return bytesSec();
	}
}

h128 SecretStore::importSecret(bytesSec const& _s, string const& _pass)
{
	h128 r = h128::random();
	EncryptedKey key{encrypt(_s.ref(), _pass), toUUID(r), KeyPair(Secret(_s)).address()};
	m_cached[r] = _s;
	m_keys[r] = move(key);
	save();
	return r;
}

h128 SecretStore::importSecret(bytesConstRef _s, string const& _pass)
{
	h128 r = h128::random();
	EncryptedKey key{encrypt(_s, _pass), toUUID(r), KeyPair(Secret(_s)).address()};
	m_cached[r] = bytesSec(_s);
	m_keys[r] = move(key);
	save();
	return r;
}

void SecretStore::kill(h128 const& _uuid)
{
	m_cached.erase(_uuid);
	if (m_keys.count(_uuid))
	{
		fs::remove(m_keys[_uuid].filename);
		m_keys.erase(_uuid);
	}
}

void SecretStore::clearCache() const
{
	m_cached.clear();
}

void SecretStore::save(string const& _keysPath)
{
	fs::path p(_keysPath);
	fs::create_directories(p);
	DEV_IGNORE_EXCEPTIONS(fs::permissions(p, fs::owner_all));
	for (auto& k: m_keys)
	{
		string uuid = toUUID(k.first);
		string filename = (p / uuid).string() + ".json";
		js::mObject v;
		js::mValue crypto;
		js::read_string(k.second.encryptedKey, crypto);
		v["address"] = k.second.address.hex();
		v["crypto"] = crypto;
		v["id"] = uuid;
		v["version"] = c_keyFileVersion;
		writeFile(filename, js::write_string(js::mValue(v), true));
		swap(k.second.filename, filename);
		if (!filename.empty() && !fs::equivalent(filename, k.second.filename))
			fs::remove(filename);
	}
}

bool SecretStore::noteAddress(h128 const& _uuid, Address const& _address)
{
	if (m_keys.find(_uuid) != m_keys.end() && m_keys[_uuid].address == ZeroAddress)
	{
		m_keys[_uuid].address =	_address;
		return true;
	}
	return false;
}

void SecretStore::load(string const& _keysPath)
{
	fs::path p(_keysPath);
	try
	{
		for (fs::directory_iterator it(p); it != fs::directory_iterator(); ++it)
			if (fs::is_regular_file(it->path()))
				readKey(it->path().string(), true);
	}
	catch (...) {}
}

h128 SecretStore::readKey(string const& _file, bool _takeFileOwnership)
{
	ctrace << "Reading" << _file;
	return readKeyContent(contentsString(_file), _takeFileOwnership ? _file : string());
}

h128 SecretStore::readKeyContent(string const& _content, string const& _file)
{
	try
	{
		js::mValue u = upgraded(_content);
		if (u.type() == js::obj_type)
		{
			js::mObject& o = u.get_obj();
			auto uuid = fromUUID(o["id"].get_str());
			Address address = ZeroAddress;
			if (o.find("address") != o.end() && isHex(o["address"].get_str()))
				address = Address(o["address"].get_str());
			else
				cwarn << "Account address is either not defined or not in hex format" << _file;
			m_keys[uuid] = EncryptedKey{js::write_string(o["crypto"], false), _file, address};
			return uuid;
		}
		else
			cwarn << "Invalid JSON in key file" << _file;
		return h128();
	}
	catch (...)
	{
		return h128();
	}
}

bool SecretStore::recode(Address const& _address, string const& _newPass, function<string()> const& _pass, KDF _kdf)
{
	if (auto k = key(_address))
	{
		bytesSec s = secret(_address, _pass);
		if (s.empty())
			return false;
		else
		{
			k->second.encryptedKey = encrypt(s.ref(), _newPass, _kdf);
			save();
			return true;
		}
	}
	return false;
}

pair<h128 const, SecretStore::EncryptedKey> const* SecretStore::key(Address const& _address) const
{
	for (auto const& k: m_keys)
		if (k.second.address == _address)
			return &k;
	return nullptr;
}

pair<h128 const, SecretStore::EncryptedKey>* SecretStore::key(Address const& _address)
{
	for (auto& k: m_keys)
		if (k.second.address == _address)
			return &k;
	return nullptr;
}

bool SecretStore::recode(h128 const& _uuid, string const& _newPass, function<string()> const& _pass, KDF _kdf)
{
	bytesSec s = secret(_uuid, _pass, true);
	if (s.empty())
		return false;
	m_cached.erase(_uuid);
	m_keys[_uuid].encryptedKey = encrypt(s.ref(), _newPass, _kdf);
	save();
	return true;
}

static bytesSec deriveNewKey(string const& _pass, KDF _kdf, js::mObject& o_ret)
{
	unsigned dklen = 32;
	unsigned iterations = 1 << 18;
	bytes salt = h256::random().asBytes();
	if (_kdf == KDF::Scrypt)
	{
		unsigned p = 1;
		unsigned r = 8;
		o_ret["kdf"] = "scrypt";
		{
			js::mObject params;
			params["n"] = int64_t(iterations);
			params["r"] = int(r);
			params["p"] = int(p);
			params["dklen"] = int(dklen);
			params["salt"] = toHex(salt);
			o_ret["kdfparams"] = params;
		}
		return scrypt(_pass, salt, iterations, r, p, dklen);
	}
	else
	{
		o_ret["kdf"] = "pbkdf2";
		{
			js::mObject params;
			params["prf"] = "hmac-sha256";
			params["c"] = int(iterations);
			params["salt"] = toHex(salt);
			params["dklen"] = int(dklen);
			o_ret["kdfparams"] = params;
		}
		return pbkdf2(_pass, salt, iterations, dklen);
	}
}

string SecretStore::encrypt(bytesConstRef _v, string const& _pass, KDF _kdf)
{
	js::mObject ret;

	bytesSec derivedKey = deriveNewKey(_pass, _kdf, ret);
	if (derivedKey.empty())
		BOOST_THROW_EXCEPTION(crypto::CryptoException() << errinfo_comment("Key derivation failed."));

	ret["cipher"] = "aes-128-ctr";
	SecureFixedHash<16> key(derivedKey, h128::AlignLeft);
	h128 iv = h128::random();
	{
		js::mObject params;
		params["iv"] = toHex(iv.ref());
		ret["cipherparams"] = params;
	}

	// cipher text
	bytes cipherText = encryptSymNoAuth(key, iv, _v);
	if (cipherText.empty())
		BOOST_THROW_EXCEPTION(crypto::CryptoException() << errinfo_comment("Key encryption failed."));
	ret["ciphertext"] = toHex(cipherText);

	// and mac.
	h256 mac = sha3(derivedKey.ref().cropped(16, 16).toBytes() + cipherText);
	ret["mac"] = toHex(mac.ref());

	return js::write_string(js::mValue(ret), true);
}

bytesSec SecretStore::decrypt(string const& _v, string const& _pass)
{
	js::mObject o;
	{
		js::mValue ov;
		js::read_string(_v, ov);
		o = ov.get_obj();
	}

	// derive key
	bytesSec derivedKey;
	if (o["kdf"].get_str() == "pbkdf2")
	{
		auto params = o["kdfparams"].get_obj();
		if (params["prf"].get_str() != "hmac-sha256")
		{
			cwarn << "Unknown PRF for PBKDF2" << params["prf"].get_str() << "not supported.";
			return bytesSec();
		}
		unsigned iterations = params["c"].get_int();
		bytes salt = fromHex(params["salt"].get_str());
		derivedKey = pbkdf2(_pass, salt, iterations, params["dklen"].get_int());
	}
	else if (o["kdf"].get_str() == "scrypt")
	{
		auto p = o["kdfparams"].get_obj();
		derivedKey = scrypt(_pass, fromHex(p["salt"].get_str()), p["n"].get_int(), p["r"].get_int(), p["p"].get_int(), p["dklen"].get_int());
	}
	else
	{
		cwarn << "Unknown KDF" << o["kdf"].get_str() << "not supported.";
		return bytesSec();
	}

	if (derivedKey.size() < 32 && !(o.count("compat") && o["compat"].get_str() == "2"))
	{
		cwarn << "Derived key's length too short (<32 bytes)";
		return bytesSec();
	}

	bytes cipherText = fromHex(o["ciphertext"].get_str());

	// check MAC
	if (o.count("mac"))
	{
		h256 mac(o["mac"].get_str());
		h256 macExp;
		if (o.count("compat") && o["compat"].get_str() == "2")
			macExp = sha3(derivedKey.ref().cropped(derivedKey.size() - 16).toBytes() + cipherText);
		else
			macExp = sha3(derivedKey.ref().cropped(16, 16).toBytes() + cipherText);
		if (mac != macExp)
		{
			cwarn << "Invalid key - MAC mismatch; expected" << toString(macExp) << ", got" << toString(mac);
			return bytesSec();
		}
	}
	else if (o.count("sillymac"))
	{
		h256 mac(o["sillymac"].get_str());
		h256 macExp = sha3(asBytes(o["sillymacjson"].get_str()) + derivedKey.ref().cropped(derivedKey.size() - 16).toBytes() + cipherText);
		if (mac != macExp)
		{
			cwarn << "Invalid key - MAC mismatch; expected" << toString(macExp) << ", got" << toString(mac);
			return bytesSec();
		}
	}
	else
		cwarn << "No MAC. Proceeding anyway.";

	// decrypt
	if (o["cipher"].get_str() == "aes-128-ctr")
	{
		auto params = o["cipherparams"].get_obj();
		h128 iv(params["iv"].get_str());
		if (o.count("compat") && o["compat"].get_str() == "2")
		{
			SecureFixedHash<16> key(sha3Secure(derivedKey.ref().cropped(derivedKey.size() - 16)), h128::AlignRight);
			return decryptSymNoAuth(key, iv, &cipherText);
		}
		else
			return decryptSymNoAuth(SecureFixedHash<16>(derivedKey, h128::AlignLeft), iv, &cipherText);
	}
	else
	{
		cwarn << "Unknown cipher" << o["cipher"].get_str() << "not supported.";
		return bytesSec();
	}
}
