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
/** @file StateTestFixtureBase.h
 * @authors:
 *   Dimitry Khokhlov <dimitry@ethereum.org>
 * @date 2019
 */

#pragma once
#include <test/tools/libtesteth/TestHelper.h>

namespace dev
{
namespace test
{
enum class TestExecution
{
    RequireOptionAll,
    NotRefillable
};

static std::vector<std::string> const c_timeConsumingTestSuites{
    std::string{"stTimeConsuming"}, std::string{"stQuadraticComplexityTest"}};

template <class T>
class StateTestFixtureBase
{
public:
    explicit StateTestFixtureBase(std::set<TestExecution> const& _execFlags)
    {
        T suite;
        if (_execFlags.count(TestExecution::NotRefillable) &&
            (Options::get().fillchain || Options::get().filltests))
            BOOST_FAIL("Tests are sealed and not refillable!");

        std::string const casename = boost::unit_test::framework::current_test_case().p_name;
        boost::filesystem::path suiteFillerPath = suite.getFullPathFiller(casename).parent_path();

        bool skipTheTest = false;
        if (test::inArray(c_timeConsumingTestSuites, casename) && !test::Options::get().all)
            skipTheTest = true;
        else if (_execFlags.count(TestExecution::RequireOptionAll) && !Options::get().all)
            skipTheTest = true;

        if (skipTheTest)
        {
            std::cout << "Skipping " << casename << " because --all option is not specified.\n";
            test::TestOutputHelper::get().markTestFolderAsFinished(suiteFillerPath, casename);
            return;
        }
        suite.runAllTestsInFolder(casename);
        test::TestOutputHelper::get().markTestFolderAsFinished(suiteFillerPath, casename);
    }
};

}  // namespace test
}  // namespace dev
