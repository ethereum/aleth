// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once
#include <test/tools/jsontests/StateTestFixtureBase.h>
#include <test/tools/libtesteth/TestSuite.h>
#include <boost/filesystem/path.hpp>

namespace dev
{
namespace test
{

class StateTestSuite: public TestSuite
{
public:
    json_spirit::mValue doTests(json_spirit::mValue const& _input, bool _fillin) const override;
    boost::filesystem::path suiteFolder() const override;
    boost::filesystem::path suiteFillerFolder() const override;
};


class GeneralTestFixture : public StateTestFixtureBase<StateTestSuite>
{
public:
    GeneralTestFixture() : StateTestFixtureBase({}) {}
};
}
}
