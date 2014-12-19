
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
/**
 * @author Christian <c@ethdev.com>
 * @date 2014
 * Unit tests for the solidity expression compiler, testing the behaviour of the code.
 */

#include <string>
#include <tuple>
#include <boost/test/unit_test.hpp>
#include <libdevcrypto/SHA3.h>
#include <test/solidityExecutionFramework.h>

using namespace std;

namespace dev
{
namespace solidity
{
namespace test
{

BOOST_FIXTURE_TEST_SUITE(SolidityCompilerEndToEndTest, ExecutionFramework)

BOOST_AUTO_TEST_CASE(smoke_test)
{
	char const* sourceCode = "contract test {\n"
							 "  function f(uint a) returns(uint d) { return a * 7; }\n"
							 "}\n";
	compileAndRun(sourceCode);
	testSolidityAgainstCppOnRange(0, [](u256 const& a) -> u256 { return a * 7; }, 0, 100);
}

BOOST_AUTO_TEST_CASE(empty_contract)
{
	char const* sourceCode = "contract test {\n"
							 "}\n";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction(0, bytes()).empty());
}

BOOST_AUTO_TEST_CASE(recursive_calls)
{
	char const* sourceCode = "contract test {\n"
							 "  function f(uint n) returns(uint nfac) {\n"
							 "    if (n <= 1) return 1;\n"
							 "    else return n * f(n - 1);\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	function<u256(u256)> recursive_calls_cpp = [&recursive_calls_cpp](u256 const& n) -> u256
	{
		if (n <= 1)
			return 1;
		else
			return n * recursive_calls_cpp(n - 1);
	};

	testSolidityAgainstCppOnRange(0, recursive_calls_cpp, 0, 5);
}

BOOST_AUTO_TEST_CASE(multiple_functions)
{
	char const* sourceCode = "contract test {\n"
							 "  function a() returns(uint n) { return 0; }\n"
							 "  function b() returns(uint n) { return 1; }\n"
							 "  function c() returns(uint n) { return 2; }\n"
							 "  function f() returns(uint n) { return 3; }\n"
							 "}\n";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction(0, bytes()) == toBigEndian(u256(0)));
	BOOST_CHECK(callContractFunction(1, bytes()) == toBigEndian(u256(1)));
	BOOST_CHECK(callContractFunction(2, bytes()) == toBigEndian(u256(2)));
	BOOST_CHECK(callContractFunction(3, bytes()) == toBigEndian(u256(3)));
	BOOST_CHECK(callContractFunction(4, bytes()) == bytes());
}

BOOST_AUTO_TEST_CASE(while_loop)
{
	char const* sourceCode = "contract test {\n"
							 "  function f(uint n) returns(uint nfac) {\n"
							 "    nfac = 1;\n"
							 "    var i = 2;\n"
							 "    while (i <= n) nfac *= i++;\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);

	auto while_loop_cpp = [](u256 const& n) -> u256
	{
		u256 nfac = 1;
		u256 i = 2;
		while (i <= n)
			nfac *= i++;

		return nfac;
	};

	testSolidityAgainstCppOnRange(0, while_loop_cpp, 0, 5);
}

BOOST_AUTO_TEST_CASE(break_outside_loop)
{
	// break and continue outside loops should be simply ignored
	char const* sourceCode = "contract test {\n"
							 "  function f(uint x) returns(uint y) {\n"
							 "    break; continue; return 2;\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	testSolidityAgainstCpp(0, [](u256 const&) -> u256 { return 2; }, u256(0));
}

BOOST_AUTO_TEST_CASE(nested_loops)
{
	// tests that break and continue statements in nested loops jump to the correct place
	char const* sourceCode = "contract test {\n"
							 "  function f(uint x) returns(uint y) {\n"
							 "    while (x > 1) {\n"
							 "      if (x == 10) break;\n"
							 "      while (x > 5) {\n"
							 "        if (x == 8) break;\n"
							 "        x--;\n"
							 "        if (x == 6) continue;\n"
							 "        return x;\n"
							 "      }\n"
							 "      x--;\n"
							 "      if (x == 3) continue;\n"
							 "      break;\n"
							 "    }\n"
							 "    return x;\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);

	auto nested_loops_cpp = [](u256  n) -> u256
	{
		while (n > 1)
		{
			if (n == 10)
				break;
			while (n > 5)
			{
				if (n == 8)
					break;
				n--;
				if (n == 6)
					continue;
				return n;
			}
			n--;
			if (n == 3)
				continue;
			break;
		}

		return n;
	};

	testSolidityAgainstCppOnRange(0, nested_loops_cpp, 0, 12);
}

BOOST_AUTO_TEST_CASE(calling_other_functions)
{
	// note that the index of a function is its index in the sorted sequence of functions
	char const* sourceCode = "contract collatz {\n"
							 "  function run(uint x) returns(uint y) {\n"
							 "    while ((y = x) > 1) {\n"
							 "      if (x % 2 == 0) x = evenStep(x);\n"
							 "      else x = oddStep(x);\n"
							 "    }\n"
							 "  }\n"
							 "  function evenStep(uint x) returns(uint y) {\n"
							 "    return x / 2;\n"
							 "  }\n"
							 "  function oddStep(uint x) returns(uint y) {\n"
							 "    return 3 * x + 1;\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);

	auto evenStep_cpp = [](u256 const& n) -> u256
	{
		return n / 2;
	};

	auto oddStep_cpp = [](u256 const& n) -> u256
	{
		return 3 * n + 1;
	};

	auto collatz_cpp = [&evenStep_cpp, &oddStep_cpp](u256 n) -> u256
	{
		u256 y;
		while ((y = n) > 1)
		{
			if (n % 2 == 0)
				n = evenStep_cpp(n);
			else
				n = oddStep_cpp(n);
		}
		return y;
	};

	testSolidityAgainstCpp(2, collatz_cpp, u256(0));
	testSolidityAgainstCpp(2, collatz_cpp, u256(1));
	testSolidityAgainstCpp(2, collatz_cpp, u256(2));
	testSolidityAgainstCpp(2, collatz_cpp, u256(8));
	testSolidityAgainstCpp(2, collatz_cpp, u256(127));
}

BOOST_AUTO_TEST_CASE(many_local_variables)
{
	char const* sourceCode = "contract test {\n"
							 "  function run(uint x1, uint x2, uint x3) returns(uint y) {\n"
							 "    var a = 0x1; var b = 0x10; var c = 0x100;\n"
							 "    y = a + b + c + x1 + x2 + x3;\n"
							 "    y += b + x2;\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	auto f = [](u256 const& x1, u256 const& x2, u256 const& x3) -> u256
	{
		u256 a = 0x1;
		u256 b = 0x10;
		u256 c = 0x100;
		u256 y = a + b + c + x1 + x2 + x3;
		return y + b + x2;
	};
	testSolidityAgainstCpp(0, f, u256(0x1000), u256(0x10000), u256(0x100000));
}

BOOST_AUTO_TEST_CASE(packing_unpacking_types)
{
	char const* sourceCode = "contract test {\n"
							 "  function run(bool a, uint32 b, uint64 c) returns(uint256 y) {\n"
							 "    if (a) y = 1;\n"
							 "    y = y * 0x100000000 | ~b;\n"
							 "    y = y * 0x10000000000000000 | ~c;\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction(0, fromHex("01""0f0f0f0f""f0f0f0f0f0f0f0f0"))
				== fromHex("00000000000000000000000000000000000000""01""f0f0f0f0""0f0f0f0f0f0f0f0f"));
}

