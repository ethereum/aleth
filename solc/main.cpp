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
 * Solidity commandline compiler.
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
#include <libsolidity/Compiler.h>
#include <libsolidity/SourceReferenceFormatter.h>

using namespace std;
using namespace dev;
using namespace solidity;

void help()
{
	cout << "Usage solc [OPTIONS] <file>" << endl
		 << "Options:" << endl
		 << "    -o,--optimize Optimize the bytecode for size." << endl
		 << "    -h,--help     Show this help message and exit." << endl
		 << "    -V,--version  Show the version and exit." << endl;
	exit(0);
}

void version()
{
	cout << "solc, the solidity complier commandline interface " << dev::Version << endl
		 << "  by Christian <c@ethdev.com>, (c) 2014." << endl
		 << "Build: " << DEV_QUOTED(ETH_BUILD_PLATFORM) << "/" << DEV_QUOTED(ETH_BUILD_TYPE) << endl;
	exit(0);
}

int main(int argc, char** argv)
{
	string infile;
	bool optimize = false;
	for (int i = 1; i < argc; ++i)
	{
		string arg = argv[i];
		if (arg == "-o" || arg == "--optimize")
			optimize = true;
		else if (arg == "-h" || arg == "--help")
			help();
		else if (arg == "-V" || arg == "--version")
			version();
		else
			infile = argv[i];
	}
	string sourceCode;
	if (infile.empty())
	{
		string s;
		while (!cin.eof())
		{
			getline(cin, s);
			sourceCode.append(s);
		}
	}
	else
		sourceCode = asString(dev::contents(infile));

	ASTPointer<ContractDefinition> ast;
	shared_ptr<Scanner> scanner = make_shared<Scanner>(CharStream(sourceCode));
	Parser parser;
	bytes instructions;
	Compiler compiler;
	try
	{
		ast = parser.parse(scanner);

		NameAndTypeResolver resolver;
		resolver.resolveNamesAndTypes(*ast.get());

		cout << "Syntax tree for the contract:" << endl;
		dev::solidity::ASTPrinter printer(ast, sourceCode);
		printer.print(cout);

		compiler.compileContract(*ast);
		instructions = compiler.getAssembledBytecode(optimize);
	}
	catch (ParserError const& exception)
	{
		SourceReferenceFormatter::printExceptionInformation(cerr, exception, "Parser error", *scanner);
		return -1;
	}
	catch (DeclarationError const& exception)
	{
		SourceReferenceFormatter::printExceptionInformation(cerr, exception, "Declaration error", *scanner);
		return -1;
	}
	catch (TypeError const& exception)
	{
		SourceReferenceFormatter::printExceptionInformation(cerr, exception, "Type error", *scanner);
		return -1;
	}
	catch (CompilerError const& exception)
	{
		SourceReferenceFormatter::printExceptionInformation(cerr, exception, "Compiler error", *scanner);
		return -1;
	}
	catch (InternalCompilerError const& exception)
	{
		cerr << "Internal compiler error: " << boost::diagnostic_information(exception) << endl;
		return -1;
	}
	catch (Exception const& exception)
	{
		cerr << "Exception during compilation: " << boost::diagnostic_information(exception) << endl;
		return -1;
	}
	catch (...)
	{
		cerr << "Unknown exception during compilation." << endl;
		return -1;
	}

	cout << "EVM assembly:" << endl;
	compiler.streamAssembly(cout);
	cout << "Opcodes:" << endl;
	cout << eth::disassemble(instructions) << endl;
	cout << "Binary: " << toHex(instructions) << endl;

	return 0;
}
