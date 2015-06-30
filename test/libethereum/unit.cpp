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
/** @file unit.cpp
 * @author Dimitry Khokhlov <Dimitry@ethdev.com>
 * @date 2015
 * libethereum unit test functions coverage.
 */

#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>
#include <test/TestHelper.h>
#include "../JsonSpiritHeaders.h"

#include <libdevcore/TransientDirectory.h>

#include <libethereum/Defaults.h>
#include <libethereum/AccountDiff.h>
#include <libethereum/BlockChain.h>
#include <libethereum/BlockQueue.h>

using namespace dev;
using namespace eth;

std::string const c_genesisInfoTest =
R"ETHEREUM(
{
	"a94f5374fce5edbc8e2a8697c15331677e6ebf0b": { "wei": "1606938044258990275541962092341162602522202993782792835301376" },
	"0000000000000000000000000000000000000001": { "wei": "1" },
	"0000000000000000000000000000000000000002": { "wei": "1" },
	"0000000000000000000000000000000000000003": { "wei": "1" },
	"0000000000000000000000000000000000000004": { "wei": "1" },
	"dbdbdb2cbd23b783741e8d7fcf51e459b497e4a6": { "wei": "1606938044258990275541962092341162602522202993782792835301376" },
	"e6716f9544a56c530d868e4bfbacb172315bdead": { "wei": "1606938044258990275541962092341162602522202993782792835301376" },
	"b9c015918bdaba24b4ff057a92a3873d6eb201be": { "wei": "1606938044258990275541962092341162602522202993782792835301376" },
	"1a26338f0d905e295fccb71fa9ea849ffa12aaf4": { "wei": "1606938044258990275541962092341162602522202993782792835301376" },
	"2ef47100e0787b915105fd5e3f4ff6752079d5cb": { "wei": "1606938044258990275541962092341162602522202993782792835301376" },
	"cd2a3d9f938e13cd947ec05abc7fe734df8dd826": { "wei": "1606938044258990275541962092341162602522202993782792835301376" },
	"6c386a4b26f73c802f34673f7248bb118f97424a": { "wei": "1606938044258990275541962092341162602522202993782792835301376" },
	"e4157b34ea9615cfbde6b4fda419828124b70c78": { "wei": "1606938044258990275541962092341162602522202993782792835301376" },
}
)ETHEREUM";


BOOST_AUTO_TEST_SUITE(libethereum)

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
	BOOST_CHECK_MESSAGE(strcmp(dev::eth::lead((dev::eth::AccountChange)123), "") != 0, "Not expected output when dev::eth::lead on unexpected value");
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


