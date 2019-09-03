/*
	This file is part of aleth.

	aleth is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	aleth is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with aleth.  If not, see <http://www.gnu.org/licenses/>.
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

    // Mark the _folderName as executed for a given _suitePath (to filler files)
    void markTestFolderAsFinished(
        boost::filesystem::path const& _suitePath, std::string const& _folderName);

private:
	TestOutputHelper() {}
    void checkUnfinishedTestFolders();  // Checkup that all test folders are active during the test
                                        // run
    Timer m_timer;
	size_t m_currTest;
	size_t m_maxTests;
	std::string m_currentTestName;
	std::string m_currentTestCaseName;
	boost::filesystem::path m_currentTestFileName;
	typedef std::pair<double, std::string> execTimeName;
	std::vector<execTimeName> m_execTimeResults;
    typedef std::set<std::string> FolderNameSet;
    std::map<boost::filesystem::path, FolderNameSet> m_finishedTestFoldersMap;
};

class TestOutputHelperFixture
{
public:
	TestOutputHelperFixture() { TestOutputHelper::get().initTest(); }
	~TestOutputHelperFixture() { TestOutputHelper::get().finishTest(); }
};

} //namespace test
} //namespace dev
