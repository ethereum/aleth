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

#include <fstream>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcrypto/SecretStore.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/TrieDB.h>
#include <libdevcore/TrieHash.h>
#include <libdevcore/TransientDirectory.h>
#include "MemTrie.h"
#include <test/tools/libtesteth/TestOutputHelper.h>
using namespace std;
using namespace dev;
using namespace dev::test;

namespace js = json_spirit;
namespace fs = boost::filesystem;
namespace utf = boost::unit_test;

BOOST_AUTO_TEST_SUITE(Crypto)

BOOST_FIXTURE_TEST_SUITE(KeyStore, TestOutputHelperFixture)

BOOST_AUTO_TEST_CASE(basic_tests)
{
	fs::path testPath = test::getTestPath();

	testPath /= fs::path("KeyStoreTests");

	cnote << "Testing Key Store...";
	js::mValue v;
	string const s = contentsString(testPath / fs::path("basic_tests.json"));
	BOOST_REQUIRE_MESSAGE(s.length() > 0, "Contents of 'KeyStoreTests/basic_tests.json' is empty. Have you cloned the 'tests' repo branch develop?");
	js::read_string(s, v);
	for (auto& i: v.get_obj())
	{
		cnote << i.first;
		js::mObject& o = i.second.get_obj();
		TransientDirectory tmpDir;
		SecretStore store(tmpDir.path());
		h128 u = store.readKeyContent(js::write_string(o["json"], false));
		cdebug << "read uuid" << u;
		bytesSec s = store.secret(u, [&](){ return o["password"].get_str(); });
		cdebug << "got secret" << toHex(s.makeInsecure());
		BOOST_REQUIRE_EQUAL(toHex(s.makeInsecure()), o["priv"].get_str());
	}
}

BOOST_AUTO_TEST_CASE(import_key_from_file)
{
	// Imports a key from an external file. Tests that the imported key is there
	// and that the external file is not deleted.
	TransientDirectory importDir;
	string importFile = importDir.path() + "/import.json";
	TransientDirectory storeDir;
	string keyData = R"({
		"version": 3,
		"crypto": {
			"ciphertext": "d69313b6470ac1942f75d72ebf8818a0d484ac78478a132ee081cd954d6bd7a9",
			"cipherparams": { "iv": "ffffffffffffffffffffffffffffffff" },
			"kdf": "pbkdf2",
			"kdfparams": { "dklen": 32,  "c": 262144,  "prf": "hmac-sha256",  "salt": "c82ef14476014cbf438081a42709e2ed" },
			"mac": "cf6bfbcc77142a22c4a908784b4a16f1023a1d0e2aff404c20158fa4f1587177",
			"cipher": "aes-128-ctr",
			"version": 1
		},
		"id": "abb67040-8dbe-0dad-fc39-2b082ef0ee5f"
	})";
	string password = "bar";
	string priv = "0202020202020202020202020202020202020202020202020202020202020202";
	writeFile(importFile, keyData);

	h128 uuid;
	{
		SecretStore store(storeDir.path());
		BOOST_CHECK_EQUAL(store.keys().size(), 0);
		uuid = store.importKey(importFile);
		BOOST_CHECK(!!uuid);
		BOOST_CHECK(contentsString(importFile) == keyData);
		BOOST_CHECK_EQUAL(priv, toHex(store.secret(uuid, [&](){ return password; }).makeInsecure()));
		BOOST_CHECK_EQUAL(store.keys().size(), 1);
	}
	fs::remove(importFile);
	// now do it again to check whether SecretStore properly stored it on disk
	{
		SecretStore store(storeDir.path());
		BOOST_CHECK_EQUAL(store.keys().size(), 1);
		BOOST_CHECK_EQUAL(priv, toHex(store.secret(uuid, [&](){ return password; }).makeInsecure()));
	}
}

BOOST_AUTO_TEST_CASE(import_secret)
{
	for (string const& password: {"foobar", ""})
	{
		TransientDirectory storeDir;
		string priv = "0202020202020202020202020202020202020202020202020202020202020202";

		h128 uuid;
		{
			SecretStore store(storeDir.path());
			BOOST_CHECK_EQUAL(store.keys().size(), 0);
			uuid = store.importSecret(bytesSec(fromHex(priv)), password);
			BOOST_CHECK(!!uuid);
			BOOST_CHECK_EQUAL(priv, toHex(store.secret(uuid, [&](){ return password; }).makeInsecure()));
			BOOST_CHECK_EQUAL(store.keys().size(), 1);
		}
		{
			SecretStore store(storeDir.path());
			BOOST_CHECK_EQUAL(store.keys().size(), 1);
			BOOST_CHECK_EQUAL(priv, toHex(store.secret(uuid, [&](){ return password; }).makeInsecure()));
		}
	}
}

