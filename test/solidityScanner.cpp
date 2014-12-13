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
 * Unit tests for the solidity scanner.
 */

#include <libsolidity/Scanner.h>
#include <boost/test/unit_test.hpp>

namespace dev
{
namespace solidity
{
namespace test
{

BOOST_AUTO_TEST_SUITE(SolidityScanner)

BOOST_AUTO_TEST_CASE(test_empty)
{
	Scanner scanner(CharStream(""));
	BOOST_CHECK_EQUAL(scanner.getCurrentToken(), Token::EOS);
}

BOOST_AUTO_TEST_CASE(smoke_test)
{
	Scanner scanner(CharStream("function break;765  \t  \"string1\",'string2'\nidentifier1"));
	BOOST_CHECK_EQUAL(scanner.getCurrentToken(), Token::FUNCTION);
	BOOST_CHECK_EQUAL(scanner.next(), Token::BREAK);
	BOOST_CHECK_EQUAL(scanner.next(), Token::SEMICOLON);
	BOOST_CHECK_EQUAL(scanner.next(), Token::NUMBER);
	BOOST_CHECK_EQUAL(scanner.getCurrentLiteral(), "765");
	BOOST_CHECK_EQUAL(scanner.next(), Token::STRING_LITERAL);
	BOOST_CHECK_EQUAL(scanner.getCurrentLiteral(), "string1");
	BOOST_CHECK_EQUAL(scanner.next(), Token::COMMA);
	BOOST_CHECK_EQUAL(scanner.next(), Token::STRING_LITERAL);
	BOOST_CHECK_EQUAL(scanner.getCurrentLiteral(), "string2");
	BOOST_CHECK_EQUAL(scanner.next(), Token::IDENTIFIER);
	BOOST_CHECK_EQUAL(scanner.getCurrentLiteral(), "identifier1");
	BOOST_CHECK_EQUAL(scanner.next(), Token::EOS);
}

BOOST_AUTO_TEST_CASE(string_escapes)
{
	Scanner scanner(CharStream("  { \"a\\x61\""));
	BOOST_CHECK_EQUAL(scanner.getCurrentToken(), Token::LBRACE);
	BOOST_CHECK_EQUAL(scanner.next(), Token::STRING_LITERAL);
	BOOST_CHECK_EQUAL(scanner.getCurrentLiteral(), "aa");
}

BOOST_AUTO_TEST_CASE(string_escapes_with_zero)
{
	Scanner scanner(CharStream("  { \"a\\x61\\x00abc\""));
	BOOST_CHECK_EQUAL(scanner.getCurrentToken(), Token::LBRACE);
	BOOST_CHECK_EQUAL(scanner.next(), Token::STRING_LITERAL);
	BOOST_CHECK_EQUAL(scanner.getCurrentLiteral(), std::string("aa\0abc", 6));
}

BOOST_AUTO_TEST_CASE(string_escape_illegal)
{
	Scanner scanner(CharStream(" bla \"\\x6rf\" (illegalescape)"));
	BOOST_CHECK_EQUAL(scanner.getCurrentToken(), Token::IDENTIFIER);
	BOOST_CHECK_EQUAL(scanner.next(), Token::ILLEGAL);
	BOOST_CHECK_EQUAL(scanner.getCurrentLiteral(), "");
	// TODO recovery from illegal tokens should be improved
	BOOST_CHECK_EQUAL(scanner.next(), Token::ILLEGAL);
	BOOST_CHECK_EQUAL(scanner.next(), Token::IDENTIFIER);
	BOOST_CHECK_EQUAL(scanner.next(), Token::ILLEGAL);
	BOOST_CHECK_EQUAL(scanner.next(), Token::EOS);
}

BOOST_AUTO_TEST_CASE(hex_numbers)
{
	Scanner scanner(CharStream("var x = 0x765432536763762734623472346;"));
	BOOST_CHECK_EQUAL(scanner.getCurrentToken(), Token::VAR);
	BOOST_CHECK_EQUAL(scanner.next(), Token::IDENTIFIER);
	BOOST_CHECK_EQUAL(scanner.next(), Token::ASSIGN);
	BOOST_CHECK_EQUAL(scanner.next(), Token::NUMBER);
	BOOST_CHECK_EQUAL(scanner.getCurrentLiteral(), "0x765432536763762734623472346");
	BOOST_CHECK_EQUAL(scanner.next(), Token::SEMICOLON);
	BOOST_CHECK_EQUAL(scanner.next(), Token::EOS);
}

BOOST_AUTO_TEST_CASE(negative_numbers)
{
	Scanner scanner(CharStream("var x = -.2 + -0x78 + -7.3 + 8.9;"));
	BOOST_CHECK_EQUAL(scanner.getCurrentToken(), Token::VAR);
	BOOST_CHECK_EQUAL(scanner.next(), Token::IDENTIFIER);
	BOOST_CHECK_EQUAL(scanner.next(), Token::ASSIGN);
	BOOST_CHECK_EQUAL(scanner.next(), Token::NUMBER);
	BOOST_CHECK_EQUAL(scanner.getCurrentLiteral(), "-.2");
	BOOST_CHECK_EQUAL(scanner.next(), Token::ADD);
	BOOST_CHECK_EQUAL(scanner.next(), Token::NUMBER);
	BOOST_CHECK_EQUAL(scanner.getCurrentLiteral(), "-0x78");
	BOOST_CHECK_EQUAL(scanner.next(), Token::ADD);
	BOOST_CHECK_EQUAL(scanner.next(), Token::NUMBER);
	BOOST_CHECK_EQUAL(scanner.getCurrentLiteral(), "-7.3");
	BOOST_CHECK_EQUAL(scanner.next(), Token::ADD);
	BOOST_CHECK_EQUAL(scanner.next(), Token::NUMBER);
	BOOST_CHECK_EQUAL(scanner.getCurrentLiteral(), "8.9");
	BOOST_CHECK_EQUAL(scanner.next(), Token::SEMICOLON);
	BOOST_CHECK_EQUAL(scanner.next(), Token::EOS);
}

BOOST_AUTO_TEST_CASE(locations)
{
	Scanner scanner(CharStream("function_identifier has ; -0x743/*comment*/\n ident //comment"));
	BOOST_CHECK_EQUAL(scanner.getCurrentToken(), Token::IDENTIFIER);
	BOOST_CHECK_EQUAL(scanner.getCurrentLocation().start, 0);
	BOOST_CHECK_EQUAL(scanner.getCurrentLocation().end, 19);
	BOOST_CHECK_EQUAL(scanner.next(), Token::IDENTIFIER);
	BOOST_CHECK_EQUAL(scanner.getCurrentLocation().start, 20);
	BOOST_CHECK_EQUAL(scanner.getCurrentLocation().end, 23);
	BOOST_CHECK_EQUAL(scanner.next(), Token::SEMICOLON);
	BOOST_CHECK_EQUAL(scanner.getCurrentLocation().start, 24);
	BOOST_CHECK_EQUAL(scanner.getCurrentLocation().end, 25);
	BOOST_CHECK_EQUAL(scanner.next(), Token::NUMBER);
	BOOST_CHECK_EQUAL(scanner.getCurrentLocation().start, 26);
	BOOST_CHECK_EQUAL(scanner.getCurrentLocation().end, 32);
	BOOST_CHECK_EQUAL(scanner.next(), Token::IDENTIFIER);
	BOOST_CHECK_EQUAL(scanner.getCurrentLocation().start, 45);
	BOOST_CHECK_EQUAL(scanner.getCurrentLocation().end, 50);
	BOOST_CHECK_EQUAL(scanner.next(), Token::EOS);
}

BOOST_AUTO_TEST_CASE(ambiguities)
{
	// test scanning of some operators which need look-ahead
	Scanner scanner(CharStream("<=""<""+ +=a++ =>""<<"));
	BOOST_CHECK_EQUAL(scanner.getCurrentToken(), Token::LTE);
	BOOST_CHECK_EQUAL(scanner.next(), Token::LT);
	BOOST_CHECK_EQUAL(scanner.next(), Token::ADD);
	BOOST_CHECK_EQUAL(scanner.next(), Token::ASSIGN_ADD);
	BOOST_CHECK_EQUAL(scanner.next(), Token::IDENTIFIER);
	BOOST_CHECK_EQUAL(scanner.next(), Token::INC);
	BOOST_CHECK_EQUAL(scanner.next(), Token::ARROW);
	BOOST_CHECK_EQUAL(scanner.next(), Token::SHL);
}


BOOST_AUTO_TEST_SUITE_END()

}
}
} // end namespaces
