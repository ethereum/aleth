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
/** @file blockqueue.cpp
 * @author Dimitry Khokhlov <dimitry@ethdev.com>
 * @date 2015
 * BlockQueue test functions.
 */

#include <libethereum/BlockQueue.h>
#include <test/libtesteth/TestHelper.h>
#include <test/libtesteth/BlockChainHelper.h>
#include <test/libtesteth/JsonSpiritHeaders.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::test;

//BOOST_FIXTURE_TEST_SUITE(BlockQueueSuite, TestOutputHelper)

//BOOST_AUTO_TEST_CASE(BlockQueueImport)
//{
//	TestBlock genesisBlock = TestBlockChain::defaultGenesisBlock();
//	TestBlockChain blockchain(genesisBlock);
//	TestBlockChain blockchain2(genesisBlock);

//	TestBlock block1;
//	TestTransaction transaction1 = TestTransaction::defaultTransaction(1);
//	block1.addTransaction(transaction1);
//	block1.mine(blockchain);

//	BlockQueue blockQueue;
//	blockQueue.setChain(blockchain.interface());
//	ImportResult res = blockQueue.import(&block1.bytes());
//	BOOST_REQUIRE_MESSAGE(res == ImportResult::Success, "Simple block import to BlockQueue should have return Success");

//	res = blockQueue.import(&block1.bytes());
//	BOOST_REQUIRE_MESSAGE(res == ImportResult::AlreadyKnown, "Simple block import to BlockQueue should have return AlreadyKnown");

//	blockQueue.clear();
//	blockchain.addBlock(block1);
//	res = blockQueue.import(&block1.bytes());
//	BOOST_REQUIRE_MESSAGE(res == ImportResult::AlreadyInChain, "Simple block import to BlockQueue should have return AlreadyInChain");

//	TestBlock block2;
//	block2.mine(blockchain);
//	BlockHeader block2Header = block2.blockHeader();
//	block2Header.setTimestamp(block2Header.timestamp() + 100);
//	block2.setBlockHeader(block2Header);
//	block2.updateNonce(blockchain);
//	res = blockQueue.import(&block2.bytes());
//	BOOST_REQUIRE_MESSAGE(res == ImportResult::FutureTimeKnown, "Simple block import to BlockQueue should have return FutureTimeKnown");

//	TestBlock block3;
//	block3.mine(blockchain);
//	BlockHeader block3Header = block3.blockHeader();
//	block3Header.clear();
//	block3.setBlockHeader(block3Header);
//	res = blockQueue.import(&block3.bytes());
//	BOOST_REQUIRE_MESSAGE(res == ImportResult::Malformed, "Simple block import to BlockQueue should have return Malformed");

//	TestBlock block4;
//	block4.mine(blockchain2);
//	blockchain2.addBlock(block4);
//	TestBlock block5;
//	block5.mine(blockchain2);
//	BlockHeader block5Header = block5.blockHeader();
//	block5Header.setTimestamp(block5Header.timestamp() + 100);
//	block5.setBlockHeader(block5Header);
//	block5.updateNonce(blockchain2);
//	res = blockQueue.import(&block5.bytes());
//	BOOST_REQUIRE_MESSAGE(res == ImportResult::FutureTimeUnknown, "Simple block import to BlockQueue should have return FutureTimeUnknown");

//	blockQueue.clear();
//	TestBlock block3a;
//	block3a.mine(blockchain2);
//	blockchain2.addBlock(block3a);

//	TestBlock block3b;
//	block3b.mine(blockchain2);
//	BlockHeader block3bHeader = block3b.blockHeader();
//	block3bHeader.setTimestamp(block3bHeader.timestamp() - 40);
//	block3b.setBlockHeader(block3bHeader);
//	block3b.updateNonce(blockchain2);
//	res = blockQueue.import(&block3b.bytes());
//	BOOST_REQUIRE_MESSAGE(res == ImportResult::UnknownParent, "Simple block import to BlockQueue should have return UnknownParent");
//}

//BOOST_AUTO_TEST_SUITE_END()
