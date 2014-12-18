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
 * Solidity abstract syntax tree.
 */

#pragma once


#include <string>
#include <vector>
#include <memory>
#include <boost/noncopyable.hpp>
#include <libsolidity/Utils.h>
#include <libsolidity/ASTForward.h>
#include <libsolidity/BaseTypes.h>
#include <libsolidity/Token.h>
#include <libsolidity/Types.h>
#include <libsolidity/Exceptions.h>

namespace dev
{
namespace solidity
{

class ASTVisitor;
class ASTConstVisitor;


/**
 * The root (abstract) class of the AST inheritance tree.
 * It is possible to traverse all direct and indirect children of an AST node by calling
 * accept, providing an ASTVisitor.
 */
class ASTNode: private boost::noncopyable
{
public:
	explicit ASTNode(Location const& _location): m_location(_location) {}

	virtual ~ASTNode() {}

	virtual void accept(ASTVisitor& _visitor) = 0;
	virtual void accept(ASTConstVisitor& _visitor) const = 0;
	template <class T>
	static void listAccept(std::vector<ASTPointer<T>>& _list, ASTVisitor& _visitor)
	{
		for (ASTPointer<T>& element: _list)
			element->accept(_visitor);
	}
	template <class T>
	static void listAccept(std::vector<ASTPointer<T>> const& _list, ASTConstVisitor& _visitor)
	{
		for (ASTPointer<T> const& element: _list)
			element->accept(_visitor);
	}

	/// Returns the source code location of this node.
	Location const& getLocation() const { return m_location; }

	/// Creates a @ref TypeError exception and decorates it with the location of the node and
	/// the given description
	TypeError createTypeError(std::string const& _description) const;

	///@{
	///@name equality operators
	/// Equality relies on the fact that nodes cannot be copied.
	bool operator==(ASTNode const& _other) const { return this == &_other; }
	bool operator!=(ASTNode const& _other) const { return !operator==(_other); }
	///@}

private:
	Location m_location;
};

/**
 * Source unit containing import directives and contract definitions.
 */
class SourceUnit: public ASTNode
{
public:
	SourceUnit(Location const& _location, std::vector<ASTPointer<ASTNode>> const& _nodes):
		ASTNode(_location), m_nodes(_nodes) {}

	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;

	std::vector<ASTPointer<ASTNode>> getNodes() const { return m_nodes; }

private:
	std::vector<ASTPointer<ASTNode>> m_nodes;
};

/**
 * Import directive for referencing other files / source objects.
 * Example: import "abc.sol"
 * Source objects are identified by a string which can be a file name but does not have to be.
 */
class ImportDirective: public ASTNode
{
public:
	ImportDirective(Location const& _location, ASTPointer<ASTString> const& _identifier):
		ASTNode(_location), m_identifier(_identifier) {}

	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;

	ASTString const& getIdentifier() const { return *m_identifier; }

private:
	ASTPointer<ASTString> m_identifier;
};

/**
 * Abstract AST class for a declaration (contract, function, struct, variable).
 */
class Declaration: public ASTNode
{
public:
	Declaration(Location const& _location, ASTPointer<ASTString> const& _name):
		ASTNode(_location), m_name(_name), m_scope(nullptr) {}

	/// @returns the declared name.
	ASTString const& getName() const { return *m_name; }
	/// @returns the scope this declaration resides in. Can be nullptr if it is the global scope.
	/// Available only after name and type resolution step.
	Declaration const* getScope() const { return m_scope; }
	void setScope(Declaration const* _scope) { m_scope = _scope; }

private:
	ASTPointer<ASTString> m_name;
	Declaration const* m_scope;
};

/**
 * Definition of a contract. This is the only AST nodes where child nodes are not visited in
 * document order. It first visits all struct declarations, then all variable declarations and
 * finally all function declarations.
 */
class ContractDefinition: public Declaration
{
public:
	ContractDefinition(Location const& _location,
					   ASTPointer<ASTString> const& _name,
					   ASTPointer<ASTString> const& _documentation,
					   std::vector<ASTPointer<StructDefinition>> const& _definedStructs,
					   std::vector<ASTPointer<VariableDeclaration>> const& _stateVariables,
					   std::vector<ASTPointer<FunctionDefinition>> const& _definedFunctions):
		Declaration(_location, _name),
		m_definedStructs(_definedStructs),
		m_stateVariables(_stateVariables),
		m_definedFunctions(_definedFunctions),
		m_documentation(_documentation)
	{}

	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;

