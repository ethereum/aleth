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
/** @file transactionTests.cpp
 * @author Dmitrii Khokhlov <winsvega@mail.ru>
 * @date 2015
 * Transaction test functions.
 */

#include <libethcore/SealEngine.h>
#include <libethashseal/GenesisInfo.h>
#include <libethereum/ChainParams.h>
#include <test/libtestutils/Common.h>
#include <test/libtesteth/TestHelper.h>
#include <test/fuzzTesting/fuzzHelper.h>

using namespace std;
using namespace json_spirit;
using namespace dev;
using namespace dev::eth;

namespace dev {  namespace test {

void doTransactionTests(json_spirit::mValue& _v, bool _fillin)
{
	TestOutputHelper::initTest(_v);
	unique_ptr<SealEngineFace> se(ChainParams(genesisInfo(eth::Network::MainNetworkTest)).createSealEngine());
	for (auto& i: _v.get_obj())
	{
		string testname = i.first;
		json_spirit::mObject& o = i.second.get_obj();

		if (!TestOutputHelper::passTest(o, testname))
			continue;

		BOOST_REQUIRE(o.count("blocknumber") > 0);
		u256 transactionBlock = toInt(o["blocknumber"].get_str());
		BlockHeader bh;
		bh.setNumber(transactionBlock);

		if (_fillin)
		{
			BOOST_REQUIRE(o.count("transaction") > 0);
			mObject tObj = o["transaction"].get_obj();

			//Construct Rlp of the given transaction
			RLPStream rlpStream = createRLPStreamFromTransactionFields(tObj);
			o["rlp"] = toHex(rlpStream.out(), 2, HexPrefix::Add);

			try
			{
				Transaction txFromFields(rlpStream.out(), CheckTransaction::Everything);
				if (!txFromFields.signature().isValid())
					BOOST_THROW_EXCEPTION(Exception() << errinfo_comment(testname + "transaction from RLP signature is invalid") );
				se->verifyTransaction(ImportRequirements::Everything, txFromFields, bh);

				if (o.count("sender") > 0)
				{
					string expectSender = toString(o["sender"].get_str());
					BOOST_CHECK_MESSAGE(toString(txFromFields.sender()) == expectSender, "Error filling transaction test " + TestOutputHelper::testName() + ": expected another sender address! (got: " + toString(txFromFields.sender()) + "), expected: (" + expectSender + ")");
				}
				o["sender"] = toString(txFromFields.sender());
				o["transaction"] = ImportTest::makeAllFieldsHex(tObj);
				o["hash"] = toString(txFromFields.sha3());
			}
			catch(Exception const& _e)
			{
				//Transaction is InValid
				cnote << "Transaction Exception: " << diagnostic_information(_e);
				o.erase(o.find("transaction"));
				if (o.count("expect") > 0)
				{
					bool expectInValid = (o["expect"].get_str() == "invalid");
					if (Options::get().checkstate)
							BOOST_CHECK_MESSAGE(expectInValid, testname + "Check state: Transaction '" << i.first << "' is expected to be valid!");
						else
							BOOST_WARN_MESSAGE(expectInValid, testname + "Check state: Transaction '" << i.first << "' is expected to be valid!");

					o.erase(o.find("expect"));
				}
			}

			//Transaction is Valid
			if (o.count("expect") > 0)
			{
				bool expectValid = (o["expect"].get_str() == "valid");
				if (Options::get().checkstate)
						BOOST_CHECK_MESSAGE(expectValid, testname + "Check state: Transaction '" << i.first << "' is expected to be invalid!");
					else
						BOOST_WARN_MESSAGE(expectValid, testname + "Check state: Transaction '" << i.first << "' is expected to be invalid!");

				o.erase(o.find("expect"));
			}
		}
		else
		{
			BOOST_REQUIRE(o.count("rlp") > 0);
			Transaction txFromRlp;
			try
			{
				bytes stream = importByteArray(o["rlp"].get_str());
				RLP rlp(stream);
				txFromRlp = Transaction(rlp.data(), CheckTransaction::Everything);
				se->verifyTransaction(ImportRequirements::Everything, txFromRlp, bh);
				if (!txFromRlp.signature().isValid())
					BOOST_THROW_EXCEPTION(Exception() << errinfo_comment(testname + "transaction from RLP signature is invalid") );
			}
			catch(Exception const& _e)
			{
				cnote << i.first;
				cnote << "Transaction Exception: " << diagnostic_information(_e);
				BOOST_CHECK_MESSAGE(o.count("transaction") == 0, testname + "A transaction object should not be defined because the RLP is invalid!");
				continue;
			}
			catch(...)
			{
				BOOST_CHECK_MESSAGE(o.count("transaction") == 0, testname + "A transaction object should not be defined because the RLP is invalid!");
				continue;
			}

			BOOST_REQUIRE_MESSAGE(o.count("transaction") > 0, testname + "Expected a valid transaction!");

			mObject tObj = o["transaction"].get_obj();
			h256 txSha3Expected = h256(o["hash"].get_str());
			Transaction txFromFields(createRLPStreamFromTransactionFields(tObj).out(), CheckTransaction::Everything);

			//Check the fields restored from RLP to original fields
			BOOST_CHECK_MESSAGE(txFromFields.data() == txFromRlp.data(), testname + "Data in given RLP not matching the Transaction data!");
			BOOST_CHECK_MESSAGE(txFromFields.value() == txFromRlp.value(), testname + "Value in given RLP not matching the Transaction value!");
			BOOST_CHECK_MESSAGE(txFromFields.gasPrice() == txFromRlp.gasPrice(), testname + "GasPrice in given RLP not matching the Transaction gasPrice!");
			BOOST_CHECK_MESSAGE(txFromFields.gas() == txFromRlp.gas(), testname + "Gas in given RLP not matching the Transaction gas!");
			BOOST_CHECK_MESSAGE(txFromFields.nonce() == txFromRlp.nonce(), testname + "Nonce in given RLP not matching the Transaction nonce!");
			BOOST_CHECK_MESSAGE(txFromFields.receiveAddress() == txFromRlp.receiveAddress(), testname + "Receive address in given RLP not matching the Transaction 'to' address!");
			BOOST_CHECK_MESSAGE(txFromFields.sender() == txFromRlp.sender(), testname + "Transaction sender address in given RLP not matching the Transaction 'vrs' signature!");
			BOOST_CHECK_MESSAGE(txFromFields.sha3() == txFromRlp.sha3(), testname + "Transaction sha3 hash in given RLP not matching the Transaction 'vrs' signature!");
			BOOST_CHECK_MESSAGE(txFromFields.sha3() == txSha3Expected, testname + "Expected different transaction hash!");
			BOOST_CHECK_MESSAGE(txFromFields == txFromRlp, testname + "However, txFromFields != txFromRlp!");
			BOOST_REQUIRE (o.count("sender") > 0);

			Address addressReaded = Address(o["sender"].get_str());
			BOOST_CHECK_MESSAGE(txFromFields.sender() == addressReaded || txFromRlp.sender() == addressReaded, testname + "Signature address of sender does not match given sender address!");
		}
	}//for
	dev::test::TestOutputHelper::finishTest();
}//doTransactionTests

} }// Namespace Close


