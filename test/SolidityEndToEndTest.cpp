
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
 * @author Gav Wood <g@ethdev.com>
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

BOOST_FIXTURE_TEST_SUITE(SolidityEndToEndTest, ExecutionFramework)

BOOST_AUTO_TEST_CASE(smoke_test)
{
	char const* sourceCode = "contract test {\n"
							 "  function f(uint a) returns(uint d) { return a * 7; }\n"
							 "}\n";
	compileAndRun(sourceCode);
	testSolidityAgainstCppOnRange("f(uint256)", [](u256 const& a) -> u256 { return a * 7; }, 0, 100);
}

BOOST_AUTO_TEST_CASE(empty_contract)
{
	char const* sourceCode = "contract test {\n"
							 "}\n";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("i_am_not_there()", bytes()).empty());
}

BOOST_AUTO_TEST_CASE(exp_operator)
{
	char const* sourceCode = R"(
		contract test {
			function f(uint a) returns(uint d) { return 2 ** a; }
		})";
	compileAndRun(sourceCode);
	testSolidityAgainstCppOnRange("f(uint256)", [](u256 const& a) -> u256 { return u256(1 << a.convert_to<int>()); }, 0, 16);
}

BOOST_AUTO_TEST_CASE(exp_operator_const)
{
	char const* sourceCode = R"(
		contract test {
			function f() returns(uint d) { return 2 ** 3; }
		})";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("f()", bytes()) == toBigEndian(u256(8)));
}

BOOST_AUTO_TEST_CASE(exp_operator_const_signed)
{
	char const* sourceCode = R"(
		contract test {
			function f() returns(int d) { return (-2) ** 3; }
		})";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("f()", bytes()) == toBigEndian(u256(-8)));
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

	testSolidityAgainstCppOnRange("f(uint256)", recursive_calls_cpp, 0, 5);
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
	BOOST_CHECK(callContractFunction("a()", bytes()) == toBigEndian(u256(0)));
	BOOST_CHECK(callContractFunction("b()", bytes()) == toBigEndian(u256(1)));
	BOOST_CHECK(callContractFunction("c()", bytes()) == toBigEndian(u256(2)));
	BOOST_CHECK(callContractFunction("f()", bytes()) == toBigEndian(u256(3)));
	BOOST_CHECK(callContractFunction("i_am_not_there()", bytes()) == bytes());
}

BOOST_AUTO_TEST_CASE(named_args)
{
	char const* sourceCode = "contract test {\n"
							 "  function a(uint a, uint b, uint c) returns (uint r) { r = a * 100 + b * 10 + c * 1; }\n"
							 "  function b() returns (uint r) { r = a({a: 1, b: 2, c: 3}); }\n"
							 "}\n";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("b()", bytes()) == toBigEndian(u256(123)));
}

BOOST_AUTO_TEST_CASE(disorder_named_args)
{
	char const* sourceCode = "contract test {\n"
							 "  function a(uint a, uint b, uint c) returns (uint r) { r = a * 100 + b * 10 + c * 1; }\n"
							 "  function b() returns (uint r) { r = a({c: 3, a: 1, b: 2}); }\n"
							 "}\n";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("b()", bytes()) == toBigEndian(u256(123)));
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

	testSolidityAgainstCppOnRange("f(uint256)", while_loop_cpp, 0, 5);
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
	testSolidityAgainstCpp("f(uint256)", [](u256 const&) -> u256 { return 2; }, u256(0));
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

	testSolidityAgainstCppOnRange("f(uint256)", nested_loops_cpp, 0, 12);
}

BOOST_AUTO_TEST_CASE(for_loop)
{
	char const* sourceCode = "contract test {\n"
							 "  function f(uint n) returns(uint nfac) {\n"
							 "    nfac = 1;\n"
							 "    for (var i = 2; i <= n; i++)\n"
							 "        nfac *= i;\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);

	auto for_loop_cpp = [](u256 const& n) -> u256
	{
		u256 nfac = 1;
		for (auto i = 2; i <= n; i++)
			nfac *= i;
		return nfac;
	};

	testSolidityAgainstCppOnRange("f(uint256)", for_loop_cpp, 0, 5);
}

BOOST_AUTO_TEST_CASE(for_loop_empty)
{
	char const* sourceCode = "contract test {\n"
							 "  function f() returns(uint ret) {\n"
							 "    ret = 1;\n"
							 "    for (;;)\n"
							 "    {\n"
							 "        ret += 1;\n"
							 "        if (ret >= 10) break;\n"
							 "    }\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);

	auto for_loop_empty_cpp = []() -> u256
	{
		u256 ret = 1;
		for (;;)
		{
			ret += 1;
			if (ret >= 10) break;
		}
		return ret;
	};

	testSolidityAgainstCpp("f()", for_loop_empty_cpp);
}

BOOST_AUTO_TEST_CASE(for_loop_simple_init_expr)
{
	char const* sourceCode = "contract test {\n"
							 "  function f(uint n) returns(uint nfac) {\n"
							 "    nfac = 1;\n"
							 "    uint256 i;\n"
							 "    for (i = 2; i <= n; i++)\n"
							 "        nfac *= i;\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);

	auto for_loop_simple_init_expr_cpp = [](u256 const& n) -> u256
	{
		u256 nfac = 1;
		u256 i;
		for (i = 2; i <= n; i++)
			nfac *= i;
		return nfac;
	};

	testSolidityAgainstCppOnRange("f(uint256)", for_loop_simple_init_expr_cpp, 0, 5);
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

	testSolidityAgainstCpp("run(uint256)", collatz_cpp, u256(0));
	testSolidityAgainstCpp("run(uint256)", collatz_cpp, u256(1));
	testSolidityAgainstCpp("run(uint256)", collatz_cpp, u256(2));
	testSolidityAgainstCpp("run(uint256)", collatz_cpp, u256(8));
	testSolidityAgainstCpp("run(uint256)", collatz_cpp, u256(127));
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
	testSolidityAgainstCpp("run(uint256,uint256,uint256)", f, u256(0x1000), u256(0x10000), u256(0x100000));
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
	BOOST_CHECK(callContractFunction("run(bool,uint32,uint64)", true, fromHex("0f0f0f0f"), fromHex("f0f0f0f0f0f0f0f0"))
				== fromHex("00000000000000000000000000000000000000""01""f0f0f0f0""0f0f0f0f0f0f0f0f"));
}

BOOST_AUTO_TEST_CASE(packing_signed_types)
{
	char const* sourceCode = "contract test {\n"
							 "  function run() returns(int8 y) {\n"
							 "    uint8 x = 0xfa;\n"
							 "    return int8(x);\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("run()")
				== fromHex("fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffa"));
}

BOOST_AUTO_TEST_CASE(multiple_return_values)
{
	char const* sourceCode = "contract test {\n"
							 "  function run(bool x1, uint x2) returns(uint y1, bool y2, uint y3) {\n"
							 "    y1 = x2; y2 = x1;\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("run(bool,uint256)", true, 0xcd) == encodeArgs(0xcd, true, 0));
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
		(void)(n == 0 || (n = 8) > 0);
		return n;
	};

	testSolidityAgainstCppOnRange("run(uint256)", short_circuiting_cpp, 0, 2);
}

BOOST_AUTO_TEST_CASE(high_bits_cleaning)
{
	char const* sourceCode = "contract test {\n"
							 "  function run() returns(uint256 y) {\n"
							 "    uint32 t = uint32(0xffffffff);\n"
							 "    uint32 x = t + 10;\n"
							 "    if (x >= 0xffffffff) return 0;\n"
							 "    return x;"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	auto high_bits_cleaning_cpp = []() -> u256
	{
		uint32_t t = uint32_t(0xffffffff);
		uint32_t x = t + 10;
		if (x >= 0xffffffff)
			return 0;
		return x;
	};
	testSolidityAgainstCpp("run()", high_bits_cleaning_cpp);
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
	testSolidityAgainstCpp("run()", sign_extension_cpp);
}

BOOST_AUTO_TEST_CASE(small_unsigned_types)
{
	char const* sourceCode = "contract test {\n"
							 "  function run() returns(uint256 y) {\n"
							 "    uint32 t = uint32(0xffffff);\n"
							 "    uint32 x = t * 0xffffff;\n"
							 "    return x / 0x100;"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	auto small_unsigned_types_cpp = []() -> u256
	{
		uint32_t t = uint32_t(0xffffff);
		uint32_t x = t * 0xffffff;
		return x / 0x100;
	};
	testSolidityAgainstCpp("run()", small_unsigned_types_cpp);
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
	testSolidityAgainstCpp("run()", small_signed_types_cpp);
}

BOOST_AUTO_TEST_CASE(strings)
{
	char const* sourceCode = "contract test {\n"
							 "  function fixed() returns(bytes32 ret) {\n"
							 "    return \"abc\\x00\\xff__\";\n"
							 "  }\n"
							 "  function pipeThrough(bytes2 small, bool one) returns(bytes16 large, bool oneRet) {\n"
							 "    oneRet = one;\n"
							 "    large = small;\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("fixed()") == encodeArgs(string("abc\0\xff__", 7)));
	BOOST_CHECK(callContractFunction("pipeThrough(bytes2,bool)", string("\0\x02", 2), true) == encodeArgs(string("\0\x2", 2), true));
}

BOOST_AUTO_TEST_CASE(empty_string_on_stack)
{
	char const* sourceCode = "contract test {\n"
							 "  function run(bytes0 empty, uint8 inp) returns(uint16 a, bytes0 b, bytes4 c) {\n"
							 "    var x = \"abc\";\n"
							 "    var y = \"\";\n"
							 "    var z = inp;\n"
							 "    a = z; b = y; c = x;"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("run(bytes0,uint8)", string(), byte(0x02)) == encodeArgs(0x2, string(""), string("abc\0")));
}

BOOST_AUTO_TEST_CASE(inc_dec_operators)
{
	char const* sourceCode = R"(
		contract test {
			uint8 x;
			uint v;
			function f() returns (uint r) {
				uint a = 6;
				r = a;
				r += (a++) * 0x10;
				r += (++a) * 0x100;
				v = 3;
				r += (v++) * 0x1000;
				r += (++v) * 0x10000;
			}
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("f()") == encodeArgs(0x53866));
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
	BOOST_CHECK(callContractFunction("get(uint8)", byte(0x00)) == encodeArgs(0));
	BOOST_CHECK(callContractFunction("get(uint8)", byte(0x01)) == encodeArgs(0));
	BOOST_CHECK(callContractFunction("set(uint8,uint256)", byte(0x00), 0x1234) == encodeArgs());
	BOOST_CHECK(callContractFunction("set(uint8,uint256)", byte(0x01), 0x8765) == encodeArgs());
	BOOST_CHECK(callContractFunction("get(uint8)", byte( 0x00)) == encodeArgs(0x1234));
	BOOST_CHECK(callContractFunction("get(uint8)", byte(0x01)) == encodeArgs(0x8765));
	BOOST_CHECK(callContractFunction("set(uint8,uint256)", byte(0x00), 0x3) == encodeArgs());
	BOOST_CHECK(callContractFunction("get(uint8)", byte(0x00)) == encodeArgs(0x3));
}