	std::vector<ASTPointer<StructDefinition>> const& getDefinedStructs() const { return m_definedStructs; }
	std::vector<ASTPointer<VariableDeclaration>> const& getStateVariables() const { return m_stateVariables; }
	std::vector<ASTPointer<FunctionDefinition>> const& getDefinedFunctions() const { return m_definedFunctions; }

	/// Checks that the constructor does not have a "returns" statement and calls
	/// checkTypeRequirements on all its functions.
	void checkTypeRequirements();

	/// @return A shared pointer of an ASTString.
	/// Can contain a nullptr in which case indicates absence of documentation
	ASTPointer<ASTString> const& getDocumentation() const { return m_documentation; }

	/// Returns the functions that make up the calling interface in the intended order.
	std::vector<FunctionDefinition const*> getInterfaceFunctions() const;

	/// Returns the constructor or nullptr if no constructor was specified
	FunctionDefinition const* getConstructor() const;

private:
	std::vector<ASTPointer<StructDefinition>> m_definedStructs;
	std::vector<ASTPointer<VariableDeclaration>> m_stateVariables;
	std::vector<ASTPointer<FunctionDefinition>> m_definedFunctions;
	ASTPointer<ASTString> m_documentation;
};

class StructDefinition: public Declaration
{
public:
	StructDefinition(Location const& _location,
					 ASTPointer<ASTString> const& _name,
					 std::vector<ASTPointer<VariableDeclaration>> const& _members):
		Declaration(_location, _name), m_members(_members) {}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;

	std::vector<ASTPointer<VariableDeclaration>> const& getMembers() const { return m_members; }

	/// Checks that the members do not include any recursive structs and have valid types
	/// (e.g. no functions).
	void checkValidityOfMembers() const;

private:
	void checkMemberTypes() const;
	void checkRecursion() const;

	std::vector<ASTPointer<VariableDeclaration>> m_members;
};

/**
 * Parameter list, used as function parameter list and return list.
 * None of the parameters is allowed to contain mappings (not even recursively
 * inside structs).
 */
class ParameterList: public ASTNode
{
public:
	ParameterList(Location const& _location,
				  std::vector<ASTPointer<VariableDeclaration>> const& _parameters):
		ASTNode(_location), m_parameters(_parameters) {}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;

	std::vector<ASTPointer<VariableDeclaration>> const& getParameters() const { return m_parameters; }

private:
	std::vector<ASTPointer<VariableDeclaration>> m_parameters;
};

class FunctionDefinition: public Declaration
{
public:
	FunctionDefinition(Location const& _location, ASTPointer<ASTString> const& _name,
					bool _isPublic,
					ASTPointer<ASTString> const& _documentation,
					ASTPointer<ParameterList> const& _parameters,
					bool _isDeclaredConst,
					ASTPointer<ParameterList> const& _returnParameters,
					ASTPointer<Block> const& _body):
	Declaration(_location, _name), m_isPublic(_isPublic),
	m_parameters(_parameters),
	m_isDeclaredConst(_isDeclaredConst),
	m_returnParameters(_returnParameters),
	m_body(_body),
	m_documentation(_documentation)
	{}

	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;

	bool isPublic() const { return m_isPublic; }
	bool isDeclaredConst() const { return m_isDeclaredConst; }
	std::vector<ASTPointer<VariableDeclaration>> const& getParameters() const { return m_parameters->getParameters(); }
	ParameterList const& getParameterList() const { return *m_parameters; }
	std::vector<ASTPointer<VariableDeclaration>> const& getReturnParameters() const { return m_returnParameters->getParameters(); }
	ASTPointer<ParameterList> const& getReturnParameterList() const { return m_returnParameters; }
	Block const& getBody() const { return *m_body; }
	/// @return A shared pointer of an ASTString.
	/// Can contain a nullptr in which case indicates absence of documentation
	ASTPointer<ASTString> const& getDocumentation() const { return m_documentation; }

