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
/** @file createRandomTest.cpp
 * @author Dimitry Khokhlov <winsvega@mail.ru>
 * @date 2015
 */

#include <string>
#include <iostream>

#include <libdevcore/CommonData.h>
#include <test/libtesteth/TestHelper.h>
#include <test/fuzzTesting/fuzzHelper.h>
#include <libevm/VMFactory.h>
#include <libdevcore/Common.h>

//String Variables
extern std::string const c_testExampleStateTest;
extern std::string const c_testExampleTransactionTest;
extern std::string const c_testExampleVMTest;
extern std::string const c_testExampleBlockchainTest;
extern std::string const c_testExampleRLPTest;

//Main Test functinos
void fillRandomTest(std::function<void(json_spirit::mValue&, bool)> _doTests, std::string const& _testString, bool _debug = false);
int checkRandomTest(std::function<void(json_spirit::mValue&, bool)> _doTests, json_spirit::mValue& _value, bool _debug = false);

namespace dev { namespace test {
int createRandomTest(std::vector<char*> const& _parameters)
{
	std::string testSuite;
	std::string testFillString;
	json_spirit::mValue testmValue;
	bool checktest = false;
	bool filldebug = false;
	bool debug = false;
	bool filltest = false;

	dev::test::Options& options = const_cast<dev::test::Options&>(dev::test::Options::get());

	testSuite = options.rCurrentTestSuite;
	if (testSuite != "BlockChainTests" && testSuite != "TransactionTests" && testSuite != "StateTests"
				&& testSuite != "VMTests")
		testSuite = "";

	for (size_t i = 0; i < _parameters.size(); ++i)
	{
		auto arg = std::string{_parameters.at(i)};

		if (arg == "--fulloutput")
			options.fulloutput = true;
		else
		if (arg == "--checktest" || arg == "--filltest")
		{
			std::string s = options.rCheckTest;
			BOOST_REQUIRE_MESSAGE(s.length() > 0, "Error! Content of argument is empty! (Usage -checktest textstream)");

			if (arg == "--filltest")
			{
				testFillString = s;
				filltest = true;
			}
			else
			{
				if (s.find(".json") != std::string::npos)
				{
					boost::filesystem::path p(__FILE__);
					s = asString(dev::contents(s));
				}
				read_string(s, testmValue);
				checktest = true;
			}
		}
		else
		if (arg == "--debug")
			debug = true;
		else
		if (arg == "--filldebug")
			filldebug = true;
	}

	if (testSuite == "")
	{
		std::cerr << "Error! Test suite not supported! (Usage -t TestSuite)" << std::endl;
		return 1;
	}
	else
	{
		if (checktest)
			std::cout << "Testing: " << testSuite.substr(0, testSuite.length() - 1) << "... ";

		if (testSuite == "BlockChainTests")
		{
			if (checktest)
				return checkRandomTest(dev::test::doBlockchainTests, testmValue, debug);
			else
				fillRandomTest(dev::test::doBlockchainTests, (filltest) ? testFillString : c_testExampleBlockchainTest, filldebug);
		}
		else
		if (testSuite == "TransactionTests")
		{
			if (checktest)
				return checkRandomTest(dev::test::doTransactionTests, testmValue, debug);
			else
				fillRandomTest(dev::test::doTransactionTests, (filltest) ? testFillString : c_testExampleTransactionTest, filldebug);
		}
		else
		if (testSuite == "VMTests")
		{
			if (checktest)
			{
				dev::eth::VMFactory::setKind(dev::eth::VMKind::JIT);
				return checkRandomTest(dev::test::doVMTests, testmValue, debug);
			}
			else
				fillRandomTest(dev::test::doVMTests, (filltest) ? testFillString : c_testExampleVMTest, filldebug);
		}
	}

	return 0;
}
}} //namespaces

int checkRandomTest(std::function<void(json_spirit::mValue&, bool)> _doTests, json_spirit::mValue& _value, bool _debug)
{
	bool ret = 0;
	try
	{
		//redirect all output to the stream
		std::ostringstream strCout;
		std::streambuf* oldCoutStreamBuf = std::cout.rdbuf();
		if (!_debug)
		{
			std::cout.rdbuf( strCout.rdbuf() );
			std::cerr.rdbuf( strCout.rdbuf() );
		}

		_doTests(_value, false);

		//restroe output
		if (!_debug)
		{
			std::cout.rdbuf(oldCoutStreamBuf);
			std::cerr.rdbuf(oldCoutStreamBuf);
		}
	}
	catch (dev::Exception const& _e)
	{
		std::cout << "Failed test with Exception: " << diagnostic_information(_e) << std::endl;
		ret = 1;
	}
	catch (std::exception const& _e)
	{
		std::cout << "Failed test with Exception: " << _e.what() << std::endl;
		ret = 1;
	}
	return ret;
}

