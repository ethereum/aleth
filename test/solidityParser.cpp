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
 * Unit tests for the solidity parser.
 */

#include <string>

#include <libdevcore/Log.h>
#include <libsolidity/Scanner.h>
#include <libsolidity/Parser.h>
#include <libsolidity/Exceptions.h>
#include <boost/test/unit_test.hpp>

namespace dev
{
namespace solidity
{
namespace test
{

namespace
{
ASTPointer<ASTNode> parseText(std::string const& _source)
{
	Parser parser;
	return parser.parse(std::make_shared<Scanner>(CharStream(_source)));
}
}

BOOST_AUTO_TEST_SUITE(SolidityParser)

BOOST_AUTO_TEST_CASE(smoke_test)
{
	char const* text = "contract test {\n"
					   "  uint256 stateVariable1;\n"
					   "}\n";
	BOOST_CHECK_NO_THROW(parseText(text));
}

BOOST_AUTO_TEST_CASE(missing_variable_name_in_declaration)
{
	char const* text = "contract test {\n"
					   "  uint256 ;\n"
					   "}\n";
	BOOST_CHECK_THROW(parseText(text), ParserError);
}

BOOST_AUTO_TEST_CASE(empty_function)
{
	char const* text = "contract test {\n"
					   "  uint256 stateVar;\n"
					   "  function functionName(hash160 arg1, address addr) const\n"
					   "    returns (int id)\n"
					   "  { }\n"
					   "}\n";
	BOOST_CHECK_NO_THROW(parseText(text));
}

BOOST_AUTO_TEST_CASE(no_function_params)
{
	char const* text = "contract test {\n"
					   "  uint256 stateVar;\n"
					   "  function functionName() {}\n"
					   "}\n";
	BOOST_CHECK_NO_THROW(parseText(text));
}

BOOST_AUTO_TEST_CASE(single_function_param)
{
	char const* text = "contract test {\n"
					   "  uint256 stateVar;\n"
					   "  function functionName(hash hashin) returns (hash hashout) {}\n"
					   "}\n";
	BOOST_CHECK_NO_THROW(parseText(text));
}

BOOST_AUTO_TEST_CASE(struct_definition)
{
	char const* text = "contract test {\n"
					   "  uint256 stateVar;\n"
					   "  struct MyStructName {\n"
					   "    address addr;\n"
					   "    uint256 count;\n"
					   "  }\n"
					   "}\n";
	BOOST_CHECK_NO_THROW(parseText(text));
}

BOOST_AUTO_TEST_CASE(mapping)
{
	char const* text = "contract test {\n"
					   "  mapping(address => string) names;\n"
					   "}\n";
	BOOST_CHECK_NO_THROW(parseText(text));
}

BOOST_AUTO_TEST_CASE(mapping_in_struct)
{
	char const* text = "contract test {\n"
					   "  struct test_struct {\n"
					   "    address addr;\n"
					   "    uint256 count;\n"
					   "    mapping(hash => test_struct) self_reference;\n"
					   "  }\n"
					   "}\n";
	BOOST_CHECK_NO_THROW(parseText(text));
}

BOOST_AUTO_TEST_CASE(mapping_to_mapping_in_struct)
{
	char const* text = "contract test {\n"
					   "  struct test_struct {\n"
					   "    address addr;\n"
					   "    mapping (uint64 => mapping (hash => uint)) complex_mapping;\n"
					   "  }\n"
					   "}\n";
	BOOST_CHECK_NO_THROW(parseText(text));
}

BOOST_AUTO_TEST_CASE(variable_definition)
{
	char const* text = "contract test {\n"
					   "  function fun(uint256 a) {\n"
					   "    var b;\n"
					   "    uint256 c;\n"
					   "    mapping(address=>hash) d;\n"
					   "    customtype varname;\n"
					   "  }\n"
					   "}\n";
	BOOST_CHECK_NO_THROW(parseText(text));
}

BOOST_AUTO_TEST_CASE(variable_definition_with_initialization)
{
	char const* text = "contract test {\n"
					   "  function fun(uint256 a) {\n"
					   "    var b = 2;\n"
					   "    uint256 c = 0x87;\n"
					   "    mapping(address=>hash) d;\n"
					   "    string name = \"Solidity\";"
					   "    customtype varname;\n"
					   "  }\n"
					   "}\n";
	BOOST_CHECK_NO_THROW(parseText(text));
}

BOOST_AUTO_TEST_CASE(operator_expression)
{
	char const* text = "contract test {\n"
					   "  function fun(uint256 a) {\n"
					   "    uint256 x = (1 + 4) || false && (1 - 12) + -9;\n"
					   "  }\n"
					   "}\n";
	BOOST_CHECK_NO_THROW(parseText(text));
}

BOOST_AUTO_TEST_CASE(complex_expression)
{
	char const* text = "contract test {\n"
					   "  function fun(uint256 a) {\n"
					   "    uint256 x = (1 + 4).member(++67)[a/=9] || true;\n"
					   "  }\n"
					   "}\n";
	BOOST_CHECK_NO_THROW(parseText(text));
}

BOOST_AUTO_TEST_CASE(while_loop)
{
	char const* text = "contract test {\n"
					   "  function fun(uint256 a) {\n"
					   "    while (true) { uint256 x = 1; break; continue; } x = 9;\n"
					   "  }\n"
					   "}\n";
	BOOST_CHECK_NO_THROW(parseText(text));
}

BOOST_AUTO_TEST_CASE(if_statement)
{
	char const* text = "contract test {\n"
					   "  function fun(uint256 a) {\n"
					   "    if (a >= 8) return 2; else { var b = 7; }\n"
					   "  }\n"
					   "}\n";
	BOOST_CHECK_NO_THROW(parseText(text));
}

BOOST_AUTO_TEST_CASE(else_if_statement)
{
	char const* text = "contract test {\n"
					   "  function fun(uint256 a) returns (address b) {\n"
					   "    if (a < 0) b = 0x67; else if (a == 0) b = 0x12; else b = 0x78;\n"
					   "  }\n"
					   "}\n";
	BOOST_CHECK_NO_THROW(parseText(text));
}

BOOST_AUTO_TEST_CASE(statement_starting_with_type_conversion)
{
	char const* text = "contract test {\n"
					   "  function fun() {\n"
					   "    uint64(2);\n"
					   "  }\n"
					   "}\n";
	BOOST_CHECK_NO_THROW(parseText(text));
}

BOOST_AUTO_TEST_SUITE_END()

}
}
} // end namespaces

