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
/** @file transactionqueue.cpp
 * @author Christoph Jentzsch <cj@ethdev.com>
 * @date 2015
 * TransactionQueue test functions.
 */

#include <libethereum/TransactionQueue.h>
#include <test/libtesteth/TestHelper.h>
#include <test/libtesteth/BlockChainHelper.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

BOOST_FIXTURE_TEST_SUITE(TransactionQueueSuite, TestOutputHelper)

BOOST_AUTO_TEST_CASE(tqMaxNonce)
{
	dev::eth::TransactionQueue txq;

	// from a94f5374fce5edbc8e2a8697c15331677e6ebf0b
	const u256 gasCost =  10 * szabo;
	const u256 gas = 25000;
	Address dest = Address("0x095e7baea6a6c7c4c2dfeb977efac326af552d87");
	Address to = Address("0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b");
	Secret sec = Secret("0x45a915e4d060149eb4365960e6a7a45f334393093061116b197e3240065ff2d8");
	Transaction tx0(0, gasCost, gas, dest, bytes(), 0, sec );
	Transaction tx0_1(1, gasCost, gas, dest, bytes(), 0, sec );
	Transaction tx1(0, gasCost, gas, dest, bytes(), 1, sec );
	Transaction tx2(0, gasCost, gas, dest, bytes(), 2, sec );
	Transaction tx9(0, gasCost, gas, dest, bytes(), 9, sec );

	txq.import(tx0);
	BOOST_CHECK(1 == txq.maxNonce(to));
	txq.import(tx0);
	BOOST_CHECK(1 == txq.maxNonce(to));
	txq.import(tx0_1);
	BOOST_CHECK(1 == txq.maxNonce(to));
	txq.import(tx1);
	BOOST_CHECK(2 == txq.maxNonce(to));
	txq.import(tx9);
	BOOST_CHECK(10 == txq.maxNonce(to));
	txq.import(tx2);
	BOOST_CHECK(10 == txq.maxNonce(to));

}

BOOST_AUTO_TEST_CASE(tqPriority)
{
	dev::eth::TransactionQueue txq;

	const u256 gasCostCheap =  10 * szabo;
	const u256 gasCostMed =  20 * szabo;
	const u256 gasCostHigh =  30 * szabo;
	const u256 gas = 25000;
	Address dest = Address("0x095e7baea6a6c7c4c2dfeb977efac326af552d87");
	Secret sender1 = Secret("0x3333333333333333333333333333333333333333333333333333333333333333");
	Secret sender2 = Secret("0x4444444444444444444444444444444444444444444444444444444444444444");
	Transaction tx0(0, gasCostCheap, gas, dest, bytes(), 0, sender1 );
	Transaction tx0_1(1, gasCostMed, gas, dest, bytes(), 0, sender1 );
	Transaction tx1(0, gasCostCheap, gas, dest, bytes(), 1, sender1 );
	Transaction tx2(0, gasCostHigh, gas, dest, bytes(), 0, sender2 );
	Transaction tx3(0, gasCostCheap + 1, gas, dest, bytes(), 1, sender2 );
	Transaction tx4(0, gasCostHigh, gas, dest, bytes(), 2, sender1 );
	Transaction tx5(0, gasCostMed, gas, dest, bytes(), 2, sender2 );

	txq.import(tx0);
	BOOST_CHECK(Transactions { tx0 } == txq.topTransactions(256));
	txq.import(tx0);
	BOOST_CHECK(Transactions { tx0 } == txq.topTransactions(256));
	txq.import(tx0_1);
	BOOST_CHECK(Transactions { tx0_1 } == txq.topTransactions(256));
	txq.import(tx1);
	BOOST_CHECK((Transactions { tx0_1, tx1 }) == txq.topTransactions(256));
	txq.import(tx2);
	BOOST_CHECK((Transactions { tx2, tx0_1, tx1 }) == txq.topTransactions(256));
	txq.import(tx3);
	BOOST_CHECK((Transactions { tx2, tx0_1, tx1, tx3 }) == txq.topTransactions(256));
	txq.import(tx4);
	BOOST_CHECK((Transactions { tx2, tx0_1, tx1, tx3, tx4 }) == txq.topTransactions(256));
	txq.import(tx5);
	BOOST_CHECK((Transactions { tx2, tx0_1, tx1, tx3, tx5, tx4 }) == txq.topTransactions(256));

	txq.drop(tx0_1.sha3());
	BOOST_CHECK((Transactions { tx2, tx1, tx3, tx5, tx4 }) == txq.topTransactions(256));
	txq.drop(tx1.sha3());
	BOOST_CHECK((Transactions { tx2, tx3, tx5, tx4 }) == txq.topTransactions(256));
	txq.drop(tx5.sha3());
	BOOST_CHECK((Transactions { tx2, tx3, tx4 }) == txq.topTransactions(256));

	Transaction tx6(0, gasCostMed, gas, dest, bytes(), 20, sender1 );
	txq.import(tx6);
	BOOST_CHECK((Transactions { tx2, tx3, tx4, tx6 }) == txq.topTransactions(256));

	Transaction tx7(0, gasCostMed, gas, dest, bytes(), 2, sender2 );
	txq.import(tx7);
	// deterministic signature: hash of tx5 and tx7 will be same
	BOOST_CHECK((Transactions { tx2, tx3, tx4, tx6 }) == txq.topTransactions(256));

}

