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
#include <test/tools/libtesteth/TestHelper.h>
#include <test/tools/fuzzTesting/fuzzHelper.h>
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
	bool debug = false;
	TestOutputHelper::initTest(1);
	dev::test::Options& options = const_cast<dev::test::Options&>(dev::test::Options::get());

	std::string const& testSuite = options.rCurrentTestSuite;
	if (testSuite != "BlockchainTests" && testSuite != "TransactionTests" && testSuite != "StateTestsGeneral"
		&& testSuite != "VMTests")
	{
		std::cerr << "Error! Test suite '" + testSuite +"' not supported! (Usage -t TestSuite)" << std::endl;
		return 1;
	}

	//parse options for createRandomTest special test suite
	for (size_t i = 0; i < _parameters.size(); ++i)
	{
		auto arg = std::string{_parameters.at(i)};

		if (arg == "--debug")
			debug = true;
	}

	if (testSuite == "StateTestsGeneral")
		fillRandomTest(dev::test::doStateTests, c_testExampleStateTest, debug);

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
		std::cout << " Failed test with Exception: " << diagnostic_information(_e) << std::endl;
		ret = 1;
	}
	catch (std::exception const& _e)
	{
		std::cout << " Failed test with Exception: " << _e.what() << std::endl;
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
		for (auto& obj: v.get_obj())
			if (!obj.second.get_obj().count("post"))
				BOOST_ERROR("An error occured when executing random state test!");
	}
	catch(...)
	{
		BOOST_ERROR("Error generating random test!");
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
	dev::test::RandomCodeOptions options; //use default options
	std::vector<std::string> types = getTypes();

	for (std::map<std::string, std::string>::const_iterator it = _varMap.begin(); it != _varMap.end(); it++)
		types.push_back(it->first);

	for (auto const& type: types)
	{
		std::size_t pos = _test.find(type);
		while (pos != std::string::npos)
		{
			if (type == "[RLP]")
			{
				std::string debug;
				int randomDepth = 1 + (int)dev::test::RandomCode::randomUniInt() % 10;
				_test.replace(pos, 5, dev::test::RandomCode::rndRLPSequence(randomDepth, debug));
				cnote << debug;
			}
			else if (type == "[CODE]")
			{
				int random = (int)dev::test::RandomCode::randomUniInt() % 100;
				if (random < 90)
					_test.replace(pos, type.length(), "0x"+dev::test::RandomCode::generate(5, options));
				else
					_test.replace(pos, type.length(), "");
			}
			else if (type == "[HEX]")
				_test.replace(pos, type.length(), dev::test::RandomCode::randomUniIntHex());
			else if (type == "[HEX32]")
			{
				int random = (int)dev::test::RandomCode::randomUniInt() % 100;
				if (random < 90)
					_test.replace(pos, type.length(), dev::test::RandomCode::randomUniIntHex(std::numeric_limits<uint32_t>::max()));
				else
					_test.replace(pos, type.length(), "0x00");
			}
			else if (type == "[GASLIMIT]")
				_test.replace(pos, type.length(), dev::test::RandomCode::randomUniIntHex(dev::u256("36028797018963967"), dev::u256("18014398509481983")));
			else if (type == "[GASPRICE]")
				_test.replace(pos, type.length(), dev::test::RandomCode::randomUniIntHex(dev::u256("5")));
			else if (type == "[HASH20]")
				_test.replace(pos, type.length(), dev::test::RandomCode::rndByteSequence(20));
			else if (type == "[ADDRESS]")
				_test.replace(pos, type.length(), toString(options.getRandomAddress()));
			else if (type == "[0xADDRESS]")
				_test.replace(pos, type.length(), "0x" + toString(options.getRandomAddress()));
			else if (type == "[0xDESTADDRESS]")
			{
				int random = (int)dev::test::RandomCode::randomUniInt() % 100;
				if (random < 50)
					_test.replace(pos, type.length(), "0x" + toString(options.getRandomAddress()));
				else
					_test.replace(pos, type.length(), "");
			}
			else if (type == "[0xHASH32]")
				_test.replace(pos, type.length(), "0x" + dev::test::RandomCode::rndByteSequence(32));
			else if (type == "[HASH32]")
				_test.replace(pos, type.length(), dev::test::RandomCode::rndByteSequence(32));
			else if (type == "[V]")
			{
				int random = (int)dev::test::RandomCode::randomUniInt() % 100;
				if (random < 30)
					_test.replace(pos, type.length(), "0x1c");
				else
				if (random < 60)
					_test.replace(pos, type.length(), "0x1d");
				else
					_test.replace(pos, type.length(), "0x" + dev::test::RandomCode::rndByteSequence(1));
			}
			else
			{
				//Replace type from varMap if varMap is set
				if (_varMap.count(type))
					_test.replace(pos, type.length(), _varMap.at(type));
				BOOST_ERROR("Skipping undeclared type: " + type);
			}

			pos = _test.find(type);
		}
	}
}

std::vector<std::string> dev::test::RandomCode::getTypes()
{
	//declare possible types
	return {"[RLP]", "[CODE]", "[HEX]", "[HEX32]", "[HASH20]", "[HASH32]", "[0xHASH32]", "[V]", "[GASLIMIT]", "[GASPRICE]", "[ADDRESS]", "[0xADDRESS]", "[0xDESTADDRESS]"};
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
		"currentCoinbase" : "[ADDRESS]",
		"currentDifficulty" : "0x20000",
		"currentGasLimit" : "[GASLIMIT]",
		"currentNumber" : "1",
		"currentTimestamp" : "1000",
		"previousHash" : "[HASH32]"
		},
	"pre" : {
		"[ADDRESS]" : {
			"balance" : "[HEX]",
			"code" : "[CODE]",
			"nonce" : "[V]",
			"storage" : {
			}
		},
		"[ADDRESS]" : {
			"balance" : "[HEX]",
			"code" : "[CODE]",
			"nonce" : "[V]",
			"storage" : {
			}
		},
		"[ADDRESS]" : {
			"balance" : "[HEX]",
			"code" : "[CODE]",
			"nonce" : "[V]",
			"storage" : {
			}
		},
		"[ADDRESS]" : {
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
		"data" : [
			"[CODE]"
		],
		"gasLimit" : [
			"[HEX32]"
		],
		"gasPrice" : "[GASPRICE]",
		"nonce" : "0",
		"secretKey" : "0x45a915e4d060149eb4365960e6a7a45f334393093061116b197e3240065ff2d8",
		"to" : "[0xDESTADDRESS]",
		"value" : [
			"[HEX32]"
		]
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
			"0x0f572e5295c57f15886f9b263e2f6d2d6c7b5ec6" : {
				"balance" : "[HEX]",
				"nonce" : "[HEX]",
				"code" : "[CODE]",
				"storage": {}
		}
		},
		"exec" : {
				"address" : "0x0f572e5295c57f15886f9b263e2f6d2d6c7b5ec6",
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
			 "bloom" : "0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
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
						 "secretKey" : "0x45a915e4d060149eb4365960e6a7a45f334393093061116b197e3240065ff2d8",
						 "to" : "0x095e7baea6a6c7c4c2dfeb977efac326af552d87",
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
