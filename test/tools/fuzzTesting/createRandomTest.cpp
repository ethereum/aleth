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
		else if (arg == "--debug")
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
	dev::test::RandomCodeOptions options; //use default options
	std::vector<std::string> types = getTypes();

	for (std::map<std::string, std::string>::const_iterator it = _varMap.begin(); it != _varMap.end(); it++)
		types.push_back(it->first);

	for (auto const& type: types)
	{
		std::size_t pos = _test.find(type);
		while (pos != std::string::npos)
		{
			std::string replace;
			if (type == "[RLP]")
			{
				std::string debug;
				int randomDepth = 1 + (int)dev::test::RandomCode::randomUniInt() % 10;
				replace = dev::test::RandomCode::rndRLPSequence(randomDepth, debug);
				cnote << debug;
			}
			else if (type == "[CODE]")
				replace = dev::test::RandomCode::generate(10, options);
			else if (type == "[HEX]")
				replace = dev::test::RandomCode::randomUniIntHex();
			else if (type == "[HEX32]")
				replace = dev::test::RandomCode::randomUniIntHex(0, std::numeric_limits<uint32_t>::max());
			else if (type == "[HASH20]")
				replace = dev::test::RandomCode::rndByteSequence(20);
			else if (type == "[HASH32]")
				replace = dev::test::RandomCode::rndByteSequence(32);
			else if (type == "[0xHASH32]")
				replace = "0x" + dev::test::RandomCode::rndByteSequence(32);
			else if (type == "[V]")
			{
				int random = test::RandomCode::randomPercent();
				if (random < 30)
					replace = "0x1c";
				else if (random < 60)
					replace = "0x1d";
				else
					replace = "0x" + dev::test::RandomCode::rndByteSequence(1);
			}
			else if (type == "[BLOCKGASLIMIT]")
				replace = test::RandomCode::randomUniIntHex(dev::u256("100000"), dev::u256("36028797018963967"));
			else if (type == "[DESTADDRESS]")
			{
				Address address = options.getRandomAddress(RandomCodeOptions::AddressType::CallOnlyOrStateOrCreate);
				if (address != ZeroAddress) //else transaction creation
					replace = "0x" + toString(address);
			}
			else if (type == "[ADDRESS]")
				replace = toString(options.getRandomAddress(RandomCodeOptions::AddressType::StateAccount));
			else if (type == "[0xADDRESS]")
				replace = "0x" + toString(options.getRandomAddress(RandomCodeOptions::AddressType::StateAccount));
			else if (type == "[TRANSACTIONGASLIMIT]")
				replace = test::RandomCode::randomUniIntHex(dev::u256("5000"), dev::u256("10000000"));
			else if (type == "[GASPRICE]")
				replace = test::RandomCode::randomUniIntHex(0, dev::u256("10"));
			else
			{
				//Replace type from varMap if varMap is set
				if (_varMap.count(type))
					replace = _varMap.at(type);
				else
					BOOST_ERROR("Skipping undeclared type: " + type);
			}

			if (replace.empty() && type != "[DESTADDRESS]" && type != "[CODE]")
				BOOST_ERROR("Empty replace of type occured: " + type);

			_test.replace(pos, type.length(), replace);
			pos = _test.find(type);
		}
	}
}

std::vector<std::string> dev::test::RandomCode::getTypes()
{
	return {
		"[RLP]",				//Random RLP String
		"[CODE]",				//Random bytecode (could be empty string)
		"[HEX]",				//Random hex value string 0x...  max value uint64
		"[HEX32]",				//Random hex value string 0x...  max value uint32
		"[HASH20]",				//Random hash 20 byte length
		"[HASH32]",				//Random hash 32 byte length
		"[0xHASH32]",			//Random hash string 0x...  32 byte length
		"[V]",					//Random V value for transaction sig. could be invalid.
		"[BLOCKGASLIMIT]",		//Random block gas limit with max of 2**55-1
		"[DESTADDRESS]",		//Random destination address for transaction (could be empty string)
		"[ADDRESS]",			//Random account address
		"[0xADDRESS]",			//Random account address
		"[TRANSACTIONGASLIMIT]", //Random reasonable gas limit for a transaction
		"[GASPRICE]"			//Random reasonable gas price for transaction (could be 0)
	};
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
		"currentCoinbase" : "[0xADDRESS]",
		"currentDifficulty" : "0x20000",
		"currentGasLimit" : "[BLOCKGASLIMIT]",
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
			"balance" : "[BALANCE]",
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
			"[TRANSACTIONGASLIMIT]"
		],
		"gasPrice" : "[GASPRICE]",
		"nonce" : "0",
		"secretKey" : "0x45a915e4d060149eb4365960e6a7a45f334393093061116b197e3240065ff2d8",
		"to" : "[DESTADDRESS]",
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
				"currentGasLimit" : "[BLOCKGASLIMIT]",
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
