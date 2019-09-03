/*
	This file is part of aleth.

	aleth is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	aleth is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with aleth.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file fuzzHelper.h
 * @author Dimitry Khokhlov <winsvega@mail.ru>
 * @date 2015
 */

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