void fillRandomTest(std::function<void(json_spirit::mValue&, bool)> _doTests, std::string const& _testString, bool _debug)
{
	//redirect all output to the stream
	std::ostringstream strCout;
	std::streambuf* oldCoutStreamBuf = std::cout.rdbuf();
	if (!_debug)
	{
		std::cout.rdbuf( strCout.rdbuf() );
		std::cerr.rdbuf( strCout.rdbuf() );
	}

	json_spirit::mValue v;
	try
	{
		std::string newTest = _testString;
		std::map<std::string, std::string> nullReplaceMap;
		dev::test::RandomCode::parseTestWithTypes(newTest, nullReplaceMap);
		json_spirit::read_string(newTest, v);
		_doTests(v, true);
	}
	catch(...)
	{
		std::cerr << "Test fill exception!";
	}

	//restroe output
	if (!_debug)
	{
		std::cout.rdbuf(oldCoutStreamBuf);
		std::cerr.rdbuf(oldCoutStreamBuf);
	}
	std::cout << json_spirit::write_string(v, true);
}

/// Parse Test string replacing keywords to fuzzed values
void dev::test::RandomCode::parseTestWithTypes(std::string& _test, std::map<std::string, std::string> const& _varMap)
{
	dev::test::RandomCodeOptions options;
	options.setWeight(dev::eth::Instruction::STOP, 10);		//default 50
	options.setWeight(dev::eth::Instruction::SSTORE, 70);
	options.setWeight(dev::eth::Instruction::CALL, 75);
	options.setWeight(dev::eth::Instruction::CALLCODE, 55);
	options.addAddress(dev::Address("0xffffffffffffffffffffffffffffffffffffffff"));
	options.addAddress(dev::Address("0x1000000000000000000000000000000000000000"));
	options.addAddress(dev::Address("0x095e7baea6a6c7c4c2dfeb977efac326af552d87"));
	options.addAddress(dev::Address("0x945304eb96065b2a98b57a48a06ae28d285a71b5"));
	options.addAddress(dev::Address("0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b"));
	options.addAddress(dev::Address("0x0f572e5295c57f15886f9b263e2f6d2d6c7b5ec6"));
	options.addAddress(dev::Address("0x0000000000000000000000000000000000000001"));
	options.addAddress(dev::Address("0x0000000000000000000000000000000000000002"));
	options.addAddress(dev::Address("0x0000000000000000000000000000000000000003"));
	options.addAddress(dev::Address("0x0000000000000000000000000000000000000004"));
	options.smartCodeProbability = 60;

	std::vector<std::string> types = getTypes();

	for (std::map<std::string, std::string>::const_iterator it = _varMap.begin(); it != _varMap.end(); it++)
		types.push_back(it->first);

	for (unsigned i = 0; i < types.size(); i++)
	{
		std::size_t pos = _test.find(types.at(i));
		while (pos != std::string::npos)
		{
			if (types.at(i) == "[RLP]")
			{
				std::string debug;
				int randomDepth = 1 + (int)dev::test::RandomCode::randomUniInt() % 10;
				_test.replace(pos, 5, dev::test::RandomCode::rndRLPSequence(randomDepth, debug));
				cnote << debug;
			}
			else
			if (types.at(i) == "[CODE]")
				_test.replace(pos, 6, "0x"+dev::test::RandomCode::generate(10, options));
			else
			if (types.at(i) == "[HEX]")
				_test.replace(pos, 5, dev::test::RandomCode::randomUniIntHex());
			else
			if (types.at(i) == "[HEX32]")
				_test.replace(pos, 7, dev::test::RandomCode::randomUniIntHex(std::numeric_limits<uint32_t>::max()));
			else
			if (types.at(i) == "[GASLIMIT]")
				_test.replace(pos, 10, dev::test::RandomCode::randomUniIntHex(dev::u256("3000000000")));
			else
			if (types.at(i) == "[HASH20]")
				_test.replace(pos, 8, dev::test::RandomCode::rndByteSequence(20));
			else
			if (types.at(i) == "[0xHASH32]")
				_test.replace(pos, 10, "0x" + dev::test::RandomCode::rndByteSequence(32));
			else
			if (types.at(i) == "[HASH32]")
				_test.replace(pos, 8, dev::test::RandomCode::rndByteSequence(32));
			else
			if (types.at(i) == "[V]")
			{
				int random = (int)dev::test::RandomCode::randomUniInt() % 100;
				if (random < 30)
					_test.replace(pos, 3, "0x1c");
				else
				if (random < 60)
					_test.replace(pos, 3, "0x1d");
				else
					_test.replace(pos, 3, "0x" + dev::test::RandomCode::rndByteSequence(1));
			}
			else
				_test.replace(pos, types.at(i).size(), _varMap.at(types.at(i)));

			pos = _test.find(types.at(i));
		}
	}
}

std::vector<std::string> dev::test::RandomCode::getTypes()
{
	return {"[RLP]", "[CODE]", "[HEX]", "[HEX32]", "[HASH20]", "[HASH32]", "[0xHASH32]", "[V]", "[GASLIMIT]"};
}

