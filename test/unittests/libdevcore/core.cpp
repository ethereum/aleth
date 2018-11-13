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

#include <libdevcore/CommonIO.h>
#include <libdevcore/FixedHash.h>

#include <gtest/gtest.h>

TEST(core, toHex)
{
    dev::bytes b = dev::fromHex("f0e1d2c3b4a59687");
    EXPECT_EQ(dev::toHex(b), "f0e1d2c3b4a59687");
    EXPECT_EQ(dev::toHexPrefixed(b), "0xf0e1d2c3b4a59687");

    dev::h256 h("705a1849c02140e7197fbde82987a9eb623f97e32fc479a3cd8e4b3b52dcc4b2");
    EXPECT_EQ(dev::toHex(h), "705a1849c02140e7197fbde82987a9eb623f97e32fc479a3cd8e4b3b52dcc4b2");
    EXPECT_EQ(dev::toHexPrefixed(h),
        "0x705a1849c02140e7197fbde82987a9eb623f97e32fc479a3cd8e4b3b52dcc4b2");
}

TEST(core, toCompactHex)
{
    dev::u256 i("0x123456789abcdef");
    EXPECT_EQ(dev::toCompactHex(i), "0123456789abcdef");
    EXPECT_EQ(dev::toCompactHexPrefixed(i), "0x0123456789abcdef");
}

TEST(core, byteRef)
{
    dev::bytes originalSequence =
        dev::fromHex("0102030405060708091011121314151617181920212223242526272829303132");
    dev::bytesRef out(&originalSequence.at(0), 32);
    dev::h256 hash32("1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347");
    hash32.ref().copyTo(out);

    EXPECT_EQ(out.size(), 32)
        << "Error wrong result size when h256::ref().copyTo(dev::bytesRef out)";
    EXPECT_EQ(out.toBytes(), originalSequence)
        << "Error when h256::ref().copyTo(dev::bytesRef out)";
}

TEST(core, isHex)
{
    EXPECT_TRUE(dev::isHex("0x"));
    EXPECT_TRUE(dev::isHex("0xA"));
    EXPECT_TRUE(dev::isHex("0xAB"));
    EXPECT_TRUE(dev::isHex("0x0AA"));
    EXPECT_FALSE(dev::isHex("0x0Ag"));
    EXPECT_FALSE(dev::isHex("0Ag"));
    EXPECT_FALSE(dev::isHex(" "));
    EXPECT_TRUE(dev::isHex("aa"));
    EXPECT_TRUE(dev::isHex("003"));
}
