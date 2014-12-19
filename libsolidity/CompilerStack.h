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
 * Full-stack compiler that converts a source code string to bytecode.
 */

#pragma once

#include <ostream>
#include <string>
#include <memory>
#include <boost/noncopyable.hpp>
#include <libdevcore/Common.h>

namespace dev {
namespace solidity {

// forward declarations
class Scanner;
class ContractDefinition;
class SourceUnit;
class Compiler;
class GlobalContext;
class InterfaceHandler;

enum class DocumentationType: uint8_t
{
	NATSPEC_USER = 1,
	NATSPEC_DEV,
	ABI_INTERFACE
};

/**
 * Easy to use and self-contained Solidity compiler with as few header dependencies as possible.
 * It holds state and can be used to either step through the compilation stages (and abort e.g.
 * before compilation to bytecode) or run the whole compilation in one call.
 */
class CompilerStack: boost::noncopyable
{
public:
	CompilerStack(): m_parseSuccessful(false) {}

	/// Adds a source object (e.g. file) to the parser. After this, parse has to be called again.
	/// @returns true if a source object by the name already existed and was replaced.
	bool addSource(std::string const& _name, std::string const& _content);
	void setSource(std::string const& _sourceCode);
	/// Parses all source units that were added
	void parse();
	/// Sets the given source code as the only source unit and parses it.
	void parse(std::string const& _sourceCode);
	/// Returns a list of the contract names in the sources.
	std::vector<std::string> getContractNames() const;

	/// Compiles the source units that were previously added and parsed.
	void compile(bool _optimize = false);
	/// Parses and compiles the given source code.
	/// @returns the compiled bytecode
	bytes const& compile(std::string const& _sourceCode, bool _optimize = false);

	bytes const& getBytecode(std::string const& _contractName = "") const;
	/// Streams a verbose version of the assembly to @a _outStream.
	/// Prerequisite: Successful compilation.
	void streamAssembly(std::ostream& _outStream, std::string const& _contractName = "") const;

	/// Returns a string representing the contract interface in JSON.
	/// Prerequisite: Successful call to parse or compile.
	std::string const& getInterface(std::string const& _contractName = "") const;
	/// Returns a string representing the contract's documentation in JSON.
	/// Prerequisite: Successful call to parse or compile.
	/// @param type The type of the documentation to get.
	/// Can be one of 3 types defined at @c DocumentationType
	std::string const& getJsonDocumentation(std::string const& _contractName, DocumentationType _type) const;

	/// @returns the previously used scanner, useful for counting lines during error reporting.
	Scanner const& getScanner(std::string const& _sourceName = "") const;
	/// @returns the parsed source unit with the supplied name.
	SourceUnit const& getAST(std::string const& _sourceName = "") const;
	/// @returns the parsed contract with the supplied name. Throws an exception if the contract
	/// does not exist.
	ContractDefinition const& getContractDefinition(std::string const& _contractName) const;

	/// Compile the given @a _sourceCode to bytecode. If a scanner is provided, it is used for
	/// scanning the source code - this is useful for printing exception information.
	static bytes staticCompile(std::string const& _sourceCode, bool _optimize = false);

private:
	/**
	 * Information pertaining to one source unit, filled gradually during parsing and compilation.
	 */
	struct Source
	{
		std::shared_ptr<Scanner> scanner;
		std::shared_ptr<SourceUnit> ast;
		std::string interface;
		void reset() { scanner.reset(); ast.reset(); interface.clear(); }
	};

	struct Contract
	{
		ContractDefinition const* contract;
		std::shared_ptr<Compiler> compiler;
		bytes bytecode;
		std::shared_ptr<InterfaceHandler> interfaceHandler;
		mutable std::unique_ptr<std::string const> interface;
		mutable std::unique_ptr<std::string const> userDocumentation;
		mutable std::unique_ptr<std::string const> devDocumentation;

		Contract();
	};

	void reset(bool _keepSources = false);
	void resolveImports();

	Contract const& getContract(std::string const& _contractName = "") const;
	Source const& getSource(std::string const& _sourceName = "") const;

	bool m_parseSuccessful;
	std::map<std::string const, Source> m_sources;
	std::shared_ptr<GlobalContext> m_globalContext;
	std::vector<Source const*> m_sourceOrder;
	std::map<std::string const, Contract> m_contracts;
};

}
}
