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

#include <libethashseal/Ethash.h>
#include <libethcore/BasicAuthority.h>
#include <test/tools/libtesteth/Options.h>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <boost/filesystem.hpp>
#include <boost/io/ios_state.hpp>
#include <boost/test/unit_test.hpp>
#include <numeric>

using namespace std;
using namespace dev;
using namespace dev::test;
using namespace dev::eth;
using namespace boost;

void TestOutputHelper::initTest(size_t _maxTests)
{
	Ethash::init();
	NoProof::init();
	BasicAuthority::init();
	m_currentTestName = "n/a";
	m_currentTestFileName = string();
    m_timer = Timer();
	m_currentTestCaseName = boost::unit_test::framework::current_test_case().p_name;
	if (!Options::get().createRandomTest)
		std::cout << "Test Case \"" + m_currentTestCaseName + "\": \n";
	m_maxTests = _maxTests;
	m_currTest = 0;
}

bool TestOutputHelper::checkTest(std::string const& _testName)
{
	if (test::Options::get().singleTest && test::Options::get().singleTestName != _testName)
		return false;

	m_currentTestName = _testName;
	return true;
}

void TestOutputHelper::showProgress()
{
	m_currTest++;
	int m_testsPerProgs = std::max(1, (int)(m_maxTests / 4));
	if (!test::Options::get().createRandomTest && (m_currTest % m_testsPerProgs == 0 || m_currTest ==  m_maxTests))
	{
		int percent = int(m_currTest*100/m_maxTests);
		std::cout << percent << "%";
		if (percent != 100)
			std::cout << "...";
		std::cout << "\n";
	}
}

void TestOutputHelper::finishTest()
{
	if (Options::get().exectimelog)
	{
		execTimeName res;
		res.first = m_timer.elapsed();
		res.second = caseName();
		std::cout << res.second + " time: " + toString(res.first) << "\n";
		m_execTimeResults.push_back(res);
	}
}

void TestOutputHelper::printTestExecStats()
{
    checkUnfinishedTestFolders();
    if (Options::get().exectimelog)
    {
        boost::io::ios_flags_saver saver(cout);
        std::cout << std::left;
        std::sort(m_execTimeResults.begin(), m_execTimeResults.end(), [](execTimeName _a, execTimeName _b) { return (_b.first < _a.first); });
        int totalTime = std::accumulate(m_execTimeResults.begin(), m_execTimeResults.end(), 0,
                            [](int a, execTimeName const& b)
                            {
                                return a + b.first;
                            });
        std::cout << setw(45) << "Total Time: " << setw(25) << "     : " + toString(totalTime) << "\n";
        for (auto const& t: m_execTimeResults)
            std::cout << setw(45) << t.second << setw(25) << " time: " + toString(t.first) << "\n";
        saver.restore();
    }
}

// check if a boost path contain no test files
bool pathHasTests(boost::filesystem::path const& _path)
{
    using fsIterator = boost::filesystem::directory_iterator;
    for (fsIterator it(_path); it != fsIterator(); ++it)
    {
        // if the extention of a test file
        if (boost::filesystem::is_regular_file(it->path()) &&
            (it->path().extension() == ".json" || it->path().extension() == ".yml"))
        {
            // if the filename ends with Filler/Copier type
            std::string const name = it->path().stem().filename().string();
            std::string const suffix =
                (name.length() > 7) ? name.substr(name.length() - 6) : string();
            if (suffix == "Filler" || suffix == "Copier")
                return true;
        }
    }
    return false;
}

void TestOutputHelper::checkUnfinishedTestFolders()
{
    // Unit tests does not mark test folders
    if (m_finishedTestFoldersMap.size() == 0)
        return;

    // -t SuiteName/caseName   parse caseName as filter
    // rCurrentTestSuite is empty if run without -t argument
    string filter;
    size_t pos = Options::get().rCurrentTestSuite.rfind('/');
    if (pos != string::npos)
        filter = Options::get().rCurrentTestSuite.substr(pos + 1);

    std::map<boost::filesystem::path, FolderNameSet>::const_iterator singleTest =
        m_finishedTestFoldersMap.begin();
    if (!filter.empty() && boost::filesystem::exists(singleTest->first / filter))
    {
        if (m_finishedTestFoldersMap.size() > 1)
        {
            std::cerr << "ERROR: Expected a single test to be passed: " << filter << "\n";
            return;
        }

        if (!pathHasTests(singleTest->first / filter))
        {
            std::cerr << "WARNING: Test folder " << singleTest->first / filter
                      << " appears to have no tests!"
                      << "\n";
            return;
        }
    }
    else
    {
        for (auto const& allTestsIt : m_finishedTestFoldersMap)
        {
            boost::filesystem::path path = allTestsIt.first;
            set<string> allFolders;
            using fsIterator = boost::filesystem::directory_iterator;
            for (fsIterator it(path); it != fsIterator(); ++it)
            {
                if (boost::filesystem::is_directory(*it))
                {
                    allFolders.insert(it->path().filename().string());
                    if (!pathHasTests(it->path()))
                        std::cerr << "WARNING: Test folder " << it->path()
                                  << " appears to have no tests!"
                                  << "\n";
                }
            }

            std::vector<string> diff;
            FolderNameSet finishedFolders = allTestsIt.second;
            std::set_difference(allFolders.begin(), allFolders.end(), finishedFolders.begin(),
                finishedFolders.end(), std::back_inserter(diff));
            for (auto const& it : diff)
            {
                std::cerr << "WARNING: Test folder " << path / it << " appears to be unused!"
                          << "\n";
            }
        }
    }
}

void TestOutputHelper::markTestFolderAsFinished(
    boost::filesystem::path const& _suitePath, string const& _folderName)
{
    // Mark test folder _folderName as finished for the test suite path _suitePath
    m_finishedTestFoldersMap[_suitePath].emplace(_folderName);
}