BOOST_AUTO_TEST_CASE(multiple_return_values)
{
	char const* sourceCode = "contract test {\n"
							 "  function run(bool x1, uint x2) returns(uint y1, bool y2, uint y3) {\n"
							 "    y1 = x2; y2 = x1;\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction(0, bytes(1, 1) + toBigEndian(u256(0xcd)))
				== toBigEndian(u256(0xcd)) + bytes(1, 1) + toBigEndian(u256(0)));
}

BOOST_AUTO_TEST_CASE(short_circuiting)
{
	char const* sourceCode = "contract test {\n"
							 "  function run(uint x) returns(uint y) {\n"
							 "    x == 0 || ((x = 8) > 0);\n"
							 "    return x;"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);

	auto short_circuiting_cpp = [](u256 n) -> u256
	{
		n == 0 || (n = 8) > 0;
		return n;
	};

	testSolidityAgainstCppOnRange(0, short_circuiting_cpp, 0, 2);
}

BOOST_AUTO_TEST_CASE(high_bits_cleaning)
{
	char const* sourceCode = "contract test {\n"
							 "  function run() returns(uint256 y) {\n"
							 "    uint32 x = uint32(0xffffffff) + 10;\n"
							 "    if (x >= 0xffffffff) return 0;\n"
							 "    return x;"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	auto high_bits_cleaning_cpp = []() -> u256
	{
		uint32_t x = uint32_t(0xffffffff) + 10;
		if (x >= 0xffffffff)
			return 0;
		return x;
	};
	testSolidityAgainstCpp(0, high_bits_cleaning_cpp);
}

BOOST_AUTO_TEST_CASE(sign_extension)
{
	char const* sourceCode = "contract test {\n"
							 "  function run() returns(uint256 y) {\n"
							 "    int64 x = -int32(0xff);\n"
							 "    if (x >= 0xff) return 0;\n"
							 "    return -uint256(x);"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	auto sign_extension_cpp = []() -> u256
	{
		int64_t x = -int32_t(0xff);
		if (x >= 0xff)
			return 0;
		return u256(x) * -1;
	};
	testSolidityAgainstCpp(0, sign_extension_cpp);
}