	void addLocalVariable(VariableDeclaration const& _localVariable) { m_localVariables.push_back(&_localVariable); }
	std::vector<VariableDeclaration const*> const& getLocalVariables() const { return m_localVariables; }

	/// Checks that all parameters have allowed types and calls checkTypeRequirements on the body.
	void checkTypeRequirements();

private:
	bool m_isPublic;
	ASTPointer<ParameterList> m_parameters;
	bool m_isDeclaredConst;
	ASTPointer<ParameterList> m_returnParameters;
	ASTPointer<Block> m_body;
	ASTPointer<ASTString> m_documentation;

	std::vector<VariableDeclaration const*> m_localVariables;
};

/**
 * Declaration of a variable. This can be used in various places, e.g. in function parameter
 * lists, struct definitions and even function bodys.
 */
class VariableDeclaration: public Declaration
{
public:
	VariableDeclaration(Location const& _location, ASTPointer<TypeName> const& _type,
						ASTPointer<ASTString> const& _name):
		Declaration(_location, _name), m_typeName(_type) {}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;

	TypeName const* getTypeName() const { return m_typeName.get(); }

	/// Returns the declared or inferred type. Can be an empty pointer if no type was explicitly
	/// declared and there is no assignment to the variable that fixes the type.
	std::shared_ptr<Type const> const& getType() const { return m_type; }
	void setType(std::shared_ptr<Type const> const& _type) { m_type = _type; }

	bool isLocalVariable() const { return !!dynamic_cast<FunctionDefinition const*>(getScope()); }

private:
	ASTPointer<TypeName> m_typeName; ///< can be empty ("var")

	std::shared_ptr<Type const> m_type; ///< derived type, initially empty
};

/**
 * Pseudo AST node that is used as declaration for "this", "msg", "tx", "block" and the global
 * functions when such an identifier is encountered. Will never have a valid location in the source code.
 */
class MagicVariableDeclaration: public Declaration
{
public:
	MagicVariableDeclaration(ASTString const& _name, std::shared_ptr<Type const> const& _type):
		Declaration(Location(), std::make_shared<ASTString>(_name)), m_type(_type) {}
	virtual void accept(ASTVisitor&) override { BOOST_THROW_EXCEPTION(InternalCompilerError()
							<< errinfo_comment("MagicVariableDeclaration used inside real AST.")); }
	virtual void accept(ASTConstVisitor&) const override { BOOST_THROW_EXCEPTION(InternalCompilerError()
							<< errinfo_comment("MagicVariableDeclaration used inside real AST.")); }

	std::shared_ptr<Type const> const& getType() const { return m_type; }

private:
	std::shared_ptr<Type const> m_type;
};

/// Types
/// @{

/**
 * Abstract base class of a type name, can be any built-in or user-defined type.
 */
class TypeName: public ASTNode
{
public:
	explicit TypeName(Location const& _location): ASTNode(_location) {}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;

	/// Retrieve the element of the type hierarchy this node refers to. Can return an empty shared
	/// pointer until the types have been resolved using the @ref NameAndTypeResolver.
	/// If it returns an empty shared pointer after that, this indicates that the type was not found.
	virtual std::shared_ptr<Type const> toType() const = 0;
};

/**
 * Any pre-defined type name represented by a single keyword, i.e. it excludes mappings,
 * contracts, functions, etc.
 */
class ElementaryTypeName: public TypeName
{
public:
	explicit ElementaryTypeName(Location const& _location, Token::Value _type):
		TypeName(_location), m_type(_type)
	{
		solAssert(Token::isElementaryTypeName(_type), "");
	}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;
	virtual std::shared_ptr<Type const> toType() const override { return Type::fromElementaryTypeName(m_type); }

	Token::Value getTypeName() const { return m_type; }

private:
	Token::Value m_type;
};

/**
 * Name referring to a user-defined type (i.e. a struct, contract, etc.).
 */
class UserDefinedTypeName: public TypeName
{
public:
	UserDefinedTypeName(Location const& _location, ASTPointer<ASTString> const& _name):
		TypeName(_location), m_name(_name), m_referencedDeclaration(nullptr) {}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;
	virtual std::shared_ptr<Type const> toType() const override { return Type::fromUserDefinedTypeName(*this); }