BOOST_AUTO_TEST_CASE(tqFuture)
{
	dev::eth::TransactionQueue txq;

	// from a94f5374fce5edbc8e2a8697c15331677e6ebf0b
	const u256 gasCostMed =  20 * szabo;
	const u256 gas = 25000;
	Address dest = Address("0x095e7baea6a6c7c4c2dfeb977efac326af552d87");
	Secret sender = Secret("0x3333333333333333333333333333333333333333333333333333333333333333");
	Transaction tx0(0, gasCostMed, gas, dest, bytes(), 0, sender );
	Transaction tx1(0, gasCostMed, gas, dest, bytes(), 1, sender );
	Transaction tx2(0, gasCostMed, gas, dest, bytes(), 2, sender );
	Transaction tx3(0, gasCostMed, gas, dest, bytes(), 3, sender );
	Transaction tx4(0, gasCostMed, gas, dest, bytes(), 4, sender );

	txq.import(tx0);
	txq.import(tx1);
	txq.import(tx2);
	txq.import(tx3);
	txq.import(tx4);
	BOOST_CHECK((Transactions { tx0, tx1, tx2, tx3, tx4 }) == txq.topTransactions(256));

	txq.setFuture(tx2.sha3());
	BOOST_CHECK((Transactions { tx0, tx1 }) == txq.topTransactions(256));

	Transaction tx2_2(1, gasCostMed, gas, dest, bytes(), 2, sender );
	txq.import(tx2_2);
	BOOST_CHECK((Transactions { tx0, tx1, tx2_2, tx3, tx4 }) == txq.topTransactions(256));
}


BOOST_AUTO_TEST_CASE(tqLimits)
{
	dev::eth::TransactionQueue txq(3, 3);
	const u256 gasCostMed =  20 * szabo;
	const u256 gas = 25000;
	Address dest = Address("0x095e7baea6a6c7c4c2dfeb977efac326af552d87");
	Secret sender = Secret("0x3333333333333333333333333333333333333333333333333333333333333333");
	Secret sender2 = Secret("0x4444444444444444444444444444444444444444444444444444444444444444");
	Transaction tx0(0, gasCostMed, gas, dest, bytes(), 0, sender );
	Transaction tx1(0, gasCostMed, gas, dest, bytes(), 1, sender );
	Transaction tx2(0, gasCostMed, gas, dest, bytes(), 2, sender );
	Transaction tx3(0, gasCostMed, gas, dest, bytes(), 3, sender );
	Transaction tx4(0, gasCostMed, gas, dest, bytes(), 4, sender );
	Transaction tx5(0, gasCostMed + 1, gas, dest, bytes(), 0, sender2 );

	txq.import(tx0);
	txq.import(tx1);
	txq.import(tx2);
	txq.import(tx3);
	txq.import(tx4);
	txq.import(tx5);
	BOOST_CHECK((Transactions { tx5, tx0, tx1 }) == txq.topTransactions(256));
}

BOOST_AUTO_TEST_CASE(tqOutput)
{
	BOOST_REQUIRE(string(TransactionQueueChannel().name()) == string(EthCyan "┉┅▶"));
}


