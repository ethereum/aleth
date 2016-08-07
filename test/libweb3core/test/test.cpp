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
/** @file test.cpp
 * @author Dimitry Khokhlov <dimitry@ethdev.com>
 * @date 2015
 * Stub for generating main boost.test module.
 * Original code taken from boost sources.
 */

#define BOOST_TEST_MODULE Web3CoreTest
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#define BOOST_DISABLE_WIN32 //disables SEH warning
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
		else if (arg == "--verbosity" && i + 1 < argc)
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

using namespace boost;
string TestOutputHelper::m_currentTestName = "n/a";
string TestOutputHelper::m_currentTestCaseName = "n/a";

void TestOutputHelper::initTest()
{
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

}} //namespaces
