
#include <boost/test/unit_test.hpp>
#include "TestUtils.h"

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

BOOST_FIXTURE_TEST_SUITE(InterfaceStub, InterfaceStubFixture)

BOOST_AUTO_TEST_CASE(number)
{
	enumerateInterfaces([](Json::Value const& _json, dev::eth::InterfaceStub& _client) -> void
	{
		BOOST_CHECK_EQUAL(_client.number(), _json["blocks"].size());
	});
}

BOOST_AUTO_TEST_SUITE_END()