BOOST_AUTO_TEST_CASE(compound_assign)
{
	char const* sourceCode = "contract test {\n"
							 "  uint value1;\n"
							 "  uint value2;\n"
							 "  function f(uint x, uint y) returns (uint w) {\n"
							 "    uint value3 = y;"
							 "    value1 += x;\n"
							 "    value3 *= x;"
							 "    value2 *= value3 + value1;\n"
							 "    return value2 += 7;"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);

	u256 value1;
	u256 value2;
	auto f = [&](u256 const& _x, u256 const& _y) -> u256
	{
		u256 value3 = _y;
		value1 += _x;
		value3 *= _x;
		value2 *= value3 + value1;
		return value2 += 7;
	};
	testSolidityAgainstCpp("f(uint256,uint256)", f, u256(0), u256(6));
	testSolidityAgainstCpp("f(uint256,uint256)", f, u256(1), u256(3));
	testSolidityAgainstCpp("f(uint256,uint256)", f, u256(2), u256(25));
	testSolidityAgainstCpp("f(uint256,uint256)", f, u256(3), u256(69));
	testSolidityAgainstCpp("f(uint256,uint256)", f, u256(4), u256(84));
	testSolidityAgainstCpp("f(uint256,uint256)", f, u256(5), u256(2));
	testSolidityAgainstCpp("f(uint256,uint256)", f, u256(6), u256(51));
	testSolidityAgainstCpp("f(uint256,uint256)", f, u256(7), u256(48));
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

	BOOST_CHECK(callContractFunction("get(uint8)", byte(0)) == encodeArgs(byte(0x00)));
	BOOST_CHECK(callContractFunction("get(uint8)", byte(0x01)) == encodeArgs(byte(0x00)));
	BOOST_CHECK(callContractFunction("get(uint8)", byte(0xa7)) == encodeArgs(byte(0x00)));
	callContractFunction("set(uint8,uint8)", byte(0x01), byte(0xa1));
	BOOST_CHECK(callContractFunction("get(uint8)", byte(0x00)) == encodeArgs(byte(0x00)));
	BOOST_CHECK(callContractFunction("get(uint8)", byte(0x01)) == encodeArgs(byte(0xa1)));
	BOOST_CHECK(callContractFunction("get(uint8)", byte(0xa7)) == encodeArgs(byte(0x00)));
	callContractFunction("set(uint8,uint8)", byte(0x00), byte(0xef));
	BOOST_CHECK(callContractFunction("get(uint8)", byte(0x00)) == encodeArgs(byte(0xef)));
	BOOST_CHECK(callContractFunction("get(uint8)", byte(0x01)) == encodeArgs(byte(0xa1)));
	BOOST_CHECK(callContractFunction("get(uint8)", byte(0xa7)) == encodeArgs(byte(0x00)));
	callContractFunction("set(uint8,uint8)", byte(0x01), byte(0x05));
	BOOST_CHECK(callContractFunction("get(uint8)", byte(0x00)) == encodeArgs(byte(0xef)));
	BOOST_CHECK(callContractFunction("get(uint8)", byte(0x01)) == encodeArgs(byte(0x05)));
	BOOST_CHECK(callContractFunction("get(uint8)", byte(0xa7)) == encodeArgs(byte(0x00)));
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
	testSolidityAgainstCpp("getVoteCount(address)", getVoteCount, u160(0));
	testSolidityAgainstCpp("getVoteCount(address)", getVoteCount, u160(1));
	testSolidityAgainstCpp("getVoteCount(address)", getVoteCount, u160(2));
	// voting without vote right should be rejected
	testSolidityAgainstCpp("vote(address,address)", vote, u160(0), u160(2));
	testSolidityAgainstCpp("getVoteCount(address)", getVoteCount, u160(0));
	testSolidityAgainstCpp("getVoteCount(address)", getVoteCount, u160(1));
	testSolidityAgainstCpp("getVoteCount(address)", getVoteCount, u160(2));
	// grant vote rights
	testSolidityAgainstCpp("grantVoteRight(address)", grantVoteRight, u160(0));
	testSolidityAgainstCpp("grantVoteRight(address)", grantVoteRight, u160(1));
	// vote, should increase 2's vote count
	testSolidityAgainstCpp("vote(address,address)", vote, u160(0), u160(2));
	testSolidityAgainstCpp("getVoteCount(address)", getVoteCount, u160(0));
	testSolidityAgainstCpp("getVoteCount(address)", getVoteCount, u160(1));
	testSolidityAgainstCpp("getVoteCount(address)", getVoteCount, u160(2));
	// vote again, should be rejected
	testSolidityAgainstCpp("vote(address,address)", vote, u160(0), u160(1));
	testSolidityAgainstCpp("getVoteCount(address)", getVoteCount, u160(0));
	testSolidityAgainstCpp("getVoteCount(address)", getVoteCount, u160(1));
	testSolidityAgainstCpp("getVoteCount(address)", getVoteCount, u160(2));
	// vote without right to vote
	testSolidityAgainstCpp("vote(address,address)", vote, u160(2), u160(1));
	testSolidityAgainstCpp("getVoteCount(address)", getVoteCount, u160(0));
	testSolidityAgainstCpp("getVoteCount(address)", getVoteCount, u160(1));
	testSolidityAgainstCpp("getVoteCount(address)", getVoteCount, u160(2));
	// grant vote right and now vote again
	testSolidityAgainstCpp("grantVoteRight(address)", grantVoteRight, u160(2));
	testSolidityAgainstCpp("vote(address,address)", vote, u160(2), u160(1));
	testSolidityAgainstCpp("getVoteCount(address)", getVoteCount, u160(0));
	testSolidityAgainstCpp("getVoteCount(address)", getVoteCount, u160(1));
	testSolidityAgainstCpp("getVoteCount(address)", getVoteCount, u160(2));
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
	testSolidityAgainstCppOnRange("f(uint256)", f, 0, 5);
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
	testSolidityAgainstCpp("f(uint256,uint256,uint256)", f, u256(4), u256(5), u256(0));
	testSolidityAgainstCpp("f(uint256,uint256,uint256)", f, u256(5), u256(4), u256(0));
	testSolidityAgainstCpp("f(uint256,uint256,uint256)", f, u256(4), u256(5), u256(9));
	testSolidityAgainstCpp("f(uint256,uint256,uint256)", f, u256(4), u256(5), u256(0));
	testSolidityAgainstCpp("f(uint256,uint256,uint256)", f, u256(5), u256(4), u256(0));
	testSolidityAgainstCpp("f(uint256,uint256,uint256)", f, u256(5), u256(4), u256(7));
	testSolidityAgainstCpp("f(uint256,uint256,uint256)", f, u256(4), u256(5), u256(0));
	testSolidityAgainstCpp("f(uint256,uint256,uint256)", f, u256(5), u256(4), u256(0));
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
	BOOST_CHECK(callContractFunction("check()") == encodeArgs(false));
	BOOST_CHECK(callContractFunction("set()") == bytes());
	BOOST_CHECK(callContractFunction("check()") == encodeArgs(true));
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
	BOOST_CHECK(callContractFunction("check()") == encodeArgs(false));
	BOOST_CHECK(callContractFunction("set()") == bytes());
	BOOST_CHECK(callContractFunction("check()") == encodeArgs(true));
}

BOOST_AUTO_TEST_CASE(deleteStruct)
{
	char const* sourceCode = R"(
		contract test {
			struct topStruct {
				nestedStruct nstr;
				emptyStruct empty;
				uint topValue;
				mapping (uint => uint) topMapping;
			}
			uint toDelete;
			topStruct str;
			struct nestedStruct {
				uint nestedValue;
				mapping (uint => bool) nestedMapping;
			}
			struct emptyStruct{
			}
			function test(){
				toDelete = 5;
				str.topValue = 1;
				str.topMapping[0] = 1;
				str.topMapping[1] = 2;

				str.nstr.nestedValue = 2;
				str.nstr.nestedMapping[0] = true;
				str.nstr.nestedMapping[1] = false;
				delete str;
				delete toDelete;
			}
			function getToDelete() returns (uint res){
				res = toDelete;
			}
			function getTopValue() returns(uint topValue){
				topValue = str.topValue;
			}
			function getNestedValue() returns(uint nestedValue){
				nestedValue = str.nstr.nestedValue;
			}
			function getTopMapping(uint index) returns(uint ret) {
			   ret = str.topMapping[index];
			}
			function getNestedMapping(uint index) returns(bool ret) {
				return str.nstr.nestedMapping[index];
			}
		})";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("getToDelete()") == encodeArgs(0));
	BOOST_CHECK(callContractFunction("getTopValue()") == encodeArgs(0));
	BOOST_CHECK(callContractFunction("getNestedValue()") == encodeArgs(0));
	// mapping values should be the same
	BOOST_CHECK(callContractFunction("getTopMapping(uint256)", 0) == encodeArgs(1));
	BOOST_CHECK(callContractFunction("getTopMapping(uint256)", 1) == encodeArgs(2));
	BOOST_CHECK(callContractFunction("getNestedMapping(uint256)", 0) == encodeArgs(true));
	BOOST_CHECK(callContractFunction("getNestedMapping(uint256)", 1) == encodeArgs(false));
}

BOOST_AUTO_TEST_CASE(deleteLocal)
{
	char const* sourceCode = R"(
		contract test {
			function delLocal() returns (uint res){
				uint v = 5;
				delete v;
				res = v;
			}
		})";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("delLocal()") == encodeArgs(0));
}

BOOST_AUTO_TEST_CASE(deleteLocals)
{
	char const* sourceCode = R"(
		contract test {
			function delLocal() returns (uint res1, uint res2){
				uint v = 5;
				uint w = 6;
				uint x = 7;
				delete v;
				res1 = w;
				res2 = x;
			}
		})";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("delLocal()") == encodeArgs(6, 7));
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
	testSolidityAgainstCpp("get(uint256)", get, u256(6));
	testSolidityAgainstCpp("get(uint256)", get, u256(7));
}

BOOST_AUTO_TEST_CASE(simple_accessor)
{
	char const* sourceCode = "contract test {\n"
							 "  uint256 public data;\n"
							 "  function test() {\n"
							 "    data = 8;\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("data()") == encodeArgs(8));
}

BOOST_AUTO_TEST_CASE(multiple_elementary_accessors)
{
	char const* sourceCode = "contract test {\n"
							 "  uint256 public data;\n"
							 "  bytes6 public name;\n"
							 "  bytes32 public a_hash;\n"
							 "  address public an_address;\n"
							 "  function test() {\n"
							 "    data = 8;\n"
							 "    name = \"Celina\";\n"
							 "    a_hash = sha3(123);\n"
							 "    an_address = address(0x1337);\n"
							 "    super_secret_data = 42;\n"
							 "  }\n"
							 "  uint256 super_secret_data;"
							 "}\n";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("data()") == encodeArgs(8));
	BOOST_CHECK(callContractFunction("name()") == encodeArgs("Celina"));
	BOOST_CHECK(callContractFunction("a_hash()") == encodeArgs(dev::sha3(bytes(1, 0x7b))));
	BOOST_CHECK(callContractFunction("an_address()") == encodeArgs(toBigEndian(u160(0x1337))));
	BOOST_CHECK(callContractFunction("super_secret_data()") == bytes());
}

BOOST_AUTO_TEST_CASE(complex_accessors)
{
	char const* sourceCode = R"(
		contract test {
			mapping(uint256 => bytes4) public to_string_map;
			mapping(uint256 => bool) public to_bool_map;
			mapping(uint256 => uint256) public to_uint_map;
			mapping(uint256 => mapping(uint256 => uint256)) public to_multiple_map;
			function test() {
				to_string_map[42] = "24";
				to_bool_map[42] = false;
				to_uint_map[42] = 12;
				to_multiple_map[42][23] = 31;
			}
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("to_string_map(uint256)", 42) == encodeArgs("24"));
	BOOST_CHECK(callContractFunction("to_bool_map(uint256)", 42) == encodeArgs(false));
	BOOST_CHECK(callContractFunction("to_uint_map(uint256)", 42) == encodeArgs(12));
	BOOST_CHECK(callContractFunction("to_multiple_map(uint256,uint256)", 42, 23) == encodeArgs(31));
}

BOOST_AUTO_TEST_CASE(struct_accessor)
{
	char const* sourceCode = R"(
		contract test {
			struct Data { uint a; uint8 b; mapping(uint => uint) c; bool d; }
			mapping(uint => Data) public data;
			function test() {
				data[7].a = 1;
				data[7].b = 2;
				data[7].c[0] = 3;
				data[7].d = true;
			}
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("data(uint256)", 7) == encodeArgs(1, 2, true));
}

BOOST_AUTO_TEST_CASE(balance)
{
	char const* sourceCode = "contract test {\n"
							 "  function getBalance() returns (uint256 balance) {\n"
							 "    return address(this).balance;\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode, 23);
	BOOST_CHECK(callContractFunction("getBalance()") == encodeArgs(23));
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
	BOOST_CHECK(callContractFunctionWithValue("someInfo()", 28) == encodeArgs(28, 0, 1));
}

BOOST_AUTO_TEST_CASE(msg_sig)
{
	char const* sourceCode = R"(
		contract test {
			function foo(uint256 a) returns (bytes4 value) {
				return msg.sig;
			}
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunctionWithValue("foo(uint256)", 13) == encodeArgs(asString(FixedHash<4>(dev::sha3("foo(uint256)")).asBytes())));
}

BOOST_AUTO_TEST_CASE(msg_sig_after_internal_call_is_same)
{
	char const* sourceCode = R"(
		contract test {
			function boo() returns (bytes4 value) {
				return msg.sig;
			}
			function foo(uint256 a) returns (bytes4 value) {
				return boo();
			}
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunctionWithValue("foo(uint256)", 13) == encodeArgs(asString(FixedHash<4>(dev::sha3("foo(uint256)")).asBytes())));
}

BOOST_AUTO_TEST_CASE(now)
{
	char const* sourceCode = "contract test {\n"
							 "  function someInfo() returns (bool success) {\n"
							 "    return block.timestamp == now && now > 0;\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("someInfo()") == encodeArgs(true));
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
	BOOST_CHECK(callContractFunction("a(bool)", false) == encodeArgs(11));
	BOOST_CHECK(callContractFunction("a(bool)", true) == encodeArgs(12));
}

