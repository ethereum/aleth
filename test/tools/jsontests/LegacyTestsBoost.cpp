#include <test/tools/jsontests/BlockChainTests.h>
#include <test/tools/jsontests/StateTests.h>
#include <test/tools/libtesteth/TestHelper.h>

#include <boost/test/unit_test.hpp>

using namespace dev::test;
namespace fs = boost::filesystem;

fs::path StateTestSuiteLegacyConstantinople::suiteFolder() const
{
    return "LegacyTests/Constantinople/GeneralStateTests";
}

fs::path StateTestSuiteLegacyConstantinople::suiteFillerFolder() const
{
    return "LegacyTests/Constantinople/GeneralStateTestsFiller";
}

fs::path BCGeneralStateTestsSuiteLegacyConstantinople::suiteFolder() const
{
    return "LegacyTests/Constantinople/BlockchainTests/GeneralStateTests";
}

fs::path BCGeneralStateTestsSuiteLegacyConstantinople::suiteFillerFolder() const
{
    return "LegacyTests/Constantinople/BlockchainTestsFiller/GeneralStateTests";
}

template <class T>
class LegacyTestFixtureBase
{
public:
    LegacyTestFixtureBase(bool _onlyRunIfOptionsAllSet)
    {
        T suite;
        if (Options::get().fillchain || Options::get().filltests)
            BOOST_FAIL("Legacy tests are sealed and not refillable!");

        string casename = boost::unit_test::framework::current_test_case().p_name;
        boost::filesystem::path suiteFillerPath = suite.getFullPathFiller(casename).parent_path();
        static vector<string> const timeConsumingTestSuites{
            string{"stTimeConsuming"}, string{"stQuadraticComplexityTest"}};

        bool skipTheTest = false;
        if (test::inArray(timeConsumingTestSuites, casename) && !test::Options::get().all)
            skipTheTest = true;
        if (_onlyRunIfOptionsAllSet && !Options::get().all)
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

class LegacyConstantinopleGeneralStateTestFixture
  : public LegacyTestFixtureBase<StateTestSuiteLegacyConstantinople>
{
public:
    LegacyConstantinopleGeneralStateTestFixture() : LegacyTestFixtureBase(false) {}
};

class LegacyConstantinopleBCGeneralStateTestFixture
  : public LegacyTestFixtureBase<BCGeneralStateTestsSuiteLegacyConstantinople>
{
public:
    LegacyConstantinopleBCGeneralStateTestFixture() : LegacyTestFixtureBase(true) {}
};


BOOST_AUTO_TEST_SUITE(LegacyTests)
    BOOST_AUTO_TEST_SUITE(Constantinople)
        BOOST_FIXTURE_TEST_SUITE(GeneralStateTests, LegacyConstantinopleGeneralStateTestFixture)
            // Frontier Tests
            BOOST_AUTO_TEST_CASE(stCallCodes) {}
            BOOST_AUTO_TEST_CASE(stCallCreateCallCodeTest) {}
            BOOST_AUTO_TEST_CASE(stExample) {}
            BOOST_AUTO_TEST_CASE(stInitCodeTest) {}
            BOOST_AUTO_TEST_CASE(stLogTests) {}
            BOOST_AUTO_TEST_CASE(stMemoryTest) {}
            BOOST_AUTO_TEST_CASE(stPreCompiledContracts) {}
            BOOST_AUTO_TEST_CASE(stPreCompiledContracts2) {}
            BOOST_AUTO_TEST_CASE(stRandom) {}
            BOOST_AUTO_TEST_CASE(stRandom2) {}
            BOOST_AUTO_TEST_CASE(stRecursiveCreate) {}
            BOOST_AUTO_TEST_CASE(stRefundTest) {}
            BOOST_AUTO_TEST_CASE(stSolidityTest) {}
            BOOST_AUTO_TEST_CASE(stSpecialTest) {}
            BOOST_AUTO_TEST_CASE(stSystemOperationsTest) {}
            BOOST_AUTO_TEST_CASE(stTransactionTest) {}
            BOOST_AUTO_TEST_CASE(stTransitionTest) {}
            BOOST_AUTO_TEST_CASE(stWalletTest) {}

            // Homestead Tests
            BOOST_AUTO_TEST_CASE(stCallDelegateCodesCallCodeHomestead) {}
            BOOST_AUTO_TEST_CASE(stCallDelegateCodesHomestead) {}
            BOOST_AUTO_TEST_CASE(stHomesteadSpecific) {}
            BOOST_AUTO_TEST_CASE(stDelegatecallTestHomestead) {}

            // EIP150 Tests
            BOOST_AUTO_TEST_CASE(stChangedEIP150) {}
            BOOST_AUTO_TEST_CASE(stEIP150singleCodeGasPrices) {}
            BOOST_AUTO_TEST_CASE(stMemExpandingEIP150Calls) {}
            BOOST_AUTO_TEST_CASE(stEIP150Specific) {}

            // EIP158 Tests
            BOOST_AUTO_TEST_CASE(stEIP158Specific) {}
            BOOST_AUTO_TEST_CASE(stNonZeroCallsTest) {}
            BOOST_AUTO_TEST_CASE(stZeroCallsTest) {}
            BOOST_AUTO_TEST_CASE(stZeroCallsRevert) {}
            BOOST_AUTO_TEST_CASE(stCodeSizeLimit) {}
            BOOST_AUTO_TEST_CASE(stCreateTest) {}
            BOOST_AUTO_TEST_CASE(stRevertTest) {}

            // Metropolis Tests
            BOOST_AUTO_TEST_CASE(stStackTests) {}
            BOOST_AUTO_TEST_CASE(stStaticCall) {}
            BOOST_AUTO_TEST_CASE(stReturnDataTest) {}
            BOOST_AUTO_TEST_CASE(stZeroKnowledge) {}
            BOOST_AUTO_TEST_CASE(stZeroKnowledge2) {}
            BOOST_AUTO_TEST_CASE(stCodeCopyTest) {}
            BOOST_AUTO_TEST_CASE(stBugs) {}

            // Constantinople Tests
            BOOST_AUTO_TEST_CASE(stShift) {}
            BOOST_AUTO_TEST_CASE(stCreate2) {}
            BOOST_AUTO_TEST_CASE(stExtCodeHash) {}
            BOOST_AUTO_TEST_CASE(stSStoreTest) {}

            // Stress Tests
            BOOST_AUTO_TEST_CASE(stAttackTest) {}
            BOOST_AUTO_TEST_CASE(stMemoryStressTest) {}
            BOOST_AUTO_TEST_CASE(stQuadraticComplexityTest) {}

            // Invalid Opcode Tests
            BOOST_AUTO_TEST_CASE(stBadOpcode) {}

            // New Tests
            BOOST_AUTO_TEST_CASE(stArgsZeroOneBalance) {}
            BOOST_AUTO_TEST_CASE(stEWASMTests) {}
            BOOST_AUTO_TEST_CASE(stTimeConsuming) {}
        BOOST_AUTO_TEST_SUITE_END()  // GeneralStateTests Constantinople Legacy

        BOOST_FIXTURE_TEST_SUITE(BCGeneralStateTests, LegacyConstantinopleBCGeneralStateTestFixture)
            // Frontier Tests
            BOOST_AUTO_TEST_CASE(stCallCodes) {}
            BOOST_AUTO_TEST_CASE(stCallCreateCallCodeTest) {}
            BOOST_AUTO_TEST_CASE(stExample) {}
            BOOST_AUTO_TEST_CASE(stInitCodeTest) {}
            BOOST_AUTO_TEST_CASE(stLogTests) {}
            BOOST_AUTO_TEST_CASE(stMemoryTest) {}
            BOOST_AUTO_TEST_CASE(stPreCompiledContracts) {}
            BOOST_AUTO_TEST_CASE(stPreCompiledContracts2) {}
            BOOST_AUTO_TEST_CASE(stRandom) {}
            BOOST_AUTO_TEST_CASE(stRandom2) {}
            BOOST_AUTO_TEST_CASE(stRecursiveCreate) {}
            BOOST_AUTO_TEST_CASE(stRefundTest) {}
            BOOST_AUTO_TEST_CASE(stSolidityTest) {}
            BOOST_AUTO_TEST_CASE(stSpecialTest) {}
            BOOST_AUTO_TEST_CASE(stSystemOperationsTest) {}
            BOOST_AUTO_TEST_CASE(stTransactionTest) {}
            BOOST_AUTO_TEST_CASE(stTransitionTest) {}
            BOOST_AUTO_TEST_CASE(stWalletTest) {}

            // Homestead Tests
            BOOST_AUTO_TEST_CASE(stCallDelegateCodesCallCodeHomestead) {}
            BOOST_AUTO_TEST_CASE(stCallDelegateCodesHomestead) {}
            BOOST_AUTO_TEST_CASE(stHomesteadSpecific) {}
            BOOST_AUTO_TEST_CASE(stDelegatecallTestHomestead) {}

            // EIP150 Tests
            BOOST_AUTO_TEST_CASE(stChangedEIP150) {}
            BOOST_AUTO_TEST_CASE(stEIP150singleCodeGasPrices) {}
            BOOST_AUTO_TEST_CASE(stMemExpandingEIP150Calls) {}
            BOOST_AUTO_TEST_CASE(stEIP150Specific) {}

            // EIP158 Tests
            BOOST_AUTO_TEST_CASE(stEIP158Specific) {}
            BOOST_AUTO_TEST_CASE(stNonZeroCallsTest) {}
            BOOST_AUTO_TEST_CASE(stZeroCallsTest) {}
            BOOST_AUTO_TEST_CASE(stZeroCallsRevert) {}
            BOOST_AUTO_TEST_CASE(stCodeSizeLimit) {}
            BOOST_AUTO_TEST_CASE(stCreateTest) {}
            BOOST_AUTO_TEST_CASE(stRevertTest) {}

            // Metropolis Tests
            BOOST_AUTO_TEST_CASE(stStackTests) {}
            BOOST_AUTO_TEST_CASE(stStaticCall) {}
            BOOST_AUTO_TEST_CASE(stReturnDataTest) {}
            BOOST_AUTO_TEST_CASE(stZeroKnowledge) {}
            BOOST_AUTO_TEST_CASE(stZeroKnowledge2) {}
            BOOST_AUTO_TEST_CASE(stCodeCopyTest) {}
            BOOST_AUTO_TEST_CASE(stBugs) {}

            // Constantinople Tests
            BOOST_AUTO_TEST_CASE(stShift) {}
            BOOST_AUTO_TEST_CASE(stCreate2) {}
            BOOST_AUTO_TEST_CASE(stExtCodeHash) {}
            BOOST_AUTO_TEST_CASE(stSStoreTest) {}

            // Stress Tests
            BOOST_AUTO_TEST_CASE(stAttackTest) {}
            BOOST_AUTO_TEST_CASE(stMemoryStressTest) {}
            BOOST_AUTO_TEST_CASE(stQuadraticComplexityTest) {}

            // Bad opcodes test
            BOOST_AUTO_TEST_CASE(stBadOpcode) {}

            // New Tests
            BOOST_AUTO_TEST_CASE(stArgsZeroOneBalance) {}
            BOOST_AUTO_TEST_CASE(stTimeConsuming) {}
        BOOST_AUTO_TEST_SUITE_END()  // BCGeneralStateTests Constantinople Legacy

    BOOST_AUTO_TEST_SUITE_END()  // Constantinople
BOOST_AUTO_TEST_SUITE_END()  // LegacyTests
