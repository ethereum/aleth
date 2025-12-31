// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once
#include <string>
#include <test/tools/fuzzTesting/BoostRandomCode.h>

//Test Templates
extern std::string const c_testExampleStateTest;
extern std::string const c_testExampleTransactionTest;
extern std::string const c_testExampleVMTest;
extern std::string const c_testExampleBlockchainTest;
extern std::string const c_testExampleRLPTest;

namespace dev
{
namespace test
{

struct RlpDebug
{
	std::string rlp;
	int insertions;
};

class RandomCode
{
public:
	static BoostRandomCode& get()
	{
		static RandomCode instance;
		return instance.generator;
	}
	RandomCode(RandomCode const&) = delete;
	void operator=(RandomCode const&) = delete;

private:
	RandomCode() = default;
	BoostRandomCode generator;
};

}
}
