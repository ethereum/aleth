
#include <boost/test/unit_test.hpp>
#include <libdevcore/CommonIO.h>
#include "TestUtils.h"

// used methods from TestHelper:
// getTestPath
#include "TestHelper.h"

using namespace std;
using namespace dev;
using namespace dev::test;

bool dev::test::getCommandLineOption(string const& _name)
{
	auto argc = boost::unit_test::framework::master_test_suite().argc;
	auto argv = boost::unit_test::framework::master_test_suite().argv;
	bool result = false;
	for (auto i = 0; !result && i < argc; ++i)
		result = _name == argv[i];
	return result;
}

std::string getCommandLineArgument(string const& _name, bool _require)
{
	auto argc = boost::unit_test::framework::master_test_suite().argc;
	auto argv = boost::unit_test::framework::master_test_suite().argv;
	for (auto i = 0; !result && i < argc; ++i)
		if (_name == argv[i]) {
			if (i + 1 < argc)
				return argv[i + 1];
			else
				BOOST_ERROR("Failed getting command line argument: " << _name << " from: " << argv);
		}
	if (_require)
		BOOST_ERROR("Failed getting command line argument: " << _name << " from: " << argv);
	return "";
}

std::string loadFile(std::string const& _path)
{
	string s = asString(dev::contents(testPath + "/" + _name + ".json"));
	BOOST_REQUIRE_MESSAGE(s.length() > 0, "Contents of " + testPath + "/" + _name + ".json is empty. Have you cloned the 'tests' repo branch develop and set ETHEREUM_TEST_PATH to its path?");
	return s;
}

std::string loadTestFile(std::string const& _filename)
{
	return loadFile(getTestPath() + "/" + _filename + ".json");
}
