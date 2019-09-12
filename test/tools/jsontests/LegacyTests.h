// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include <test/tools/jsontests/BlockChainTests.h>
#include <test/tools/jsontests/StateTestFixtureBase.h>
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
      : StateTestFixtureBase({TestExecution::NotRefillable})
    {}
};

class LegacyConstantinopleBCGeneralStateTestFixture
  : public StateTestFixtureBase<BCGeneralStateTestsSuiteLegacyConstantinople>
{
public:
    LegacyConstantinopleBCGeneralStateTestFixture()
      : StateTestFixtureBase({TestExecution::NotRefillable, TestExecution::RequireOptionAll})
    {}
};


}  // namespace test
}  // namespace dev