BOOST_AUTO_TEST_CASE(import_secret_bytesConstRef)
{
	for (string const& password: {"foobar", ""})
	{
		TransientDirectory storeDir;
		string priv = "0202020202020202020202020202020202020202020202020202020202020202";

		h128 uuid;
		{
			SecretStore store(storeDir.path());
			BOOST_CHECK_EQUAL(store.keys().size(), 0);
			bytes privateBytes = fromHex(priv);
			uuid = store.importSecret(&privateBytes, password);
			BOOST_CHECK(!!uuid);
			BOOST_CHECK_EQUAL(priv, toHex(store.secret(uuid, [&](){ return password; }).makeInsecure()));
			BOOST_CHECK_EQUAL(store.keys().size(), 1);
		}
		{
			SecretStore store(storeDir.path());
			BOOST_CHECK_EQUAL(store.keys().size(), 1);
			BOOST_CHECK_EQUAL(priv, toHex(store.secret(uuid, [&](){ return password; }).makeInsecure()));
		}
	}
}

BOOST_AUTO_TEST_CASE(wrong_password)
{
	TransientDirectory storeDir;
	SecretStore store(storeDir.path());
	string password = "foobar";
	string priv = "0202020202020202020202020202020202020202020202020202020202020202";

	h128 uuid;
	{
		SecretStore store(storeDir.path());
		BOOST_CHECK_EQUAL(store.keys().size(), 0);
		uuid = store.importSecret(bytesSec(fromHex(priv)), password);
		BOOST_CHECK(!!uuid);
		BOOST_CHECK_EQUAL(priv, toHex(store.secret(uuid, [&](){ return password; }).makeInsecure()));
		BOOST_CHECK_EQUAL(store.keys().size(), 1);
		// password will not be queried
		BOOST_CHECK_EQUAL(priv, toHex(store.secret(uuid, [&](){ return "abcdefg"; }).makeInsecure()));
	}
	{
		SecretStore store(storeDir.path());
		BOOST_CHECK_EQUAL(store.keys().size(), 1);
		BOOST_CHECK(store.secret(uuid, [&](){ return "abcdefg"; }).empty());
	}
}

BOOST_AUTO_TEST_CASE(recode)
{
	TransientDirectory storeDir;
	SecretStore store(storeDir.path());
	string password = "foobar";
	string changedPassword = "abcdefg";
	string priv = "0202020202020202020202020202020202020202020202020202020202020202";

	h128 uuid;
	{
		SecretStore store(storeDir.path());
		BOOST_CHECK_EQUAL(store.keys().size(), 0);
		uuid = store.importSecret(bytesSec(fromHex(priv)), password);
		BOOST_CHECK(!!uuid);
		BOOST_CHECK_EQUAL(priv, toHex(store.secret(uuid, [&](){ return password; }).makeInsecure()));
		BOOST_CHECK_EQUAL(store.keys().size(), 1);
	}
	{
		SecretStore store(storeDir.path());
		BOOST_CHECK_EQUAL(store.keys().size(), 1);
		BOOST_CHECK(store.secret(uuid, [&](){ return "abcdefg"; }).empty());
		BOOST_CHECK(store.recode(uuid, changedPassword, [&](){ return password; }));
		BOOST_CHECK_EQUAL(store.keys().size(), 1);
		BOOST_CHECK_EQUAL(priv, toHex(store.secret(uuid, [&](){ return changedPassword; }).makeInsecure()));
		store.clearCache();
		BOOST_CHECK(store.secret(uuid, [&](){ return password; }).empty());
		BOOST_CHECK_EQUAL(priv, toHex(store.secret(uuid, [&](){ return changedPassword; }).makeInsecure()));
	}
	{
		SecretStore store(storeDir.path());
		BOOST_CHECK_EQUAL(store.keys().size(), 1);
		BOOST_CHECK(store.secret(uuid, [&](){ return password; }).empty());
		BOOST_CHECK_EQUAL(priv, toHex(store.secret(uuid, [&](){ return changedPassword; }).makeInsecure()));
	}
}

BOOST_AUTO_TEST_CASE(keyImport_PBKDF2SHA256)
{
	// Imports a key from an external file. Tests that the imported key is there
	// and that the external file is not deleted.
	TransientDirectory importDir;
	string importFile = importDir.path() + "/import.json";
	TransientDirectory storeDir;
	string keyData = R"({
		"version": 3,
		"crypto": {
			"ciphertext": "5318b4d5bcd28de64ee5559e671353e16f075ecae9f99c7a79a38af5f869aa46",
			"cipherparams": { "iv": "6087dab2f9fdbbfaddc31a909735c1e6" },
			"kdf": "pbkdf2",
			"kdfparams": { "dklen": 32,  "c": 262144,  "prf": "hmac-sha256",  "salt": "ae3cd4e7013836a3df6bd7241b12db061dbe2c6785853cce422d148a624ce0bd" },
			"mac": "517ead924a9d0dc3124507e3393d175ce3ff7c1e96529c6c555ce9e51205e9b2",
			"cipher": "aes-128-ctr",
			"version": 3
		},
		"id": "3198bc9c-6672-5ab3-d995-4942343ae5b6"
	})";
	string password = "testpassword";
	string priv = "7a28b5ba57c53603b0b07b56bba752f7784bf506fa95edc395f5cf6c7514fe9d";
	writeFile(importFile, keyData);

	h128 uuid;
	{
		SecretStore store(storeDir.path());
		BOOST_CHECK_EQUAL(store.keys().size(), 0);
		uuid = store.importKey(importFile);
		BOOST_CHECK(!!uuid);
		BOOST_CHECK_EQUAL(uuid, h128("3198bc9c66725ab3d9954942343ae5b6"));
		BOOST_CHECK(contentsString(importFile) == keyData);
		BOOST_CHECK_EQUAL(priv, toHex(store.secret(uuid, [&](){ return password; }).makeInsecure()));
		BOOST_CHECK_EQUAL(store.keys().size(), 1);
		BOOST_CHECK_EQUAL(store.address(uuid), Address("008aeeda4d805471df9b2a5b0f38a0c3bcba786b"));
	}
	fs::remove(importFile);
}

