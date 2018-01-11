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
/** @file core.cpp
 * @author Dimitry Khokhlov <winsvega@mail.ru>
 * @date 2014
 * CORE test functions.
 */

#include <boost/test/unit_test.hpp>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Log.h>
#include <test/tools/libtesteth/TestOutputHelper.h>

using namespace dev::test;

BOOST_FIXTURE_TEST_SUITE(CoreLibTests, TestOutputHelperFixture)

BOOST_AUTO_TEST_CASE(toHex)
{
	dev::bytes b = dev::fromHex("f0e1d2c3b4a59687");
	BOOST_CHECK_EQUAL(dev::toHex(b), "f0e1d2c3b4a59687");
	BOOST_CHECK_EQUAL(dev::toHexPrefixed(b), "0xf0e1d2c3b4a59687");

	dev::h256 h("705a1849c02140e7197fbde82987a9eb623f97e32fc479a3cd8e4b3b52dcc4b2");
	BOOST_CHECK_EQUAL(dev::toHex(h), "705a1849c02140e7197fbde82987a9eb623f97e32fc479a3cd8e4b3b52dcc4b2");
	BOOST_CHECK_EQUAL(dev::toHexPrefixed(h), "0x705a1849c02140e7197fbde82987a9eb623f97e32fc479a3cd8e4b3b52dcc4b2");
}

BOOST_AUTO_TEST_CASE(toCompactHex)
{
	dev::u256 i("0x123456789abcdef");
	BOOST_CHECK_EQUAL(dev::toCompactHex(i), "0123456789abcdef");
	BOOST_CHECK_EQUAL(dev::toCompactHexPrefixed(i), "0x0123456789abcdef");
}

BOOST_AUTO_TEST_CASE(byteRef)
{	
	cnote << "bytesRef copyTo and toString...";
	dev::bytes originalSequence = dev::fromHex("0102030405060708091011121314151617181920212223242526272829303132");
	dev::bytesRef out(&originalSequence.at(0), 32);
	dev::h256 hash32("1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347");
	hash32.ref().copyTo(out);

	BOOST_CHECK_MESSAGE(out.size() == 32, "Error wrong result size when h256::ref().copyTo(dev::bytesRef out)");
	BOOST_CHECK_MESSAGE(out.toBytes() == originalSequence, "Error when h256::ref().copyTo(dev::bytesRef out)");
}

BOOST_AUTO_TEST_CASE(isHex)
{
	BOOST_CHECK(dev::isHex("0x"));
	BOOST_CHECK(dev::isHex("0xA"));
	BOOST_CHECK(dev::isHex("0xAB"));
	BOOST_CHECK(dev::isHex("0x0AA"));
	BOOST_CHECK(!dev::isHex("0x0Ag"));
	BOOST_CHECK(!dev::isHex("0Ag"));
	BOOST_CHECK(!dev::isHex(" "));
	BOOST_CHECK(dev::isHex("aa"));
	BOOST_CHECK(dev::isHex("003"));
}

BOOST_AUTO_TEST_SUITE_END()