BOOST_AUTO_TEST_CASE(small_unsigned_types)
{
	char const* sourceCode = "contract test {\n"
							 "  function run() returns(uint256 y) {\n"
							 "    uint32 x = uint32(0xffffff) * 0xffffff;\n"
							 "    return x / 0x100;"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	auto small_unsigned_types_cpp = []() -> u256
	{
		uint32_t x = uint32_t(0xffffff) * 0xffffff;
		return x / 0x100;
	};
	testSolidityAgainstCpp(0, small_unsigned_types_cpp);
}

BOOST_AUTO_TEST_CASE(small_signed_types)
{
	char const* sourceCode = "contract test {\n"
							 "  function run() returns(int256 y) {\n"
							 "    return -int32(10) * -int64(20);\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	auto small_signed_types_cpp = []() -> u256
	{
		return -int32_t(10) * -int64_t(20);
	};
	testSolidityAgainstCpp(0, small_signed_types_cpp);
}

BOOST_AUTO_TEST_CASE(strings)
{
	char const* sourceCode = "contract test {\n"
							 "  function fixed() returns(string32 ret) {\n"
							 "    return \"abc\\x00\\xff__\";\n"
							 "  }\n"
							 "  function pipeThrough(string2 small, bool one) returns(string16 large, bool oneRet) {\n"
							 "    oneRet = one;\n"
							 "    large = small;\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	bytes expectation(32, 0);
	expectation[0] = byte('a');
	expectation[1] = byte('b');
	expectation[2] = byte('c');
	expectation[3] = byte(0);
	expectation[4] = byte(0xff);
	expectation[5] = byte('_');
	expectation[6] = byte('_');
	BOOST_CHECK(callContractFunction(0, bytes()) == expectation);
	expectation = bytes(17, 0);
	expectation[0] = 0;
	expectation[1] = 2;
	expectation[16] = 1;
	BOOST_CHECK(callContractFunction(1, bytes({0x00, 0x02, 0x01})) == expectation);
}

BOOST_AUTO_TEST_CASE(empty_string_on_stack)
{
	char const* sourceCode = "contract test {\n"
							 "  function run(string0 empty, uint8 inp) returns(uint16 a, string0 b, string4 c) {\n"
							 "    var x = \"abc\";\n"
							 "    var y = \"\";\n"
							 "    var z = inp;\n"
							 "    a = z; b = y; c = x;"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction(0, bytes(1, 0x02)) == bytes({0x00, 0x02, 0x61/*'a'*/, 0x62/*'b'*/, 0x63/*'c'*/, 0x00}));
}

BOOST_AUTO_TEST_CASE(state_smoke_test)
{
	char const* sourceCode = "contract test {\n"
							 "  uint256 value1;\n"
							 "  uint256 value2;\n"
							 "  function get(uint8 which) returns (uint256 value) {\n"
							 "    if (which == 0) return value1;\n"
							 "    else return value2;\n"
							 "  }\n"
							 "  function set(uint8 which, uint256 value) {\n"
							 "    if (which == 0) value1 = value;\n"
							 "    else value2 = value;\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction(0, bytes(1, 0x00)) == toBigEndian(u256(0)));
	BOOST_CHECK(callContractFunction(0, bytes(1, 0x01)) == toBigEndian(u256(0)));
	BOOST_CHECK(callContractFunction(1, bytes(1, 0x00) + toBigEndian(u256(0x1234))) == bytes());
	BOOST_CHECK(callContractFunction(1, bytes(1, 0x01) + toBigEndian(u256(0x8765))) == bytes());
	BOOST_CHECK(callContractFunction(0, bytes(1, 0x00)) == toBigEndian(u256(0x1234)));
	BOOST_CHECK(callContractFunction(0, bytes(1, 0x01)) == toBigEndian(u256(0x8765)));
	BOOST_CHECK(callContractFunction(1, bytes(1, 0x00) + toBigEndian(u256(0x3))) == bytes());
	BOOST_CHECK(callContractFunction(0, bytes(1, 0x00)) == toBigEndian(u256(0x3)));
}

