#include <test/tools/jsontests/BlockChainTests.h>
#include <test/tools/jsontests/Common.h>
#include <test/tools/jsontests/StateTests.h>

namespace fs = boost::filesystem;
namespace dev
{
namespace test
{
class StateTestSuiteLegacyConstantinople : public StateTestSuite
{
public:
    boost::filesystem::path suiteFolder() const override
    {
        return "LegacyTests/Constantinople/GeneralStateTests";
    }
    boost::filesystem::path suiteFillerFolder() const override
    {
        return "LegacyTests/Constantinople/GeneralStateTestsFiller";
    }
};


class BCGeneralStateTestsSuiteLegacyConstantinople : public BCGeneralStateTestsSuite
{
    boost::filesystem::path suiteFolder() const override
    {
        return "LegacyTests/Constantinople/BlockchainTests/GeneralStateTests";
    }
    boost::filesystem::path suiteFillerFolder() const override
    {
        return "LegacyTests/Constantinople/BlockchainTestsFiller/GeneralStateTests";
    }
};


class LegacyConstantinopleGeneralStateTestFixture
  : public StateTestFixtureBase<StateTestSuiteLegacyConstantinople>
{
public:
    LegacyConstantinopleGeneralStateTestFixture()
      : StateTestFixtureBase(TestExecution::NOT_REFILLABLE)
    {}
};

class LegacyConstantinopleBCGeneralStateTestFixture
  : public StateTestFixtureBase<BCGeneralStateTestsSuiteLegacyConstantinople>
{
public:
    LegacyConstantinopleBCGeneralStateTestFixture()
      : StateTestFixtureBase(TestExecution::REQUIRE_OPTION_ALL_NOT_REFILLABLE)
    {}
};


}  // namespace test
}  // namespace dev
