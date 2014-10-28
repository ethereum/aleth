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
 * Solidity javascript compiler.
 */

#include <string>
#include <iostream>

#include <libdevcore/Common.h>
#include <libdevcore/CommonData.h>
#include <libdevcore/CommonIO.h>
#include <libsolidity/Scanner.h>
#include <libsolidity/Parser.h>
#include <libsolidity/ASTPrinter.h>
#include <libsolidity/NameAndTypeResolver.h>
#include <libsolidity/Exceptions.h>
#include <libsolidity/SourceReferenceFormatter.h>

using namespace std;
using namespace dev;
using namespace solidity;

bool compile(string _input, ostream& o_output)
{
	ASTPointer<ContractDefinition> ast;
	shared_ptr<Scanner> scanner = make_shared<Scanner>(CharStream(_input));
	Parser parser;
	try
	{
		ast = parser.parse(scanner);
	}
	catch (ParserError const& exception)
	{
		SourceReferenceFormatter::printExceptionInformation(o_output, exception, "Parser error", *scanner);
		return false;
	}

	NameAndTypeResolver resolver;
	try
	{
		resolver.resolveNamesAndTypes(*ast.get());
	}
	catch (DeclarationError const& exception)
	{
		SourceReferenceFormatter::printExceptionInformation(o_output, exception, "Parser error", *scanner);
		return false;
	}
	catch (TypeError const& exception)
	{
		SourceReferenceFormatter::printExceptionInformation(o_output, exception, "Parser error", *scanner);
		return false;
	}

	o_output << "Syntax tree for the contract:" << endl;
	ASTPrinter printer(ast, _input);
	printer.print(o_output);

	return true;
}

extern "C" char const* compileString(char const* _input)
{
	static string outputBuffer;
	ostringstream outputStream;
	compile(_input, outputStream);
	outputBuffer = outputStream.str();
	return outputBuffer.c_str();
}