	ASTString const& getName() const { return *m_name; }
	void setReferencedDeclaration(Declaration const& _referencedDeclaration) { m_referencedDeclaration = &_referencedDeclaration; }
	Declaration const* getReferencedDeclaration() const { return m_referencedDeclaration; }

private:
	ASTPointer<ASTString> m_name;

	Declaration const* m_referencedDeclaration;
};

/**
 * A mapping type. Its source form is "mapping('keyType' => 'valueType')"
 */
class Mapping: public TypeName
{
public:
	Mapping(Location const& _location, ASTPointer<ElementaryTypeName> const& _keyType,
			ASTPointer<TypeName> const& _valueType):
		TypeName(_location), m_keyType(_keyType), m_valueType(_valueType) {}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;
	virtual std::shared_ptr<Type const> toType() const override { return Type::fromMapping(*this); }

	ElementaryTypeName const& getKeyType() const { return *m_keyType; }
	TypeName const& getValueType() const { return *m_valueType; }

private:
	ASTPointer<ElementaryTypeName> m_keyType;
	ASTPointer<TypeName> m_valueType;
};

/// @}

/// Statements
/// @{


/**
 * Abstract base class for statements.
 */
class Statement: public ASTNode
{
public:
	explicit Statement(Location const& _location): ASTNode(_location) {}

	/// Check all type requirements, throws exception if some requirement is not met.
	/// This includes checking that operators are applicable to their arguments but also that
	/// the number of function call arguments matches the number of formal parameters and so forth.
	virtual void checkTypeRequirements() = 0;
};

/**
 * Brace-enclosed block containing zero or more statements.
 */
class Block: public Statement
{
public:
	Block(Location const& _location, std::vector<ASTPointer<Statement>> const& _statements):
		Statement(_location), m_statements(_statements) {}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;

	virtual void checkTypeRequirements() override;

private:
	std::vector<ASTPointer<Statement>> m_statements;
};

/**
 * If-statement with an optional "else" part. Note that "else if" is modeled by having a new
 * if-statement as the false (else) body.
 */
class IfStatement: public Statement
{
public:
	IfStatement(Location const& _location, ASTPointer<Expression> const& _condition,
				ASTPointer<Statement> const& _trueBody, ASTPointer<Statement> const& _falseBody):
		Statement(_location),
		m_condition(_condition), m_trueBody(_trueBody), m_falseBody(_falseBody) {}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;
	virtual void checkTypeRequirements() override;

	Expression const& getCondition() const { return *m_condition; }
	Statement const& getTrueStatement() const { return *m_trueBody; }
	/// @returns the "else" part of the if statement or nullptr if there is no "else" part. 
	Statement const* getFalseStatement() const { return m_falseBody.get(); }

private:
	ASTPointer<Expression> m_condition;
	ASTPointer<Statement> m_trueBody;
	ASTPointer<Statement> m_falseBody; ///< "else" part, optional
};

/**
 * Statement in which a break statement is legal (abstract class).
 */
class BreakableStatement: public Statement
{
public:
	BreakableStatement(Location const& _location): Statement(_location) {}
};

class WhileStatement: public BreakableStatement
{
public:
	WhileStatement(Location const& _location, ASTPointer<Expression> const& _condition,
				   ASTPointer<Statement> const& _body):
		BreakableStatement(_location), m_condition(_condition), m_body(_body) {}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;
	virtual void checkTypeRequirements() override;

	Expression const& getCondition() const { return *m_condition; }
	Statement const& getBody() const { return *m_body; }

private:
	ASTPointer<Expression> m_condition;
	ASTPointer<Statement> m_body;
};

/**
 * For loop statement
 */
class ForStatement: public BreakableStatement
{
public:
	ForStatement(Location const& _location,
				 ASTPointer<Statement> const& _initExpression,
				 ASTPointer<Expression> const& _conditionExpression,
				 ASTPointer<ExpressionStatement> const& _loopExpression,
				 ASTPointer<Statement> const& _body):
		BreakableStatement(_location),
		m_initExpression(_initExpression),
		m_condExpression(_conditionExpression),
		m_loopExpression(_loopExpression),
		m_body(_body) {}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;
	virtual void checkTypeRequirements() override;