BOOST_AUTO_TEST_CASE(keyImport_Scrypt)
{
	// Imports a key from an external file. Tests that the imported key is there
	// and that the external file is not deleted.
	TransientDirectory importDir;
	string importFile = importDir.path() + "/import.json";
	TransientDirectory storeDir;
	string keyData = R"({
		"version": 3,
		"crypto": {
			"ciphertext": "d172bf743a674da9cdad04534d56926ef8358534d458fffccd4e6ad2fbde479c",
			"cipherparams": { "iv": "83dbcc02d8ccb40e466191a123791e0e" },
			"kdf": "scrypt",
			"kdfparams": { "dklen": 32,  "n": 262144,  "r": 1,  "p": 8,  "salt": "ab0c7876052600dd703518d6fc3fe8984592145b591fc8fb5c6d43190334ba19" },
			"mac": "2103ac29920d71da29f15d75b4a16dbe95cfd7ff8faea1056c33131d846e3097",
			"cipher": "aes-128-ctr",
			"version": 3
		},
		"id": "3198bc9c-6672-5ab3-d995-4942343ae5b6"
	})";
	string password = "testpassword";
	string priv = "7a28b5ba57c53603b0b07b56bba752f7784bf506fa95edc395f5cf6c7514fe9d";
	writeFile(importFile, keyData);

	h128 uuid;
	{
		SecretStore store(storeDir.path());
		BOOST_CHECK_EQUAL(store.keys().size(), 0);
		uuid = store.importKey(importFile);
		BOOST_CHECK(!!uuid);
		BOOST_CHECK_EQUAL(uuid, h128("3198bc9c66725ab3d9954942343ae5b6"));
		BOOST_CHECK(contentsString(importFile) == keyData);
		BOOST_CHECK_EQUAL(priv, toHex(store.secret(uuid, [&](){ return password; }).makeInsecure()));
		BOOST_CHECK_EQUAL(store.keys().size(), 1);
		BOOST_CHECK_EQUAL(store.address(uuid), Address("008aeeda4d805471df9b2a5b0f38a0c3bcba786b"));
	}
	fs::remove(importFile);
}

BOOST_AUTO_TEST_CASE(keyImport__ScryptV2, *utf::expected_failures(2) *utf::disabled())
{
	// Imports a key from an external file. Tests that the imported key is there
	// and that the external file is not deleted.
	TransientDirectory importDir;
	string importFile = importDir.path() + "/import.json";
	TransientDirectory storeDir;
	string keyData = R"({
		"version": 2,
		"crypto": {
			"ciphertext": "07533e172414bfa50e99dba4a0ce603f654ebfa1ff46277c3e0c577fdc87f6bb4e4fe16c5a94ce6ce14cfa069821ef9b",
			"cipherparams": { "iv": "16d67ba0ce5a339ff2f07951253e6ba8" },
			"kdf": "scrypt",
			"kdfparams": { "dklen": 32,  "n": 262144,  "r": 1,  "p": 8,  "salt": "06870e5e6a24e183a5c807bd1c43afd86d573f7db303ff4853d135cd0fd3fe91" },
			"mac": "8ccded24da2e99a11d48cda146f9cc8213eb423e2ea0d8427f41c3be414424dd",
			"cipher": "aes-128-cbc",
			"version": 1
		},
		"id": "0498f19a-59db-4d54-ac95-33901b4f1870"
	})";
	string password = "testpassword";
	string priv = "7a28b5ba57c53603b0b07b56bba752f7784bf506fa95edc395f5cf6c7514fe9d";
	writeFile(importFile, keyData);

	h128 uuid;
	{
		SecretStore store(storeDir.path());
		BOOST_CHECK_EQUAL(store.keys().size(), 0);
		uuid = store.importKey(importFile);
		BOOST_CHECK(!!uuid);
		BOOST_CHECK_EQUAL(uuid, h128("0498f19a59db4d54ac9533901b4f1870"));
		BOOST_CHECK(contentsString(importFile) == keyData);
		BOOST_CHECK_EQUAL(priv, toHex(store.secret(uuid, [&](){ return password; }).makeInsecure()));
		BOOST_CHECK_EQUAL(store.keys().size(), 1);
		BOOST_CHECK_EQUAL(store.address(uuid), Address("008aeeda4d805471df9b2a5b0f38a0c3bcba786b"));
	}
	fs::remove(importFile);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
