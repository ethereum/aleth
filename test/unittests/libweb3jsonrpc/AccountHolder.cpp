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

#include <libweb3jsonrpc/AccountHolder.h>

#include <gtest/gtest.h>

using namespace std;
using namespace dev;
using namespace eth;

TEST(AccountHolder, proxyAccountUseCase)
{
    FixedAccountHolder h = FixedAccountHolder(function<Interface*()>(), vector<KeyPair>());

    EXPECT_TRUE(h.allAccounts().empty());
    EXPECT_TRUE(h.realAccounts().empty());
    Address addr("abababababababababababababababababababab");
    Address addr2("abababababababababababababababababababab");
    int id = h.addProxyAccount(addr);
    EXPECT_TRUE(h.queuedTransactions(id).empty());
    // register it again
    int secondID = h.addProxyAccount(addr);
    EXPECT_TRUE(h.queuedTransactions(secondID).empty());

    eth::TransactionSkeleton t1;
    eth::TransactionSkeleton t2;
    t1.from = addr;
    t1.data = fromHex("12345678");
    t2.from = addr;
    t2.data = fromHex("abcdef");
    EXPECT_TRUE(h.queuedTransactions(id).empty());
    h.queueTransaction(t1);
    EXPECT_EQ(1, h.queuedTransactions(id).size());
    h.queueTransaction(t2);
    ASSERT_EQ(2, h.queuedTransactions(id).size());

    // second proxy should not see transactions
    EXPECT_TRUE(h.queuedTransactions(secondID).empty());

    EXPECT_TRUE(h.queuedTransactions(id)[0].data == t1.data);
    EXPECT_TRUE(h.queuedTransactions(id)[1].data == t2.data);

    h.clearQueue(id);
    EXPECT_TRUE(h.queuedTransactions(id).empty());
    // removing fails because it never existed
    EXPECT_TRUE(!h.removeProxyAccount(secondID));
    EXPECT_TRUE(h.removeProxyAccount(id));
}