BOOST_AUTO_TEST_CASE(simple_mapping)
{
	char const* sourceCode = "contract test {\n"
							 "  mapping(uint8 => uint8) table;\n"
							 "  function get(uint8 k) returns (uint8 v) {\n"
							 "    return table[k];\n"
							 "  }\n"
							 "  function set(uint8 k, uint8 v) {\n"
							 "    table[k] = v;\n"
							 "  }\n"
							 "}";
	compileAndRun(sourceCode);
	
	// msvc seems to have problems with initializer-list, when there is only 1 param in the list
	BOOST_CHECK(callContractFunction(0, bytes(1, 0x00)) == bytes(1, 0x00));
	BOOST_CHECK(callContractFunction(0, bytes(1, 0x01)) == bytes(1, 0x00));
	BOOST_CHECK(callContractFunction(0, bytes(1, 0xa7)) == bytes(1, 0x00));
	callContractFunction(1, bytes({0x01, 0xa1}));
	BOOST_CHECK(callContractFunction(0, bytes(1, 0x00)) == bytes(1, 0x00));
	BOOST_CHECK(callContractFunction(0, bytes(1, 0x01)) == bytes(1, 0xa1));
	BOOST_CHECK(callContractFunction(0, bytes(1, 0xa7)) == bytes(1, 0x00));
	callContractFunction(1, bytes({0x00, 0xef}));
	BOOST_CHECK(callContractFunction(0, bytes(1, 0x00)) == bytes(1, 0xef));
	BOOST_CHECK(callContractFunction(0, bytes(1, 0x01)) == bytes(1, 0xa1));
	BOOST_CHECK(callContractFunction(0, bytes(1, 0xa7)) == bytes(1, 0x00));
	callContractFunction(1, bytes({0x01, 0x05}));
	BOOST_CHECK(callContractFunction(0, bytes(1, 0x00)) == bytes(1, 0xef));
	BOOST_CHECK(callContractFunction(0, bytes(1, 0x01)) == bytes(1, 0x05));
	BOOST_CHECK(callContractFunction(0, bytes(1, 0xa7)) == bytes(1, 0x00));
}

BOOST_AUTO_TEST_CASE(mapping_state)
{
	char const* sourceCode = "contract Ballot {\n"
							 "  mapping(address => bool) canVote;\n"
							 "  mapping(address => uint) voteCount;\n"
							 "  mapping(address => bool) voted;\n"
							 "  function getVoteCount(address addr) returns (uint retVoteCount) {\n"
							 "    return voteCount[addr];\n"
							 "  }\n"
							 "  function grantVoteRight(address addr) {\n"
							 "    canVote[addr] = true;\n"
							 "  }\n"
							 "  function vote(address voter, address vote) returns (bool success) {\n"
							 "    if (!canVote[voter] || voted[voter]) return false;\n"
							 "    voted[voter] = true;\n"
							 "    voteCount[vote] = voteCount[vote] + 1;\n"
							 "    return true;\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	class Ballot
	{
	public:
		u256 getVoteCount(u160 _address) { return m_voteCount[_address]; }
		void grantVoteRight(u160 _address) { m_canVote[_address] = true; }
		bool vote(u160 _voter, u160 _vote)
		{
			if (!m_canVote[_voter] || m_voted[_voter]) return false;
			m_voted[_voter] = true;
			m_voteCount[_vote]++;
			return true;
		}
	private:
		map<u160, bool> m_canVote;
		map<u160, u256> m_voteCount;
		map<u160, bool> m_voted;
	} ballot;

	auto getVoteCount = bind(&Ballot::getVoteCount, &ballot, _1);
	auto grantVoteRight = bind(&Ballot::grantVoteRight, &ballot, _1);
	auto vote = bind(&Ballot::vote, &ballot, _1, _2);
	testSolidityAgainstCpp(0, getVoteCount, u160(0));
	testSolidityAgainstCpp(0, getVoteCount, u160(1));
	testSolidityAgainstCpp(0, getVoteCount, u160(2));
	// voting without vote right shourd be rejected
	testSolidityAgainstCpp(2, vote, u160(0), u160(2));
	testSolidityAgainstCpp(0, getVoteCount, u160(0));
	testSolidityAgainstCpp(0, getVoteCount, u160(1));
	testSolidityAgainstCpp(0, getVoteCount, u160(2));
	// grant vote rights
	testSolidityAgainstCpp(1, grantVoteRight, u160(0));
	testSolidityAgainstCpp(1, grantVoteRight, u160(1));
	// vote, should increase 2's vote count
	testSolidityAgainstCpp(2, vote, u160(0), u160(2));
	testSolidityAgainstCpp(0, getVoteCount, u160(0));
	testSolidityAgainstCpp(0, getVoteCount, u160(1));
	testSolidityAgainstCpp(0, getVoteCount, u160(2));
	// vote again, should be rejected
	testSolidityAgainstCpp(2, vote, u160(0), u160(1));
	testSolidityAgainstCpp(0, getVoteCount, u160(0));
	testSolidityAgainstCpp(0, getVoteCount, u160(1));
	testSolidityAgainstCpp(0, getVoteCount, u160(2));
	// vote without right to vote
	testSolidityAgainstCpp(2, vote, u160(2), u160(1));
	testSolidityAgainstCpp(0, getVoteCount, u160(0));
	testSolidityAgainstCpp(0, getVoteCount, u160(1));
	testSolidityAgainstCpp(0, getVoteCount, u160(2));
	// grant vote right and now vote again
	testSolidityAgainstCpp(1, grantVoteRight, u160(2));
	testSolidityAgainstCpp(2, vote, u160(2), u160(1));
	testSolidityAgainstCpp(0, getVoteCount, u160(0));
	testSolidityAgainstCpp(0, getVoteCount, u160(1));
	testSolidityAgainstCpp(0, getVoteCount, u160(2));
}

