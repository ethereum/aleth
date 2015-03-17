
#include <boost/test/unit_test.hpp>
#include "TestUtils.h"

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

BOOST_FIXTURE_TEST_SUITE(InterfaceStub, InterfaceStubFixture)

BOOST_AUTO_TEST_CASE(number)
{
	enumerateInterfaces([](dev::eth::InterfaceStub& _client, Json::Value const& _json) -> void
	{
		unsigned number = _client.number();
	});
}

BOOST_AUTO_TEST_SUITE_END()
