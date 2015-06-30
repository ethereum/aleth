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
 * Solidity AST to EVM bytecode compiler for expressions.
 */

#include <functional>
#include <memory>
#include <boost/noncopyable.hpp>
#include <libdevcore/Common.h>
#include <libevmasm/SourceLocation.h>
#include <libsolidity/ASTVisitor.h>
#include <libsolidity/LValue.h>
#include <libsolidity/Utils.h>

namespace dev {
namespace eth
{
class AssemblyItem; // forward
}
namespace solidity {

// forward declarations
class CompilerContext;
class CompilerUtils;
class Type;
class IntegerType;
class ArrayType;

/**
 * Compiler for expressions, i.e. converts an AST tree whose root is an Expression into a stream
 * of EVM instructions. It needs a compiler context that is the same for the whole compilation
 * unit.
 */
class ExpressionCompiler: private ASTConstVisitor
{
public:
	/// Appends code for a State Variable accessor function
	static void appendStateVariableAccessor(CompilerContext& _context, VariableDeclaration const& _varDecl, bool _optimize = false);

	explicit ExpressionCompiler(CompilerContext& _compilerContext, bool _optimize = false):
		m_optimize(_optimize), m_context(_compilerContext) {}

	/// Compile the given @a _expression and leave its value on the stack.
	void compile(Expression const& _expression);

	/// Appends code to set a state variable to its initial value/expression.
	void appendStateVariableInitialization(VariableDeclaration const& _varDecl);

	/// Appends code for a State Variable accessor function
	void appendStateVariableAccessor(VariableDeclaration const& _varDecl);

private:
	virtual bool visit(Assignment const& _assignment) override;
	virtual bool visit(UnaryOperation const& _unaryOperation) override;
	virtual bool visit(BinaryOperation const& _binaryOperation) override;
	virtual bool visit(FunctionCall const& _functionCall) override;
	virtual bool visit(NewExpression const& _newExpression) override;
	virtual void endVisit(MemberAccess const& _memberAccess) override;
	virtual bool visit(IndexAccess const& _indexAccess) override;
	virtual void endVisit(Identifier const& _identifier) override;
	virtual void endVisit(Literal const& _literal) override;

	///@{
	///@name Append code for various operator types
	void appendAndOrOperatorCode(BinaryOperation const& _binaryOperation);
	void appendCompareOperatorCode(Token::Value _operator, Type const& _type);
	void appendOrdinaryBinaryOperatorCode(Token::Value _operator, Type const& _type);

	void appendArithmeticOperatorCode(Token::Value _operator, Type const& _type);
	void appendBitOperatorCode(Token::Value _operator);
	void appendShiftOperatorCode(Token::Value _operator);
	/// @}

	/// Appends code to call a function of the given type with the given arguments.
	void appendExternalFunctionCall(
		FunctionType const& _functionType,
		std::vector<ASTPointer<Expression const>> const& _arguments
	);
	/// Appends code that evaluates a single expression and moves the result to memory. The memory offset is
	/// expected to be on the stack and is updated by this call.
	void appendExpressionCopyToMemory(Type const& _expectedType, Expression const& _expression);

	/// Sets the current LValue to a new one (of the appropriate type) from the given declaration.
	/// Also retrieves the value if it was not requested by @a _expression.
	void setLValueFromDeclaration(Declaration const& _declaration, Expression const& _expression);
	/// Sets the current LValue to a StorageItem holding the type of @a _expression. The reference is assumed
	/// to be on the stack.
	/// Also retrieves the value if it was not requested by @a _expression.
	void setLValueToStorageItem(Expression const& _expression);
	/// Sets the current LValue to a new LValue constructed from the arguments.
	/// Also retrieves the value if it was not requested by @a _expression.
	template <class _LValueType, class... _Arguments>
	void setLValue(Expression const& _expression, _Arguments const&... _arguments);

	/// @returns the CompilerUtils object containing the current context.
	CompilerUtils utils();

	bool m_optimize;
	CompilerContext& m_context;
	std::unique_ptr<LValue> m_currentLValue;

};

template <class _LValueType, class... _Arguments>
void ExpressionCompiler::setLValue(Expression const& _expression, _Arguments const&... _arguments)
{
	solAssert(!m_currentLValue, "Current LValue not reset before trying to set new one.");
	std::unique_ptr<_LValueType> lvalue(new _LValueType(m_context, _arguments...));
	if (_expression.lvalueRequested())
		m_currentLValue = move(lvalue);
	else
		lvalue->retrieveValue(_expression.getLocation(), true);
}

}
}