BOOST_AUTO_TEST_CASE(mapping_state_inc_dec)
{
	char const* sourceCode = "contract test {\n"
							 "  uint value;\n"
							 "  mapping(uint => uint) table;\n"
							 "  function f(uint x) returns (uint y) {\n"
							 "    value = x;\n"
							 "    if (x > 0) table[++value] = 8;\n"
							 "    if (x > 1) value--;\n"
							 "    if (x > 2) table[value]++;\n"
							 "    return --table[value++];\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);

	u256 value = 0;
	map<u256, u256> table;
	auto f = [&](u256 const& _x) -> u256
	{
		value = _x;
		if (_x > 0)
			table[++value] = 8;
		if (_x > 1)
			value --;
		if (_x > 2)
			table[value]++;
		return --table[value++];
	};
	testSolidityAgainstCppOnRange(0, f, 0, 5);
}

BOOST_AUTO_TEST_CASE(multi_level_mapping)
{
	char const* sourceCode = "contract test {\n"
							 "  mapping(uint => mapping(uint => uint)) table;\n"
							 "  function f(uint x, uint y, uint z) returns (uint w) {\n"
							 "    if (z == 0) return table[x][y];\n"
							 "    else return table[x][y] = z;\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);

	map<u256, map<u256, u256>> table;
	auto f = [&](u256 const& _x, u256 const& _y, u256 const& _z) -> u256
	{
		if (_z == 0) return table[_x][_y];
		else return table[_x][_y] = _z;
	};
	testSolidityAgainstCpp(0, f, u256(4), u256(5), u256(0));
	testSolidityAgainstCpp(0, f, u256(5), u256(4), u256(0));
	testSolidityAgainstCpp(0, f, u256(4), u256(5), u256(9));
	testSolidityAgainstCpp(0, f, u256(4), u256(5), u256(0));
	testSolidityAgainstCpp(0, f, u256(5), u256(4), u256(0));
	testSolidityAgainstCpp(0, f, u256(5), u256(4), u256(7));
	testSolidityAgainstCpp(0, f, u256(4), u256(5), u256(0));
	testSolidityAgainstCpp(0, f, u256(5), u256(4), u256(0));
}

BOOST_AUTO_TEST_CASE(structs)
{
	char const* sourceCode = "contract test {\n"
							 "  struct s1 {\n"
							 "    uint8 x;\n"
							 "    bool y;\n"
							 "  }\n"
							 "  struct s2 {\n"
							 "    uint32 z;\n"
							 "    s1 s1data;\n"
							 "    mapping(uint8 => s2) recursive;\n"
							 "  }\n"
							 "  s2 data;\n"
							 "  function check() returns (bool ok) {\n"
							 "    return data.z == 1 && data.s1data.x == 2 && \n"
							 "        data.s1data.y == true && \n"
							 "        data.recursive[3].recursive[4].z == 5 && \n"
							 "        data.recursive[4].recursive[3].z == 6 && \n"
							 "        data.recursive[0].s1data.y == false && \n"
							 "        data.recursive[4].z == 9;\n"
							 "  }\n"
							 "  function set() {\n"
							 "    data.z = 1;\n"
							 "    data.s1data.x = 2;\n"
							 "    data.s1data.y = true;\n"
							 "    data.recursive[3].recursive[4].z = 5;\n"
							 "    data.recursive[4].recursive[3].z = 6;\n"
							 "    data.recursive[0].s1data.y = false;\n"
							 "    data.recursive[4].z = 9;\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction(0) == bytes(1, 0x00));
	BOOST_CHECK(callContractFunction(1) == bytes());
	BOOST_CHECK(callContractFunction(0) == bytes(1, 0x01));
}

BOOST_AUTO_TEST_CASE(struct_reference)
{
	char const* sourceCode = "contract test {\n"
							 "  struct s2 {\n"
							 "    uint32 z;\n"
							 "    mapping(uint8 => s2) recursive;\n"
							 "  }\n"
							 "  s2 data;\n"
							 "  function check() returns (bool ok) {\n"
							 "    return data.z == 2 && \n"
							 "        data.recursive[0].z == 3 && \n"
							 "        data.recursive[0].recursive[1].z == 0 && \n"
							 "        data.recursive[0].recursive[0].z == 1;\n"
							 "  }\n"
							 "  function set() {\n"
							 "    data.z = 2;\n"
							 "    var map = data.recursive;\n"
							 "    s2 inner = map[0];\n"
							 "    inner.z = 3;\n"
							 "    inner.recursive[0].z = inner.recursive[1].z + 1;\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction(0) == bytes(1, 0x00));
	BOOST_CHECK(callContractFunction(1) == bytes());
	BOOST_CHECK(callContractFunction(0) == bytes(1, 0x01));
}

