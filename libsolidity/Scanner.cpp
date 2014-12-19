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

	This file is derived from the file "scanner.cc", which was part of the
	V8 project. The original copyright header follows:

	Copyright 2006-2012, the V8 project authors. All rights reserved.
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are
	met:

	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above
	  copyright notice, this list of conditions and the following
	  disclaimer in the documentation and/or other materials provided
	  with the distribution.
	* Neither the name of Google Inc. nor the names of its
	  contributors may be used to endorse or promote products derived
	  from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
	A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
	OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
	LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**
 * @author Christian <c@ethdev.com>
 * @date 2014
 * Solidity scanner.
 */

#include <algorithm>
#include <tuple>
#include <libsolidity/Utils.h>
#include <libsolidity/Scanner.h>

using namespace std;

namespace dev
{
namespace solidity
{

namespace
{
bool isDecimalDigit(char c)
{
	return '0' <= c && c <= '9';
}
bool isHexDigit(char c)
{
	return isDecimalDigit(c)
		   || ('a' <= c && c <= 'f')
		   || ('A' <= c && c <= 'F');
}
bool isLineTerminator(char c)
{
	return c == '\n';
}
bool isWhiteSpace(char c)
{
	return c == ' ' || c == '\n' || c == '\t';
}
bool isIdentifierStart(char c)
{
	return c == '_' || c == '$' || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}
bool isIdentifierPart(char c)
{
	return isIdentifierStart(c) || isDecimalDigit(c);
}

int hexValue(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	else if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	else return -1;
}
} // end anonymous namespace



/// Scoped helper for literal recording. Automatically drops the literal
/// if aborting the scanning before it's complete.
enum LiteralType {
	LITERAL_TYPE_STRING,
	LITERAL_TYPE_NUMBER, // not really different from string type in behaviour
	LITERAL_TYPE_COMMENT
};

class LiteralScope
{
public:
	explicit LiteralScope(Scanner* _self, enum LiteralType _type): m_type(_type)
	, m_scanner(_self)
	, m_complete(false)
	{
		if (_type == LITERAL_TYPE_COMMENT)
			m_scanner->m_nextSkippedComment.literal.clear();
		else
			m_scanner->m_nextToken.literal.clear();
	}
	~LiteralScope()
	{
		if (!m_complete)
		{
			if (m_type == LITERAL_TYPE_COMMENT)
				m_scanner->m_nextSkippedComment.literal.clear();
			else
				m_scanner->m_nextToken.literal.clear();
		}
	}
	void complete() { m_complete = true; }

private:
	enum LiteralType m_type;
	Scanner* m_scanner;
	bool m_complete;
}; // end of LiteralScope class


void Scanner::reset(CharStream const& _source, string const& _sourceName)
{
	m_source = _source;
	m_sourceName = make_shared<string const>(_sourceName);
	reset();
}

void Scanner::reset()
{
	m_source.reset();
	m_char = m_source.get();
	skipWhitespace();
	scanToken();
	next();
}

bool Scanner::scanHexByte(char& o_scannedByte)
{
	char x = 0;
	for (int i = 0; i < 2; i++)
	{
		int d = hexValue(m_char);
		if (d < 0)
		{
			rollback(i);
			return false;
		}
		x = x * 16 + d;
		advance();
	}
	o_scannedByte = x;
	return true;
}


// Ensure that tokens can be stored in a byte.
BOOST_STATIC_ASSERT(Token::NUM_TOKENS <= 0x100);

Token::Value Scanner::next()
{
	m_currentToken = m_nextToken;
	m_skippedComment = m_nextSkippedComment;
	scanToken();

	return m_currentToken.token;
}

Token::Value Scanner::selectToken(char _next, Token::Value _then, Token::Value _else)
{
	advance();
	if (m_char == _next)
		return selectToken(_then);
	else
		return _else;
}

bool Scanner::skipWhitespace()
{
	int const startPosition = getSourcePos();
	while (isWhiteSpace(m_char))
		advance();
	// Return whether or not we skipped any characters.
	return getSourcePos() != startPosition;
}

bool Scanner::skipWhitespaceExceptLF()
{
	int const startPosition = getSourcePos();
	while (m_char == ' ' || m_char == '\t')
		advance();
	// Return whether or not we skipped any characters.
	return getSourcePos() != startPosition;
}

Token::Value Scanner::skipSingleLineComment()
{
	// The line terminator at the end of the line is not considered
	// to be part of the single-line comment; it is recognized
	// separately by the lexical grammar and becomes part of the
	// stream of input elements for the syntactic grammar
	while (advance() && !isLineTerminator(m_char)) { };
	return Token::WHITESPACE;
}

Token::Value Scanner::scanSingleLineDocComment()
{
	LiteralScope literal(this, LITERAL_TYPE_COMMENT);
	advance(); //consume the last '/' at ///
	skipWhitespaceExceptLF();
	while (!isSourcePastEndOfInput())
	{
		if (isLineTerminator(m_char))
		{
			// check if next line is also a documentation comment
			skipWhitespace();
			if (!m_source.isPastEndOfInput(3) &&
				m_source.get(0) == '/' &&
				m_source.get(1) == '/' &&
				m_source.get(2) == '/')
			{
				addCommentLiteralChar('\n');
				m_char = m_source.advanceAndGet(3);
			}
			else
				break; // next line is not a documentation comment, we are done

		}
		addCommentLiteralChar(m_char);
		advance();
	}
	literal.complete();
	return Token::COMMENT_LITERAL;
}

Token::Value Scanner::skipMultiLineComment()
{
	advance();
	while (!isSourcePastEndOfInput())
	{
		char ch = m_char;
		advance();

		// If we have reached the end of the multi-line comment, we
		// consume the '/' and insert a whitespace. This way all
		// multi-line comments are treated as whitespace.
		if (ch == '*' && m_char == '/')
		{
			m_char = ' ';
			return Token::WHITESPACE;
		}
	}
	// Unterminated multi-line comment.
	return Token::ILLEGAL;
}

Token::Value Scanner::scanMultiLineDocComment()
{
	LiteralScope literal(this, LITERAL_TYPE_COMMENT);
	bool endFound = false;
	bool charsAdded = false;

	advance(); //consume the last '*' at /**
	skipWhitespaceExceptLF();
	while (!isSourcePastEndOfInput())
	{
		//handle newlines in multline comments
		if (isLineTerminator(m_char))
		{
			skipWhitespace();
			if (!m_source.isPastEndOfInput(1) && m_source.get(0) == '*' && m_source.get(1) != '/')
			{ // skip first '*' in subsequent lines
				if (charsAdded)
					addCommentLiteralChar('\n');
				m_char = m_source.advanceAndGet(2);
			}
			else if (!m_source.isPastEndOfInput(1) && m_source.get(0) == '*' && m_source.get(1) == '/')
			{ // if after newline the comment ends, don't insert the newline
				m_char = m_source.advanceAndGet(2);
				endFound = true;
				break;
			}
			else
				addCommentLiteralChar('\n');
		}

		if (!m_source.isPastEndOfInput(1) && m_source.get(0) == '*' && m_source.get(1) == '/')
		{
			m_char = m_source.advanceAndGet(2);
			endFound = true;
			break;
		}
		addCommentLiteralChar(m_char);
		charsAdded = true;
		advance();
	}
	literal.complete();
	if (!endFound)
		return Token::ILLEGAL;
	else
		return Token::COMMENT_LITERAL;
}

void Scanner::scanToken()
{
	m_nextToken.literal.clear();
	m_nextSkippedComment.literal.clear();
	Token::Value token;
	do
	{
		// Remember the position of the next token
		m_nextToken.location.start = getSourcePos();
		switch (m_char)
		{
		case '\n': // fall-through
		case ' ':
		case '\t':
			token = selectToken(Token::WHITESPACE);
			break;
		case '"':
		case '\'':
			token = scanString();
			break;
		case '<':
			// < <= << <<=
			advance();
			if (m_char == '=')
				token = selectToken(Token::LTE);
			else if (m_char == '<')
				token = selectToken('=', Token::ASSIGN_SHL, Token::SHL);
			else
				token = Token::LT;
			break;
		case '>':
			// > >= >> >>= >>> >>>=
			advance();
			if (m_char == '=')
				token = selectToken(Token::GTE);
			else if (m_char == '>')
			{
				// >> >>= >>> >>>=
				advance();
				if (m_char == '=')
					token = selectToken(Token::ASSIGN_SAR);
				else if (m_char == '>')
					token = selectToken('=', Token::ASSIGN_SHR, Token::SHR);
				else
					token = Token::SAR;
			}
			else
				token = Token::GT;
			break;
		case '=':
			// = == =>
			advance();
			if (m_char == '=')
				token = selectToken(Token::EQ);
			else if (m_char == '>')
				token = selectToken(Token::ARROW);
			else
				token = Token::ASSIGN;
			break;
		case '!':
			// ! !=
			advance();
			if (m_char == '=')
				token = selectToken(Token::NE);
			else
				token = Token::NOT;
			break;
		case '+':
			// + ++ +=
			advance();
			if (m_char == '+')
				token = selectToken(Token::INC);
			else if (m_char == '=')
				token = selectToken(Token::ASSIGN_ADD);
			else
				token = Token::ADD;
			break;
		case '-':
			// - -- -= Number
			advance();
			if (m_char == '-')
			{
				advance();
				token = Token::DEC;
			}
			else if (m_char == '=')
				token = selectToken(Token::ASSIGN_SUB);
			else if (m_char == '.' || isDecimalDigit(m_char))
				token = scanNumber('-');
			else
				token = Token::SUB;
			break;
		case '*':
			// * *=
			token = selectToken('=', Token::ASSIGN_MUL, Token::MUL);
			break;
		case '%':
			// % %=
			token = selectToken('=', Token::ASSIGN_MOD, Token::MOD);
			break;
		case '/':
			// /  // /* /=
			advance();
			if (m_char == '/')
			{
				if (!advance()) /* double slash comment directly before EOS */
					token = Token::WHITESPACE;
				else if (m_char == '/')
				{
					Token::Value comment;
					m_nextSkippedComment.location.start = getSourcePos();
					comment = scanSingleLineDocComment();
					m_nextSkippedComment.location.end = getSourcePos();
					m_nextSkippedComment.token = comment;
					token = Token::WHITESPACE;
				}
				else
					token = skipSingleLineComment();
			}
			else if (m_char == '*')
			{
				if (!advance()) /* slash star comment before EOS */
					token = Token::WHITESPACE;
				else if (m_char == '*')
				{
					Token::Value comment;
					m_nextSkippedComment.location.start = getSourcePos();
					comment = scanMultiLineDocComment();
					m_nextSkippedComment.location.end = getSourcePos();
					m_nextSkippedComment.token = comment;
					token = Token::WHITESPACE;
				}
				else
					token = skipMultiLineComment();
			}
			else if (m_char == '=')
				token = selectToken(Token::ASSIGN_DIV);
			else
				token = Token::DIV;
			break;
		case '&':
			// & && &=
			advance();
			if (m_char == '&')
				token = selectToken(Token::AND);
			else if (m_char == '=')
				token = selectToken(Token::ASSIGN_BIT_AND);
			else
				token = Token::BIT_AND;
			break;
		case '|':
			// | || |=
			advance();
			if (m_char == '|')
				token = selectToken(Token::OR);
			else if (m_char == '=')
				token = selectToken(Token::ASSIGN_BIT_OR);
			else
				token = Token::BIT_OR;
			break;
		case '^':
			// ^ ^=
			token = selectToken('=', Token::ASSIGN_BIT_XOR, Token::BIT_XOR);
			break;
		case '.':
			// . Number
			advance();
			if (isDecimalDigit(m_char))
				token = scanNumber('.');
			else
				token = Token::PERIOD;
			break;
		case ':':
			token = selectToken(Token::COLON);
			break;
		case ';':
			token = selectToken(Token::SEMICOLON);
			break;
		case ',':
			token = selectToken(Token::COMMA);
			break;
		case '(':
			token = selectToken(Token::LPAREN);
			break;
		case ')':
			token = selectToken(Token::RPAREN);
			break;
		case '[':
			token = selectToken(Token::LBRACK);
			break;
		case ']':
			token = selectToken(Token::RBRACK);
			break;
		case '{':
			token = selectToken(Token::LBRACE);
			break;
		case '}':
			token = selectToken(Token::RBRACE);
			break;
		case '?':
			token = selectToken(Token::CONDITIONAL);
			break;
		case '~':
			token = selectToken(Token::BIT_NOT);
			break;
		default:
			if (isIdentifierStart(m_char))
				token = scanIdentifierOrKeyword();
			else if (isDecimalDigit(m_char))
				token = scanNumber();
			else if (skipWhitespace())
				token = Token::WHITESPACE;
			else if (isSourcePastEndOfInput())
				token = Token::EOS;
			else
				token = selectToken(Token::ILLEGAL);
			break;
		}
		// Continue scanning for tokens as long as we're just skipping
		// whitespace.
	}
	while (token == Token::WHITESPACE);
	m_nextToken.location.end = getSourcePos();
	m_nextToken.token = token;
}

bool Scanner::scanEscape()
{
	char c = m_char;
	advance();
	// Skip escaped newlines.
	if (isLineTerminator(c))
		return true;
	switch (c)
	{
	case '\'':  // fall through
	case '"':  // fall through
	case '\\':
		break;
	case 'b':
		c = '\b';
		break;
	case 'f':
		c = '\f';
		break;
	case 'n':
		c = '\n';
		break;
	case 'r':
		c = '\r';
		break;
	case 't':
		c = '\t';
		break;
	case 'v':
		c = '\v';
		break;
	case 'x':
		if (!scanHexByte(c))
			return false;
		break;
	}

	addLiteralChar(c);
	return true;
}

Token::Value Scanner::scanString()
{
	char const quote = m_char;
	advance();  // consume quote
	LiteralScope literal(this, LITERAL_TYPE_STRING);
	while (m_char != quote && !isSourcePastEndOfInput() && !isLineTerminator(m_char))
	{
		char c = m_char;
		advance();
		if (c == '\\')
		{
			if (isSourcePastEndOfInput() || !scanEscape())
				return Token::ILLEGAL;
		}
		else
			addLiteralChar(c);
	}
	if (m_char != quote)
		return Token::ILLEGAL;
	literal.complete();
	advance();  // consume quote
	return Token::STRING_LITERAL;
}

void Scanner::scanDecimalDigits()
{
	while (isDecimalDigit(m_char))
		addLiteralCharAndAdvance();
}

Token::Value Scanner::scanNumber(char _charSeen)
{
	enum { DECIMAL, HEX, BINARY } kind = DECIMAL;
	LiteralScope literal(this, LITERAL_TYPE_NUMBER);
	if (_charSeen == '.')
	{
		// we have already seen a decimal point of the float
		addLiteralChar('.');
		scanDecimalDigits();  // we know we have at least one digit
	}
	else
	{
		if (_charSeen == '-')
			addLiteralChar('-');
		// if the first character is '0' we must check for octals and hex
		if (m_char == '0')
		{
			addLiteralCharAndAdvance();
			// either 0, 0exxx, 0Exxx, 0.xxx or a hex number
			if (m_char == 'x' || m_char == 'X')
			{
				// hex number
				kind = HEX;
				addLiteralCharAndAdvance();
				if (!isHexDigit(m_char))
					return Token::ILLEGAL; // we must have at least one hex digit after 'x'/'X'
				while (isHexDigit(m_char))
					addLiteralCharAndAdvance();
			}
		}
		// Parse decimal digits and allow trailing fractional part.
		if (kind == DECIMAL)
		{
			scanDecimalDigits();  // optional
			if (m_char == '.')
			{
				addLiteralCharAndAdvance();
				scanDecimalDigits();  // optional
			}
		}
	}
	// scan exponent, if any
	if (m_char == 'e' || m_char == 'E')
	{
		solAssert(kind != HEX, "'e'/'E' must be scanned as part of the hex number");
		if (kind != DECIMAL)
			return Token::ILLEGAL;
		// scan exponent
		addLiteralCharAndAdvance();
		if (m_char == '+' || m_char == '-')
			addLiteralCharAndAdvance();
		if (!isDecimalDigit(m_char))
			return Token::ILLEGAL; // we must have at least one decimal digit after 'e'/'E'
		scanDecimalDigits();
	}
	// The source character immediately following a numeric literal must
	// not be an identifier start or a decimal digit; see ECMA-262
	// section 7.8.3, page 17 (note that we read only one decimal digit
	// if the value is 0).
	if (isDecimalDigit(m_char) || isIdentifierStart(m_char))
		return Token::ILLEGAL;
	literal.complete();
	return Token::NUMBER;
}


// ----------------------------------------------------------------------------
// Keyword Matcher


static Token::Value keywordOrIdentifierToken(string const& _input)
{
	// The following macros are used inside TOKEN_LIST and cause non-keyword tokens to be ignored
	// and keywords to be put inside the keywords variable.
#define KEYWORD(name, string, precedence) {string, Token::name},
#define TOKEN(name, string, precedence)
	static const map<string, Token::Value> keywords({TOKEN_LIST(TOKEN, KEYWORD)});
#undef KEYWORD
#undef TOKEN
	auto it = keywords.find(_input);
	return it == keywords.end() ? Token::IDENTIFIER : it->second;
}

Token::Value Scanner::scanIdentifierOrKeyword()
{
	solAssert(isIdentifierStart(m_char), "");
	LiteralScope literal(this, LITERAL_TYPE_STRING);
	addLiteralCharAndAdvance();
	// Scan the rest of the identifier characters.
	while (isIdentifierPart(m_char))
		addLiteralCharAndAdvance();
	literal.complete();
	return keywordOrIdentifierToken(m_nextToken.literal);
}

char CharStream::advanceAndGet(size_t _chars)
{
	if (isPastEndOfInput())
		return 0;
	m_pos += _chars;
	if (isPastEndOfInput())
		return 0;
	return m_source[m_pos];
}

char CharStream::rollback(size_t _amount)
{
	solAssert(m_pos >= _amount, "");
	m_pos -= _amount;
	return get();
}

string CharStream::getLineAtPosition(int _position) const
{
	// if _position points to \n, it returns the line before the \n
	using size_type = string::size_type;
	size_type searchStart = min<size_type>(m_source.size(), _position);
	if (searchStart > 0)
		searchStart--;
	size_type lineStart = m_source.rfind('\n', searchStart);
	if (lineStart == string::npos)
		lineStart = 0;
	else
		lineStart++;
	return m_source.substr(lineStart, min(m_source.find('\n', lineStart),
										  m_source.size()) - lineStart);
}

tuple<int, int> CharStream::translatePositionToLineColumn(int _position) const
{
	using size_type = string::size_type;
	size_type searchPosition = min<size_type>(m_source.size(), _position);
	int lineNumber = count(m_source.begin(), m_source.begin() + searchPosition, '\n');
	size_type lineStart;
	if (searchPosition == 0)
		lineStart = 0;
	else
	{
		lineStart = m_source.rfind('\n', searchPosition - 1);
		lineStart = lineStart == string::npos ? 0 : lineStart + 1;
	}
	return tuple<int, int>(lineNumber, searchPosition - lineStart);
}


}
}
