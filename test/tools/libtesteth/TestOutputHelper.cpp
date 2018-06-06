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

#include <boost/test/unit_test.hpp>
#include <boost/io/ios_state.hpp>
#include <libethashseal/Ethash.h>
#include <libethcore/BasicAuthority.h>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <test/tools/libtesteth/Options.h>
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
