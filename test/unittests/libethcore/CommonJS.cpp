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
