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
/** @file LegacyTestsBoost.h
 * @authors:
 *   Dimitry Khokhlov <dimitry@ethereum.org>
 * @date 2019
 */

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
      : StateTestFixtureBase(TestExecution::NotRefillable)
    {}
};

class LegacyConstantinopleBCGeneralStateTestFixture
  : public StateTestFixtureBase<BCGeneralStateTestsSuiteLegacyConstantinople>
{
public:
    LegacyConstantinopleBCGeneralStateTestFixture()
      : StateTestFixtureBase(TestExecution::RequireOptionAllNotRefillable)
    {}
};


}  // namespace test
}  // namespace dev