BOOST_AUTO_TEST_CASE(constructor)
{
	char const* sourceCode = "contract test {\n"
							 "  mapping(uint => uint) data;\n"
							 "  function test() {\n"
							 "    data[7] = 8;\n"
							 "  }\n"
							 "  function get(uint key) returns (uint value) {\n"
							 "    return data[key];"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	map<u256, byte> data;
	data[7] = 8;
	auto get = [&](u256 const& _x) -> u256
	{
		return data[_x];
	};
	testSolidityAgainstCpp(0, get, u256(6));
	testSolidityAgainstCpp(0, get, u256(7));
}

BOOST_AUTO_TEST_CASE(balance)
{
	char const* sourceCode = "contract test {\n"
							 "  function getBalance() returns (uint256 balance) {\n"
							 "    return address(this).balance;\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode, 23);
	BOOST_CHECK(callContractFunction(0) == toBigEndian(u256(23)));
}

BOOST_AUTO_TEST_CASE(blockchain)
{
	char const* sourceCode = "contract test {\n"
							 "  function someInfo() returns (uint256 value, address coinbase, uint256 blockNumber) {\n"
							 "    value = msg.value;\n"
							 "    coinbase = block.coinbase;\n"
							 "    blockNumber = block.number;\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode, 27);
	BOOST_CHECK(callContractFunction(0, bytes{0}, u256(28)) == toBigEndian(u256(28)) + bytes(20, 0) + toBigEndian(u256(1)));
}

BOOST_AUTO_TEST_CASE(function_types)
{
	char const* sourceCode = "contract test {\n"
							 "  function a(bool selector) returns (uint b) {\n"
							 "    var f = fun1;\n"
							 "    if (selector) f = fun2;\n"
							 "    return f(9);\n"
							 "  }\n"
							 "  function fun1(uint x) returns (uint b) {\n"
							 "    return 11;\n"
							 "  }\n"
							 "  function fun2(uint x) returns (uint b) {\n"
							 "    return 12;\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction(0, bytes{0}) == toBigEndian(u256(11)));
	BOOST_CHECK(callContractFunction(0, bytes{1}) == toBigEndian(u256(12)));
}

BOOST_AUTO_TEST_CASE(send_ether)
{
	char const* sourceCode = "contract test {\n"
							 "  function a(address addr, uint amount) returns (uint ret) {\n"
							 "    addr.send(amount);\n"
							 "    return address(this).balance;\n"
							 "  }\n"
							 "}\n";
	u256 amount(130);
	compileAndRun(sourceCode, amount + 1);
	u160 address(23);
	BOOST_CHECK(callContractFunction(0, address, amount) == toBigEndian(u256(1)));
	BOOST_CHECK_EQUAL(m_state.balance(address), amount);
}

BOOST_AUTO_TEST_CASE(suicide)
{
	char const* sourceCode = "contract test {\n"
							 "  function a(address receiver) returns (uint ret) {\n"
							 "    suicide(receiver);\n"
							 "    return 10;\n"
							 "  }\n"
							 "}\n";
	u256 amount(130);
	compileAndRun(sourceCode, amount);
	u160 address(23);
	BOOST_CHECK(callContractFunction(0, address) == bytes());
	BOOST_CHECK(!m_state.addressHasCode(m_contractAddress));
	BOOST_CHECK_EQUAL(m_state.balance(address), amount);
}

BOOST_AUTO_TEST_CASE(sha3)
{
	char const* sourceCode = "contract test {\n"
							 "  function a(hash input) returns (hash sha3hash) {\n"
							 "    return sha3(input);\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	auto f = [&](u256 const& _x) -> u256
	{
		return dev::sha3(toBigEndian(_x));
	};
	testSolidityAgainstCpp(0, f, u256(4));
	testSolidityAgainstCpp(0, f, u256(5));
	testSolidityAgainstCpp(0, f, u256(-1));
}

BOOST_AUTO_TEST_CASE(sha256)
{
	char const* sourceCode = "contract test {\n"
							 "  function a(hash input) returns (hash sha256hash) {\n"
							 "    return sha256(input);\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	auto f = [&](u256 const& _input) -> u256
	{
		h256 ret;
		dev::sha256(dev::ref(toBigEndian(_input)), bytesRef(&ret[0], 32));
		return ret;
	};
	testSolidityAgainstCpp(0, f, u256(4));
	testSolidityAgainstCpp(0, f, u256(5));
	testSolidityAgainstCpp(0, f, u256(-1));
}