BOOST_AUTO_TEST_CASE(type_conversions_cleanup)
{
	// 22-byte integer converted to a contract (i.e. address, 20 bytes), converted to a 32 byte
	// integer should drop the first two bytes
	char const* sourceCode = R"(
		contract Test {
			function test() returns (uint ret) { return uint(address(Test(address(0x11223344556677889900112233445566778899001122)))); }
		})";
	compileAndRun(sourceCode);
	BOOST_REQUIRE(callContractFunction("test()") == bytes({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
														   0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x11, 0x22,
														   0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0x11, 0x22}));
}

BOOST_AUTO_TEST_CASE(convert_fixed_bytes_to_fixed_bytes_smaller_size)
{
	char const* sourceCode = R"(
		contract Test {
			function pipeTrough(bytes3 input) returns (bytes2 ret) {
				return bytes2(input);
			}
		})";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("pipeTrough(bytes3)", "abc") == encodeArgs("ab"));
}

BOOST_AUTO_TEST_CASE(convert_uint_to_fixed_bytes_same_size)
{
	char const* sourceCode = R"(
		contract Test {
			function uintToBytes(uint256 h) returns (bytes32 s) {
				return bytes32(h);
			}
		})";
	compileAndRun(sourceCode);
	u256 a("0x6162630000000000000000000000000000000000000000000000000000000000");
	BOOST_CHECK(callContractFunction("uintToBytes(uint256)", a) == encodeArgs(a));
}

BOOST_AUTO_TEST_CASE(convert_uint_to_fixed_bytes_different_size)
{
	char const* sourceCode = R"(
		contract Test {
			function uintToBytes(uint160 h) returns (bytes20 s) {
				return bytes20(h);
			}
		})";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("uintToBytes(uint160)",
			u160("0x6161626361626361626361616263616263616263")) == encodeArgs(string("aabcabcabcaabcabcabc")));
}

BOOST_AUTO_TEST_CASE(convert_fixed_bytes_to_uint_same_size)
{
	char const* sourceCode = R"(
		contract Test {
			function bytesToUint(bytes32 s) returns (uint256 h) {
				return uint(s);
			}
		})";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("bytesToUint(bytes32)", string("abc2")) ==
		encodeArgs(u256("0x6162633200000000000000000000000000000000000000000000000000000000")));
}

BOOST_AUTO_TEST_CASE(convert_fixed_bytes_to_uint_different_size)
{
	char const* sourceCode = R"(
		contract Test {
			function bytesToUint(bytes20 s) returns (uint160 h) {
				return uint160(s);
			}
		})";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("bytesToUint(bytes20)", string("aabcabcabcaabcabcabc")) ==
		encodeArgs(u160("0x6161626361626361626361616263616263616263")));
}


BOOST_AUTO_TEST_CASE(convert_fixed_bytes_to_uint_different_min_size)
{
	char const* sourceCode = R"(
		contract Test {
			function bytesToUint(bytes1 s) returns (uint8 h) {
				return uint8(s);
			}
		})";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("bytesToUint(bytes1)", string("a")) ==
		encodeArgs(u256("0x61")));
}


BOOST_AUTO_TEST_CASE(convert_uint_to_fixed_bytes_different_min_size)
{
	char const* sourceCode = R"(
		contract Test {
			function UintToBytes(uint8 h) returns (bytes1 s) {
				return bytes1(h);
			}
		})";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("UintToBytes(uint8)", u256("0x61")) ==
		encodeArgs(string("a")));
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
	BOOST_CHECK(callContractFunction("a(address,uint256)", address, amount) == encodeArgs(1));
	BOOST_CHECK_EQUAL(m_state.balance(address), amount);
}

BOOST_AUTO_TEST_CASE(log0)
{
	char const* sourceCode = "contract test {\n"
							 "  function a() {\n"
							 "    log0(1);\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	callContractFunction("a()");
	BOOST_CHECK_EQUAL(m_logs.size(), 1);
	BOOST_CHECK_EQUAL(m_logs[0].address, m_contractAddress);
	BOOST_CHECK_EQUAL(h256(m_logs[0].data), h256(u256(1)));
	BOOST_CHECK_EQUAL(m_logs[0].topics.size(), 0);
}

BOOST_AUTO_TEST_CASE(log1)
{
	char const* sourceCode = "contract test {\n"
							 "  function a() {\n"
							 "    log1(1, 2);\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	callContractFunction("a()");
	BOOST_CHECK_EQUAL(m_logs.size(), 1);
	BOOST_CHECK_EQUAL(m_logs[0].address, m_contractAddress);
	BOOST_CHECK_EQUAL(h256(m_logs[0].data), h256(u256(1)));
	BOOST_CHECK_EQUAL(m_logs[0].topics.size(), 1);
	BOOST_CHECK_EQUAL(m_logs[0].topics[0], h256(u256(2)));
}

BOOST_AUTO_TEST_CASE(log2)
{
	char const* sourceCode = "contract test {\n"
							 "  function a() {\n"
							 "    log2(1, 2, 3);\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	callContractFunction("a()");
	BOOST_CHECK_EQUAL(m_logs.size(), 1);
	BOOST_CHECK_EQUAL(m_logs[0].address, m_contractAddress);
	BOOST_CHECK_EQUAL(h256(m_logs[0].data), h256(u256(1)));
	BOOST_CHECK_EQUAL(m_logs[0].topics.size(), 2);
	for (unsigned i = 0; i < 2; ++i)
		BOOST_CHECK_EQUAL(m_logs[0].topics[i], h256(u256(i + 2)));
}

BOOST_AUTO_TEST_CASE(log3)
{
	char const* sourceCode = "contract test {\n"
							 "  function a() {\n"
							 "    log3(1, 2, 3, 4);\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	callContractFunction("a()");
	BOOST_CHECK_EQUAL(m_logs.size(), 1);
	BOOST_CHECK_EQUAL(m_logs[0].address, m_contractAddress);
	BOOST_CHECK_EQUAL(h256(m_logs[0].data), h256(u256(1)));
	BOOST_CHECK_EQUAL(m_logs[0].topics.size(), 3);
	for (unsigned i = 0; i < 3; ++i)
		BOOST_CHECK_EQUAL(m_logs[0].topics[i], h256(u256(i + 2)));
}

BOOST_AUTO_TEST_CASE(log4)
{
	char const* sourceCode = "contract test {\n"
							 "  function a() {\n"
							 "    log4(1, 2, 3, 4, 5);\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	callContractFunction("a()");
	BOOST_CHECK_EQUAL(m_logs.size(), 1);
	BOOST_CHECK_EQUAL(m_logs[0].address, m_contractAddress);
	BOOST_CHECK_EQUAL(h256(m_logs[0].data), h256(u256(1)));
	BOOST_CHECK_EQUAL(m_logs[0].topics.size(), 4);
	for (unsigned i = 0; i < 4; ++i)
		BOOST_CHECK_EQUAL(m_logs[0].topics[i], h256(u256(i + 2)));
}

BOOST_AUTO_TEST_CASE(log_in_constructor)
{
	char const* sourceCode = "contract test {\n"
							 "  function test() {\n"
							 "    log1(1, 2);\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	BOOST_CHECK_EQUAL(m_logs.size(), 1);
	BOOST_CHECK_EQUAL(m_logs[0].address, m_contractAddress);
	BOOST_CHECK_EQUAL(h256(m_logs[0].data), h256(u256(1)));
	BOOST_CHECK_EQUAL(m_logs[0].topics.size(), 1);
	BOOST_CHECK_EQUAL(m_logs[0].topics[0], h256(u256(2)));
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
	BOOST_CHECK(callContractFunction("a(address)", address) == bytes());
	BOOST_CHECK(!m_state.addressHasCode(m_contractAddress));
	BOOST_CHECK_EQUAL(m_state.balance(address), amount);
}

BOOST_AUTO_TEST_CASE(sha3)
{
	char const* sourceCode = "contract test {\n"
							 "  function a(bytes32 input) returns (bytes32 sha3hash) {\n"
							 "    return sha3(input);\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	auto f = [&](u256 const& _x) -> u256
	{
		return dev::sha3(toBigEndian(_x));
	};
	testSolidityAgainstCpp("a(bytes32)", f, u256(4));
	testSolidityAgainstCpp("a(bytes32)", f, u256(5));
	testSolidityAgainstCpp("a(bytes32)", f, u256(-1));
}

BOOST_AUTO_TEST_CASE(sha256)
{
	char const* sourceCode = "contract test {\n"
							 "  function a(bytes32 input) returns (bytes32 sha256hash) {\n"
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
	testSolidityAgainstCpp("a(bytes32)", f, u256(4));
	testSolidityAgainstCpp("a(bytes32)", f, u256(5));
	testSolidityAgainstCpp("a(bytes32)", f, u256(-1));
}

BOOST_AUTO_TEST_CASE(ripemd)
{
	char const* sourceCode = "contract test {\n"
							 "  function a(bytes32 input) returns (bytes32 sha256hash) {\n"
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
	testSolidityAgainstCpp("a(bytes32)", f, u256(4));
	testSolidityAgainstCpp("a(bytes32)", f, u256(5));
	testSolidityAgainstCpp("a(bytes32)", f, u256(-1));
}

BOOST_AUTO_TEST_CASE(ecrecover)
{
	char const* sourceCode = "contract test {\n"
							 "  function a(bytes32 h, uint8 v, bytes32 r, bytes32 s) returns (address addr) {\n"
							 "    return ecrecover(h, v, r, s);\n"
							 "  }\n"
							 "}\n";
	compileAndRun(sourceCode);
	u256 h("0x18c547e4f7b0f325ad1e56f57e26c745b09a3e503d86e00e5255ff7f715d3d1c");
	byte v = 28;
	u256 r("0x73b1693892219d736caba55bdb67216e485557ea6b6af75f37096c9aa6a5a75f");
	u256 s("0xeeb940b1d03b21e36b0e47e79769f095fe2ab855bd91e3a38756b7d75a9c4549");
	u160 addr("0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b");
	BOOST_CHECK(callContractFunction("a(bytes32,uint8,bytes32,bytes32)", h, v, r, s) == encodeArgs(addr));
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
	u160 const c_helperAddress = m_contractAddress;
	compileAndRun(sourceCode, 0, "Main");
	BOOST_REQUIRE(callContractFunction("setHelper(address)", c_helperAddress) == bytes());
	BOOST_REQUIRE(callContractFunction("getHelper()", c_helperAddress) == encodeArgs(c_helperAddress));
	u256 a(3456789);
	u256 b("0x282837623374623234aa74");
	BOOST_REQUIRE(callContractFunction("callHelper(uint256,uint256)", a, b) == encodeArgs(a * b));
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
	u160 const c_helperAddress = m_contractAddress;
	compileAndRun(sourceCode, 0, "Main");
	BOOST_REQUIRE(callContractFunction("setHelper(address)", c_helperAddress) == bytes());
	BOOST_REQUIRE(callContractFunction("getHelper()", c_helperAddress) == encodeArgs(c_helperAddress));
	u256 a(3456789);
	u256 b("0x282837623374623234aa74");
	BOOST_REQUIRE(callContractFunction("callHelper(uint256,bool,uint256)", a, true, b) == encodeArgs(a * 3));
	BOOST_REQUIRE(callContractFunction("callHelper(uint256,bool,uint256)", a, false, b) == encodeArgs(b * 3));
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
	u160 const c_helperAddress = m_contractAddress;
	compileAndRun(sourceCode, 0, "Main");
	BOOST_REQUIRE(callContractFunction("setHelper(address)", c_helperAddress) == bytes());
	BOOST_REQUIRE(callContractFunction("getHelper()", c_helperAddress) == encodeArgs(c_helperAddress));
	BOOST_REQUIRE(callContractFunction("callHelper()") == encodeArgs(c_helperAddress));
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
	u160 const c_helperAddress = m_contractAddress;
	compileAndRun(sourceCode, 0, "Main");
	BOOST_REQUIRE(callContractFunction("setHelper(address)", c_helperAddress) == bytes());
	BOOST_REQUIRE(callContractFunction("getHelper()", c_helperAddress) == encodeArgs(c_helperAddress));
	u256 a(3456789);
	u256 b("0x282837623374623234aa74");
	BOOST_REQUIRE(callContractFunction("callHelper(uint256,uint256)", a, b) == encodeArgs(a * b + 10));
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
	u160 const c_helperAddress = m_contractAddress;
	compileAndRun(sourceCode, 0, "Main");
	BOOST_REQUIRE(callContractFunction("setHelper(address)", c_helperAddress) == bytes());
	BOOST_REQUIRE(callContractFunction("getHelper()", c_helperAddress) == encodeArgs(c_helperAddress));
	u256 a(3456789);
	u256 b("0x282837623374623234aa74");
	BOOST_REQUIRE(callContractFunction("callHelper(uint256,uint256)", a, b) == encodeArgs(a * b + 9));
}