	Statement const* getInitializationExpression() const { return m_initExpression.get(); }
	Expression const* getCondition() const { return m_condExpression.get(); }
	ExpressionStatement const* getLoopExpression() const { return m_loopExpression.get(); }
	Statement const& getBody() const { return *m_body; }

private:
	/// For statement's initialization expresion. for(XXX; ; ). Can be empty
	ASTPointer<Statement> m_initExpression;
	/// For statement's condition expresion. for(; XXX ; ). Can be empty
	ASTPointer<Expression> m_condExpression;
	/// For statement's loop expresion. for(;;XXX). Can be empty
	ASTPointer<ExpressionStatement> m_loopExpression;
	/// The body of the loop
	ASTPointer<Statement> m_body;
};

class Continue: public Statement
{
public:
	Continue(Location const& _location): Statement(_location) {}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;
	virtual void checkTypeRequirements() override {}
};

class Break: public Statement
{
public:
	Break(Location const& _location): Statement(_location) {}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;
	virtual void checkTypeRequirements() override {}
};

class Return: public Statement
{
public:
	Return(Location const& _location, ASTPointer<Expression> _expression):
		Statement(_location), m_expression(_expression), m_returnParameters(nullptr) {}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;
	virtual void checkTypeRequirements() override;

	void setFunctionReturnParameters(ParameterList& _parameters) { m_returnParameters = &_parameters; }
	ParameterList const& getFunctionReturnParameters() const
	{
		solAssert(m_returnParameters, "");
		return *m_returnParameters;
	}
	Expression const* getExpression() const { return m_expression.get(); }

private:
	ASTPointer<Expression> m_expression; ///< value to return, optional

	/// Pointer to the parameter list of the function, filled by the @ref NameAndTypeResolver.
	ParameterList* m_returnParameters;
};

/**
 * Definition of a variable as a statement inside a function. It requires a type name (which can
 * also be "var") but the actual assignment can be missing.
 * Examples: var a = 2; uint256 a;
 */
class VariableDefinition: public Statement
{
public:
	VariableDefinition(Location const& _location, ASTPointer<VariableDeclaration> _variable,
					   ASTPointer<Expression> _value):
		Statement(_location), m_variable(_variable), m_value(_value) {}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;
	virtual void checkTypeRequirements() override;

	VariableDeclaration const& getDeclaration() const { return *m_variable; }
	Expression const* getExpression() const { return m_value.get(); }

private:
	ASTPointer<VariableDeclaration> m_variable;
	ASTPointer<Expression> m_value; ///< the assigned value, can be missing
};

/**
 * A statement that contains only an expression (i.e. an assignment, function call, ...).
 */
class ExpressionStatement: public Statement
{
public:
	ExpressionStatement(Location const& _location, ASTPointer<Expression> _expression):
		Statement(_location), m_expression(_expression) {}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;
	virtual void checkTypeRequirements() override;

	Expression const& getExpression() const { return *m_expression; }

private:
	ASTPointer<Expression> m_expression;
};

/// @}

/// Expressions
/// @{

/**
 * An expression, i.e. something that has a value (which can also be of type "void" in case
 * of some function calls).
 * @abstract
 */
class Expression: public ASTNode
{
protected:
	enum class LValueType { NONE, LOCAL, STORAGE };

public:
	Expression(Location const& _location): ASTNode(_location), m_lvalue(LValueType::NONE), m_lvalueRequested(false) {}
	virtual void checkTypeRequirements() = 0;

	std::shared_ptr<Type const> const& getType() const { return m_type; }
	bool isLValue() const { return m_lvalue != LValueType::NONE; }
	bool isLocalLValue() const { return m_lvalue == LValueType::LOCAL; }

