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
#include <libdevcore/TransientDirectory.h>
#include <libethcore/KeyManager.h>
#include <boost/filesystem/path.hpp>

#include <gtest/gtest.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
namespace fs = boost::filesystem;

TEST(KeyManager, keyInfoDefaultConstructor)
{
    KeyInfo kiDefault;
    EXPECT_EQ(kiDefault.accountName, "");
    EXPECT_EQ(kiDefault.passHash, h256());
}

TEST(KeyManager, keyInfoConstructor)
{
    h256 passHash("0x2a");
    string accountName = "myAccount";
    KeyInfo ki(passHash, accountName);
    EXPECT_EQ(ki.accountName, "myAccount");
    EXPECT_EQ(ki.passHash, h256("0x2a"));
}

TEST(KeyManager, keyManagerConstructor)
{
    KeyManager km;
    EXPECT_EQ(km.keysFile(), km.defaultPath());
    EXPECT_EQ(km.defaultPath(), getDataDir("ethereum") / fs::path("keys.info"));
    EXPECT_EQ(km.store().keys(), SecretStore(SecretStore::defaultPath()).keys());
    for (auto a : km.accounts())
        km.kill(a);
}

TEST(KeyManager, keyManagerKeysFile)
{
    KeyManager km;
    string password = "hardPassword";
    EXPECT_TRUE(!km.load(password));

    // set to valid path
    TransientDirectory tmpDir;
    km.setKeysFile(tmpDir.path());
    EXPECT_FALSE(km.exists());
    EXPECT_THROW(km.create(password), boost::filesystem::filesystem_error);
    km.setKeysFile(tmpDir.path() + "/notExistingDir/keysFile.json");
    EXPECT_NO_THROW(km.create(password));
    EXPECT_TRUE(km.exists());
    km.setKeysFile(tmpDir.path() + "keysFile.json");
    EXPECT_NO_THROW(km.create(password));
    km.save(password);
    EXPECT_TRUE(km.load(password));

    for (auto a : km.accounts())
        km.kill(a);
}

TEST(KeyManager, keyManagerHints)
{
    KeyManager km;
    string password = "hardPassword";

    // set to valid path
    TransientDirectory tmpDir;
    km.setKeysFile(tmpDir.path() + "keysFile.json");
    km.create(password);
    km.save(password);

    EXPECT_TRUE(!km.haveHint(password + "2"));
    km.notePassword(password);
    EXPECT_TRUE(km.haveHint(password));

    for (auto a : km.accounts())
        km.kill(a);
}

TEST(KeyManager, keyManagerAccounts)
{
    string password = "hardPassword";

    TransientDirectory tmpDir;
    KeyManager km(tmpDir.path() + "keysFile.json", tmpDir.path());

    EXPECT_NO_THROW(km.create(password));
    EXPECT_TRUE(km.accounts().empty());
    EXPECT_TRUE(km.load(password));

    for (auto a : km.accounts())
        km.kill(a);
}

TEST(KeyManager, keyManagerKill)
{
    string password = "hardPassword";
    TransientDirectory tmpDir;
    KeyPair kp = KeyPair::create();

    {
        KeyManager km(tmpDir.path() + "keysFile.json", tmpDir.path());
        EXPECT_NO_THROW(km.create(password));
        EXPECT_TRUE(km.accounts().empty());
        EXPECT_TRUE(km.load(password));
        EXPECT_TRUE(km.import(kp.secret(), "test"));
    }
    {
        KeyManager km(tmpDir.path() + "keysFile.json", tmpDir.path());
        EXPECT_TRUE(km.load(password));
        Addresses addresses = km.accounts();
        EXPECT_TRUE(addresses.size() == 1 && addresses[0] == kp.address());
        km.kill(addresses[0]);
    }
    {
        KeyManager km(tmpDir.path() + "keysFile.json", tmpDir.path());
        EXPECT_TRUE(km.load(password));
        EXPECT_TRUE(km.accounts().empty());
    }
}
