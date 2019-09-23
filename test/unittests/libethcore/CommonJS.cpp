// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include <libethcore/CommonJS.h>

#include <gtest/gtest.h>

using namespace std;
using namespace dev;
using namespace dev::eth;

TEST(CommonJS, jsToPublic)
{
	KeyPair kp = KeyPair::create();
	string s = toJS(kp.pub());
	Public pub = dev::jsToPublic(s);
	EXPECT_EQ(kp.pub(), pub);
}

TEST(CommonJS, jsToAddress)
{
	KeyPair kp = KeyPair::create();
	string s = toJS(kp.address());
	Address address = dev::jsToAddress(s);
	EXPECT_EQ(kp.address(), address);
}

TEST(CommonJS, jsToSecret)
{
	KeyPair kp = KeyPair::create();
	string s = toJS(kp.secret().makeInsecure());
	Secret secret = dev::jsToSecret(s);
	EXPECT_EQ(kp.secret().makeInsecure(), secret.makeInsecure());
}
