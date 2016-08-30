
#define BOOST_TEST_MODULE Web3Test

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4535) // calling _set_se_translator requires /EHa
#endif
#include <boost/test/included/unit_test.hpp>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "test.h"
#include <libdevcore/Log.h>
#include <clocale>

using namespace boost;
using namespace std;

namespace dev
{
namespace test
{

Options::Options()
{
	auto argc = boost::unit_test::framework::master_test_suite().argc;
	auto argv = boost::unit_test::framework::master_test_suite().argv;

	for (auto i = 0; i < argc; ++i)
	{
		auto arg = std::string{argv[i]};
		if (arg == "--verbosity" && i + 1 < argc)
		{
			static std::ostringstream strCout; //static string to redirect logs to
			std::string indentLevel = std::string{argv[i + 1]};
			if (indentLevel == "0")
			{
				logVerbosity = Verbosity::None;
				std::cout.rdbuf(strCout.rdbuf());
				std::cerr.rdbuf(strCout.rdbuf());
				g_logVerbosity = -1;
			}
			else if (indentLevel == "1")
				logVerbosity = Verbosity::NiceReport;
			else
				logVerbosity = Verbosity::Full;

			int indentLevelInt = atoi(argv[i + 1]);
			if (indentLevelInt > g_logVerbosity)
				g_logVerbosity = indentLevelInt;
		}
	}
}

Options const& Options::get()
{
	static Options instance;
	return instance;
}


string TestOutputHelper::m_currentTestName = "n/a";
string TestOutputHelper::m_currentTestCaseName = "n/a";

/*
The equivalent of setlocale(LC_ALL, “C”) is called before any user code is run.
If the user has an invalid environment setting then it is possible for the call
to set locale to fail, so there are only two possible actions, the first is to
throw a runtime exception and cause the program to quit (default behaviour),
or the second is to modify the environment to something sensible (least
surprising behaviour).

The follow code produces the least surprising behaviour. It will use the user
specified default locale if it is valid, and if not then it will modify the
environment the process is running in to use a sensible default. This also means
that users do not need to install language packs for their OS.
*/
void setDefaultOrCLocale()
{
#if __unix__
	if (!std::setlocale(LC_ALL, ""))
	{
		setenv("LC_ALL", "C", 1);
	}
#endif
}

void TestOutputHelper::initTest()
{
	setDefaultOrCLocale();
	if (Options::get().logVerbosity ==  Options::Verbosity::NiceReport || Options::get().logVerbosity ==  Options::Verbosity::None)
		g_logVerbosity = -1;

	m_currentTestCaseName = boost::unit_test::framework::current_test_case().p_name;
	std::cout << "Test Case \"" + m_currentTestCaseName + "\": " << std::endl;
}

bool TestOutputHelper::passTest(std::string& _testName)
{
	cnote << _testName;
	_testName = "(" + _testName + ") ";
	m_currentTestName = _testName;
	return true;
}

}}
