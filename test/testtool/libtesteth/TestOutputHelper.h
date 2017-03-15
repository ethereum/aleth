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
/** @file
 * Fixture class for boost output when running testeth
 */

#pragma once
#include <test/libtestutils/Common.h>
#include <test/libtesteth/JsonSpiritHeaders.h>

namespace dev
{
namespace test
{

class TestOutputHelper
{
public:
	TestOutputHelper() { TestOutputHelper::initTest(); }
	static void initTest(int _maxTests = 1);
	static void initTest(json_spirit::mValue& _v);
	static bool passTest(json_spirit::mObject& _o, std::string& _testName);
	static void setMaxTests(int _count) { m_maxTests = _count; }
	static void setCurrentTestFileName(std::string _name) { m_currentTestFileName = _name; }
	static std::string const& testName() { return m_currentTestName; }
	static std::string const& caseName() { return m_currentTestCaseName; }
	static std::string const& testFileName() { return m_currentTestFileName; }
	static void finishTest();
	static void printTestExecStats();
	~TestOutputHelper() { TestOutputHelper::finishTest(); }
private:
	static Timer m_timer;
	static size_t m_currTest;
	static size_t m_maxTests;
	static std::string m_currentTestName;
	static std::string m_currentTestCaseName;
	static std::string m_currentTestFileName;
	typedef std::pair<double, std::string> execTimeName;
	static std::vector<execTimeName> m_execTimeResults;
};

} //namespace test
} //namespace dev
