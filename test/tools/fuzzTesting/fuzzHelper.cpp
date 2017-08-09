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
#include <boost/random.hpp>
#include <boost/filesystem/path.hpp>
#include <libevmcore/Instruction.h>
#include <test/tools/fuzzTesting/fuzzHelper.h>
#include <test/tools/libtesteth/TestOutputHelper.h>

using namespace dev;
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
	eth::Instruction::XSHUFFLE,
}};

namespace dev
{
namespace test
{

boost::random::mt19937 RandomCode::gen;
boostIntDistrib RandomCode::opCodeDist = boostIntDistrib (0, 255);
boostIntDistrib RandomCode::opLengDist = boostIntDistrib (1, 32);
boostIntDistrib RandomCode::opMemrDist = boostIntDistrib (0, 10485760);
boostIntDistrib RandomCode::uniIntDist = boostIntDistrib (0, 0x7fffffff);
boostUint64 RandomCode::uInt64Dist = boostUint64 (0, std::numeric_limits<uint64_t>::max());

boostIntGenerator RandomCode::randOpCodeGen = boostIntGenerator(gen, opCodeDist);
boostIntGenerator RandomCode::randOpLengGen = boostIntGenerator(gen, opLengDist);
boostIntGenerator RandomCode::randOpMemrGen = boostIntGenerator(gen, opMemrDist);
boostIntGenerator RandomCode::randUniIntGen = boostIntGenerator(gen, uniIntDist);
boostUInt64Generator RandomCode::randUInt64Gen = boostUInt64Generator(gen, uInt64Dist);

int RandomCode::recursiveRLP(std::string& _result, int _depth, std::string& _debug)
{
	bool genValidRlp = true;
	int bugProbability = randUniIntGen() % 100;
	if (bugProbability < 80)
		genValidRlp = false;

	if (_depth > 1)
	{
		//create rlp blocks
		int size = 1 + randUniIntGen() % 4;
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
		int rnd = randUniIntGen() % 100;
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
		int bugProbability = randUniIntGen() % 100;
		if (bugProbability < 50 && !genValidRlp)
			genbug = true;
		bugProbability = randUniIntGen() % 100;   //more randomness
		if (bugProbability < 50 && !genValidRlp)
			genbug2 = true;

		std::string emptyZeros = genValidRlp ? "" : genbug ? "00" : "";
		std::string emptyZeros2 = genValidRlp ? "" : genbug2 ? "00" : "";

		int rnd = randUniIntGen() % 5;
		switch (rnd)
		{
		case 0:
		{
			//single byte [0x00, 0x7f]
			std::string rlp = emptyZeros + toCompactHex(genbug ? randUniIntGen() % 255 : randUniIntGen() % 128, 1);
			_result.insert(0, rlp);
			_debug.insert(0, "[" + rlp + "]");
			return 1;
		}
		case 1:
		{
			//string 0-55 [0x80, 0xb7] + string
			int len = genbug ? randUniIntGen() % 255 : randUniIntGen() % 55;
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
			int len = randUniIntGen() % 100;
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
			int len = genbug ? randUniIntGen() % 255 : randUniIntGen() % 55;
			std::string hex = emptyZeros + rndByteSequence(len);
			_result.insert(0, toCompactHex(192 + len) + hex);
			_debug.insert(0, "[" + toCompactHex(192 + len) + "(" + toString(len) + "){3}]" + hex);
			return len + 1;
		}
		case 4:
		{
			//list more 55 [0xf8, 0xff] + length + data
			int len = randUniIntGen() % 100;
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

std::string RandomCode::rndRLPSequence(int _depth, std::string& _debug)
{
	refreshSeed();
	std::string hash;
	_depth = std::min(std::max(1, _depth), 7); //limit depth to avoid overkill
	recursiveRLP(hash, _depth, _debug);
	return hash;
}

std::string RandomCode::rndByteSequence(int _length, SizeStrictness _sizeType)
{
	refreshSeed();
	std::string hash = "";
	_length = (_sizeType == SizeStrictness::Strict) ? std::max(0, _length) : (int)randomUniInt() % _length;
	for (auto i = 0; i < _length; i++)
	{
		uint8_t byte = randOpCodeGen();
		hash += toCompactHex(byte, 1);
	}
	return hash;
}

//generate smart random code
std::string RandomCode::generate(int _maxOpNumber, RandomCodeOptions _options)
{
	refreshSeed();
	std::string code;

	//random opCode amount
	boostIntDistrib sizeDist (1, _maxOpNumber);
	boostIntGenerator rndSizeGen(gen, sizeDist);
	int size = (int)rndSizeGen();

	boostWeightGenerator randOpCodeWeight (gen, _options.opCodeProbability);
	bool weightsDefined = _options.opCodeProbability.probabilities().size() == 255;

	for (auto i = 0; i < size; i++)
	{
		uint8_t opcode = weightsDefined ? randOpCodeWeight() : randOpCodeGen();
		eth::Instruction inst = (eth::Instruction) opcode;
		eth::InstructionInfo info = eth::instructionInfo(inst);

		if (info.name.find("INVALID_INSTRUCTION") != std::string::npos || info.name.empty()
			|| std::find(invalidOpcodes.begin(), invalidOpcodes.end(), inst) != invalidOpcodes.end())
		{
			if (_options.useUndefinedOpCodes)
				code += toCompactHex(opcode, 1);
			else
			{
				//Byte code is yet not implemented. do not count it.
				i--;
				continue;
			}
		}
		else
		{
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
	return code;
}

std::string RandomCode::randomUniIntHex(u256 _maxVal, u256 _minVal)
{
	if (_maxVal == 0)
		_maxVal = std::numeric_limits<uint64_t>::max();
	refreshSeed();
	int rand = randUniIntGen() % 100;
	if (rand < 50)
		return toCompactHexPrefixed(_minVal + (u256)randUniIntGen() % _maxVal, 1);
	return toCompactHexPrefixed(_minVal + (u256)randUInt64Gen() % _maxVal, 1);
}

u256 RandomCode::randomUniInt(u256 _maxVal)
{
	if (_maxVal == 0)
		_maxVal = std::numeric_limits<uint64_t>::max();
	refreshSeed();
	return (u256)randUInt64Gen() % _maxVal;
}

void RandomCode::refreshSeed()
{
	auto now = std::chrono::steady_clock::now().time_since_epoch();
	auto timeSinceEpoch = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
	gen.seed(static_cast<unsigned int>(timeSinceEpoch));
}

std::string RandomCode::getPushCode(std::string const& _hex)
{
	assert(_hex.length() % 2 == 0);
	std::string hexVal = _hex;
	if (hexVal.empty())
		hexVal = "00";
	int length = hexVal.length() / 2;
	int pushCode = 96 + length - 1;
	return toCompactHex(pushCode) + hexVal;
}

std::string RandomCode::getPushCode(int _value)
{
	std::string hexString = toCompactHex(_value, 1);
	return getPushCode(hexString);
}

std::string RandomCode::fillArguments(eth::Instruction _opcode, RandomCodeOptions const& _options)
{
	eth::InstructionInfo info = eth::instructionInfo(_opcode);

	std::string code;
	bool smart = false;
	unsigned argsNum = info.args;
	int rand = randUniIntGen() % 100;
	if (rand < _options.smartCodeProbability)
		smart = true;

	if (smart)
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
			if (isDUP)
				times = int(_opcode) - int(eth::Instruction::DUP1) + 1;

			for (int i = 0; i < times; i ++)
				code += getPushCode(rndByteSequence(randOpLengGen()));

			return code;
		}

		switch (_opcode)
		{
		case eth::Instruction::MSTORE:
			code += getPushCode(rndByteSequence(randOpLengGen()));	//code
			code += getPushCode(randOpMemrGen());					//index
		break;
		case eth::Instruction::EXTCODECOPY:
			code += getPushCode(randOpMemrGen());	//memstart2
			code += getPushCode(randOpMemrGen());	//memlen1
			code += getPushCode(randOpMemrGen());	//memstart1
			code += getPushCode(toString(_options.getRandomAddress()));//address
		break;
		case eth::Instruction::EXTCODESIZE:
			code += getPushCode(toString(_options.getRandomAddress()));//address
		break;
		case eth::Instruction::CREATE:
			//(CREATE value mem1 mem2)
			code += getPushCode(randOpMemrGen());	//memlen1
			code += getPushCode(randOpMemrGen());	//memlen1
			code += getPushCode(randUniIntGen());	//value
		break;
		case eth::Instruction::CALL:
		case eth::Instruction::CALLCODE:
			//(CALL gaslimit address value memstart1 memlen1 memstart2 memlen2)
			//(CALLCODE gaslimit address value memstart1 memlen1 memstart2 memlen2)
			code += getPushCode(randOpMemrGen());	//memlen2
			code += getPushCode(randOpMemrGen());	//memstart2
			code += getPushCode(randOpMemrGen());	//memlen1
			code += getPushCode(randOpMemrGen());	//memstart1
			code += getPushCode(randUniIntGen());	//value
			code += getPushCode(toString(_options.getRandomAddress()));//address
			code += getPushCode(randUniIntGen());	//gaslimit
		break;
		case eth::Instruction::STATICCALL:
		case eth::Instruction::DELEGATECALL:
			//(CALL gaslimit address value memstart1 memlen1 memstart2 memlen2)
			//(CALLCODE gaslimit address value memstart1 memlen1 memstart2 memlen2)
			code += getPushCode(randOpMemrGen());	//memlen2
			code += getPushCode(randOpMemrGen());	//memstart2
			code += getPushCode(randOpMemrGen());	//memlen1
			code += getPushCode(randOpMemrGen());	//memstart1
			code += getPushCode(toString(_options.getRandomAddress()));//address
			code += getPushCode(randUniIntGen());	//gaslimit
		break;
		case eth::Instruction::SUICIDE: //(SUICIDE address)
			code += getPushCode(toString(_options.getRandomAddress()));
		break;
		case eth::Instruction::RETURN:  //(RETURN memlen1 memlen2)
		case eth::Instruction::REVERT:  //(REVERT memlen1 memlen2)
			code += getPushCode(randOpMemrGen());	//memlen1
			code += getPushCode(randOpMemrGen());	//memlen1
		break;
		default:
			smart = false;
		}
	}

	//generate random parameters
	if (smart == false)
		for (unsigned i = 0; i < argsNum; i++)
			code += getPushCode(rndByteSequence(randOpLengGen()));

	return code;
}


//Default Random Code Options
RandomCodeOptions::RandomCodeOptions() : useUndefinedOpCodes(false), smartCodeProbability(100), randomAddressProbability(10)
{
	//each op code with same weight-probability
	for (auto i = 0; i < 255; i++)
		mapWeights.insert(std::pair<int, int>(i, 40));

	setWeights();

	//Probability of instructions
	setWeight(eth::Instruction::STOP, 1);
	for (int i = (int)(eth::Instruction::PUSH1); i < 32; i++)
		setWeight((eth::Instruction) i, 1);
	for (int i = (int)(eth::Instruction::SWAP1); i < 16; i++)
		setWeight((eth::Instruction) i, 10);
	for (int i = (int)(eth::Instruction::DUP1); i < 16; i++)
		setWeight((eth::Instruction) i, 10);

	setWeight(eth::Instruction::SIGNEXTEND, 100);
	setWeight(eth::Instruction::ORIGIN, 200);
	setWeight(eth::Instruction::ADDRESS, 200);
	setWeight(eth::Instruction::SLOAD, 200);
	setWeight(eth::Instruction::MLOAD, 200);
	setWeight(eth::Instruction::MSTORE, 400);
	setWeight(eth::Instruction::MSTORE8, 400);
	setWeight(eth::Instruction::SSTORE, 170);
	setWeight(eth::Instruction::CALL, 170);
	setWeight(eth::Instruction::CALLCODE, 170);
	setWeight(eth::Instruction::DELEGATECALL, 170);
	setWeight(eth::Instruction::STATICCALL, 170);
	setWeight(eth::Instruction::EXTCODECOPY, 170);
	setWeight(eth::Instruction::EXTCODESIZE, 170);

	//some smart addresses for calls
	addAddress(Address("0xffffffffffffffffffffffffffffffffffffffff"));
	addAddress(Address("0x1000000000000000000000000000000000000000"));
	addAddress(Address("0x095e7baea6a6c7c4c2dfeb977efac326af552d87"));
	addAddress(Address("0x945304eb96065b2a98b57a48a06ae28d285a71b5"));
	addAddress(Address("0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b"));
	addAddress(Address("0xb94f5374fce5edbc8e2a8697c15331677e6ebf0b"));
	addAddress(Address("0xc94f5374fce5edbc8e2a8697c15331677e6ebf0b"));
	addAddress(Address("0xd94f5374fce5edbc8e2a8697c15331677e6ebf0b"));
	addAddress(Address("0xe94f5374fce5edbc8e2a8697c15331677e6ebf0b"));
	addAddress(Address("0xf94f5374fce5edbc8e2a8697c15331677e6ebf0b"));
	addAddress(Address("0x0f572e5295c57f15886f9b263e2f6d2d6c7b5ec6"));
	addAddress(Address("0x0000000000000000000000000000000000000001"));
	addAddress(Address("0x0000000000000000000000000000000000000002"));
	addAddress(Address("0x0000000000000000000000000000000000000003"));
	addAddress(Address("0x0000000000000000000000000000000000000004"));
	addAddress(Address("0x0000000000000000000000000000000000000005"));
	addAddress(Address("0x0000000000000000000000000000000000000006"));
	addAddress(Address("0x0000000000000000000000000000000000000007"));
	addAddress(Address("0x0000000000000000000000000000000000000008"));
}

void RandomCodeOptions::setWeight(eth::Instruction _opCode, int _weight)
{
	mapWeights.at((int)_opCode) = _weight;
	setWeights();
}

void RandomCodeOptions::addAddress(Address const& _address)
{
	addressList.push_back(_address);
}

Address RandomCodeOptions::getRandomAddress() const
{
	if (addressList.size() > 0)
	{
		int random = (int)dev::test::RandomCode::randomUniInt() % 100;
		if (random < 100 - randomAddressProbability)
		{
			int index = (int)RandomCode::randomUniInt() % addressList.size();
			return addressList[index];
		}
	}
	return Address(RandomCode::rndByteSequence(20));
}

void RandomCodeOptions::setWeights()
{
	std::vector<int> weights;
	for (auto const& element: mapWeights)
		weights.push_back(element.second);
	opCodeProbability = boostDescreteDistrib(weights);
}

BOOST_FIXTURE_TEST_SUITE(RandomCodeTests, TestOutputHelper)

BOOST_AUTO_TEST_CASE(rndCode)
{
	std::string code;
	cnote << "Testing Random Code: ";
	try
	{
		code = test::RandomCode::generate(10);
	}
	catch(...)
	{
		BOOST_ERROR("Exception thrown when generating random code!");
	}
	cnote << code;
}

BOOST_AUTO_TEST_SUITE_END()

}
}