BOOST_AUTO_TEST_CASE(tqImport)
{
	TestTransaction testTransaction = TestTransaction::defaultTransaction();
	TransactionQueue tq;
	h256Hash known = tq.knownTransactions();
	BOOST_REQUIRE(known.size() == 0);

	ImportResult ir = tq.import(testTransaction.transaction().rlp());
	BOOST_REQUIRE(ir == ImportResult::Success);
	known = tq.knownTransactions();
	BOOST_REQUIRE(known.size() == 1);

	ir = tq.import(testTransaction.transaction().rlp());
	BOOST_REQUIRE(ir == ImportResult::AlreadyKnown);

	bytes rlp = testTransaction.transaction().rlp();
	rlp.at(0) = 03;
	ir = tq.import(rlp);
	BOOST_REQUIRE(ir == ImportResult::Malformed);

	known = tq.knownTransactions();
	BOOST_REQUIRE(known.size() == 1);

	TestTransaction testTransaction2 = TestTransaction::defaultTransaction(1, 2);
	TestTransaction testTransaction3 = TestTransaction::defaultTransaction(1, 1);
	TestTransaction testTransaction4 = TestTransaction::defaultTransaction(1, 4);
	ir = tq.import(testTransaction2.transaction().rlp());
	ir = tq.import(testTransaction3.transaction().rlp());
	BOOST_REQUIRE(ir == ImportResult::OverbidGasPrice);
	ir = tq.import(testTransaction4.transaction().rlp());
	known = tq.knownTransactions();
	BOOST_REQUIRE(known.size() == 1);
	Transactions ts = tq.topTransactions(4);
	BOOST_REQUIRE(ts.size() == 1);
	BOOST_REQUIRE(Transaction(ts.at(0)).gasPrice() == 4);

	tq.setFuture(Transaction(ts.at(0)).sha3());
	Address from = Transaction(ts.at(0)).from();
	ts = tq.topTransactions(4);
	BOOST_REQUIRE(ts.size() == 0);
	BOOST_REQUIRE(tq.waiting(from) == 1);
}

BOOST_AUTO_TEST_CASE(tqDrop)
{
	TransactionQueue tq;
	TestTransaction testTransaction = TestTransaction::defaultTransaction();
	tq.dropGood(testTransaction.transaction());
	tq.import(testTransaction.transaction().rlp());
	BOOST_REQUIRE(tq.topTransactions(4).size() == 1);
	tq.dropGood(testTransaction.transaction());
	BOOST_REQUIRE(tq.topTransactions(4).size() == 0);
}

BOOST_AUTO_TEST_CASE(tqLimit)
{
	TransactionQueue tq(5, 3);
	Address from;
	for (size_t i = 1; i < 7; i++)
	{
		TestTransaction testTransaction = TestTransaction::defaultTransaction(i);
		ImportResult res = tq.import(testTransaction.transaction());
		from = testTransaction.transaction().from();
		BOOST_REQUIRE(res == ImportResult::Success);
	}

	//5 is imported and 6th is dropped
	BOOST_REQUIRE(tq.waiting(from) == 5);

	Transactions topTr = tq.topTransactions(10);
	BOOST_REQUIRE(topTr.size() == 5);

	for (int i = topTr.size() - 1; i >= 0 ; i--)
		tq.setFuture(topTr.at(i).sha3());

	topTr = tq.topTransactions(10);
	BOOST_REQUIRE(topTr.size() == 0);

	TestTransaction testTransaction = TestTransaction::defaultTransaction(7);
	BOOST_REQUIRE(tq.waiting(from) == 5);

	//Drop out of bound feauture
	ImportResult res = tq.import(testTransaction.transaction());
	BOOST_REQUIRE(res == ImportResult::Success);

	//future list size is now 3  + 1 imported transaction
	BOOST_REQUIRE(tq.waiting(testTransaction.transaction().from()) == 4);

	topTr = tq.topTransactions(10);
	BOOST_REQUIRE(topTr.size() == 1); // 1 imported transaction
}

BOOST_AUTO_TEST_CASE(tqEqueue)
{
	TransactionQueue tq;
	TestTransaction testTransaction = TestTransaction::defaultTransaction();

	bytes payloadToDecode = testTransaction.transaction().rlp();

	RLPStream rlpStream(2);
	rlpStream.appendRaw(payloadToDecode);
	rlpStream.appendRaw(payloadToDecode);

	RLP tRlp(rlpStream.out());

	//check that Transaction could be recreated from the RLP
	Transaction tRlpTransaction(payloadToDecode, CheckTransaction::Cheap);
	BOOST_REQUIRE(tRlpTransaction.data() == testTransaction.transaction().data());

	//try to import transactions
	string hashStr = "01020304050607080910111213141516171819202122232425262728293031320102030405060708091011121314151617181920212223242526272829303132";
	tq.enqueue(tRlp, h512(hashStr));
	tq.enqueue(tRlp, h512(hashStr));
	this_thread::sleep_for(chrono::seconds(1));

	//at least 1 transaction should be imported through RLP
	Transactions topTr = tq.topTransactions(10);
	BOOST_REQUIRE(topTr.size() == 1);
}

BOOST_AUTO_TEST_SUITE_END()