BOOST_AUTO_TEST_CASE(fixed_bytes_in_calls)
{
	char const* sourceCode = R"(
		contract Helper {
			function invoke(bytes3 x, bool stop) returns (bytes4 ret) {
				return x;
			}
		}
		contract Main {
			Helper h;
			function callHelper(bytes2 x, bool stop) returns (bytes5 ret) {
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
	u160 const c_helperAddress = m_contractAddress;
	compileAndRun(sourceCode, 0, "Main");
	BOOST_REQUIRE(callContractFunction("setHelper(address)", c_helperAddress) == bytes());
	BOOST_REQUIRE(callContractFunction("getHelper()", c_helperAddress) == encodeArgs(c_helperAddress));
	BOOST_CHECK(callContractFunction("callHelper(bytes2,bool)", string("\0a", 2), true) == encodeArgs(string("\0a\0\0\0", 5)));
}

BOOST_AUTO_TEST_CASE(constructor_arguments)
{
	char const* sourceCode = R"(
		contract Helper {
			bytes3 name;
			bool flag;

			function Helper(bytes3 x, bool f) {
				name = x;
				flag = f;
			}
			function getName() returns (bytes3 ret) { return name; }
			function getFlag() returns (bool ret) { return flag; }
		}
		contract Main {
			Helper h;
			function Main() {
				h = new Helper("abc", true);
			}
			function getFlag() returns (bool ret) { return h.getFlag(); }
			function getName() returns (bytes3 ret) { return h.getName(); }
		})";
	compileAndRun(sourceCode, 0, "Main");
	BOOST_REQUIRE(callContractFunction("getFlag()") == encodeArgs(true));
	BOOST_REQUIRE(callContractFunction("getName()") == encodeArgs("abc"));
}

BOOST_AUTO_TEST_CASE(functions_called_by_constructor)
{
	char const* sourceCode = R"(
		contract Test {
			bytes3 name;
			bool flag;
			function Test() {
				setName("abc");
			}
			function getName() returns (bytes3 ret) { return name; }
			function setName(bytes3 _name) private { name = _name; }
		})";
	compileAndRun(sourceCode);
	BOOST_REQUIRE(callContractFunction("getName()") == encodeArgs("abc"));
}

BOOST_AUTO_TEST_CASE(contracts_as_addresses)
{
	char const* sourceCode = R"(
		contract helper {
		}
		contract test {
			helper h;
			function test() { h = new helper(); h.send(5); }
			function getBalance() returns (uint256 myBalance, uint256 helperBalance) {
				myBalance = this.balance;
				helperBalance = h.balance;
			}
		}
	)";
	compileAndRun(sourceCode, 20);
	BOOST_REQUIRE(callContractFunction("getBalance()") == encodeArgs(u256(20 - 5), u256(5)));
}

BOOST_AUTO_TEST_CASE(gas_and_value_basic)
{
	char const* sourceCode = R"(
		contract helper {
			bool flag;
			function getBalance() returns (uint256 myBalance) {
				return this.balance;
			}
			function setFlag() { flag = true; }
			function getFlag() returns (bool fl) { return flag; }
		}
		contract test {
			helper h;
			function test() { h = new helper(); }
			function sendAmount(uint amount) returns (uint256 bal) {
				return h.getBalance.value(amount)();
			}
			function outOfGas() returns (bool ret) {
				h.setFlag.gas(2)(); // should fail due to OOG
				return true;
			}
			function checkState() returns (bool flagAfter, uint myBal) {
				flagAfter = h.getFlag();
				myBal = this.balance;
			}
		}
	)";
	compileAndRun(sourceCode, 20);
	BOOST_REQUIRE(callContractFunction("sendAmount(uint256)", 5) == encodeArgs(5));
	// call to helper should not succeed but amount should be transferred anyway
	BOOST_REQUIRE(callContractFunction("outOfGas()", 5) == bytes());
	BOOST_REQUIRE(callContractFunction("checkState()", 5) == encodeArgs(false, 20 - 5));
}

BOOST_AUTO_TEST_CASE(gas_for_builtin)
{
	char const* sourceCode = R"(
		contract Contract {
			function test(uint g) returns (bytes32 data, bool flag) {
				data = ripemd160.gas(g)("abc");
				flag = true;
			}
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("test(uint256)", 500) == bytes());
	BOOST_CHECK(callContractFunction("test(uint256)", 800) == encodeArgs(u256("0x8eb208f7e05d987a9b044a8e98c6b087f15a0bfc"), true));
}

BOOST_AUTO_TEST_CASE(value_complex)
{
	char const* sourceCode = R"(
		contract helper {
			function getBalance() returns (uint256 myBalance) {
				return this.balance;
			}
		}
		contract test {
			helper h;
			function test() { h = new helper(); }
			function sendAmount(uint amount) returns (uint256 bal) {
				var x1 = h.getBalance.value(amount);
				uint someStackElement = 20;
				var x2 = x1.gas(1000);
				return x2.value(amount + 3)();// overwrite value
			}
		}
	)";
	compileAndRun(sourceCode, 20);
	BOOST_REQUIRE(callContractFunction("sendAmount(uint256)", 5) == encodeArgs(8));
}

BOOST_AUTO_TEST_CASE(value_insane)
{
	char const* sourceCode = R"(
		contract helper {
			function getBalance() returns (uint256 myBalance) {
				return this.balance;
			}
		}
		contract test {
			helper h;
			function test() { h = new helper(); }
			function sendAmount(uint amount) returns (uint256 bal) {
				var x1 = h.getBalance.value;
				var x2 = x1(amount).gas;
				var x3 = x2(1000).value;
				return x3(amount + 3)();// overwrite value
			}
		}
	)";
	compileAndRun(sourceCode, 20);
	BOOST_REQUIRE(callContractFunction("sendAmount(uint256)", 5) == encodeArgs(8));
}

BOOST_AUTO_TEST_CASE(value_for_constructor)
{
	char const* sourceCode = R"(
		contract Helper {
			bytes3 name;
			bool flag;
			function Helper(bytes3 x, bool f) {
				name = x;
				flag = f;
			}
			function getName() returns (bytes3 ret) { return name; }
			function getFlag() returns (bool ret) { return flag; }
		}
		contract Main {
			Helper h;
			function Main() {
				h = new Helper.value(10)("abc", true);
			}
			function getFlag() returns (bool ret) { return h.getFlag(); }
			function getName() returns (bytes3 ret) { return h.getName(); }
			function getBalances() returns (uint me, uint them) { me = this.balance; them = h.balance;}
		})";
	compileAndRun(sourceCode, 22, "Main");
	BOOST_REQUIRE(callContractFunction("getFlag()") == encodeArgs(true));
	BOOST_REQUIRE(callContractFunction("getName()") == encodeArgs("abc"));
	BOOST_REQUIRE(callContractFunction("getBalances()") == encodeArgs(12, 10));
}

BOOST_AUTO_TEST_CASE(virtual_function_calls)
{
	char const* sourceCode = R"(
		contract Base {
			function f() returns (uint i) { return g(); }
			function g() returns (uint i) { return 1; }
		}
		contract Derived is Base {
			function g() returns (uint i) { return 2; }
		}
	)";
	compileAndRun(sourceCode, 0, "Derived");
	BOOST_CHECK(callContractFunction("g()") == encodeArgs(2));
	BOOST_CHECK(callContractFunction("f()") == encodeArgs(2));
}

BOOST_AUTO_TEST_CASE(access_base_storage)
{
	char const* sourceCode = R"(
		contract Base {
			uint dataBase;
			function getViaBase() returns (uint i) { return dataBase; }
		}
		contract Derived is Base {
			uint dataDerived;
			function setData(uint base, uint derived) returns (bool r) {
				dataBase = base;
				dataDerived = derived;
				return true;
			}
			function getViaDerived() returns (uint base, uint derived) {
				base = dataBase;
				derived = dataDerived;
			}
		}
	)";
	compileAndRun(sourceCode, 0, "Derived");
	BOOST_CHECK(callContractFunction("setData(uint256,uint256)", 1, 2) == encodeArgs(true));
	BOOST_CHECK(callContractFunction("getViaBase()") == encodeArgs(1));
	BOOST_CHECK(callContractFunction("getViaDerived()") == encodeArgs(1, 2));
}

BOOST_AUTO_TEST_CASE(single_copy_with_multiple_inheritance)
{
	char const* sourceCode = R"(
		contract Base {
			uint data;
			function setData(uint i) { data = i; }
			function getViaBase() returns (uint i) { return data; }
		}
		contract A is Base { function setViaA(uint i) { setData(i); } }
		contract B is Base { function getViaB() returns (uint i) { return getViaBase(); } }
		contract Derived is Base, B, A { }
	)";
	compileAndRun(sourceCode, 0, "Derived");
	BOOST_CHECK(callContractFunction("getViaB()") == encodeArgs(0));
	BOOST_CHECK(callContractFunction("setViaA(uint256)", 23) == encodeArgs());
	BOOST_CHECK(callContractFunction("getViaB()") == encodeArgs(23));
}

BOOST_AUTO_TEST_CASE(explicit_base_cass)
{
	char const* sourceCode = R"(
		contract BaseBase { function g() returns (uint r) { return 1; } }
		contract Base is BaseBase { function g() returns (uint r) { return 2; } }
		contract Derived is Base {
			function f() returns (uint r) { return BaseBase.g(); }
			function g() returns (uint r) { return 3; }
		}
	)";
	compileAndRun(sourceCode, 0, "Derived");
	BOOST_CHECK(callContractFunction("g()") == encodeArgs(3));
	BOOST_CHECK(callContractFunction("f()") == encodeArgs(1));
}

BOOST_AUTO_TEST_CASE(base_constructor_arguments)
{
	char const* sourceCode = R"(
		contract BaseBase {
			uint m_a;
			function BaseBase(uint a) {
				m_a = a;
			}
		}
		contract Base is BaseBase(7) {
			function Base() {
				m_a *= m_a;
			}
		}
		contract Derived is Base() {
			function getA() returns (uint r) { return m_a; }
		}
	)";
	compileAndRun(sourceCode, 0, "Derived");
	BOOST_CHECK(callContractFunction("getA()") == encodeArgs(7 * 7));
}

BOOST_AUTO_TEST_CASE(function_usage_in_constructor_arguments)
{
	char const* sourceCode = R"(
		contract BaseBase {
			uint m_a;
			function BaseBase(uint a) {
				m_a = a;
			}
			function g() returns (uint r) { return 2; }
		}
		contract Base is BaseBase(BaseBase.g()) {
		}
		contract Derived is Base() {
			function getA() returns (uint r) { return m_a; }
		}
	)";
	compileAndRun(sourceCode, 0, "Derived");
	BOOST_CHECK(callContractFunction("getA()") == encodeArgs(2));
}

BOOST_AUTO_TEST_CASE(virtual_function_usage_in_constructor_arguments)
{
	char const* sourceCode = R"(
		contract BaseBase {
			uint m_a;
			function BaseBase(uint a) {
				m_a = a;
			}
			function overridden() returns (uint r) { return 1; }
			function g() returns (uint r) { return overridden(); }
		}
		contract Base is BaseBase(BaseBase.g()) {
		}
		contract Derived is Base() {
			function getA() returns (uint r) { return m_a; }
			function overridden() returns (uint r) { return 2; }
		}
	)";
	compileAndRun(sourceCode, 0, "Derived");
	BOOST_CHECK(callContractFunction("getA()") == encodeArgs(2));
}

BOOST_AUTO_TEST_CASE(constructor_argument_overriding)
{
	char const* sourceCode = R"(
		contract BaseBase {
			uint m_a;
			function BaseBase(uint a) {
				m_a = a;
			}
		}
		contract Base is BaseBase(2) { }
		contract Derived is BaseBase(3), Base {
			function getA() returns (uint r) { return m_a; }
		}
	)";
	compileAndRun(sourceCode, 0, "Derived");
	BOOST_CHECK(callContractFunction("getA()") == encodeArgs(3));
}

BOOST_AUTO_TEST_CASE(function_modifier)
{
	char const* sourceCode = R"(
		contract C {
			function getOne() nonFree returns (uint r) { return 1; }
			modifier nonFree { if (msg.value > 0) _ }
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("getOne()") == encodeArgs(0));
	BOOST_CHECK(callContractFunctionWithValue("getOne()", 1) == encodeArgs(1));
}

BOOST_AUTO_TEST_CASE(function_modifier_local_variables)
{
	char const* sourceCode = R"(
		contract C {
			modifier mod1 { var a = 1; var b = 2; _ }
			modifier mod2(bool a) { if (a) return; else _ }
			function f(bool a) mod1 mod2(a) returns (uint r) { return 3; }
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("f(bool)", true) == encodeArgs(0));
	BOOST_CHECK(callContractFunction("f(bool)", false) == encodeArgs(3));
}

