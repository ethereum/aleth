
#include <boost/test/unit_test.hpp>
#include <libdevcore/CommonJS.h>
#include "TestUtils.h"

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

BOOST_FIXTURE_TEST_SUITE(InterfaceStub, InterfaceStubFixture)

BOOST_AUTO_TEST_CASE(balanceAt)
{
	enumerateInterfaces([](Json::Value const& _json, dev::eth::InterfaceStub& _client) -> void
	{
		for (string const& name: _json["postState"].getMemberNames())
		{
			Json::Value o = _json["postState"][name];
			Address address(name);
			
			u256 expectedBalance = u256(o["balance"].asString());
			u256 balance = _client.balanceAt(address);
			BOOST_CHECK_EQUAL(expectedBalance, balance);
		}
	});
}

BOOST_AUTO_TEST_CASE(countAt)
{
	enumerateInterfaces([](Json::Value const& _json, dev::eth::InterfaceStub& _client) -> void
	{
		for (string const& name: _json["postState"].getMemberNames())
		{
			Json::Value o = _json["postState"][name];
			Address address(name);
			
			u256 expectedCount = u256(o["nonce"].asString());
			u256 count = _client.countAt(address);
			BOOST_CHECK_EQUAL(expectedCount, count);
		}
	});
}

BOOST_AUTO_TEST_CASE(blocks)
{
	enumerateInterfaces([](Json::Value const& _json, dev::eth::InterfaceStub& _client) -> void
	{
		BOOST_CHECK_EQUAL(_json["blocks"].size(), _client.number());
	});
}

BOOST_AUTO_TEST_SUITE_END()