BOOST_AUTO_TEST_SUITE(TransactionTests)

BOOST_AUTO_TEST_CASE(ttMetropolisTests)
{
	dev::test::executeTests("ttMetropolisTest", "/TransactionTests/Metropolis", "/TransactionTestsFiller/Metropolis", dev::test::doTransactionTests);
}

BOOST_AUTO_TEST_CASE(ttTransactionTestEip155VitaliksTests)
{
	dev::test::executeTests("ttTransactionTestEip155VitaliksTests", "/TransactionTests/EIP155", "/TransactionTestsFiller/EIP155", dev::test::doTransactionTests);
}

BOOST_AUTO_TEST_CASE(ttTransactionTestEip155VCheck)
{
	dev::test::executeTests("ttTransactionTestVRule", "/TransactionTests/EIP155", "/TransactionTestsFiller/EIP155", dev::test::doTransactionTests);
}

BOOST_AUTO_TEST_CASE(ttTransactionTestEip155)
{
	dev::test::executeTests("ttTransactionTest", "/TransactionTests/EIP155", "/TransactionTestsFiller/EIP155", dev::test::doTransactionTests);
}

BOOST_AUTO_TEST_CASE(ttTransactionTestEip155VitaliksTestsHomestead)
{
	dev::test::executeTests("ttTransactionTestEip155VitaliksTests", "/TransactionTests/Homestead", "/TransactionTestsFiller/Homestead", dev::test::doTransactionTests);
}