std::string const c_testExampleTransactionTest = R"(
{
	"randomTransactionTest" : {
		"transaction" :
		{
			"data" : "[CODE]",
			"gasLimit" : "[HEX]",
			"gasPrice" : "[HEX]",
			"nonce" : "[HEX]",
			"to" : "[HASH20]",
			"value" : "[HEX]",
			"v" : "[V]",
			"r" : "[0xHASH32]",
			"s" : "[0xHASH32]"
		}
	}
}
)";

std::string const c_testExampleStateTest = R"(
{
	"randomStatetest" : {
		"env" : {
		"currentCoinbase" : "[HASH20]",
		"currentDifficulty" : "[HEX]",
		"currentGasLimit" : "[GASLIMIT]",
		"currentNumber" : "[HEX32]",
		"currentTimestamp" : "[HEX]",
		"previousHash" : "[HASH32]"
		},
	"pre" : {
		"095e7baea6a6c7c4c2dfeb977efac326af552d87" : {
			"balance" : "[HEX]",
			"code" : "[CODE]",
			"nonce" : "[V]",
			"storage" : {
			}
		},
		"945304eb96065b2a98b57a48a06ae28d285a71b5" : {
			"balance" : "[HEX]",
			"code" : "[CODE]",
			"nonce" : "[V]",
			"storage" : {
			}
		},
		"a94f5374fce5edbc8e2a8697c15331677e6ebf0b" : {
			"balance" : "[HEX]",
			"code" : "0x",
			"nonce" : "0",
			"storage" : {
			}
		}
	},
	"transaction" : {
		"data" : "[CODE]",
		"gasLimit" : "[HEX]",
		"gasPrice" : "[HEX32]",
		"nonce" : "0",
		"secretKey" : "45a915e4d060149eb4365960e6a7a45f334393093061116b197e3240065ff2d8",
		"to" : "095e7baea6a6c7c4c2dfeb977efac326af552d87",
		"value" : "[HEX]"
		}
	}
}
)";

std::string const c_testExampleVMTest = R"(
{
	"randomVMTest": {
		"env" : {
				"previousHash" : "[HASH32]",
				"currentNumber" : "[HEX]",
				"currentGasLimit" : "[GASLIMIT]",
				"currentDifficulty" : "[HEX]",
				"currentTimestamp" : "[HEX]",
				"currentCoinbase" : "[HASH20]"
		},
		"pre" : {
		   "0f572e5295c57f15886f9b263e2f6d2d6c7b5ec6" : {
				"balance" : "[HEX]",
				"nonce" : "[HEX]",
				"code" : "[CODE]",
				"storage": {}
		   }
		},
		"exec" : {
				"address" : "0f572e5295c57f15886f9b263e2f6d2d6c7b5ec6",
				"origin" : "[HASH20]",
				"caller" : "[HASH20]",
				"value" : "[HEX]",
				"data" : "[CODE]",
				"gasPrice" : "[V]",
				"gas" : "[HEX]"
		   }
	   }
}
)";

std::string const c_testExampleRLPTest = R"(
{
	"randomRLPTest" : {
			"out" : "[RLP]"
		}
}
)";

std::string const c_testExampleBlockchainTest = R"(
{
 "randomBlockTest" : {
		 "genesisBlockHeader" : {
			 "bloom" : "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			 "coinbase" : "[HASH20]",
			 "difficulty" : "131072",
			 "extraData" : "[CODE]",
			 "gasLimit" : "4712388",
			 "gasUsed" : "0",
			 "mixHash" : "[0xHASH32]",
			 "nonce" : "0x0102030405060708",
			 "number" : "0",
			 "parentHash" : "0x0000000000000000000000000000000000000000000000000000000000000000",
			 "receiptTrie" : "[0xHASH32]",
			 "stateRoot" : "[0xHASH32]",
			 "timestamp" : "[HEX]",
			 "transactionsTrie" : "0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
			 "uncleHash" : "0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347"
		 },
		 "pre" : {
			"a94f5374fce5edbc8e2a8697c15331677e6ebf0b" : {
				 "balance" : "[HEX]",
				 "nonce" : "0",
				 "code" : "",
				 "storage": {}
			 },
			"095e7baea6a6c7c4c2dfeb977efac326af552d87" : {
				"balance" : "[HEX]",
				"nonce" : "0",
				"code" : "[CODE]",
				"storage": {}
			}
		 },
		 "blocks" : [
			 {
				 "transactions" : [
					 {
						 "data" : "[CODE]",
						 "gasLimit" : "[HEX]",
						 "gasPrice" : "[V]",
						 "nonce" : "0",
						 "secretKey" : "45a915e4d060149eb4365960e6a7a45f334393093061116b197e3240065ff2d8",
						 "to" : "095e7baea6a6c7c4c2dfeb977efac326af552d87",
						 "value" : "[V]"
					 }
				 ],
				 "uncleHeaders" : [
				 ]
			 }
		 ]
	 }
}
)";
