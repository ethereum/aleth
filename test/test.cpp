#define BOOST_TEST_MODULE Web3CoreTest
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
//#define BOOST_DISABLE_WIN32 //disables SEH warning
//#define BOOST_TEST_NO_MAIN
#include <boost/test/included/unit_test.hpp>
#pragma GCC diagnostic pop
#include <test/test.h>
#include <libdevcore/Log.h>

using namespace std;

namespace dev
{
namespace test
{

std::string getTestPath()
{
	string testPath;
	const char* ptestPath = getenv("ETHEREUM_TEST_PATH");

	if (ptestPath == NULL)
	{
		cnote << " could not find environment variable ETHEREUM_TEST_PATH \n";
		testPath = "../../../tests";
	}
	else
		testPath = ptestPath;

	return testPath;
}

Options::Options()
{
	auto argc = boost::unit_test::framework::master_test_suite().argc;
	auto argv = boost::unit_test::framework::master_test_suite().argv;

	for (auto i = 0; i < argc; ++i)
	{
		auto arg = std::string{argv[i]};
		if (arg == "--performance")
			performance = true;
		else if (arg == "--nonetwork")
			nonetwork = true;

	}
}

Options const& Options::get()
{
	static Options instance;
	return instance;
}

}} //namespaces