	/// Helper function, infer the type via @ref checkTypeRequirements and then check that it
	/// is implicitly convertible to @a _expectedType. If not, throw exception.
	void expectType(Type const& _expectedType);
	/// Checks that this expression is an lvalue and also registers that an address and
	/// not a value is generated during compilation. Can be called after checkTypeRequirements()
	/// by an enclosing expression.
	void requireLValue();
	/// Returns true if @a requireLValue was previously called on this expression.
	bool lvalueRequested() const { return m_lvalueRequested; }

protected:
	//! Inferred type of the expression, only filled after a call to checkTypeRequirements().
	std::shared_ptr<Type const> m_type;
	//! If this expression is an lvalue (i.e. something that can be assigned to) and is stored
	//! locally or in storage. This is set during calls to @a checkTypeRequirements()
	LValueType m_lvalue;
	//! Whether the outer expression requested the address (true) or the value (false) of this expression.
	bool m_lvalueRequested;
};

/// Assignment, can also be a compound assignment.
/// Examples: (a = 7 + 8) or (a *= 2)
class Assignment: public Expression
{
public:
	Assignment(Location const& _location, ASTPointer<Expression> const& _leftHandSide,
			   Token::Value _assignmentOperator, ASTPointer<Expression> const& _rightHandSide):
		Expression(_location), m_leftHandSide(_leftHandSide),
		m_assigmentOperator(_assignmentOperator), m_rightHandSide(_rightHandSide)
	{
		solAssert(Token::isAssignmentOp(_assignmentOperator), "");
	}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;
	virtual void checkTypeRequirements() override;

	Expression const& getLeftHandSide() const { return *m_leftHandSide; }
	Token::Value getAssignmentOperator() const { return m_assigmentOperator; }
	Expression const& getRightHandSide() const { return *m_rightHandSide; }

private:
	ASTPointer<Expression> m_leftHandSide;
	Token::Value m_assigmentOperator;
	ASTPointer<Expression> m_rightHandSide;
};

/**
 * Operation involving a unary operator, pre- or postfix.
 * Examples: ++i, delete x or !true
 */
class UnaryOperation: public Expression
{
public:
	UnaryOperation(Location const& _location, Token::Value _operator,
				   ASTPointer<Expression> const& _subExpression, bool _isPrefix):
		Expression(_location), m_operator(_operator),
		m_subExpression(_subExpression), m_isPrefix(_isPrefix)
	{
		solAssert(Token::isUnaryOp(_operator), "");
	}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;
	virtual void checkTypeRequirements() override;

	Token::Value getOperator() const { return m_operator; }
	bool isPrefixOperation() const { return m_isPrefix; }
	Expression const& getSubExpression() const { return *m_subExpression; }

private:
	Token::Value m_operator;
	ASTPointer<Expression> m_subExpression;
	bool m_isPrefix;
};

/**
 * Operation involving a binary operator.
 * Examples: 1 + 2, true && false or 1 <= 4
 */
class BinaryOperation: public Expression
{
public:
	BinaryOperation(Location const& _location, ASTPointer<Expression> const& _left,
					Token::Value _operator, ASTPointer<Expression> const& _right):
		Expression(_location), m_left(_left), m_operator(_operator), m_right(_right)
	{
		solAssert(Token::isBinaryOp(_operator) || Token::isCompareOp(_operator), "");
	}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;
	virtual void checkTypeRequirements() override;

	Expression const& getLeftExpression() const { return *m_left; }
	Expression const& getRightExpression() const { return *m_right; }
	Token::Value getOperator() const { return m_operator; }
	Type const& getCommonType() const { return *m_commonType; }

private:
	ASTPointer<Expression> m_left;
	Token::Value m_operator;
	ASTPointer<Expression> m_right;

	/// The common type that is used for the operation, not necessarily the result type (e.g. for
	/// comparisons, this is always bool).
	std::shared_ptr<Type const> m_commonType;
};

/**
 * Can be ordinary function call, type cast or struct construction.
 */
class FunctionCall: public Expression
{
public:
	FunctionCall(Location const& _location, ASTPointer<Expression> const& _expression,
				 std::vector<ASTPointer<Expression>> const& _arguments):
		Expression(_location), m_expression(_expression), m_arguments(_arguments) {}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;
	virtual void checkTypeRequirements() override;

	Expression const& getExpression() const { return *m_expression; }
	std::vector<ASTPointer<Expression const>> getArguments() const { return {m_arguments.begin(), m_arguments.end()}; }

	/// Returns true if this is not an actual function call, but an explicit type conversion
	/// or constructor call.
	bool isTypeConversion() const;

private:
	ASTPointer<Expression> m_expression;
	std::vector<ASTPointer<Expression>> m_arguments;
};

/**
 * Expression that creates a new contract, e.g. "new SomeContract(1, 2)".
 */
class NewExpression: public Expression
{
public:
	NewExpression(Location const& _location, ASTPointer<Identifier> const& _contractName,
				  std::vector<ASTPointer<Expression>> const& _arguments):
		Expression(_location), m_contractName(_contractName), m_arguments(_arguments) {}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;
	virtual void checkTypeRequirements() override;