BOOST_AUTO_TEST_CASE(function_modifier_loop)
{
	char const* sourceCode = R"(
		contract C {
			modifier repeat(uint count) { for (var i = 0; i < count; ++i) _ }
			function f() repeat(10) returns (uint r) { r += 1; }
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("f()") == encodeArgs(10));
}

BOOST_AUTO_TEST_CASE(function_modifier_multi_invocation)
{
	char const* sourceCode = R"(
		contract C {
			modifier repeat(bool twice) { if (twice) _ _ }
			function f(bool twice) repeat(twice) returns (uint r) { r += 1; }
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("f(bool)", false) == encodeArgs(1));
	BOOST_CHECK(callContractFunction("f(bool)", true) == encodeArgs(2));
}

BOOST_AUTO_TEST_CASE(function_modifier_multi_with_return)
{
	// Here, the explicit return prevents the second execution
	char const* sourceCode = R"(
		contract C {
			modifier repeat(bool twice) { if (twice) _ _ }
			function f(bool twice) repeat(twice) returns (uint r) { r += 1; return r; }
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("f(bool)", false) == encodeArgs(1));
	BOOST_CHECK(callContractFunction("f(bool)", true) == encodeArgs(1));
}

BOOST_AUTO_TEST_CASE(function_modifier_overriding)
{
	char const* sourceCode = R"(
		contract A {
			function f() mod returns (bool r) { return true; }
			modifier mod { _ }
		}
		contract C is A {
			modifier mod { }
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("f()") == encodeArgs(false));
}

BOOST_AUTO_TEST_CASE(function_modifier_calling_functions_in_creation_context)
{
	char const* sourceCode = R"(
		contract A {
			uint data;
			function A() mod1 { f1(); }
			function f1() mod2 { data |= 0x1; }
			function f2() { data |= 0x20; }
			function f3() { }
			modifier mod1 { f2(); _ }
			modifier mod2 { f3(); }
			function getData() returns (uint r) { return data; }
		}
		contract C is A {
			modifier mod1 { f4(); _ }
			function f3() { data |= 0x300; }
			function f4() { data |= 0x4000; }
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("getData()") == encodeArgs(0x4300));
}

BOOST_AUTO_TEST_CASE(function_modifier_for_constructor)
{
	char const* sourceCode = R"(
		contract A {
			uint data;
			function A() mod1 { data |= 2; }
			modifier mod1 { data |= 1; _ }
			function getData() returns (uint r) { return data; }
		}
		contract C is A {
			modifier mod1 { data |= 4; _ }
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("getData()") == encodeArgs(4 | 2));
}

BOOST_AUTO_TEST_CASE(use_std_lib)
{
	char const* sourceCode = R"(
		import "mortal";
		contract Icarus is mortal { }
	)";
	m_addStandardSources = true;
	u256 amount(130);
	u160 address(23);
	compileAndRun(sourceCode, amount, "Icarus");
	u256 balanceBefore = m_state.balance(m_sender);
	BOOST_CHECK(callContractFunction("kill()") == bytes());
	BOOST_CHECK(!m_state.addressHasCode(m_contractAddress));
	BOOST_CHECK(m_state.balance(m_sender) > balanceBefore);
}

BOOST_AUTO_TEST_CASE(crazy_elementary_typenames_on_stack)
{
	char const* sourceCode = R"(
		contract C {
			function f() returns (uint r) {
				uint; uint; uint; uint;
				int x = -7;
				var a = uint;
				return a(x);
			}
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("f()") == encodeArgs(u256(-7)));
}

BOOST_AUTO_TEST_CASE(super)
{
	char const* sourceCode = R"(
		contract A { function f() returns (uint r) { return 1; } }
		contract B is A { function f() returns (uint r) { return super.f() | 2; } }
		contract C is A { function f() returns (uint r) { return super.f() | 4; } }
		contract D is B, C { function f() returns (uint r) { return super.f() | 8; } }
	)";
	compileAndRun(sourceCode, 0, "D");
	BOOST_CHECK(callContractFunction("f()") == encodeArgs(1 | 2 | 4 | 8));
}

BOOST_AUTO_TEST_CASE(super_in_constructor)
{
	char const* sourceCode = R"(
		contract A { function f() returns (uint r) { return 1; } }
		contract B is A { function f() returns (uint r) { return super.f() | 2; } }
		contract C is A { function f() returns (uint r) { return super.f() | 4; } }
		contract D is B, C { uint data; function D() { data = super.f() | 8; } function f() returns (uint r) { return data; } }
	)";
	compileAndRun(sourceCode, 0, "D");
	BOOST_CHECK(callContractFunction("f()") == encodeArgs(1 | 2 | 4 | 8));
}

BOOST_AUTO_TEST_CASE(fallback_function)
{
	char const* sourceCode = R"(
		contract A {
			uint data;
			function() returns (uint r) { data = 1; return 2; }
			function getData() returns (uint r) { return data; }
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("getData()") == encodeArgs(0));
	BOOST_CHECK(callContractFunction("") == encodeArgs(2));
	BOOST_CHECK(callContractFunction("getData()") == encodeArgs(1));
}

BOOST_AUTO_TEST_CASE(inherited_fallback_function)
{
	char const* sourceCode = R"(
		contract A {
			uint data;
			function() returns (uint r) { data = 1; return 2; }
			function getData() returns (uint r) { return data; }
		}
		contract B is A {}
	)";
	compileAndRun(sourceCode, 0, "B");
	BOOST_CHECK(callContractFunction("getData()") == encodeArgs(0));
	BOOST_CHECK(callContractFunction("") == encodeArgs(2));
	BOOST_CHECK(callContractFunction("getData()") == encodeArgs(1));
}

BOOST_AUTO_TEST_CASE(event)
{
	char const* sourceCode = R"(
		contract ClientReceipt {
			event Deposit(address indexed _from, bytes32 indexed _id, uint _value);
			function deposit(bytes32 _id, bool _manually) {
				if (_manually) {
					bytes32 s = 0x19dacbf83c5de6658e14cbf7bcae5c15eca2eedecf1c66fbca928e4d351bea0f;
					log3(bytes32(msg.value), s, bytes32(msg.sender), _id);
				} else
					Deposit(msg.sender, _id, msg.value);
			}
		}
	)";
	compileAndRun(sourceCode);
	u256 value(18);
	u256 id(0x1234);
	for (bool manually: {true, false})
	{
		callContractFunctionWithValue("deposit(bytes32,bool)", value, id, manually);
		BOOST_REQUIRE_EQUAL(m_logs.size(), 1);
		BOOST_CHECK_EQUAL(m_logs[0].address, m_contractAddress);
		BOOST_CHECK_EQUAL(h256(m_logs[0].data), h256(u256(value)));
		BOOST_REQUIRE_EQUAL(m_logs[0].topics.size(), 3);
		BOOST_CHECK_EQUAL(m_logs[0].topics[0], dev::sha3(string("Deposit(address,bytes32,uint256)")));
		BOOST_CHECK_EQUAL(m_logs[0].topics[1], h256(m_sender));
		BOOST_CHECK_EQUAL(m_logs[0].topics[2], h256(id));
	}
}

BOOST_AUTO_TEST_CASE(event_no_arguments)
{
	char const* sourceCode = R"(
		contract ClientReceipt {
			event Deposit;
			function deposit() {
				Deposit();
			}
		}
	)";
	compileAndRun(sourceCode);
	callContractFunction("deposit()");
	BOOST_REQUIRE_EQUAL(m_logs.size(), 1);
	BOOST_CHECK_EQUAL(m_logs[0].address, m_contractAddress);
	BOOST_CHECK(m_logs[0].data.empty());
	BOOST_REQUIRE_EQUAL(m_logs[0].topics.size(), 1);
	BOOST_CHECK_EQUAL(m_logs[0].topics[0], dev::sha3(string("Deposit()")));
}

BOOST_AUTO_TEST_CASE(event_anonymous)
{
	char const* sourceCode = R"(
		contract ClientReceipt {
			event Deposit() anonymous;
			function deposit() {
				Deposit();
			}
		}
	)";
	compileAndRun(sourceCode);
	callContractFunction("deposit()");
	BOOST_REQUIRE_EQUAL(m_logs[0].topics.size(), 0);
}

BOOST_AUTO_TEST_CASE(event_anonymous_with_topics)
{
	char const* sourceCode = R"(
		contract ClientReceipt {
			event Deposit(address indexed _from, bytes32 indexed _id, uint _value) anonymous;
			function deposit(bytes32 _id, bool _manually) {
				Deposit(msg.sender, _id, msg.value);
			}
		}
	)";
	compileAndRun(sourceCode);
	u256 value(18);
	u256 id(0x1234);
	callContractFunctionWithValue("deposit(bytes32,bool)", value, id);
	BOOST_REQUIRE_EQUAL(m_logs.size(), 1);
	BOOST_CHECK_EQUAL(m_logs[0].address, m_contractAddress);
	BOOST_CHECK_EQUAL(h256(m_logs[0].data), h256(u256(value)));
	BOOST_REQUIRE_EQUAL(m_logs[0].topics.size(), 2);
	BOOST_CHECK_EQUAL(m_logs[0].topics[0], h256(m_sender));
	BOOST_CHECK_EQUAL(m_logs[0].topics[1], h256(id));
}

BOOST_AUTO_TEST_CASE(event_lots_of_data)
{
	char const* sourceCode = R"(
		contract ClientReceipt {
			event Deposit(address _from, bytes32 _id, uint _value, bool _flag);
			function deposit(bytes32 _id) {
				Deposit(msg.sender, _id, msg.value, true);
			}
		}
	)";
	compileAndRun(sourceCode);
	u256 value(18);
	u256 id(0x1234);
	callContractFunctionWithValue("deposit(bytes32)", value, id);
	BOOST_REQUIRE_EQUAL(m_logs.size(), 1);
	BOOST_CHECK_EQUAL(m_logs[0].address, m_contractAddress);
	BOOST_CHECK(m_logs[0].data == encodeArgs((u160)m_sender, id, value, true));
	BOOST_REQUIRE_EQUAL(m_logs[0].topics.size(), 1);
	BOOST_CHECK_EQUAL(m_logs[0].topics[0], dev::sha3(string("Deposit(address,bytes32,uint256,bool)")));
}

BOOST_AUTO_TEST_CASE(empty_name_input_parameter_with_named_one)
{
	char const* sourceCode = R"(
		contract test {
			function f(uint, uint k) returns(uint ret_k, uint ret_g){
				uint g = 8;
				ret_k = k;
				ret_g = g;
			}
		})";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("f(uint256,uint256)", 5, 9) != encodeArgs(5, 8));
	BOOST_CHECK(callContractFunction("f(uint256,uint256)", 5, 9) == encodeArgs(9, 8));
}

BOOST_AUTO_TEST_CASE(empty_name_return_parameter)
{
	char const* sourceCode = R"(
		contract test {
			function f(uint k) returns(uint){
				return k;
		}
		})";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("f(uint256)", 9) == encodeArgs(9));
}

BOOST_AUTO_TEST_CASE(sha3_multiple_arguments)
{
	char const* sourceCode = R"(
		contract c {
			function foo(uint a, uint b, uint c) returns (bytes32 d)
			{
				d = sha3(a, b, c);
			}
		})";
	compileAndRun(sourceCode);

	BOOST_CHECK(callContractFunction("foo(uint256,uint256,uint256)", 10, 12, 13) == encodeArgs(
					dev::sha3(
						toBigEndian(u256(10)) +
						toBigEndian(u256(12)) +
						toBigEndian(u256(13)))));
}

BOOST_AUTO_TEST_CASE(sha3_multiple_arguments_with_numeric_literals)
{
	char const* sourceCode = R"(
		contract c {
			function foo(uint a, uint16 b) returns (bytes32 d)
			{
				d = sha3(a, b, 145);
			}
		})";
	compileAndRun(sourceCode);

	BOOST_CHECK(callContractFunction("foo(uint256,uint16)", 10, 12) == encodeArgs(
					dev::sha3(
						toBigEndian(u256(10)) +
						bytes{0x0, 0xc} +
						bytes(1, 0x91))));
}

BOOST_AUTO_TEST_CASE(sha3_multiple_arguments_with_string_literals)
{
	char const* sourceCode = R"(
		contract c {
			function foo() returns (bytes32 d)
			{
				d = sha3("foo");
			}
			function bar(uint a, uint16 b) returns (bytes32 d)
			{
				d = sha3(a, b, 145, "foo");
			}
		})";
	compileAndRun(sourceCode);

	BOOST_CHECK(callContractFunction("foo()") == encodeArgs(dev::sha3("foo")));

	BOOST_CHECK(callContractFunction("bar(uint256,uint16)", 10, 12) == encodeArgs(
					dev::sha3(
						toBigEndian(u256(10)) +
						bytes{0x0, 0xc} +
						bytes(1, 0x91) +
						bytes{0x66, 0x6f, 0x6f})));
}

