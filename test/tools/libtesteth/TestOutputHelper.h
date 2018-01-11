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
#include <test/tools/libtestutils/Common.h>
#include <test/tools/libtesteth/JsonSpiritHeaders.h>

namespace dev
{
namespace test
{

class TestOutputHelper
{
public:
	static TestOutputHelper& get()
	{
		static TestOutputHelper instance;
		return instance;
	}
	TestOutputHelper(TestOutputHelper const&) = delete;
	void operator=(TestOutputHelper const&) = delete;

	void initTest(size_t _maxTests = 1);
	// Display percantage of completed tests to std::out. Has to be called before execution of every test.
	void showProgress();
	void finishTest();

	//void setMaxTests(int _count) { m_maxTests = _count; }
	bool checkTest(std::string const& _testName);
	void setCurrentTestFile(boost::filesystem::path const& _name) { m_currentTestFileName = _name; }
	void setCurrentTestName(std::string const& _name) { m_currentTestName = _name; }
	std::string const& testName() { return m_currentTestName; }
	std::string const& caseName() { return m_currentTestCaseName; }
	boost::filesystem::path const& testFile() { return m_currentTestFileName; }
	void printTestExecStats();

private:
	TestOutputHelper() {}
	Timer m_timer;
	size_t m_currTest;
	size_t m_maxTests;
	std::string m_currentTestName;
	std::string m_currentTestCaseName;
	boost::filesystem::path m_currentTestFileName;
	typedef std::pair<double, std::string> execTimeName;
	std::vector<execTimeName> m_execTimeResults;
};

class TestOutputHelperFixture
{
public:
	TestOutputHelperFixture() { TestOutputHelper::get().initTest(); }
	~TestOutputHelperFixture() { TestOutputHelper::get().finishTest(); }
};

} //namespace test
} //namespace dev
