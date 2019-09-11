// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

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
        boost::filesystem::path const suiteFillerPath = suite.getFullPathFiller(casename).parent_path();

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
