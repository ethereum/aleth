#pragma once
#include <test/tools/libtesteth/TestHelper.h>
using namespace std;

namespace dev
{
namespace test
{
enum class TestExecution
{
    REQUIRE_OPTION_ALL,
    REQUIRE_OPTION_ALL_NOT_REFILLABLE,
    NOT_REFILLABLE,
    STANDARD
};


template <class T>
class StateTestFixtureBase
{
public:
    StateTestFixtureBase(TestExecution const _execFlag)
    {
        T suite;
        if ((_execFlag == TestExecution::REQUIRE_OPTION_ALL_NOT_REFILLABLE ||
                _execFlag == TestExecution::NOT_REFILLABLE) &&
            (Options::get().fillchain || Options::get().filltests))
            BOOST_FAIL("Tests are sealed and not refillable!");

        string casename = boost::unit_test::framework::current_test_case().p_name;
        boost::filesystem::path suiteFillerPath = suite.getFullPathFiller(casename).parent_path();
        static vector<string> const timeConsumingTestSuites{
            string{"stTimeConsuming"}, string{"stQuadraticComplexityTest"}};

        bool skipTheTest = false;
        if (test::inArray(timeConsumingTestSuites, casename) && !test::Options::get().all)
            skipTheTest = true;
        if ((_execFlag == TestExecution::REQUIRE_OPTION_ALL ||
                _execFlag == TestExecution::REQUIRE_OPTION_ALL_NOT_REFILLABLE) &&
            !Options::get().all)
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
