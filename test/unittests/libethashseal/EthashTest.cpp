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
#include <test/libtesteth/TestHelper.h>
#include <libethashseal/Ethash.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

BOOST_FIXTURE_TEST_SUITE(EthashTests, TestOutputHelper)

BOOST_AUTO_TEST_CASE(verifyTransactionChecksNonceChainId)
{
	ChainOperationParams params;
	params.otherParams["metropolisForkBlock"] = "0x1000";
	params.otherParams["nonceChainID"] = "18";

	Ethash ethash;
	ethash.setChainParams(params);

	BlockHeader header;
	header.setNumber(0x2000);

	u256 invalidNonce = (u256(28) << 64) + 123;
	Transaction invalidTx(1, 1, 100000, Address(), bytes(), invalidNonce);

	BOOST_CHECK_THROW(ethash.verifyTransaction(ImportRequirements::Everything, invalidTx, header), InvalidChainIdInNonce);

	u256 validNonce = (u256(0x18) << 64) + 123;
	Transaction validTx(1, 1, 100000, Address(), bytes(), validNonce);

	ethash.verifyTransaction(ImportRequirements::Everything, validTx, header);
}

BOOST_AUTO_TEST_SUITE_END()
