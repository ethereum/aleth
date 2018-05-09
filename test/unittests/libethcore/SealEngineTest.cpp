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
/** @file SealEngineTest.cpp
 * SealEngineFace class testing.
 */

#include <boost/test/unit_test.hpp>
#include <test/tools/libtesteth/TestHelper.h>
#include <libethashseal/Ethash.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

BOOST_FIXTURE_TEST_SUITE(SealEngineTests, TestOutputHelperFixture)

BOOST_AUTO_TEST_CASE(UnsignedTransactionIsValidBeforeConstantinople)
{
	ChainOperationParams params;
	params.constantinopleForkBlock = u256(0x1000);

	Ethash ethash;
	ethash.setChainParams(params);

	BlockHeader header;
	header.clear();
	header.setNumber(1);
    header.setGasLimit(22000);

    Transaction tx(0, 0, 21000, Address("a94f5374fce5edbc8e2a8697c15331677e6ebf0b"), bytes(), 0);
    ethash.SealEngineFace::verifyTransaction(
        ImportRequirements::TransactionSignatures, tx, header, 0);  // check that it doesn't throw
}

BOOST_AUTO_TEST_SUITE_END()
