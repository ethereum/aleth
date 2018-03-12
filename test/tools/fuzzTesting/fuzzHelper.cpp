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
/** @file fuzzHelper.cpp
 * @author Dimitry Khokhlov <winsvega@mail.ru>
 * @date 2015
 */

#include <chrono>
#include <boost/filesystem/path.hpp>
#include <libevm/Instruction.h>
#include <test/tools/fuzzTesting/fuzzHelper.h>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <test/tools/jsontests/StateTests.h>
#include <json_spirit/json_spirit.h>

using namespace dev;
using namespace std;
const static std::array<eth::Instruction, 47> invalidOpcodes {{
	eth::Instruction::INVALID,
	eth::Instruction::PUSHC,
	eth::Instruction::JUMPC,
	eth::Instruction::JUMPCI,
	eth::Instruction::JUMPTO,
	eth::Instruction::JUMPIF,
	eth::Instruction::JUMPSUB,
	eth::Instruction::JUMPV,
	eth::Instruction::JUMPSUBV,
	eth::Instruction::BEGINSUB,
	eth::Instruction::BEGINDATA,
	eth::Instruction::RETURNSUB,
	eth::Instruction::PUTLOCAL,
	eth::Instruction::GETLOCAL,
	eth::Instruction::XADD,
	eth::Instruction::XMUL,
	eth::Instruction::XSUB,
	eth::Instruction::XDIV,
	eth::Instruction::XSDIV,
	eth::Instruction::XMOD,
	eth::Instruction::XSMOD,
	eth::Instruction::XLT,
	eth::Instruction::XGT,
	eth::Instruction::XSLT,
	eth::Instruction::XSGT,
	eth::Instruction::XEQ,
	eth::Instruction::XISZERO,
	eth::Instruction::XAND,
	eth::Instruction::XOR,
	eth::Instruction::XXOR,
	eth::Instruction::XNOT,
	eth::Instruction::XSHL,
	eth::Instruction::XSHR,
	eth::Instruction::XSAR,
	eth::Instruction::XROL,
	eth::Instruction::XROR,
	eth::Instruction::XPUSH,
	eth::Instruction::XMLOAD,
	eth::Instruction::XMSTORE,
	eth::Instruction::XSLOAD,
	eth::Instruction::XSSTORE,
	eth::Instruction::XVTOWIDE,
	eth::Instruction::XWIDETOV,
	eth::Instruction::XPUT,
	eth::Instruction::XGET,
	eth::Instruction::XSWIZZLE,
	eth::Instruction::XSHUFFLE
}};

