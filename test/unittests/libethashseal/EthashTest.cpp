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
/** @file ethash.cpp
 * Ethash class testing.
 */

#include <boost/test/unit_test.hpp>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <libethashseal/Ethash.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

BOOST_FIXTURE_TEST_SUITE(EthashTests, TestOutputHelperFixture)

// FIXME: Add a helper function here, because the test cases are almost identical.
// TODO: Add tests for Homestead difficulty change.

BOOST_AUTO_TEST_CASE(calculateDifficultyByzantiumWithoutUncles)
{
	ChainOperationParams params;
	params.homesteadForkBlock = 0;
	params.byzantiumForkBlock = u256(0x1000);

	Ethash ethash;
	ethash.setChainParams(params);

	BlockHeader parentHeader;
	parentHeader.clear();
	parentHeader.setNumber(0x2000);
	parentHeader.setTimestamp(100);
	parentHeader.setDifficulty(1000000);

	BlockHeader header;
	header.clear();
	header.setNumber(0x2001);
	header.setTimestamp(130);

	BOOST_CHECK_EQUAL(ethash.calculateDifficulty(header, parentHeader), 999024);
}

BOOST_AUTO_TEST_CASE(calculateDifficultyByzantiumWithUncles)
{
	ChainOperationParams params;
	params.homesteadForkBlock = 0;
	params.byzantiumForkBlock = u256(0x1000);

	Ethash ethash;
	ethash.setChainParams(params);

	BlockHeader parentHeader;
	parentHeader.clear();
	parentHeader.setNumber(0x2000);
	parentHeader.setTimestamp(100);
	parentHeader.setDifficulty(1000000);
	parentHeader.setSha3Uncles(h256("0x949d991d685738352398dff73219ab19c62c06e6f8ce899fbae755d5127ed1ef"));

	BlockHeader header;
	header.clear();
	header.setNumber(0x2001);
	header.setTimestamp(130);

	BOOST_CHECK_EQUAL(ethash.calculateDifficulty(header, parentHeader), 999512);
}

BOOST_AUTO_TEST_CASE(calculateDifficultyByzantiumMaxAdjustment)
{
	ChainOperationParams params;
	params.homesteadForkBlock = 0;
	params.byzantiumForkBlock = u256(0x1000);

	Ethash ethash;
	ethash.setChainParams(params);

	BlockHeader parentHeader;
	parentHeader.clear();
	parentHeader.setNumber(0x2000);
	parentHeader.setTimestamp(100);
	parentHeader.setDifficulty(1000000);

	BlockHeader header;
	header.clear();
	header.setNumber(0x2001);
	header.setTimestamp(1100);

	BOOST_CHECK_EQUAL(ethash.calculateDifficulty(header, parentHeader), 951688);
}

BOOST_AUTO_TEST_SUITE_END()