BOOST_AUTO_TEST_CASE(ripemd)
{
	char const* sourceCode = "contract test {\n"
							 "  function a(hash input) returns (hash sha256hash) {\n"
							 "    return ripemd160(input);\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	auto f = [&](u256 const& _input) -> u256
	{
		h256 ret;
		dev::ripemd160(dev::ref(toBigEndian(_input)), bytesRef(&ret[0], 32));
		return u256(ret) >> (256 - 160);
	};
	testSolidityAgainstCpp(0, f, u256(4));
	testSolidityAgainstCpp(0, f, u256(5));
	testSolidityAgainstCpp(0, f, u256(-1));
}

BOOST_AUTO_TEST_CASE(ecrecover)
{
	char const* sourceCode = "contract test {\n"
							 "  function a(hash h, uint8 v, hash r, hash s) returns (address addr) {\n"
							 "    return ecrecover(h, v, r, s);\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	u256 h("0x18c547e4f7b0f325ad1e56f57e26c745b09a3e503d86e00e5255ff7f715d3d1c");
	byte v = 28;
	u256 r("0x73b1693892219d736caba55bdb67216e485557ea6b6af75f37096c9aa6a5a75f");
	u256 s("0xeeb940b1d03b21e36b0e47e79769f095fe2ab855bd91e3a38756b7d75a9c4549");
	u160 addr("0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b");
	BOOST_CHECK(callContractFunction(0, h, v, r, s) == toBigEndian(addr));
}

BOOST_AUTO_TEST_CASE(inter_contract_calls)
{
	char const* sourceCode = R"(
		contract Helper {
			function multiply(uint a, uint b) returns (uint c) {
				return a * b;
			}
		}
		contract Main {
			Helper h;
			function callHelper(uint a, uint b) returns (uint c) {
				return h.multiply(a, b);
			}
			function getHelper() returns (address haddress) {
				return address(h);
			}
			function setHelper(address haddress) {
				h = Helper(haddress);
			}
		})";
	compileAndRun(sourceCode, 0, "Helper");
	u160 const helperAddress = m_contractAddress;
	compileAndRun(sourceCode, 0, "Main");
	BOOST_REQUIRE(callContractFunction(2, helperAddress) == bytes());
	BOOST_REQUIRE(callContractFunction(1, helperAddress) == toBigEndian(helperAddress));
	u256 a(3456789);
	u256 b("0x282837623374623234aa74");
	BOOST_REQUIRE(callContractFunction(0, a, b) == toBigEndian(a * b));
}

BOOST_AUTO_TEST_CASE(inter_contract_calls_with_complex_parameters)
{
	char const* sourceCode = R"(
		contract Helper {
			function sel(uint a, bool select, uint b) returns (uint c) {
				if (select) return a; else return b;
			}
		}
		contract Main {
			Helper h;
			function callHelper(uint a, bool select, uint b) returns (uint c) {
				return h.sel(a, select, b) * 3;
			}
			function getHelper() returns (address haddress) {
				return address(h);
			}
			function setHelper(address haddress) {
				h = Helper(haddress);
			}
		})";
	compileAndRun(sourceCode, 0, "Helper");
	u160 const helperAddress = m_contractAddress;
	compileAndRun(sourceCode, 0, "Main");
	BOOST_REQUIRE(callContractFunction(2, helperAddress) == bytes());
	BOOST_REQUIRE(callContractFunction(1, helperAddress) == toBigEndian(helperAddress));
	u256 a(3456789);
	u256 b("0x282837623374623234aa74");
	BOOST_REQUIRE(callContractFunction(0, a, true, b) == toBigEndian(a * 3));
	BOOST_REQUIRE(callContractFunction(0, a, false, b) == toBigEndian(b * 3));
}

BOOST_AUTO_TEST_CASE(inter_contract_calls_accessing_this)
{
	char const* sourceCode = R"(
		contract Helper {
			function getAddress() returns (address addr) {
				return address(this);
			}
		}
		contract Main {
			Helper h;
			function callHelper() returns (address addr) {
				return h.getAddress();
			}
			function getHelper() returns (address addr) {
				return address(h);
			}
			function setHelper(address addr) {
				h = Helper(addr);
			}
		})";
	compileAndRun(sourceCode, 0, "Helper");
	u160 const helperAddress = m_contractAddress;
	compileAndRun(sourceCode, 0, "Main");
	BOOST_REQUIRE(callContractFunction(2, helperAddress) == bytes());
	BOOST_REQUIRE(callContractFunction(1, helperAddress) == toBigEndian(helperAddress));
	BOOST_REQUIRE(callContractFunction(0) == toBigEndian(helperAddress));
}

