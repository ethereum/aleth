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
/** @file randomCode.h
 * @date 2017
 */

#include <string>
#include <random>
#include <boost/filesystem/path.hpp>

#include <test/tools/libtesteth/TestHelper.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/CommonData.h>
#include <libevm/Instruction.h>
#include <test/tools/libtesteth/TestSuite.h>

#pragma once

namespace dev
{
namespace test
{

using IntDistrib = std::uniform_int_distribution<>;
using DescreteDistrib = std::discrete_distribution<>;
using IntGenerator = std::function<int()>;

enum class SizeStrictness
{
	Strict,
	Random
};

struct RandomCodeOptions
{
public:
	enum AddressType{
		Precompiled,
		ByzantiumPrecompiled,
		StateAccount,
		SendingAccount,
		PrecompiledOrStateOrCreate,
		PrecompiledOrState,
		All
	};
	RandomCodeOptions();
	void setWeight(dev::eth::Instruction _opCode, int _weight);
	void addAddress(dev::Address const& _address, AddressType _type);
	dev::Address getRandomAddress(AddressType _type = AddressType::All) const;
	int getWeightedRandomOpcode() const;

	bool useUndefinedOpCodes;
	int smartCodeProbability;
	int randomAddressProbability;
	int emptyCodeProbability;
	int emptyAddressProbability;
	int precompiledAddressProbability;
	int byzPrecompiledAddressProbability;
	int precompiledDestProbability;
	int sendingAddressProbability;

private:
	std::map<int, int> mapWeights;
	typedef std::pair<dev::Address, AddressType> accountRecord;
	std::vector<accountRecord> testAccounts;
	dev::Address getRandomAddressPriv(AddressType _type) const;
};


class RandomCode
{
public:
	static RandomCode& get()
	{
		static RandomCode generator;
		return generator;
	}
	RandomCode(RandomCode const&) = delete;
	void operator=(RandomCode const&) = delete;

	/// Generate random vm code
	std::string generate(int _maxOpNumber, RandomCodeOptions const& _options);

	/// Replace keywords in given string with values
	void parseTestWithTypes(std::string& _test, std::map<std::string, std::string> const& _varMap, RandomCodeOptions const& _options);
	void parseTestWithTypes(std::string& _test, std::map<std::string, std::string> const& _varMap)
	{
		RandomCodeOptions defaultOptions;
		parseTestWithTypes(_test, _varMap, defaultOptions);
	}

	// Returns empty string if there was an error, a filled test otherwise.
	// prints test to the std::out or std::error if error when filling
	std::string fillRandomTest(dev::test::TestSuite const& _testSuite, std::string const& _testFillerTemplate, test::RandomCodeOptions const& _options);


	/// Generate random byte string of a given length
	std::string rndByteSequence(int _length = 1, SizeStrictness _sizeType = SizeStrictness::Strict);

	/// Generate random rlp byte sequence of a given depth (e.g [[[]],[]]). max depth level = 20.
	/// The _debug string contains returned rlp string with analysed sections
	/// [] - length section/ or single byte rlp encoding
	/// () - decimal representation of length
	/// {1} - Array
	/// {2} - Array more than 55
	/// {3} - List
	/// {4} - List more than 55
	std::string rndRLPSequence(int _depth, std::string& _debug);

	/// Generate random
	std::string randomUniIntHex(u256 const& _minVal = 0, u256 const& _maxVal = std::numeric_limits<uint64_t>::max());
	u256 randomUniInt(u256 const& _minVal = 0, u256 const& _maxVal = std::numeric_limits<uint64_t>::max());
	int randomPercent() { refreshSeed(); return percentDist(gen); }
	int weightedOpcode(std::vector<int>& _weights);

private:
	RandomCode()
	{
		percentDist = IntDistrib (0, 100);
		opCodeDist = IntDistrib (0, 255);
		opLengDist = IntDistrib (1, 32);
		opMemrDist = IntDistrib (0, 10485760);
		opSmallMemrDist = IntDistrib (0, 1024);
		uniIntDist = IntDistrib (0, 0x7fffffff);

		randOpCodeGen = std::bind(opCodeDist, gen);
		randOpLengGen = std::bind(opLengDist, gen);
		randOpMemrGen = std::bind(opMemrDist, gen);
		randoOpSmallMemrGen = std::bind(opSmallMemrDist, gen);
		randUniIntGen = std::bind(uniIntDist, gen);
	}

	std::string fillArguments(dev::eth::Instruction _opcode, RandomCodeOptions const& _options);
	std::string getPushCode(int _value);
	std::string getPushCode(std::string const& _hex);
	int recursiveRLP(std::string& _result, int _depth, std::string& _debug);
	void refreshSeed();
	std::vector<std::string> getTypes();

	std::mt19937_64 gen;					///< Random generator
	IntDistrib opCodeDist;					///< 0..255 opcodes
	IntDistrib percentDist;					///< 0..100 percent
	IntDistrib opLengDist;					///< 1..32  byte string
	IntDistrib opMemrDist;					///< 1..10MB  byte string
	IntDistrib opSmallMemrDist;				/// 0..1kb
	IntDistrib uniIntDist;					///< 0..0x7fffffff

	IntGenerator randUniIntGen;				///< Generate random UniformInt from uniIntDist
	IntGenerator randOpCodeGen;				///< Generate random value from opCodeDist
	IntGenerator randOpLengGen;				///< Generate random length from opLengDist
	IntGenerator randOpMemrGen;				///< Generate random length from opMemrDist
	IntGenerator randoOpSmallMemrGen;		///< Generate random length from opSmallMemrDist
};

}
}