BOOST_AUTO_TEST_CASE(ttTransactionTestHomestead)
{
	dev::test::executeTests("ttTransactionTest", "/TransactionTests/Homestead", "/TransactionTestsFiller/Homestead", dev::test::doTransactionTests);
}

BOOST_AUTO_TEST_CASE(ttTransactionTest)
{
	dev::test::executeTests("ttTransactionTest", "/TransactionTests", "/TransactionTestsFiller", dev::test::doTransactionTests);
}

BOOST_AUTO_TEST_CASE(ttWrongRLPTransactionHomestead)
{
	std::string fillersPath =  dev::test::getTestPath() + "/src/TransactionTestsFiller/Homestead";
	if (!dev::test::Options::get().filltests)
		dev::test::executeTests("ttWrongRLPTransaction", "/TransactionTests", "/TransactionTestsFiller/Homestead", dev::test::doTransactionTests);
	else
	{
		dev::test::TestOutputHelper::initTest();
		dev::test::copyFile(fillersPath + "/ttWrongRLPTransaction.json", dev::test::getTestPath() + "/TransactionTests/Homestead/ttWrongRLPTransaction.json");
	}
}

BOOST_AUTO_TEST_CASE(ttWrongRLPTransaction)
{
	std::string fillersPath = dev::test::getTestPath() + "/src/TransactionTestsFiller";
	if (!dev::test::Options::get().filltests)
		dev::test::executeTests("ttWrongRLPTransaction", "/TransactionTests", "/TransactionTestsFiller", dev::test::doTransactionTests);
	else
	{
		dev::test::TestOutputHelper::initTest();
		dev::test::copyFile(fillersPath + "/ttWrongRLPTransaction.json", dev::test::getTestPath() + "/TransactionTests/ttWrongRLPTransaction.json");
	}
}

BOOST_AUTO_TEST_CASE(tt10mbDataFieldHomestead)
{
	if (test::Options::get().bigData)
	{
		auto start = chrono::steady_clock::now();

		dev::test::executeTests("tt10mbDataField", "/TransactionTests/Homestead", "/TransactionTestsFiller/Homestead", dev::test::doTransactionTests);

		auto end = chrono::steady_clock::now();
		auto duration(chrono::duration_cast<chrono::milliseconds>(end - start));
		cnote << "test duration: " << duration.count() << " milliseconds.\n";
	}
}

BOOST_AUTO_TEST_CASE(tt10mbDataField)
{
	if (test::Options::get().bigData)
	{
		auto start = chrono::steady_clock::now();

		dev::test::executeTests("tt10mbDataField", "/TransactionTests", "/TransactionTestsFiller", dev::test::doTransactionTests);

		auto end = chrono::steady_clock::now();
		auto duration(chrono::duration_cast<chrono::milliseconds>(end - start));
		cnote << "test duration: " << duration.count() << " milliseconds.\n";
	}
}

BOOST_AUTO_TEST_CASE(userDefinedFile)
{
	dev::test::userDefinedTest(dev::test::doTransactionTests);
}

BOOST_AUTO_TEST_SUITE_END()