BOOST_AUTO_TEST_CASE(calls_to_this)
{
	char const* sourceCode = R"(
		contract Helper {
			function invoke(uint a, uint b) returns (uint c) {
				return this.multiply(a, b, 10);
			}
			function multiply(uint a, uint b, uint8 c) returns (uint ret) {
				return a * b + c;
			}
		}
		contract Main {
			Helper h;
			function callHelper(uint a, uint b) returns (uint ret) {
				return h.invoke(a, b);
			}
			function getHelper() returns (address addr) {
				return address(h);
			}
			function setHelper(address addr) {
				h = Helper(addr);
			}
		})";
	compileAndRun(sourceCode, 0, "Helper");
	u160 const helperAddress = m_contractAddress;
	compileAndRun(sourceCode, 0, "Main");
	BOOST_REQUIRE(callContractFunction(2, helperAddress) == bytes());
	BOOST_REQUIRE(callContractFunction(1, helperAddress) == toBigEndian(helperAddress));
	u256 a(3456789);
	u256 b("0x282837623374623234aa74");
	BOOST_REQUIRE(callContractFunction(0, a, b) == toBigEndian(a * b + 10));
}

BOOST_AUTO_TEST_CASE(inter_contract_calls_with_local_vars)
{
	// note that a reference to another contract's function occupies two stack slots,
	// so this tests correct stack slot allocation
	char const* sourceCode = R"(
		contract Helper {
			function multiply(uint a, uint b) returns (uint c) {
				return a * b;
			}
		}
		contract Main {
			Helper h;
			function callHelper(uint a, uint b) returns (uint c) {
				var fu = h.multiply;
				var y = 9;
				var ret = fu(a, b);
				return ret + y;
			}
			function getHelper() returns (address haddress) {
				return address(h);
			}
			function setHelper(address haddress) {
				h = Helper(haddress);
			}
		})";
	compileAndRun(sourceCode, 0, "Helper");
	u160 const helperAddress = m_contractAddress;
	compileAndRun(sourceCode, 0, "Main");
	BOOST_REQUIRE(callContractFunction(2, helperAddress) == bytes());
	BOOST_REQUIRE(callContractFunction(1, helperAddress) == toBigEndian(helperAddress));
	u256 a(3456789);
	u256 b("0x282837623374623234aa74");
	BOOST_REQUIRE(callContractFunction(0, a, b) == toBigEndian(a * b + 9));
}

BOOST_AUTO_TEST_CASE(strings_in_calls)
{
	char const* sourceCode = R"(
		contract Helper {
			function invoke(string3 x, bool stop) returns (string4 ret) {
				return x;
			}
		}
		contract Main {
			Helper h;
			function callHelper(string2 x, bool stop) returns (string5 ret) {
				return h.invoke(x, stop);
			}
			function getHelper() returns (address addr) {
				return address(h);
			}
			function setHelper(address addr) {
				h = Helper(addr);
			}
		})";
	compileAndRun(sourceCode, 0, "Helper");
	u160 const helperAddress = m_contractAddress;
	compileAndRun(sourceCode, 0, "Main");
	BOOST_REQUIRE(callContractFunction(2, helperAddress) == bytes());
	BOOST_REQUIRE(callContractFunction(1, helperAddress) == toBigEndian(helperAddress));
	BOOST_CHECK(callContractFunction(0, bytes({0, 'a', 1})) == bytes({0, 'a', 0, 0, 0}));
}

BOOST_AUTO_TEST_CASE(constructor_arguments)
{
	char const* sourceCode = R"(
		contract Helper {
			string3 name;
			bool flag;
			function Helper(string3 x, bool f) {
				name = x;
				flag = f;
			}
			function getName() returns (string3 ret) { return name; }
			function getFlag() returns (bool ret) { return flag; }
		}
		contract Main {
			Helper h;
			function Main() {
				h = new Helper("abc", true);
			}
			function getFlag() returns (bool ret) { return h.getFlag(); }
			function getName() returns (string3 ret) { return h.getName(); }
		})";
	compileAndRun(sourceCode, 0, "Main");
	BOOST_REQUIRE(callContractFunction(0) == bytes({0x01}));
	BOOST_REQUIRE(callContractFunction(1) == bytes({'a', 'b', 'c'}));
}

BOOST_AUTO_TEST_CASE(functions_called_by_constructor)
{
	char const* sourceCode = R"(
		contract Test {
			string3 name;
			bool flag;
			function Test() {
				setName("abc");
			}
			function getName() returns (string3 ret) { return name; }
		private:
			function setName(string3 _name) { name = _name; }
		})";
	compileAndRun(sourceCode);
	BOOST_REQUIRE(callContractFunction(0) == bytes({'a', 'b', 'c'}));
}

BOOST_AUTO_TEST_SUITE_END()

}
}
} // end namespaces

