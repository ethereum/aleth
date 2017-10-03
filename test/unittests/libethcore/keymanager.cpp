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
/** @file keymanager.cpp
 * @author Christoph Jentzsch <cj@ethdev.com>
 * @date 2015
 * Keymanager test functions.
 */

#include <test/tools/libtesteth/TestHelper.h>
#include <libdevcore/TransientDirectory.h>
#include <libethcore/KeyManager.h>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem/path.hpp>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;
namespace fs = boost::filesystem;

BOOST_FIXTURE_TEST_SUITE(KeyManagerTests, TestOutputHelper)

BOOST_AUTO_TEST_CASE(KeyInfoDefaultConstructor)
{
	KeyInfo kiDefault;
	ETH_CHECK_EQUAL(kiDefault.accountName, "");
	ETH_CHECK(kiDefault.passHash == h256());
}

BOOST_AUTO_TEST_CASE(KeyInfoConstructor)
{
	h256 passHash("0x2a");
	string accountName = "myAccount";
	KeyInfo ki(passHash, accountName);
	ETH_CHECK_EQUAL(ki.accountName, "myAccount");
	ETH_CHECK(ki.passHash == h256("0x2a"));
}

BOOST_AUTO_TEST_CASE(KeyManagerConstructor)
{
	KeyManager km;
	ETH_CHECK_EQUAL(km.keysFile(), km.defaultPath());
	ETH_CHECK_EQUAL(km.defaultPath(), getDataDir("ethereum") / fs::path("keys.info"));
	ETH_CHECK(km.store().keys() == SecretStore(SecretStore::defaultPath()).keys());
	for (auto a: km.accounts())
		km.kill(a);
}

BOOST_AUTO_TEST_CASE(KeyManagerKeysFile)
{
	KeyManager km;
	string password = "hardPassword";
	ETH_CHECK(!km.load(password));

	// set to valid path
	TransientDirectory tmpDir;
	km.setKeysFile(tmpDir.path());
	ETH_CHECK(!km.exists());
	ETH_CHECK_THROW(km.create(password), boost::filesystem::filesystem_error);
	km.setKeysFile(tmpDir.path() + "/notExistingDir/keysFile.json");
	ETH_CHECK_NO_THROW(km.create(password));
	ETH_CHECK(km.exists());
	km.setKeysFile(tmpDir.path() + "keysFile.json");
	ETH_CHECK_NO_THROW(km.create(password));
	km.save(password);
	ETH_CHECK(km.load(password));

	for (auto a: km.accounts())
		km.kill(a);
}

BOOST_AUTO_TEST_CASE(KeyManagerHints)
{
	KeyManager km;
	string password = "hardPassword";

	// set to valid path
	TransientDirectory tmpDir;
	km.setKeysFile(tmpDir.path() + "keysFile.json");
	km.create(password);
	km.save(password);

	ETH_CHECK(!km.haveHint(password + "2"));
	km.notePassword(password);
	ETH_CHECK(km.haveHint(password));

	for (auto a: km.accounts())
		km.kill(a);
}

BOOST_AUTO_TEST_CASE(KeyManagerAccounts)
{
	string password = "hardPassword";

	TransientDirectory tmpDir;
	KeyManager km(tmpDir.path()+ "keysFile.json", tmpDir.path());

	ETH_CHECK_NO_THROW(km.create(password));
	ETH_CHECK(km.accounts().empty());
	ETH_CHECK(km.load(password));

	for (auto a: km.accounts())
		km.kill(a);
}

BOOST_AUTO_TEST_CASE(KeyManagerKill)
{
	string password = "hardPassword";
	TransientDirectory tmpDir;
	KeyPair kp = KeyPair::create();

	{
		KeyManager km(tmpDir.path() + "keysFile.json", tmpDir.path());
		ETH_CHECK_NO_THROW(km.create(password));
		ETH_CHECK(km.accounts().empty());
		ETH_CHECK(km.load(password));
		ETH_CHECK(km.import(kp.secret(), "test"));
	}
	{
		KeyManager km(tmpDir.path() + "keysFile.json", tmpDir.path());
		ETH_CHECK(km.load(password));
		Addresses addresses = km.accounts();
		ETH_CHECK(addresses.size() == 1 && addresses[0] == kp.address());
		km.kill(addresses[0]);
	}
	{
		KeyManager km(tmpDir.path() + "keysFile.json", tmpDir.path());
		ETH_CHECK(km.load(password));
		ETH_CHECK(km.accounts().empty());
	}
}

BOOST_AUTO_TEST_SUITE_END()