namespace dev
{
namespace test
{

int RandomCodeBase::recursiveRLP(std::string& _result, int _depth, std::string& _debug)
{
	bool genValidRlp = true;
	int bugProbability = randomPercent();
	if (bugProbability < 80)
		genValidRlp = false;

	if (_depth > 1)
	{
		//create rlp blocks
		int size = 1 + randomSmallUniInt() % 4;
		for (auto i = 0; i < size; i++)
		{
			std::string blockstr;
			std::string blockDebug;
			recursiveRLP(blockstr, _depth - 1, blockDebug);
			_result += blockstr;
			_debug += blockDebug;
		}

		//make rlp header
		int length = _result.size() / 2;
		std::string header;
		int rtype = 0;
		int rnd = randomPercent();
		if (rnd < 10)
		{
			//make header as array
			if (length <= 55)
			{
				header = toCompactHex(128 + length);
				rtype = 1;
			}
			else
			{
				std::string hexlength = toCompactHex(length);
				header = toCompactHex(183 + hexlength.size() / 2) + hexlength;
				rtype = 2;
			}
		}
		else
		{
			//make header as list
			if (length <= 55)
			{
				header = toCompactHex(192 + length);
				rtype = 3;
			}
			else
			{
				std::string hexlength = toCompactHex(length, 1);
				header = toCompactHex(247 + hexlength.size() / 2) + hexlength;
				rtype = 4;
			}
		}
		_result = header + _result;
		_debug = "[" + header + "(" + toString(length) + "){" + toString(rtype) + "}]" + _debug;
		return _result.size() / 2;
	}
	if (_depth == 1)
	{
		bool genbug = false;
		bool genbug2 = false;
		int bugProbability = randomPercent();
		if (bugProbability < 50 && !genValidRlp)
			genbug = true;
		bugProbability = randomPercent();		//more randomness
		if (bugProbability < 50 && !genValidRlp)
			genbug2 = true;

		std::string emptyZeros = genValidRlp ? "" : genbug ? "00" : "";
		std::string emptyZeros2 = genValidRlp ? "" : genbug2 ? "00" : "";

		int rnd = randomSmallUniInt() % 5;
		switch (rnd)
		{
		case 0:
		{
			//single byte [0x00, 0x7f]
			std::string rlp = emptyZeros + toCompactHex(genbug ? randomSmallUniInt() % 255 : randomSmallUniInt() % 128, 1);
			_result.insert(0, rlp);
			_debug.insert(0, "[" + rlp + "]");
			return 1;
		}
		case 1:
		{
			//string 0-55 [0x80, 0xb7] + string
			int len = genbug ? randomSmallUniInt() % 255 : randomSmallUniInt() % 55;
			std::string hex = rndByteSequence(len);
			if (len == 1)
			if (genValidRlp && fromHex(hex)[0] < 128)
				hex = toCompactHex((u64)128);

			_result.insert(0, toCompactHex(128 + len) + emptyZeros + hex);
			_debug.insert(0, "[" + toCompactHex(128 + len) + "(" + toString(len) + ")]" + emptyZeros + hex);
			return len + 1;
		}
		case 2:
		{
			//string more 55 [0xb8, 0xbf] + length + string
			int len = randomPercent();
			if (len < 56 && genValidRlp)
				len = 56;

			std::string hex = rndByteSequence(len);
			std::string hexlen = emptyZeros2 + toCompactHex(len, 1);
			std::string rlpblock = toCompactHex(183 + hexlen.size() / 2) + hexlen + emptyZeros + hex;
			_debug.insert(0, "[" + toCompactHex(183 + hexlen.size() / 2) + hexlen + "(" + toString(len) + "){2}]" + emptyZeros + hex);
			_result.insert(0, rlpblock);
			return rlpblock.size() / 2;
		}
		case 3:
		{
			//list 0-55 [0xc0, 0xf7] + data
			int len = genbug ? randomSmallUniInt() % 255 : randomSmallUniInt() % 55;
			std::string hex = emptyZeros + rndByteSequence(len);
			_result.insert(0, toCompactHex(192 + len) + hex);
			_debug.insert(0, "[" + toCompactHex(192 + len) + "(" + toString(len) + "){3}]" + hex);
			return len + 1;
		}
		case 4:
		{
			//list more 55 [0xf8, 0xff] + length + data
			int len = randomPercent();
			if (len < 56 && genValidRlp)
				len = 56;
			std::string hexlen = emptyZeros2 + toCompactHex(len, 1);
			std::string rlpblock = toCompactHex(247 + hexlen.size() / 2) + hexlen + emptyZeros + rndByteSequence(len);
			_debug.insert(0, "[" + toCompactHex(247 + hexlen.size() / 2) + hexlen + "(" + toString(len) + "){4}]" + emptyZeros + rndByteSequence(len));
			_result.insert(0, rlpblock);
			return rlpblock.size() / 2;
		}
		}
	}
	return 0;
}

std::string RandomCodeBase::rndRLPSequence(int _depth, std::string& _debug)
{
	std::string hash;
	_depth = std::min(std::max(1, _depth), 7); //limit depth to avoid overkill
	recursiveRLP(hash, _depth, _debug);
	return hash;
}

std::string RandomCodeBase::rndByteSequence(int _length, SizeStrictness _sizeType)
{
	std::string hash;
	_length = (_sizeType == SizeStrictness::Strict) ? std::max(0, _length) : (int)randomUniInt(0, _length);
	for (auto i = 0; i < _length; i++)
	{
		uint8_t byte = randomOpcode();
		hash += toCompactHex(byte, 1);
	}
	return hash;
}

bool isOpcodeDefined(uint8_t _opcode)
{
	eth::Instruction inst = (eth::Instruction) _opcode;
	eth::InstructionInfo info = eth::instructionInfo(inst);
	return (info.gasPriceTier != dev::eth::Tier::Invalid && !info.name.empty()
		&& std::find(invalidOpcodes.begin(), invalidOpcodes.end(), inst) == invalidOpcodes.end());
}

uint8_t makeOpcodeDefined(uint8_t _opcode)
{
	while (!isOpcodeDefined(_opcode))
		_opcode++; //Byte code is yet not implemented. Try next one.
	return _opcode;
}

//generate smart random code
std::string RandomCodeBase::generate(int _maxOpNumber, RandomCodeOptions const& _options)
{
	std::string code;
	if (test::RandomCode::get().randomPercent() < _options.emptyCodeProbability)
		return code;

	//generate [0 ... _maxOpNumber] opcodes.
	int size = test::RandomCode::get().randomPercent() * _maxOpNumber / 100;
	assert(size <= _maxOpNumber);

	for (auto i = 0; i < size; i++)
	{
		uint8_t opcode = _options.getWeightedRandomOpcode();
		if (!isOpcodeDefined(opcode) && _options.useUndefinedOpCodes)
		{
			code += toCompactHex(opcode, 1);
			continue;
		}
		else
		{
			opcode = makeOpcodeDefined(opcode);
			eth::Instruction inst = (eth::Instruction) opcode;
			eth::InstructionInfo info = eth::instructionInfo(inst);
			if (info.name.find("PUSH") != std::string::npos)
			{
				code += toCompactHex(opcode);
				code += fillArguments(inst, _options);
			}
			else
			{
				code += fillArguments(inst, _options);
				code += toCompactHex(opcode, 1);
			}
		}
	}

	return "0x" + code;
}

std::string RandomCodeBase::randomUniIntHex(u256 const& _minVal, u256 const& _maxVal)
{
	return toCompactHexPrefixed(randomUniInt(_minVal, _maxVal), 1);
}

std::string RandomCodeBase::getPushCode(std::string const& _hex)
{
	assert(_hex.length() % 2 == 0);
	std::string hexVal = _hex;
	if (hexVal.empty())
		hexVal = "00";
	int length = hexVal.length() / 2;
	int pushCode = 96 + length - 1;
	return toCompactHex(pushCode) + hexVal;
}

std::string RandomCodeBase::getPushCode(unsigned _value)
{
	std::string hexString = toCompactHex(_value, 1);
	return getPushCode(hexString);
}

std::string RandomCodeBase::fillArguments(eth::Instruction _opcode, RandomCodeOptions const& _options)
{
	eth::InstructionInfo info = eth::instructionInfo(_opcode);

	std::string code;
	int rand = randomPercent();

	if (rand < _options.smartCodeProbability)  // Smart.
	{
		//PUSH1 ... PUSH32
		if (eth::Instruction::PUSH1 <= _opcode && _opcode <= eth::Instruction::PUSH32)
		{
			code += rndByteSequence(int(_opcode) - int(eth::Instruction::PUSH1) + 1);
			return code;
		}

		//SWAP1 ... SWAP16 || DUP1 ... DUP16
		bool isSWAP = (eth::Instruction::SWAP1 <= _opcode && _opcode <= eth::Instruction::SWAP16);
		bool isDUP = (eth::Instruction::DUP1 <= _opcode && _opcode <= eth::Instruction::DUP16);

		if (isSWAP || isDUP)
		{
			int times = 0;
			if (isSWAP)
				times = int(_opcode) - int(eth::Instruction::SWAP1) + 2;
			else
				times = int(_opcode) - int(eth::Instruction::DUP1) + 1;

			for (int i = 0; i < times; i ++)
				code += getPushCode(rndByteSequence(randomLength32()));

			return code;
		}

		switch (_opcode)
		{
		case eth::Instruction::MSTORE:
			code += getPushCode(rndByteSequence(randomLength32()));	//code
			code += getPushCode(randomSmallMemoryLength());					//index
			return code;
		// case eth::Instruction::RETURNDATASIZE:  // returndatasize takes no args
		case eth::Instruction::RETURNDATACOPY:  //(REVERT memlen1 memlen2)
			code += getPushCode(randomSmallMemoryLength());	// memory position
			code += getPushCode(randomSmallMemoryLength());	// returndata position
			code += getPushCode(randomSmallMemoryLength());	// size/num of bytes to copy
			return code;
		case eth::Instruction::EXTCODECOPY:
			code += getPushCode(randomMemoryLength());	//memstart2
			code += getPushCode(randomMemoryLength());	//memlen1
			code += getPushCode(randomMemoryLength());	//memstart1
			code += getPushCode(toString(_options.getRandomAddress()));//address
			return code;
		case eth::Instruction::EXTCODESIZE:
			code += getPushCode(toString(_options.getRandomAddress()));//address
			return code;
		case eth::Instruction::CREATE:
			//(CREATE value mem1 mem2)
			code += getPushCode(randomSmallMemoryLength());	//memlen1
			code += getPushCode(randomSmallMemoryLength());	//memlen1
			code += getPushCode((unsigned)randomUniInt());	//value
			return code;
		case eth::Instruction::CALL:
		case eth::Instruction::CALLCODE:
			//(CALL gaslimit address value memstart1 memlen1 memstart2 memlen2)
			//(CALLCODE gaslimit address value memstart1 memlen1 memstart2 memlen2)
			code += getPushCode(randomSmallMemoryLength());	//memlen2
			code += getPushCode(randomSmallMemoryLength());	//memstart2
			code += getPushCode(randomSmallMemoryLength());	//memlen1
			code += getPushCode(randomSmallMemoryLength());	//memstart1
			code += getPushCode((unsigned)randomUniInt());	//value
			code += getPushCode(toString(_options.getRandomAddress(RandomCodeOptions::AddressType::PrecompiledOrState)));//address
			code += getPushCode((unsigned)randomUniInt());	//gaslimit
			return code;
		case eth::Instruction::STATICCALL:
		case eth::Instruction::DELEGATECALL:
			//(CALL gaslimit address value memstart1 memlen1 memstart2 memlen2)
			//(CALLCODE gaslimit address value memstart1 memlen1 memstart2 memlen2)
			code += getPushCode(randomSmallMemoryLength());	//memlen2
			code += getPushCode(randomSmallMemoryLength());	//memstart2
			code += getPushCode(randomSmallMemoryLength());	//memlen1
			code += getPushCode(randomSmallMemoryLength());	//memstart1
			code += getPushCode(toString(_options.getRandomAddress(RandomCodeOptions::AddressType::PrecompiledOrState)));//address
			code += getPushCode((unsigned)randomUniInt());	//gaslimit
			return code;
		case eth::Instruction::SUICIDE: //(SUICIDE address)
			code += getPushCode(toString(_options.getRandomAddress()));
			return code;
		case eth::Instruction::RETURN:  //(RETURN memlen1 memlen2)
		case eth::Instruction::REVERT:  //(REVERT memlen1 memlen2)
			code += getPushCode(randomSmallMemoryLength());	//memlen1
			code += getPushCode(randomSmallMemoryLength());	//memlen1
			return code;
		default:
			break;
		}
	}

	//generate random parameters
	for (int i = 0; i < info.args; i++)
		code += getPushCode(rndByteSequence(randomLength32()));

	return code;
}


//Default Random Code Options
RandomCodeOptions::RandomCodeOptions() :
	useUndefinedOpCodes(false),			//spawn undefined bytecodes in code
	smartCodeProbability(99),			//spawn correct opcodes (with correct argument stack and reasonable arguments)
	randomAddressProbability(3),		//probability of generating a random address instead of defined from list
	emptyCodeProbability(2),			//probability of code being empty (empty code mean empty account)
	emptyAddressProbability(15),		//probability of generating an empty address for transaction creation
	precompiledAddressProbability(5),	//probability of generating a precompiled address for calls
	byzPrecompiledAddressProbability(10),	//probability of generating a precompiled address for calls
	precompiledDestProbability(2),	// probability of generating a precompiled address as tx destination
	sendingAddressProbability(3)	// probability of calling to the tx sending account
{
	//each op code with same weight-probability
	for (uint8_t i = 0; i < 255; i++)
		mapWeights.insert(std::pair<uint8_t, int>(i, 40));

	//Probability of instructions
	setWeight(eth::Instruction::STOP, 1);
	for (uint8_t i = static_cast<uint8_t>(eth::Instruction::PUSH1); i <= static_cast<uint8_t>(eth::Instruction::PUSH32); i++)
		setWeight((eth::Instruction) i, 1);
	for (uint8_t i = static_cast<uint8_t>(eth::Instruction::SWAP1); i <= static_cast<uint8_t>(eth::Instruction::SWAP16); i++)
		setWeight((eth::Instruction) i, 10);
	for (uint8_t i = static_cast<uint8_t>(eth::Instruction::DUP1); i <= static_cast<uint8_t>(eth::Instruction::DUP16); i++)
		setWeight((eth::Instruction) i, 10);

	setWeight(eth::Instruction::SIGNEXTEND, 100);
	setWeight(eth::Instruction::ORIGIN, 200);
	setWeight(eth::Instruction::ADDRESS, 200);
	setWeight(eth::Instruction::SLOAD, 200);
	setWeight(eth::Instruction::MLOAD, 200);
	setWeight(eth::Instruction::MSTORE, 400);
	setWeight(eth::Instruction::MSTORE8, 400);
	setWeight(eth::Instruction::SSTORE, 170);
	setWeight(eth::Instruction::CALL, 350);
	setWeight(eth::Instruction::CALLCODE, 170);
	setWeight(eth::Instruction::DELEGATECALL, 300);
	setWeight(eth::Instruction::STATICCALL, 300);
	setWeight(eth::Instruction::CREATE, 350);

	setWeight(eth::Instruction::RETURNDATASIZE, 500);
	setWeight(eth::Instruction::RETURNDATACOPY, 500);
	setWeight(eth::Instruction::REVERT, 500);

	//some smart addresses for calls
	addAddress(Address("0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b"), AddressType::SendingAccount);
	addAddress(Address("0xffffffffffffffffffffffffffffffffffffffff"), AddressType::StateAccount);
	addAddress(Address("0x1000000000000000000000000000000000000000"), AddressType::StateAccount);
	addAddress(Address("0xb94f5374fce5edbc8e2a8697c15331677e6ebf0b"), AddressType::StateAccount);
	addAddress(Address("0xc94f5374fce5edbc8e2a8697c15331677e6ebf0b"), AddressType::StateAccount);
	addAddress(Address("0xd94f5374fce5edbc8e2a8697c15331677e6ebf0b"), AddressType::StateAccount);
	addAddress(Address("0x0000000000000000000000000000000000000001"), AddressType::Precompiled);
	addAddress(Address("0x0000000000000000000000000000000000000002"), AddressType::Precompiled);
	addAddress(Address("0x0000000000000000000000000000000000000003"), AddressType::Precompiled);
	addAddress(Address("0x0000000000000000000000000000000000000004"), AddressType::Precompiled);
	addAddress(Address("0x0000000000000000000000000000000000000005"), AddressType::Precompiled);
	addAddress(Address("0x0000000000000000000000000000000000000006"), AddressType::Precompiled);
	addAddress(Address("0x0000000000000000000000000000000000000007"), AddressType::Precompiled);
	addAddress(Address("0x0000000000000000000000000000000000000008"), AddressType::Precompiled);
	addAddress(Address("0x0000000000000000000000000000000000000005"), AddressType::ByzantiumPrecompiled);
	addAddress(Address("0x0000000000000000000000000000000000000006"), AddressType::ByzantiumPrecompiled);
	addAddress(Address("0x0000000000000000000000000000000000000007"), AddressType::ByzantiumPrecompiled);
	addAddress(Address("0x0000000000000000000000000000000000000008"), AddressType::ByzantiumPrecompiled);
}

void boost_require_range(int _value, int _min, int _max)
{
	BOOST_REQUIRE(_value >= _min);
	BOOST_REQUIRE(_value <= _max);
}

int getProbability(json_spirit::mValue const& _obj)
{
	int probability = _obj.get_int();
	boost_require_range(probability, 0, 100);
	return probability;
}

void RandomCodeOptions::loadFromFile(boost::filesystem::path const& _jsonFileName)
{
	json_spirit::mValue v;
	bytes const byteContents = dev::contents(_jsonFileName);
	string const s = asString(byteContents);
	BOOST_REQUIRE_MESSAGE(s.length() > 0, "Contents of " + _jsonFileName.string() + " is empty.");
	json_spirit::read_string(s, v);

	BOOST_REQUIRE(v.type() == json_spirit::obj_type);
	json_spirit::mObject obj = v.get_obj();
	BOOST_REQUIRE(obj.count("probabilities"));
	BOOST_REQUIRE(obj.at("probabilities").type() == json_spirit::obj_type);

	//Parse Probabilities
	json_spirit::mObject probObj = obj.at("probabilities").get_obj();
	useUndefinedOpCodes = probObj.at("useUndefinedOpCodes").get_bool();
	smartCodeProbability = getProbability(probObj.at("smartCodeProbability"));
	randomAddressProbability = getProbability(probObj.at("randomAddressProbability"));
	emptyCodeProbability = getProbability(probObj.at("emptyCodeProbability"));
	emptyAddressProbability = getProbability(probObj.at("emptyAddressProbability"));
	precompiledAddressProbability = getProbability(probObj.at("precompiledAddressProbability"));
	byzPrecompiledAddressProbability = getProbability(probObj.at("byzPrecompiledAddressProbability"));
	precompiledDestProbability = getProbability(probObj.at("precompiledDestProbability"));
	sendingAddressProbability = getProbability(probObj.at("sendingAddressProbability"));
}

void RandomCodeOptions::setWeight(eth::Instruction _opCode, int _weight)
{
	mapWeights.at((uint8_t)_opCode) = _weight;
}

void RandomCodeOptions::addAddress(Address const& _address, AddressType _type)
{
	if (_type != Precompiled && _type != ByzantiumPrecompiled && _type != StateAccount && _type != SendingAccount)
		BOOST_ERROR("RandomCodeOptions::addAddress: address type could not be added! " + toString(_type));
	std::pair<Address, AddressType> record;
	record.first = _address;
	record.second = _type;
	testAccounts.push_back(record);
}

Address RandomCodeOptions::getRandomAddress(AddressType _type) const
{
	if (_type == Precompiled)
		return getRandomAddressPriv(AddressType::Precompiled);
	if (_type == ByzantiumPrecompiled)
		return getRandomAddressPriv(AddressType::ByzantiumPrecompiled);
	if (_type == StateAccount)
		return getRandomAddressPriv(AddressType::StateAccount);
	if (_type == SendingAccount)
		return getRandomAddressPriv(AddressType::SendingAccount);

	// Return Precompiled address
	if (_type == PrecompiledOrStateOrCreate || _type == PrecompiledOrState || _type == All)
	{
		if (RandomCode::get().randomPercent() < precompiledDestProbability)
		{
			if (RandomCode::get().randomPercent() < byzPrecompiledAddressProbability)
				return getRandomAddressPriv(AddressType::ByzantiumPrecompiled);
			else
				return getRandomAddressPriv(AddressType::Precompiled);
		}

		// ZeroAddress means empty account (create)
		if (_type != PrecompiledOrState)
		if (RandomCode::get().randomPercent() < emptyAddressProbability)
			return ZeroAddress;

		// Return random address
		if (test::RandomCode::get().randomPercent() < randomAddressProbability)
			return Address(RandomCode::get().rndByteSequence(20));

		// Return address of the sender (sender is  a part of state acount list)
		if (RandomCode::get().randomPercent() < sendingAddressProbability)
			return getRandomAddressPriv(AddressType::SendingAccount);
		return getRandomAddressPriv(AddressType::StateAccount);
	}

	return Address(RandomCode::get().rndByteSequence(20));
}

Address RandomCodeOptions::getRandomAddressPriv(AddressType _type) const
{
	if (_type != Precompiled && _type != ByzantiumPrecompiled && _type != StateAccount && _type != SendingAccount)
		BOOST_ERROR("RandomCodeOptions::getRandomAddressPriv: address type could not be found! " + toString(_type));

	int random = test::RandomCode::get().randomPercent();
	bool found = false;
	while (random >= 0)
	{
		for (auto& i : testAccounts)
		{
			if (i.second == _type)
			{
				found = true;
				if (random-- < 0)
					return i.first;
			}
		}
		if (!found)
		{
			BOOST_WARN("Can not find account of the specidied type. Return random instead.");
			return Address(RandomCode::get().rndByteSequence(20));
		}
	}

	BOOST_WARN("Can not find account of the specidied type. Return random instead.");
	return Address(RandomCode::get().rndByteSequence(20));
}

int RandomCodeOptions::getWeightedRandomOpcode() const
{
	std::vector<int> weights;
	for (auto const& element: mapWeights)
		weights.push_back(element.second);
	return RandomCode::get().weightedOpcode(weights);
}

BOOST_FIXTURE_TEST_SUITE(RandomCodeTests, TestOutputHelperFixture)

BOOST_AUTO_TEST_CASE(rndStateTest)
{
	try
	{
		//Mac os bug is here
		test::StateTestSuite suite;
		test::RandomCodeOptions options;
		test::TestOutputHelper::get().setCurrentTestFile(std::string());
		std::string test = test::RandomCode::get().fillRandomTest(suite, c_testExampleStateTest, options);
		BOOST_REQUIRE(!test.empty());
	}
	catch(dev::Exception const& _e)
	{
		BOOST_ERROR("Exception thrown when generating random code! " + diagnostic_information(_e));
	}
}

BOOST_AUTO_TEST_SUITE_END()

}
}