BOOST_AUTO_TEST_CASE(BlockChain)
{
	std::string genesisRLP = "0xf901fcf901f7a00000000000000000000000000000000000000000000000000000000000000000a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347948888f1f195afa192cfee860698584c030f4c9db1a07dba07d6b448a186e9612e5f737d1c909dce473e53199901a302c00646d523c1a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421b90100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000008302000080832fefd8808454c98c8142a0b28caac6b4c3c1f231ff6e4210a78d3b784988be77d89095038b843fb04440408821dbba66d491fb5ac0c0";
	dev::bytes genesisBlockRLP = dev::test::importByteArray(genesisRLP);
	BlockInfo biGenesisBlock(genesisBlockRLP);

	std::string blockRLP = "0xf90261f901f9a0cebd6f0700c0b61e4e4df58927aaf30fc9b1b068e8e90f4a907750d5dc55bb5ea01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347948888f1f195afa192cfee860698584c030f4c9db1a02c15e8b5cb6cf880faa558c76c02a591e181ef2c4450ad92e53ae6c21093dc70a0d64268d0cf50c7402dfa922dfef857526087721faea43f984bde81044b73f25ea0e9244cf7503b79c03d3a099e07a80d2dbc77bb0b502d8a89d51ac0d68dd31313b90100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000008302000001832fefd88252088455913b6e80a00af24273f5b8d5e98cb26323b1b8ddf2425d9169a41b0cf0e90b79ac39ba12f6886c6967e7d5ed2cbbf862f86080018304cb2f94095e7baea6a6c7c4c2dfeb977efac326af552d870a801ca0d61ec9388763ba5054420c9a21283d3be2dfa92624546cd19a45bf68fa905a2fa0c627aa5733f030cce584c3eb4ed2114e1afb9f5daaa68368ccb7f19f311cb2b7c0";
	std::string blockRLP2 = "0xf90261f901f9a01a8d9583240f49503c6334eff4377876df729da2ba394bb9b581e824b460ed3da01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347948888f1f195afa192cfee860698584c030f4c9db1a0a2a5e3d96e902272adb58e90c364f7b92684c539e0ded77356cf33d966f917fea0118aecddf3ec17a9871daab30786f7a62f5e26a8392c2fd0a626733659b39e24a05a750181d80a2b69fac54c1b2f7a37ebc4666ea5320e25db6603b90958686b27b90100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000008302004002832fefd88252088455913b7280a033db95aaf723724e248cf8368832543af8b7414a0983ed8d63ad66108a39650a88e312f3f2c145457ef862f86001018304cb2f94095e7baea6a6c7c4c2dfeb977efac326af552d870a801ba0a4a48ef42321f6fcb00e39dba909f8713aa6747543fb8a446948bd13488ba861a074e01427e3884274a004e14806438e60d3044f68361c8cf429c71972df8f171ec0";
	dev::bytes blockRLPbytes = dev::test::importByteArray(blockRLP);
	dev::bytes blockRLPbytes2 = dev::test::importByteArray(blockRLP2);
	BlockInfo biBlock(blockRLPbytes);

	TransientDirectory td1, td2;
	State trueState(OverlayDB(State::openDB(td1.path())), BaseState::CanonGenesis, biGenesisBlock.coinbaseAddress);
	dev::eth::BlockChain* trueBc = new dev::eth::BlockChain(genesisBlockRLP, td2.path(), WithExisting::Verify);
	//dev::eth::BlockChain* trueBc = new dev::eth::CanonBlockChain (genesisBlockRLP, td2.path(), WithExisting::Verify);

	/*json_spirit::mObject o;
	json_spirit::mObject accountaddress;
	json_spirit::mObject account;
	account["balance"] = "0x09184e72a000";
	account["code"] = "0x";
	account["nonce"] = "0x00";
	account["storage"] = json_spirit::mObject();
	accountaddress["a94f5374fce5edbc8e2a8697c15331677e6ebf0b"] = account;
	o["pre"] = accountaddress;

	dev::test::ImportTest importer(o["pre"].get_obj());
	importer.importState(o["pre"].get_obj(), trueState);
	trueState.commit();*/

	/*eth::GasPricer gp;
	TransactionQueue txs;
	importer.importTransaction(tx);*/
	//transaction
	//block
	//blockchain of blocks

	//bc = blockchain
	//bl = block
	//bl.import(tr)
	//bc.import(bl)


	//trueState.sync(trueBc);
	//trueState.sync(trueBc, txs, gp);

	trueBc->import(blockRLPbytes, trueState.db());

	std::stringstream buffer;
	buffer << *trueBc;
	BOOST_CHECK_MESSAGE(buffer.str() == "1a8d9583240f49503c6334eff4377876df729da2ba394bb9b581e824b460ed3d00:   1 @ cebd6f0700c0b61e4e4df58927aaf30fc9b1b068e8e90f4a907750d5dc55bb5e\n",	"Not expected output: '" + buffer.str() + "'");

	trueBc->garbageCollect(true);
	trueBc->import(blockRLPbytes2, trueState.db());
	delete trueBc;

	dev::eth::BlockChain trueBcReOpen(genesisBlockRLP, td2.path(), WithExisting::Verify);

	//Block Queue Test
	/*BlockQueue bcQueue;
	bcQueue.tick(trueBc);
	QueueStatus bStatus = bcQueue.blockStatus(trueBc.info().hash());
	BOOST_CHECK(bStatus == QueueStatus::Unknown);

	dev::bytesConstRef bytesConst(&blockRLPbytes.at(0), blockRLPbytes.size());
	bcQueue.import(bytesConst, trueBc);
	bStatus = bcQueue.blockStatus(biBlock.hash());
	BOOST_CHECK(bStatus == QueueStatus::Unknown);

	bcQueue.clear();*/
}

BOOST_AUTO_TEST_CASE(BlockQueue)
{

}

BOOST_AUTO_TEST_SUITE_END()