BOOST_AUTO_TEST_CASE(generic_call)
{
	char const* sourceCode = R"**(
			contract receiver {
				uint public received;
				function receive(uint256 x) { received = x; }
			}
			contract sender {
				function doSend(address rec) returns (uint d)
				{
					bytes4 signature = bytes4(bytes32(sha3("receive(uint256)")));
					rec.call.value(2)(signature, 23);
					return receiver(rec).received();
				}
			}
	)**";
	compileAndRun(sourceCode, 0, "receiver");
	u160 const c_receiverAddress = m_contractAddress;
	compileAndRun(sourceCode, 50, "sender");
	BOOST_REQUIRE(callContractFunction("doSend(address)", c_receiverAddress) == encodeArgs(23));
	BOOST_CHECK_EQUAL(m_state.balance(m_contractAddress), 50 - 2);
}

BOOST_AUTO_TEST_CASE(store_bytes)
{
	// this test just checks that the copy loop does not mess up the stack
	char const* sourceCode = R"(
		contract C {
			function save() returns (uint r) {
				r = 23;
				savedData = msg.data;
				r = 24;
			}
			bytes savedData;
		}
	)";
	compileAndRun(sourceCode);
	// empty copy loop
	BOOST_CHECK(callContractFunction("save()") == encodeArgs(24));
	BOOST_CHECK(callContractFunction("save()", "abcdefg") == encodeArgs(24));
}

BOOST_AUTO_TEST_CASE(bytes_from_calldata_to_memory)
{
	char const* sourceCode = R"(
		contract C {
			function() returns (bytes32) {
				return sha3("abc", msg.data);
			}
		}
	)";
	compileAndRun(sourceCode);
	bytes calldata1 = bytes(61, 0x22) + bytes(12, 0x12);
	sendMessage(calldata1, false);
	BOOST_CHECK(m_output == encodeArgs(dev::sha3(bytes{'a', 'b', 'c'} + calldata1)));
}

BOOST_AUTO_TEST_CASE(call_forward_bytes)
{
	char const* sourceCode = R"(
		contract receiver {
			uint public received;
			function receive(uint x) { received += x + 1; }
			function() { received = 0x80; }
		}
		contract sender {
			function sender() { rec = new receiver(); }
			function() { savedData = msg.data; }
			function forward() returns (bool) { rec.call(savedData); return true; }
			function clear() returns (bool) { delete savedData; return true; }
			function val() returns (uint) { return rec.received(); }
			receiver rec;
			bytes savedData;
		}
	)";
	compileAndRun(sourceCode, 0, "sender");
	BOOST_CHECK(callContractFunction("receive(uint256)", 7) == bytes());
	BOOST_CHECK(callContractFunction("val()") == encodeArgs(0));
	BOOST_CHECK(callContractFunction("forward()") == encodeArgs(true));
	BOOST_CHECK(callContractFunction("val()") == encodeArgs(8));
	BOOST_CHECK(callContractFunction("clear()") == encodeArgs(true));
	BOOST_CHECK(callContractFunction("val()") == encodeArgs(8));
	BOOST_CHECK(callContractFunction("forward()") == encodeArgs(true));
	BOOST_CHECK(callContractFunction("val()") == encodeArgs(0x80));
}

BOOST_AUTO_TEST_CASE(copying_bytes_multiassign)
{
	char const* sourceCode = R"(
		contract receiver {
			uint public received;
			function receive(uint x) { received += x + 1; }
			function() { received = 0x80; }
		}
		contract sender {
			function sender() { rec = new receiver(); }
			function() { savedData1 = savedData2 = msg.data; }
			function forward(bool selector) returns (bool) {
				if (selector) { rec.call(savedData1); delete savedData1; }
				else { rec.call(savedData2); delete savedData2; }
				return true;
			}
			function val() returns (uint) { return rec.received(); }
			receiver rec;
			bytes savedData1;
			bytes savedData2;
		}
	)";
	compileAndRun(sourceCode, 0, "sender");
	BOOST_CHECK(callContractFunction("receive(uint256)", 7) == bytes());
	BOOST_CHECK(callContractFunction("val()") == encodeArgs(0));
	BOOST_CHECK(callContractFunction("forward(bool)", true) == encodeArgs(true));
	BOOST_CHECK(callContractFunction("val()") == encodeArgs(8));
	BOOST_CHECK(callContractFunction("forward(bool)", false) == encodeArgs(true));
	BOOST_CHECK(callContractFunction("val()") == encodeArgs(16));
	BOOST_CHECK(callContractFunction("forward(bool)", true) == encodeArgs(true));
	BOOST_CHECK(callContractFunction("val()") == encodeArgs(0x80));
}

BOOST_AUTO_TEST_CASE(delete_removes_bytes_data)
{
	char const* sourceCode = R"(
		contract c {
			function() { data = msg.data; }
			function del() returns (bool) { delete data; return true; }
			bytes data;
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("---", 7) == bytes());
	BOOST_CHECK(!m_state.storage(m_contractAddress).empty());
	BOOST_CHECK(callContractFunction("del()", 7) == encodeArgs(true));
	BOOST_CHECK(m_state.storage(m_contractAddress).empty());
}

BOOST_AUTO_TEST_CASE(copy_from_calldata_removes_bytes_data)
{
	char const* sourceCode = R"(
		contract c {
			function set() returns (bool) { data = msg.data; return true; }
			function() { data = msg.data; }
			bytes data;
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("set()", 1, 2, 3, 4, 5) == encodeArgs(true));
	BOOST_CHECK(!m_state.storage(m_contractAddress).empty());
	sendMessage(bytes(), false);
	BOOST_CHECK(m_output == bytes());
	BOOST_CHECK(m_state.storage(m_contractAddress).empty());
}

BOOST_AUTO_TEST_CASE(copy_removes_bytes_data)
{
	char const* sourceCode = R"(
		contract c {
			function set() returns (bool) { data1 = msg.data; return true; }
			function reset() returns (bool) { data1 = data2; return true; }
			bytes data1;
			bytes data2;
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("set()", 1, 2, 3, 4, 5) == encodeArgs(true));
	BOOST_CHECK(!m_state.storage(m_contractAddress).empty());
	BOOST_CHECK(callContractFunction("reset()") == encodeArgs(true));
	BOOST_CHECK(m_state.storage(m_contractAddress).empty());
}

BOOST_AUTO_TEST_CASE(bytes_inside_mappings)
{
	char const* sourceCode = R"(
		contract c {
			function set(uint key) returns (bool) { data[key] = msg.data; return true; }
			function copy(uint from, uint to) returns (bool) { data[to] = data[from]; return true; }
			mapping(uint => bytes) data;
		}
	)";
	compileAndRun(sourceCode);
	// store a short byte array at 1 and a longer one at 2
	BOOST_CHECK(callContractFunction("set(uint256)", 1, 2) == encodeArgs(true));
	BOOST_CHECK(callContractFunction("set(uint256)", 2, 2, 3, 4, 5) == encodeArgs(true));
	BOOST_CHECK(!m_state.storage(m_contractAddress).empty());
	// copy shorter to longer
	BOOST_CHECK(callContractFunction("copy(uint256,uint256)", 1, 2) == encodeArgs(true));
	BOOST_CHECK(!m_state.storage(m_contractAddress).empty());
	// copy empty to both
	BOOST_CHECK(callContractFunction("copy(uint256,uint256)", 99, 1) == encodeArgs(true));
	BOOST_CHECK(!m_state.storage(m_contractAddress).empty());
	BOOST_CHECK(callContractFunction("copy(uint256,uint256)", 99, 2) == encodeArgs(true));
	BOOST_CHECK(m_state.storage(m_contractAddress).empty());
}

BOOST_AUTO_TEST_CASE(bytes_length_member)
{
	char const* sourceCode = R"(
		contract c {
			function set() returns (bool) { data = msg.data; return true; }
			function getLength() returns (uint) { return data.length; }
			bytes data;
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("getLength()") == encodeArgs(0));
	BOOST_CHECK(callContractFunction("set()", 1, 2) == encodeArgs(true));
	BOOST_CHECK(callContractFunction("getLength()") == encodeArgs(4+32+32));
}

BOOST_AUTO_TEST_CASE(struct_copy)
{
	char const* sourceCode = R"(
		contract c {
			struct Nested { uint x; uint y; }
			struct Struct { uint a; mapping(uint => Struct) b; Nested nested; uint c; }
			mapping(uint => Struct) data;
			function set(uint k) returns (bool) {
				data[k].a = 1;
				data[k].nested.x = 3;
				data[k].nested.y = 4;
				data[k].c = 2;
				return true;
			}
			function copy(uint from, uint to) returns (bool) {
				data[to] = data[from];
				return true;
			}
			function retrieve(uint k) returns (uint a, uint x, uint y, uint c)
			{
				a = data[k].a;
				x = data[k].nested.x;
				y = data[k].nested.y;
				c = data[k].c;
			}
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("set(uint256)", 7) == encodeArgs(true));
	BOOST_CHECK(callContractFunction("retrieve(uint256)", 7) == encodeArgs(1, 3, 4, 2));
	BOOST_CHECK(callContractFunction("copy(uint256,uint256)", 7, 8) == encodeArgs(true));
	BOOST_CHECK(callContractFunction("retrieve(uint256)", 7) == encodeArgs(1, 3, 4, 2));
	BOOST_CHECK(callContractFunction("retrieve(uint256)", 8) == encodeArgs(1, 3, 4, 2));
	BOOST_CHECK(callContractFunction("copy(uint256,uint256)", 0, 7) == encodeArgs(true));
	BOOST_CHECK(callContractFunction("retrieve(uint256)", 7) == encodeArgs(0, 0, 0, 0));
	BOOST_CHECK(callContractFunction("retrieve(uint256)", 8) == encodeArgs(1, 3, 4, 2));
	BOOST_CHECK(callContractFunction("copy(uint256,uint256)", 7, 8) == encodeArgs(true));
	BOOST_CHECK(callContractFunction("retrieve(uint256)", 8) == encodeArgs(0, 0, 0, 0));
}

BOOST_AUTO_TEST_CASE(struct_containing_bytes_copy_and_delete)
{
	char const* sourceCode = R"(
		contract c {
			struct Struct { uint a; bytes data; uint b; }
			Struct data1;
			Struct data2;
			function set(uint _a, bytes _data, uint _b) external returns (bool) {
				data1.a = _a;
				data1.b = _b;
				data1.data = _data;
				return true;
			}
			function copy() returns (bool) {
				data1 = data2;
				return true;
			}
			function del() returns (bool) {
				delete data1;
				return true;
			}
		}
	)";
	compileAndRun(sourceCode);
	string data = "123456789012345678901234567890123";
	BOOST_CHECK(m_state.storage(m_contractAddress).empty());
	BOOST_CHECK(callContractFunction("set(uint256,bytes,uint256)", 12, u256(data.length()), 13, data) == encodeArgs(true));
	BOOST_CHECK(!m_state.storage(m_contractAddress).empty());
	BOOST_CHECK(callContractFunction("copy()") == encodeArgs(true));
	BOOST_CHECK(m_state.storage(m_contractAddress).empty());
	BOOST_CHECK(callContractFunction("set(uint256,bytes,uint256)", 12, u256(data.length()), 13, data) == encodeArgs(true));
	BOOST_CHECK(!m_state.storage(m_contractAddress).empty());
	BOOST_CHECK(callContractFunction("del()") == encodeArgs(true));
	BOOST_CHECK(m_state.storage(m_contractAddress).empty());
}

BOOST_AUTO_TEST_CASE(struct_copy_via_local)
{
	char const* sourceCode = R"(
		contract c {
			struct Struct { uint a; uint b; }
			Struct data1;
			Struct data2;
			function test() returns (bool) {
				data1.a = 1;
				data1.b = 2;
				var x = data1;
				data2 = x;
				return data2.a == data1.a && data2.b == data1.b;
			}
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("test()") == encodeArgs(true));
}

BOOST_AUTO_TEST_CASE(using_enums)
{
	char const* sourceCode = R"(
			contract test {
				enum ActionChoices { GoLeft, GoRight, GoStraight, Sit }
				function test()
				{
					choices = ActionChoices.GoStraight;
				}
				function getChoice() returns (uint d)
				{
					d = uint256(choices);
				}
				ActionChoices choices;
			}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("getChoice()") == encodeArgs(2));
}

