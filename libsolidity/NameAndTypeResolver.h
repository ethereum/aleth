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
 * Parser part that determines the declarations corresponding to names and the types of expressions.
 */

#pragma once

#include <map>
#include <boost/noncopyable.hpp>

#include <libsolidity/Scope.h>
#include <libsolidity/ASTVisitor.h>

namespace dev
{
namespace solidity
{

/**
 * Resolves name references, types and checks types of all expressions.
 * Specifically, it checks that all operations are valid for the inferred types.
 * An exception is throw on the first error.
 */
class NameAndTypeResolver: private boost::noncopyable
{
public:
	NameAndTypeResolver() {}

	void resolveNamesAndTypes(ContractDefinition& _contract);

	/// Resolves the given @a _name inside the scope @a _scope. If @a _scope is omitted,
	/// the global scope is used (i.e. the one containing only the contract).
	/// @returns a pointer to the declaration on success or nullptr on failure.
	Declaration* resolveName(ASTString const& _name, Declaration const* _scope = nullptr) const;

	/// Resolves a name in the "current" scope. Should only be called during the initial
	/// resolving phase.
	Declaration* getNameFromCurrentScope(ASTString const& _name, bool _recursive = true);

private:
	void reset();

	/// Maps nodes declaring a scope to scopes, i.e. ContractDefinition, FunctionDeclaration and
	/// StructDefinition (@todo not yet implemented), where nullptr denotes the global scope.
	std::map<ASTNode const*, Scope> m_scopes;

	Scope* m_currentScope;
};

/**
 * Traverses the given AST upon construction and fills _scopes with all declarations inside the
 * AST.
 */
class DeclarationRegistrationHelper: private ASTVisitor
{
public:
	DeclarationRegistrationHelper(std::map<ASTNode const*, Scope>& _scopes, ASTNode& _astRoot);

private:
	bool visit(ContractDefinition& _contract);
	void endVisit(ContractDefinition& _contract);
	bool visit(StructDefinition& _struct);
	void endVisit(StructDefinition& _struct);
	bool visit(FunctionDefinition& _function);
	void endVisit(FunctionDefinition& _function);
	void endVisit(VariableDefinition& _variableDefinition);
	bool visit(VariableDeclaration& _declaration);

	void enterNewSubScope(ASTNode& _node);
	void closeCurrentScope();
	void registerDeclaration(Declaration& _declaration, bool _opensScope);

	std::map<ASTNode const*, Scope>& m_scopes;
	Scope* m_currentScope;
	FunctionDefinition* m_currentFunction;
};

/**
 * Resolves references to declarations (of variables and types) and also establishes the link
 * between a return statement and the return parameter list.
 */
class ReferencesResolver: private ASTVisitor
{
public:
	ReferencesResolver(ASTNode& _root, NameAndTypeResolver& _resolver, ParameterList* _returnParameters);

private:
	virtual void endVisit(VariableDeclaration& _variable) override;
	virtual bool visit(Identifier& _identifier) override;
	virtual bool visit(UserDefinedTypeName& _typeName) override;
	virtual bool visit(Mapping&) override;
	virtual bool visit(Return& _return) override;

	NameAndTypeResolver& m_resolver;
	ParameterList* m_returnParameters;
};

}
}
