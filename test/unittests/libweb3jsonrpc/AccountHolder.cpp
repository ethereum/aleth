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
/**
 * @author Christian R <c@ethdev.com>
 * @date 2015
 */

#include <boost/test/unit_test.hpp>
#include <libweb3jsonrpc/AccountHolder.h>
#include <test/tools/libtesteth/TestOutputHelper.h>

using namespace std;
using namespace dev;
using namespace eth;

namespace dev
{
namespace test
{

BOOST_FIXTURE_TEST_SUITE(AccountHolderTest, TestOutputHelper)

BOOST_AUTO_TEST_CASE(ProxyAccountUseCase)
{
	FixedAccountHolder h = FixedAccountHolder(function<Interface*()>(), vector<KeyPair>());

	BOOST_CHECK(h.allAccounts().empty());
	BOOST_CHECK(h.realAccounts().empty());
	Address addr("abababababababababababababababababababab");
	Address addr2("abababababababababababababababababababab");
	int id = h.addProxyAccount(addr);
	BOOST_CHECK(h.queuedTransactions(id).empty());
	// register it again
	int secondID = h.addProxyAccount(addr);
	BOOST_CHECK(h.queuedTransactions(secondID).empty());

	eth::TransactionSkeleton t1;
	eth::TransactionSkeleton t2;
	t1.from = addr;
	t1.data = fromHex("12345678");
	t2.from = addr;
	t2.data = fromHex("abcdef");
	BOOST_CHECK(h.queuedTransactions(id).empty());
	h.queueTransaction(t1);
	BOOST_CHECK_EQUAL(1, h.queuedTransactions(id).size());
	h.queueTransaction(t2);
	BOOST_REQUIRE_EQUAL(2, h.queuedTransactions(id).size());

	// second proxy should not see transactions
	BOOST_CHECK(h.queuedTransactions(secondID).empty());

	BOOST_CHECK(h.queuedTransactions(id)[0].data == t1.data);
	BOOST_CHECK(h.queuedTransactions(id)[1].data == t2.data);

	h.clearQueue(id);
	BOOST_CHECK(h.queuedTransactions(id).empty());
	// removing fails because it never existed
	BOOST_CHECK(!h.removeProxyAccount(secondID));
	BOOST_CHECK(h.removeProxyAccount(id));
}

BOOST_AUTO_TEST_SUITE_END()

}
}