BOOST_AUTO_TEST_CASE(constructing_enums_from_ints)
{
	char const* sourceCode = R"(
			contract c {
				enum Truth { False, True }
				function test() returns (uint)
				{
					return uint(Truth(uint8(0x701)));
				}
			}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("test()") == encodeArgs(1));
}

BOOST_AUTO_TEST_CASE(inline_member_init)
{
	char const* sourceCode = R"(
		contract test {
			function test(){
				m_b = 6;
				m_c = 8;
			}
			uint m_a = 5;
			uint m_b;
			uint m_c = 7;
			function get() returns (uint a, uint b, uint c){
				a = m_a;
				b = m_b;
				c = m_c;
			}
		})";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("get()") == encodeArgs(5, 6, 8));
}

BOOST_AUTO_TEST_CASE(inline_member_init_inheritence)
{
	char const* sourceCode = R"(
		contract Base {
			function Base(){}
			uint m_base = 5;
			function getBMember() returns (uint i) { return m_base; }
		}
		contract Derived is Base {
			function Derived(){}
			uint m_derived = 6;
			function getDMember() returns (uint i) { return m_derived; }
		})";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("getBMember()") == encodeArgs(5));
	BOOST_CHECK(callContractFunction("getDMember()") == encodeArgs(6));
}

BOOST_AUTO_TEST_CASE(inline_member_init_inheritence_without_constructor)
{
	char const* sourceCode = R"(
		contract Base {
			uint m_base = 5;
			function getBMember() returns (uint i) { return m_base; }
		}
		contract Derived is Base {
			uint m_derived = 6;
			function getDMember() returns (uint i) { return m_derived; }
		})";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("getBMember()") == encodeArgs(5));
	BOOST_CHECK(callContractFunction("getDMember()") == encodeArgs(6));
}

BOOST_AUTO_TEST_CASE(external_function)
{
	char const* sourceCode = R"(
		contract c {
			function f(uint a) returns (uint) { return a; }
			function test(uint a, uint b) external returns (uint r_a, uint r_b) {
				r_a = f(a + 7);
				r_b = b;
			}
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("test(uint256,uint256)", 2, 3) == encodeArgs(2+7, 3));
}

BOOST_AUTO_TEST_CASE(bytes_in_arguments)
{
	char const* sourceCode = R"(
		contract c {
			uint result;
			function f(uint a, uint b) { result += a + b; }
			function g(uint a) { result *= a; }
			function test(uint a, bytes data1, bytes data2, uint b) external returns (uint r_a, uint r, uint r_b, uint l) {
				r_a = a;
				this.call(data1);
				this.call(data2);
				r = result;
				r_b = b;
				l = data1.length;
			}
		}
	)";
	compileAndRun(sourceCode);
	string innercalldata1 = asString(FixedHash<4>(dev::sha3("f(uint256,uint256)")).asBytes() + encodeArgs(8, 9));
	bytes calldata1 = encodeArgs(u256(innercalldata1.length()), 12, innercalldata1, 13);
	string innercalldata2 = asString(FixedHash<4>(dev::sha3("g(uint256)")).asBytes() + encodeArgs(3));
	bytes calldata = encodeArgs(
		12, u256(innercalldata1.length()), u256(innercalldata2.length()), 13,
		innercalldata1, innercalldata2);
	BOOST_CHECK(callContractFunction("test(uint256,bytes,bytes,uint256)", calldata)
		== encodeArgs(12, (8 + 9) * 3, 13, u256(innercalldata1.length())));
}

BOOST_AUTO_TEST_CASE(fixed_arrays_in_storage)
{
	char const* sourceCode = R"(
		contract c {
			struct Data { uint x; uint y; }
			Data[2**10] data;
			uint[2**10 + 3] ids;
			function setIDStatic(uint id) { ids[2] = id; }
			function setID(uint index, uint id) { ids[index] = id; }
			function setData(uint index, uint x, uint y) { data[index].x = x; data[index].y = y; }
			function getID(uint index) returns (uint) { return ids[index]; }
			function getData(uint index) returns (uint x, uint y) { x = data[index].x; y = data[index].y; }
			function getLengths() returns (uint l1, uint l2) { l1 = data.length; l2 = ids.length; }
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("setIDStatic(uint256)", 11) == bytes());
	BOOST_CHECK(callContractFunction("getID(uint256)", 2) == encodeArgs(11));
	BOOST_CHECK(callContractFunction("setID(uint256,uint256)", 7, 8) == bytes());
	BOOST_CHECK(callContractFunction("getID(uint256)", 7) == encodeArgs(8));
	BOOST_CHECK(callContractFunction("setData(uint256,uint256,uint256)", 7, 8, 9) == bytes());
	BOOST_CHECK(callContractFunction("setData(uint256,uint256,uint256)", 8, 10, 11) == bytes());
	BOOST_CHECK(callContractFunction("getData(uint256)", 7) == encodeArgs(8, 9));
	BOOST_CHECK(callContractFunction("getData(uint256)", 8) == encodeArgs(10, 11));
	BOOST_CHECK(callContractFunction("getLengths()") == encodeArgs(u256(1) << 10, (u256(1) << 10) + 3));
}

BOOST_AUTO_TEST_CASE(dynamic_arrays_in_storage)
{
	char const* sourceCode = R"(
		contract c {
			struct Data { uint x; uint y; }
			Data[] data;
			uint[] ids;
			function setIDStatic(uint id) { ids[2] = id; }
			function setID(uint index, uint id) { ids[index] = id; }
			function setData(uint index, uint x, uint y) { data[index].x = x; data[index].y = y; }
			function getID(uint index) returns (uint) { return ids[index]; }
			function getData(uint index) returns (uint x, uint y) { x = data[index].x; y = data[index].y; }
			function getLengths() returns (uint l1, uint l2) { l1 = data.length; l2 = ids.length; }
			function setLengths(uint l1, uint l2) { data.length = l1; ids.length = l2; }
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("getLengths()") == encodeArgs(0, 0));
	BOOST_CHECK(callContractFunction("setLengths(uint256,uint256)", 48, 49) == bytes());
	BOOST_CHECK(callContractFunction("getLengths()") == encodeArgs(48, 49));
	BOOST_CHECK(callContractFunction("setIDStatic(uint256)", 11) == bytes());
	BOOST_CHECK(callContractFunction("getID(uint256)", 2) == encodeArgs(11));
	BOOST_CHECK(callContractFunction("setID(uint256,uint256)", 7, 8) == bytes());
	BOOST_CHECK(callContractFunction("getID(uint256)", 7) == encodeArgs(8));
	BOOST_CHECK(callContractFunction("setData(uint256,uint256,uint256)", 7, 8, 9) == bytes());
	BOOST_CHECK(callContractFunction("setData(uint256,uint256,uint256)", 8, 10, 11) == bytes());
	BOOST_CHECK(callContractFunction("getData(uint256)", 7) == encodeArgs(8, 9));
	BOOST_CHECK(callContractFunction("getData(uint256)", 8) == encodeArgs(10, 11));
}

BOOST_AUTO_TEST_CASE(fixed_out_of_bounds_array_access)
{
	char const* sourceCode = R"(
		contract c {
			uint[4] data;
			function set(uint index, uint value) returns (bool) { data[index] = value; return true; }
			function get(uint index) returns (uint) { return data[index]; }
			function length() returns (uint) { return data.length; }
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("length()") == encodeArgs(4));
	BOOST_CHECK(callContractFunction("set(uint256,uint256)", 3, 4) == encodeArgs(true));
	BOOST_CHECK(callContractFunction("set(uint256,uint256)", 4, 5) == bytes());
	BOOST_CHECK(callContractFunction("set(uint256,uint256)", 400, 5) == bytes());
	BOOST_CHECK(callContractFunction("get(uint256)", 3) == encodeArgs(4));
	BOOST_CHECK(callContractFunction("get(uint256)", 4) == bytes());
	BOOST_CHECK(callContractFunction("get(uint256)", 400) == bytes());
	BOOST_CHECK(callContractFunction("length()") == encodeArgs(4));
}

BOOST_AUTO_TEST_CASE(dynamic_out_of_bounds_array_access)
{
	char const* sourceCode = R"(
		contract c {
			uint[] data;
			function enlarge(uint amount) returns (uint) { return data.length += amount; }
			function set(uint index, uint value) returns (bool) { data[index] = value; return true; }
			function get(uint index) returns (uint) { return data[index]; }
			function length() returns (uint) { return data.length; }
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("length()") == encodeArgs(0));
	BOOST_CHECK(callContractFunction("get(uint256)", 3) == bytes());
	BOOST_CHECK(callContractFunction("enlarge(uint256)", 4) == encodeArgs(4));
	BOOST_CHECK(callContractFunction("length()") == encodeArgs(4));
	BOOST_CHECK(callContractFunction("set(uint256,uint256)", 3, 4) == encodeArgs(true));
	BOOST_CHECK(callContractFunction("get(uint256)", 3) == encodeArgs(4));
	BOOST_CHECK(callContractFunction("length()") == encodeArgs(4));
	BOOST_CHECK(callContractFunction("set(uint256,uint256)", 4, 8) == bytes());
	BOOST_CHECK(callContractFunction("length()") == encodeArgs(4));
}

