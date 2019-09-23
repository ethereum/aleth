// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

/// @file
/// Transaction test functions.
#pragma once
#include <test/tools/libtesteth/TestSuite.h>
#include <boost/filesystem/path.hpp>

namespace dev
{
namespace test
{

class TransactionTestSuite: public TestSuite
{
	json_spirit::mValue doTests(json_spirit::mValue const& _input, bool _fillin) const override;
	boost::filesystem::path suiteFolder() const override;
	boost::filesystem::path suiteFillerFolder() const override;
};

}
}
