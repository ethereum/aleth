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
/** @file AccountDiff.cpp
 * @author Dimitry Khokhlov <Dimitry@ethdev.com>
 * @date 2015
 * libethereum unit test functions coverage.
 */

#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>
#include <test/TestHelper.h>
#include <json_spirit/JsonSpiritHeaders.h>

#include <libdevcore/TransientDirectory.h>

#include <libethereum/Defaults.h>
#include <libethereum/AccountDiff.h>
#include <libethereum/BlockChain.h>
#include <libethereum/BlockQueue.h>

using namespace dev;
using namespace eth;
using namespace dev::test;

BOOST_FIXTURE_TEST_SUITE(libethereum, TestOutputHelper)

BOOST_AUTO_TEST_CASE(AccountDiff)
{
	dev::eth::AccountDiff accDiff;

	// Testing changeType
	// exist = true	   exist_from = true		AccountChange::Deletion
	accDiff.exist = dev::Diff<bool>(true, false);
	BOOST_CHECK_MESSAGE(accDiff.changeType() == dev::eth::AccountChange::Deletion, "Account change type expected to be Deletion!");
	BOOST_CHECK_MESSAGE(strcmp(dev::eth::lead(accDiff.changeType()), "XXX") == 0, "Deletion lead expected to be 'XXX'!");

	// exist = true	   exist_from = false		AccountChange::Creation
	accDiff.exist = dev::Diff<bool>(false, true);
	BOOST_CHECK_MESSAGE(accDiff.changeType() == dev::eth::AccountChange::Creation, "Account change type expected to be Creation!");
	BOOST_CHECK_MESSAGE(strcmp(dev::eth::lead(accDiff.changeType()), "+++") == 0, "Creation lead expected to be '+++'!");

	// exist = false	   bn = true	sc = true	AccountChange::All
	accDiff.exist = dev::Diff<bool>(false, false);
	accDiff.nonce = dev::Diff<dev::u256>(1, 2);
	accDiff.code = dev::Diff<dev::bytes>(dev::fromHex("00"), dev::fromHex("01"));
	BOOST_CHECK_MESSAGE(accDiff.changeType() == dev::eth::AccountChange::All, "Account change type expected to be All!");
	BOOST_CHECK_MESSAGE(strcmp(dev::eth::lead(accDiff.changeType()), "***") == 0, "All lead expected to be '***'!");

	// exist = false	   bn = true	sc = false  AccountChange::Intrinsic
	accDiff.exist = dev::Diff<bool>(false, false);
	accDiff.nonce = dev::Diff<dev::u256>(1, 2);
	accDiff.code = dev::Diff<dev::bytes>(dev::fromHex("00"), dev::fromHex("00"));
	BOOST_CHECK_MESSAGE(accDiff.changeType() == dev::eth::AccountChange::Intrinsic, "Account change type expected to be Intrinsic!");
	BOOST_CHECK_MESSAGE(strcmp(dev::eth::lead(accDiff.changeType()), " * ") == 0, "Intrinsic lead expected to be ' * '!");

	// exist = false	   bn = false   sc = true	AccountChange::CodeStorage
	accDiff.exist = dev::Diff<bool>(false, false);
	accDiff.nonce = dev::Diff<dev::u256>(1, 1);
	accDiff.balance = dev::Diff<dev::u256>(1, 1);
	accDiff.code = dev::Diff<dev::bytes>(dev::fromHex("00"), dev::fromHex("01"));
	BOOST_CHECK_MESSAGE(accDiff.changeType() == dev::eth::AccountChange::CodeStorage, "Account change type expected to be CodeStorage!");
	BOOST_CHECK_MESSAGE(strcmp(dev::eth::lead(accDiff.changeType()), "* *") == 0, "CodeStorage lead expected to be '* *'!");

	// exist = false	   bn = false   sc = false	AccountChange::None
	accDiff.exist = dev::Diff<bool>(false, false);
	accDiff.nonce = dev::Diff<dev::u256>(1, 1);
	accDiff.balance = dev::Diff<dev::u256>(1, 1);
	accDiff.code = dev::Diff<dev::bytes>(dev::fromHex("00"), dev::fromHex("00"));
	BOOST_CHECK_MESSAGE(accDiff.changeType() == dev::eth::AccountChange::None, "Account change type expected to be None!");
	BOOST_CHECK_MESSAGE(strcmp(dev::eth::lead(accDiff.changeType()), "   ") == 0, "None lead expected to be '   '!");

	//ofstream
	accDiff.exist = dev::Diff<bool>(false, false);
	accDiff.nonce = dev::Diff<dev::u256>(1, 2);
	accDiff.balance = dev::Diff<dev::u256>(1, 2);
	accDiff.code = dev::Diff<dev::bytes>(dev::fromHex("00"), dev::fromHex("01"));
	std::map<dev::u256, dev::Diff<dev::u256>> storage;
	storage[1] = accDiff.nonce;
	accDiff.storage = storage;
	std::stringstream buffer;

	//if (!_s.exist.to())
	buffer << accDiff;
	BOOST_CHECK_MESSAGE(buffer.str() == "",	"Not expected output: '" + buffer.str() + "'");
	buffer.str(std::string());

	accDiff.exist = dev::Diff<bool>(false, true);
	buffer << accDiff;
	BOOST_CHECK_MESSAGE(buffer.str() == "#2 (+1) 2 (+1) $[1] ([0]) \n *     0000000000000000000000000000000000000000000000000000000000000001: 2 (1)",	"Not expected output: '" + buffer.str() + "'");
	buffer.str(std::string());

	storage[1] = dev::Diff<dev::u256>(0, 0);
	accDiff.storage = storage;
	buffer << accDiff;
	BOOST_CHECK_MESSAGE(buffer.str() == "#2 (+1) 2 (+1) $[1] ([0]) \n +     0000000000000000000000000000000000000000000000000000000000000001: 0",	"Not expected output: '" + buffer.str() + "'");
	buffer.str(std::string());

	storage[1] = dev::Diff<dev::u256>(1, 0);
	accDiff.storage = storage;
	buffer << accDiff;
	BOOST_CHECK_MESSAGE(buffer.str() == "#2 (+1) 2 (+1) $[1] ([0]) \nXXX    0000000000000000000000000000000000000000000000000000000000000001 (1)",	"Not expected output: '" + buffer.str() + "'");
	buffer.str(std::string());

	BOOST_CHECK_MESSAGE(accDiff.changed() == true, "dev::eth::AccountDiff::changed(): incorrect return value");

	//unexpected value
	//BOOST_CHECK_MESSAGE(strcmp(dev::eth::lead((dev::eth::AccountChange)123), "") != 0, "Not expected output when dev::eth::lead on unexpected value");
}

BOOST_AUTO_TEST_CASE(StateDiff)
{
	dev::eth::StateDiff stateDiff;
	dev::eth::AccountDiff accDiff;

	accDiff.exist = dev::Diff<bool>(false, false);
	accDiff.nonce = dev::Diff<dev::u256>(1, 2);
	accDiff.balance = dev::Diff<dev::u256>(1, 2);
	accDiff.code = dev::Diff<dev::bytes>(dev::fromHex("00"), dev::fromHex("01"));
	std::map<dev::u256, dev::Diff<dev::u256>> storage;
	storage[1] = accDiff.nonce;
	accDiff.storage = storage;
	std::stringstream buffer;

	dev::Address address("001122334455667788991011121314151617181920");
	stateDiff.accounts[address] = accDiff;
	buffer << stateDiff;

	BOOST_CHECK_MESSAGE(buffer.str() == "1 accounts changed:\n***  0000000000000000000000000000000000000000: \n",	"Not expected output: '" + buffer.str() + "'");
}

BOOST_AUTO_TEST_SUITE_END()