BOOST_AUTO_TEST_CASE(fixed_array_cleanup)
{
	char const* sourceCode = R"(
		contract c {
			uint spacer1;
			uint spacer2;
			uint[20] data;
			function fill() {
				for (uint i = 0; i < data.length; ++i) data[i] = i+1;
			}
			function clear() { delete data; }
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(m_state.storage(m_contractAddress).empty());
	BOOST_CHECK(callContractFunction("fill()") == bytes());
	BOOST_CHECK(!m_state.storage(m_contractAddress).empty());
	BOOST_CHECK(callContractFunction("clear()") == bytes());
	BOOST_CHECK(m_state.storage(m_contractAddress).empty());
}

BOOST_AUTO_TEST_CASE(short_fixed_array_cleanup)
{
	char const* sourceCode = R"(
		contract c {
			uint spacer1;
			uint spacer2;
			uint[3] data;
			function fill() {
				for (uint i = 0; i < data.length; ++i) data[i] = i+1;
			}
			function clear() { delete data; }
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(m_state.storage(m_contractAddress).empty());
	BOOST_CHECK(callContractFunction("fill()") == bytes());
	BOOST_CHECK(!m_state.storage(m_contractAddress).empty());
	BOOST_CHECK(callContractFunction("clear()") == bytes());
	BOOST_CHECK(m_state.storage(m_contractAddress).empty());
}

BOOST_AUTO_TEST_CASE(dynamic_array_cleanup)
{
	char const* sourceCode = R"(
		contract c {
			uint[20] spacer;
			uint[] dynamic;
			function fill() {
				dynamic.length = 21;
				for (uint i = 0; i < dynamic.length; ++i) dynamic[i] = i+1;
			}
			function halfClear() { dynamic.length = 5; }
			function fullClear() { delete dynamic; }
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(m_state.storage(m_contractAddress).empty());
	BOOST_CHECK(callContractFunction("fill()") == bytes());
	BOOST_CHECK(!m_state.storage(m_contractAddress).empty());
	BOOST_CHECK(callContractFunction("halfClear()") == bytes());
	BOOST_CHECK(!m_state.storage(m_contractAddress).empty());
	BOOST_CHECK(callContractFunction("fullClear()") == bytes());
	BOOST_CHECK(m_state.storage(m_contractAddress).empty());
}

BOOST_AUTO_TEST_CASE(dynamic_multi_array_cleanup)
{
	char const* sourceCode = R"(
		contract c {
			struct s { uint[][] d; }
			s[] data;
			function fill() returns (uint) {
				data.length = 3;
				data[2].d.length = 4;
				data[2].d[3].length = 5;
				data[2].d[3][4] = 8;
				return data[2].d[3][4];
			}
			function clear() { delete data; }
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(m_state.storage(m_contractAddress).empty());
	BOOST_CHECK(callContractFunction("fill()") == encodeArgs(8));
	BOOST_CHECK(!m_state.storage(m_contractAddress).empty());
	BOOST_CHECK(callContractFunction("clear()") == bytes());
	BOOST_CHECK(m_state.storage(m_contractAddress).empty());
}

BOOST_AUTO_TEST_CASE(array_copy_storage_storage_dyn_dyn)
{
	char const* sourceCode = R"(
		contract c {
			uint[] data1;
			uint[] data2;
			function setData1(uint length, uint index, uint value) {
				data1.length = length; if (index < length) data1[index] = value;
			}
			function copyStorageStorage() { data2 = data1; }
			function getData2(uint index) returns (uint len, uint val) {
				len = data2.length; if (index < len) val = data2[index];
			}
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("setData1(uint256,uint256,uint256)", 10, 5, 4) == bytes());
	BOOST_CHECK(callContractFunction("copyStorageStorage()") == bytes());
	BOOST_CHECK(callContractFunction("getData2(uint256)", 5) == encodeArgs(10, 4));
	BOOST_CHECK(callContractFunction("setData1(uint256,uint256,uint256)", 0, 0, 0) == bytes());
	BOOST_CHECK(callContractFunction("copyStorageStorage()") == bytes());
	BOOST_CHECK(callContractFunction("getData2(uint256)", 0) == encodeArgs(0, 0));
	BOOST_CHECK(m_state.storage(m_contractAddress).empty());
}

BOOST_AUTO_TEST_CASE(array_copy_storage_storage_static_static)
{
	char const* sourceCode = R"(
		contract c {
			uint[40] data1;
			uint[20] data2;
			function test() returns (uint x, uint y){
				data1[30] = 4;
				data1[2] = 7;
				data1[3] = 9;
				data2[3] = 8;
				data1 = data2;
				x = data1[3];
				y = data1[30]; // should be cleared
			}
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("test()") == encodeArgs(8, 0));
}

BOOST_AUTO_TEST_CASE(array_copy_storage_storage_static_dynamic)
{
	char const* sourceCode = R"(
		contract c {
			uint[9] data1;
			uint[] data2;
			function test() returns (uint x, uint y){
				data1[8] = 4;
				data2 = data1;
				x = data2.length;
				y = data2[8];
			}
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("test()") == encodeArgs(9, 4));
}

BOOST_AUTO_TEST_CASE(array_copy_storage_storage_struct)
{
	char const* sourceCode = R"(
		contract c {
			struct Data { uint x; uint y; }
			Data[] data1;
			Data[] data2;
			function test() returns (uint x, uint y) {
				data1.length = 9;
				data1[8].x = 4;
				data1[8].y = 5;
				data2 = data1;
				x = data2[8].x;
				y = data2[8].y;
				data1.length = 0;
				data2 = data1;
			}
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("test()") == encodeArgs(4, 5));
	BOOST_CHECK(m_state.storage(m_contractAddress).empty());
}

BOOST_AUTO_TEST_CASE(external_array_args)
{
	char const* sourceCode = R"(
		contract c {
			function test(uint[8] a, uint[] b, uint[5] c, uint a_index, uint b_index, uint c_index)
					external returns (uint av, uint bv, uint cv) {
				av = a[a_index];
				bv = b[b_index];
				cv = c[c_index];
			}
		}
	)";
	compileAndRun(sourceCode);
	bytes params = encodeArgs(
		1, 2, 3, 4, 5, 6, 7, 8, // a
		3, // b.length
		21, 22, 23, 24, 25, // c
		0, 1, 2, // (a,b,c)_index
		11, 12, 13 // b
		);
	BOOST_CHECK(callContractFunction("test(uint256[8],uint256[],uint256[5],uint256,uint256,uint256)", params) == encodeArgs(1, 12, 23));
}

BOOST_AUTO_TEST_CASE(bytes_index_access)
{
	char const* sourceCode = R"(
		contract c {
			bytes data;
			function direct(bytes arg, uint index) external returns (uint) {
				return uint(arg[index]);
			}
			function storageCopyRead(bytes arg, uint index) external returns (uint) {
				data = arg;
				return uint(data[index]);
			}
			function storageWrite() external returns (uint) {
				data.length = 35;
				data[31] = 0x77;
				data[32] = 0x14;

				data[31] = 1;
				data[31] |= 8;
				data[30] = 1;
				data[32] = 3;
				return uint(data[30]) * 0x100 | uint(data[31]) * 0x10 | uint(data[32]);
			}
		}
	)";
	compileAndRun(sourceCode);
	string array{
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
		10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
		20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
		30, 31, 32, 33};
	BOOST_CHECK(callContractFunction("direct(bytes,uint256)", u256(array.length()), 32, array) == encodeArgs(32));
	BOOST_CHECK(callContractFunction("storageCopyRead(bytes,uint256)", u256(array.length()), 32, array) == encodeArgs(32));
	BOOST_CHECK(callContractFunction("storageWrite()") == encodeArgs(0x193));
}

BOOST_AUTO_TEST_CASE(array_copy_calldata_storage)
{
	char const* sourceCode = R"(
		contract c {
			uint[9] m_data;
			uint[] m_data_dyn;
			uint8[][] m_byte_data;
			function store(uint[9] a, uint8[3][] b) external returns (uint8) {
				m_data = a;
				m_data_dyn = a;
				m_byte_data = b;
				return b[3][1]; // note that access and declaration are reversed to each other
			}
			function retrieve() returns (uint a, uint b, uint c, uint d, uint e, uint f, uint g) {
				a = m_data.length;
				b = m_data[7];
				c = m_data_dyn.length;
				d = m_data_dyn[7];
				e = m_byte_data.length;
				f = m_byte_data[3].length;
				g = m_byte_data[3][1];
			}
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("store(uint256[9],uint8[3][])", encodeArgs(
		21, 22, 23, 24, 25, 26, 27, 28, 29, // a
		4, // size of b
		1, 2, 3, // b[0]
		11, 12, 13, // b[1]
		21, 22, 23, // b[2]
		31, 32, 33 // b[3]
	)) == encodeArgs(32));
	BOOST_CHECK(callContractFunction("retrieve()") == encodeArgs(
		9, 28, 9, 28,
		4, 3, 32));
}

BOOST_AUTO_TEST_CASE(array_copy_nested_array)
{
	char const* sourceCode = R"(
		contract c {
			uint[4][] a;
			uint[5][] b;
			uint[][] c;
			function test(uint[2][] d) external returns (uint) {
				a = d;
				b = a;
				c = b;
				return c[1][1] | c[1][2] | c[1][3] | c[1][4];
			}
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("test(uint256[2][])", encodeArgs(
		3,
		7, 8,
		9, 10,
		11, 12
	)) == encodeArgs(10));
}

BOOST_AUTO_TEST_CASE(array_copy_including_mapping)
{
	char const* sourceCode = R"(
		contract c {
			mapping(uint=>uint)[90][] large;
			mapping(uint=>uint)[3][] small;
			function test() returns (uint r) {
				large.length = small.length = 7;
				large[3][2][0] = 2;
				large[1] = large[3];
				small[3][2][0] = 2;
				small[1] = small[2];
				r = ((
					small[3][2][0] * 0x100 |
					small[1][2][0]) * 0x100 |
					large[3][2][0]) * 0x100 |
					large[1][2][0];
				delete small;
				delete large;
			}
			function clear() returns (uint r) {
				large.length = small.length = 7;
				small[3][2][0] = 0;
				large[3][2][0] = 0;
				small.length = large.length = 0;
				return 7;
			}
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("test()") == encodeArgs(0x02000200));
	// storage is not empty because we cannot delete the mappings
	BOOST_CHECK(!m_state.storage(m_contractAddress).empty());
	BOOST_CHECK(callContractFunction("clear()") == encodeArgs(7));
	BOOST_CHECK(m_state.storage(m_contractAddress).empty());
}

BOOST_AUTO_TEST_CASE(pass_dynamic_arguments_to_the_base)
{
	char const* sourceCode = R"(
		contract Base {
			function Base(uint i)
			{
				m_i = i;
			}
			uint public m_i;
		}
		contract Derived is Base(2) {
			function Derived(uint i) Base(i)
			{}
		}
		contract Final is Derived(4) {
		})";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("m_i()") == encodeArgs(4));
}

BOOST_AUTO_TEST_CASE(pass_dynamic_arguments_to_the_base_base)
{
	char const* sourceCode = R"(
		contract Base {
			function Base(uint j)
			{
				m_i = j;
			}
			uint public m_i;
		}
		contract Base1 is Base(3) {
			function Base1(uint k) Base(k*k) {}
		}
		contract Derived is Base(3), Base1(2) {
			function Derived(uint i) Base(i) Base1(i)
			{}
		}
		contract Final is Derived(4) {
		})";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("m_i()") == encodeArgs(4));
}

BOOST_AUTO_TEST_CASE(pass_dynamic_arguments_to_the_base_base_with_gap)
{
	char const* sourceCode = R"(
		contract Base {
			function Base(uint i)
			{
				m_i = i;
			}
			uint public m_i;
		}
		contract Base1 is Base(3) {}
		contract Derived is Base(2), Base1 {
			function Derived(uint i) Base(i) {}
		}
		contract Final is Derived(4) {
		})";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("m_i()") == encodeArgs(4));
}

BOOST_AUTO_TEST_CASE(simple_constant_variables_test)
{
	char const* sourceCode = R"(
		contract Foo {
			function getX() returns (uint r) { return x; }
			uint constant x = 56;
	})";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("getX()") == encodeArgs(56));
}

BOOST_AUTO_TEST_CASE(constant_variables)
{
	//for now constant specifier is valid only for uint bytesXX and enums
	char const* sourceCode = R"(
		contract Foo {
			uint constant x = 56;
			enum ActionChoices { GoLeft, GoRight, GoStraight, Sit }
			ActionChoices constant choices = ActionChoices.GoLeft;
			bytes32 constant st = "abc\x00\xff__";
	})";
	compileAndRun(sourceCode);
}

BOOST_AUTO_TEST_CASE(packed_storage_structs_uint)
{
	char const* sourceCode = R"(
		contract C {
			struct str { uint8 a; uint16 b; uint248 c; }
			str data;
			function test() returns (uint) {
				data.a = 2;
				if (data.a != 2) return 2;
				data.b = 0xabcd;
				if (data.b != 0xabcd) return 3;
				data.c = 0x1234567890;
				if (data.c != 0x1234567890) return 4;
				if (data.a != 2) return 5;
				data.a = 8;
				if (data.a != 8) return 6;
				if (data.b != 0xabcd) return 7;
				data.b = 0xdcab;
				if (data.b != 0xdcab) return 8;
				if (data.c != 0x1234567890) return 9;
				data.c = 0x9876543210;
				if (data.c != 0x9876543210) return 10;
				return 1;
			}
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("test()") == encodeArgs(1));
}

BOOST_AUTO_TEST_CASE(packed_storage_structs_enum)
{
	char const* sourceCode = R"(
		contract C {
			enum small { A, B, C, D }
			enum larger { A, B, C, D, E}
			struct str { small a; small b; larger c; larger d; }
			str data;
			function test() returns (uint) {
				data.a = small.B;
				if (data.a != small.B) return 2;
				data.b = small.C;
				if (data.b != small.C) return 3;
				data.c = larger.D;
				if (data.c != larger.D) return 4;
				if (data.a != small.B) return 5;
				data.a = small.C;
				if (data.a != small.C) return 6;
				if (data.b != small.C) return 7;
				data.b = small.D;
				if (data.b != small.D) return 8;
				if (data.c != larger.D) return 9;
				data.c = larger.B;
				if (data.c != larger.B) return 10;
				return 1;
			}
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("test()") == encodeArgs(1));
}

BOOST_AUTO_TEST_CASE(packed_storage_structs_bytes)
{
	char const* sourceCode = R"(
		contract C {
			struct s1 { byte a; byte b; bytes10 c; bytes9 d; bytes10 e; }
			struct s2 { byte a; s1 inner; byte b; byte c; }
			byte x;
			s2 data;
			byte y;
			function test() returns (bool) {
				x = 1;
				data.a = 2;
				data.inner.a = 3;
				data.inner.b = 4;
				data.inner.c = "1234567890";
				data.inner.d = "123456789";
				data.inner.e = "abcdefghij";
				data.b = 5;
				data.c = 6;
				y = 7;
				return x == 1 && data.a == 2 && data.inner.a == 3 && data.inner.b == 4 &&
					data.inner.c == "1234567890" && data.inner.d == "123456789" &&
					data.inner.e == "abcdefghij" && data.b == 5 && data.c == 6 && y == 7;
			}
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("test()") == encodeArgs(true));
}

BOOST_AUTO_TEST_CASE(packed_storage_structs_delete)
{
	char const* sourceCode = R"(
		contract C {
			struct str { uint8 a; uint16 b; uint8 c; }
			uint8 x;
			uint16 y;
			str data;
			function test() returns (uint) {
				x = 1;
				y = 2;
				data.a = 2;
				data.b = 0xabcd;
				data.c = 0xfa;
				if (x != 1 || y != 2 || data.a != 2 || data.b != 0xabcd || data.c != 0xfa)
					return 2;
				delete y;
				delete data.b;
				if (x != 1 || y != 0 || data.a != 2 || data.b != 0 || data.c != 0xfa)
					return 3;
				delete x;
				delete data;
				return 1;
			}
		}
	)";
	compileAndRun(sourceCode);
	BOOST_CHECK(callContractFunction("test()") == encodeArgs(1));
	BOOST_CHECK(m_state.storage(m_contractAddress).empty());
}

BOOST_AUTO_TEST_SUITE_END()

}
}
} // end namespaces