	std::vector<ASTPointer<Expression const>> getArguments() const { return {m_arguments.begin(), m_arguments.end()}; }

	/// Returns the referenced contract. Can only be called after type checking.
	ContractDefinition const* getContract() const { solAssert(m_contract, ""); return m_contract; }

private:
	ASTPointer<Identifier> m_contractName;
	std::vector<ASTPointer<Expression>> m_arguments;

	ContractDefinition const* m_contract = nullptr;
};

/**
 * Access to a member of an object. Example: x.name
 */
class MemberAccess: public Expression
{
public:
	MemberAccess(Location const& _location, ASTPointer<Expression> _expression,
				 ASTPointer<ASTString> const& _memberName):
		Expression(_location), m_expression(_expression), m_memberName(_memberName) {}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;
	Expression const& getExpression() const { return *m_expression; }
	ASTString const& getMemberName() const { return *m_memberName; }
	virtual void checkTypeRequirements() override;

private:
	ASTPointer<Expression> m_expression;
	ASTPointer<ASTString> m_memberName;
};

/**
 * Index access to an array. Example: a[2]
 */
class IndexAccess: public Expression
{
public:
	IndexAccess(Location const& _location, ASTPointer<Expression> const& _base,
				ASTPointer<Expression> const& _index):
		Expression(_location), m_base(_base), m_index(_index) {}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;
	virtual void checkTypeRequirements() override;

	Expression const& getBaseExpression() const { return *m_base; }
	Expression const& getIndexExpression() const { return *m_index; }

private:
	ASTPointer<Expression> m_base;
	ASTPointer<Expression> m_index;
};

/**
 * Primary expression, i.e. an expression that cannot be divided any further. Examples are literals
 * or variable references.
 */
class PrimaryExpression: public Expression
{
public:
	PrimaryExpression(Location const& _location): Expression(_location) {}
};

/**
 * An identifier, i.e. a reference to a declaration by name like a variable or function.
 */
class Identifier: public PrimaryExpression
{
public:
	Identifier(Location const& _location, ASTPointer<ASTString> const& _name):
		PrimaryExpression(_location), m_name(_name), m_referencedDeclaration(nullptr) {}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;
	virtual void checkTypeRequirements() override;

	ASTString const& getName() const { return *m_name; }

	void setReferencedDeclaration(Declaration const& _referencedDeclaration) { m_referencedDeclaration = &_referencedDeclaration; }
	Declaration const* getReferencedDeclaration() const { return m_referencedDeclaration; }

private:
	ASTPointer<ASTString> m_name;

	/// Declaration the name refers to.
	Declaration const* m_referencedDeclaration;
};

/**
 * An elementary type name expression is used in expressions like "a = uint32(2)" to change the
 * type of an expression explicitly. Here, "uint32" is the elementary type name expression and
 * "uint32(2)" is a @ref FunctionCall.
 */
class ElementaryTypeNameExpression: public PrimaryExpression
{
public:
	ElementaryTypeNameExpression(Location const& _location, Token::Value _typeToken):
		PrimaryExpression(_location), m_typeToken(_typeToken)
	{
		solAssert(Token::isElementaryTypeName(_typeToken), "");
	}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;
	virtual void checkTypeRequirements() override;

	Token::Value getTypeToken() const { return m_typeToken; }

private:
	Token::Value m_typeToken;
};

/**
 * A literal string or number. @see Type::literalToBigEndian is used to actually parse its value.
 */
class Literal: public PrimaryExpression
{
public:
	Literal(Location const& _location, Token::Value _token, ASTPointer<ASTString> const& _value):
		PrimaryExpression(_location), m_token(_token), m_value(_value) {}
	virtual void accept(ASTVisitor& _visitor) override;
	virtual void accept(ASTConstVisitor& _visitor) const override;
	virtual void checkTypeRequirements() override;

	Token::Value getToken() const { return m_token; }
	/// @returns the non-parsed value of the literal
	ASTString const& getValue() const { return *m_value; }

private:
	Token::Value m_token;
	ASTPointer<ASTString> m_value;
};

/// @}

}
}
