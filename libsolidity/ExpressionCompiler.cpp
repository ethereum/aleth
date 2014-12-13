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
 * Solidity AST to EVM bytecode compiler for expressions.
 */

#include <utility>
#include <numeric>
#include <libsolidity/AST.h>
#include <libsolidity/ExpressionCompiler.h>
#include <libsolidity/CompilerContext.h>

using namespace std;

namespace dev {
namespace solidity {

void ExpressionCompiler::compileExpression(CompilerContext& _context, Expression& _expression)
{
	ExpressionCompiler compiler(_context);
	_expression.accept(compiler);
}

void ExpressionCompiler::appendTypeConversion(CompilerContext& _context,
											  Type const& _typeOnStack, Type const& _targetType)
{
	ExpressionCompiler compiler(_context);
	compiler.appendTypeConversion(_typeOnStack, _targetType);
}

bool ExpressionCompiler::visit(Assignment& _assignment)
{
	_assignment.getRightHandSide().accept(*this);
	appendTypeConversion(*_assignment.getRightHandSide().getType(), *_assignment.getType());
	m_currentLValue.reset();
	_assignment.getLeftHandSide().accept(*this);

	Token::Value op = _assignment.getAssignmentOperator();
	if (op != Token::ASSIGN) // compound assignment
		appendOrdinaryBinaryOperatorCode(Token::AssignmentToBinaryOp(op), *_assignment.getType());
	else
		m_context << eth::Instruction::POP;

	storeInLValue(_assignment);
	return false;
}

void ExpressionCompiler::endVisit(UnaryOperation& _unaryOperation)
{
	//@todo type checking and creating code for an operator should be in the same place:
	// the operator should know how to convert itself and to which types it applies, so
	// put this code together with "Type::acceptsBinary/UnaryOperator" into a class that
	// represents the operator
	switch (_unaryOperation.getOperator())
	{
	case Token::NOT: // !
		m_context << eth::Instruction::ISZERO;
		break;
	case Token::BIT_NOT: // ~
		m_context << eth::Instruction::NOT;
		break;
	case Token::DELETE: // delete
	{
		// a -> a xor a (= 0).
		// @todo semantics change for complex types
		m_context << eth::Instruction::DUP1 << eth::Instruction::XOR;
		storeInLValue(_unaryOperation);
		break;
	}
	case Token::INC: // ++ (pre- or postfix)
	case Token::DEC: // -- (pre- or postfix)
		if (!_unaryOperation.isPrefixOperation())
			m_context << eth::Instruction::DUP1;
		m_context << u256(1);
		if (_unaryOperation.getOperator() == Token::INC)
			m_context << eth::Instruction::ADD;
		else
			m_context << eth::Instruction::SWAP1 << eth::Instruction::SUB; // @todo avoid the swap
		storeInLValue(_unaryOperation, !_unaryOperation.isPrefixOperation());
		break;
	case Token::ADD: // +
		// unary add, so basically no-op
		break;
	case Token::SUB: // -
		m_context << u256(0) << eth::Instruction::SUB;
		break;
	default:
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Invalid unary operator: " +
																		 string(Token::toString(_unaryOperation.getOperator()))));
	}
}

bool ExpressionCompiler::visit(BinaryOperation& _binaryOperation)
{
	Expression& leftExpression = _binaryOperation.getLeftExpression();
	Expression& rightExpression = _binaryOperation.getRightExpression();
	Type const& commonType = _binaryOperation.getCommonType();
	Token::Value const op = _binaryOperation.getOperator();

	if (op == Token::AND || op == Token::OR) // special case: short-circuiting
		appendAndOrOperatorCode(_binaryOperation);
	else
	{
		bool cleanupNeeded = false;
		if (commonType.getCategory() == Type::Category::INTEGER)
			if (Token::isCompareOp(op) || op == Token::DIV || op == Token::MOD)
				cleanupNeeded = true;

		rightExpression.accept(*this);
		appendTypeConversion(*rightExpression.getType(), commonType, cleanupNeeded);
		leftExpression.accept(*this);
		appendTypeConversion(*leftExpression.getType(), commonType, cleanupNeeded);
		if (Token::isCompareOp(op))
			appendCompareOperatorCode(op, commonType);
		else
			appendOrdinaryBinaryOperatorCode(op, commonType);
	}

	// do not visit the child nodes, we already did that explicitly
	return false;
}

bool ExpressionCompiler::visit(FunctionCall& _functionCall)
{
	if (_functionCall.isTypeConversion())
	{
		//@todo we only have integers and bools for now which cannot be explicitly converted
		if (asserts(_functionCall.getArguments().size() == 1))
			BOOST_THROW_EXCEPTION(InternalCompilerError());
		Expression& firstArgument = *_functionCall.getArguments().front();
		firstArgument.accept(*this);
		appendTypeConversion(*firstArgument.getType(), *_functionCall.getType());
	}
	else
	{
		// Calling convention: Caller pushes return address and arguments
		// Callee removes them and pushes return values
		m_currentLValue.reset();
		_functionCall.getExpression().accept(*this);
		if (asserts(m_currentLValue.isInCode()))
			BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Code reference expected."));
		eth::AssemblyItem functionTag(eth::PushTag, m_currentLValue.location);

		FunctionDefinition const& function = dynamic_cast<FunctionType const&>(*_functionCall.getExpression().getType()).getFunction();

		eth::AssemblyItem returnLabel = m_context.pushNewTag();
		std::vector<ASTPointer<Expression>> const& arguments = _functionCall.getArguments();
		if (asserts(arguments.size() == function.getParameters().size()))
			BOOST_THROW_EXCEPTION(InternalCompilerError());
		for (unsigned i = 0; i < arguments.size(); ++i)
		{
			arguments[i]->accept(*this);
			appendTypeConversion(*arguments[i]->getType(), *function.getParameters()[i]->getType());
		}

		m_context.appendJumpTo(functionTag);
		m_context << returnLabel;

		// callee adds return parameters, but removes arguments and return label
		m_context.adjustStackOffset(function.getReturnParameters().size() - arguments.size() - 1);

		// @todo for now, the return value of a function is its first return value, so remove
		// all others
		for (unsigned i = 1; i < function.getReturnParameters().size(); ++i)
			m_context << eth::Instruction::POP;
	}
	return false;
}

void ExpressionCompiler::endVisit(MemberAccess&)
{

}

void ExpressionCompiler::endVisit(IndexAccess&)
{

}

void ExpressionCompiler::endVisit(Identifier& _identifier)
{
	Declaration const* declaration = _identifier.getReferencedDeclaration();
	if (m_context.isLocalVariable(declaration))
		m_currentLValue = LValueLocation(LValueLocation::STACK,
										 m_context.getBaseStackOffsetOfVariable(*declaration));
	else if (m_context.isStateVariable(declaration))
		m_currentLValue = LValueLocation(LValueLocation::STORAGE,
										 m_context.getStorageLocationOfVariable(*declaration));
	else if (m_context.isFunctionDefinition(declaration))
		m_currentLValue = LValueLocation(LValueLocation::CODE,
										 m_context.getFunctionEntryLabel(dynamic_cast<FunctionDefinition const&>(*declaration)).data());
	else
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Identifier type not supported or identifier not found."));

	retrieveLValueValue(_identifier);
}

void ExpressionCompiler::endVisit(Literal& _literal)
{
	switch (_literal.getType()->getCategory())
	{
	case Type::Category::INTEGER:
	case Type::Category::BOOL:
		m_context << _literal.getType()->literalValue(_literal);
		break;
	default:
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Only integer and boolean literals implemented for now."));
	}
}

void ExpressionCompiler::appendAndOrOperatorCode(BinaryOperation& _binaryOperation)
{
	Token::Value const op = _binaryOperation.getOperator();
	if (asserts(op == Token::OR || op == Token::AND))
		BOOST_THROW_EXCEPTION(InternalCompilerError());

	_binaryOperation.getLeftExpression().accept(*this);
	m_context << eth::Instruction::DUP1;
	if (op == Token::AND)
		m_context << eth::Instruction::ISZERO;
	eth::AssemblyItem endLabel = m_context.appendConditionalJump();
	m_context << eth::Instruction::POP;
	_binaryOperation.getRightExpression().accept(*this);
	m_context << endLabel;
}

void ExpressionCompiler::appendCompareOperatorCode(Token::Value _operator, Type const& _type)
{
	if (_operator == Token::EQ || _operator == Token::NE)
	{
		m_context << eth::Instruction::EQ;
		if (_operator == Token::NE)
			m_context << eth::Instruction::ISZERO;
	}
	else
	{
		IntegerType const& type = dynamic_cast<IntegerType const&>(_type);
		bool const isSigned = type.isSigned();

		switch (_operator)
		{
		case Token::GTE:
			m_context << (isSigned ? eth::Instruction::SLT : eth::Instruction::LT)
					  << eth::Instruction::ISZERO;
			break;
		case Token::LTE:
			m_context << (isSigned ? eth::Instruction::SGT : eth::Instruction::GT)
					  << eth::Instruction::ISZERO;
			break;
		case Token::GT:
			m_context << (isSigned ? eth::Instruction::SGT : eth::Instruction::GT);
			break;
		case Token::LT:
			m_context << (isSigned ? eth::Instruction::SLT : eth::Instruction::LT);
			break;
		default:
			BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Unknown comparison operator."));
		}
	}
}

void ExpressionCompiler::appendOrdinaryBinaryOperatorCode(Token::Value _operator, Type const& _type)
{
	if (Token::isArithmeticOp(_operator))
		appendArithmeticOperatorCode(_operator, _type);
	else if (Token::isBitOp(_operator))
		appendBitOperatorCode(_operator);
	else if (Token::isShiftOp(_operator))
		appendShiftOperatorCode(_operator);
	else
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Unknown binary operator."));
}

void ExpressionCompiler::appendArithmeticOperatorCode(Token::Value _operator, Type const& _type)
{
	IntegerType const& type = dynamic_cast<IntegerType const&>(_type);
	bool const isSigned = type.isSigned();

	switch (_operator)
	{
	case Token::ADD:
		m_context << eth::Instruction::ADD;
		break;
	case Token::SUB:
		m_context << eth::Instruction::SUB;
		break;
	case Token::MUL:
		m_context << eth::Instruction::MUL;
		break;
	case Token::DIV:
		m_context  << (isSigned ? eth::Instruction::SDIV : eth::Instruction::DIV);
		break;
	case Token::MOD:
		m_context << (isSigned ? eth::Instruction::SMOD : eth::Instruction::MOD);
		break;
	default:
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Unknown arithmetic operator."));
	}
}

void ExpressionCompiler::appendBitOperatorCode(Token::Value _operator)
{
	switch (_operator)
	{
	case Token::BIT_OR:
		m_context << eth::Instruction::OR;
		break;
	case Token::BIT_AND:
		m_context << eth::Instruction::AND;
		break;
	case Token::BIT_XOR:
		m_context << eth::Instruction::XOR;
		break;
	default:
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Unknown bit operator."));
	}
}

void ExpressionCompiler::appendShiftOperatorCode(Token::Value _operator)
{
	BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Shift operators not yet implemented."));
	switch (_operator)
	{
	case Token::SHL:
		break;
	case Token::SAR:
		break;
	default:
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Unknown shift operator."));
	}
}

void ExpressionCompiler::appendTypeConversion(Type const& _typeOnStack, Type const& _targetType, bool _cleanupNeeded)
{
	// For a type extension, we need to remove all higher-order bits that we might have ignored in
	// previous operations.
	// @todo: store in the AST whether the operand might have "dirty" higher order bits

	if (_typeOnStack == _targetType && !_cleanupNeeded)
		return;
	if (_typeOnStack.getCategory() == Type::Category::INTEGER)
		appendHighBitsCleanup(dynamic_cast<IntegerType const&>(_typeOnStack));
	else if (_typeOnStack != _targetType)
		// All other types should not be convertible to non-equal types.
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Invalid type conversion requested."));
}

void ExpressionCompiler::appendHighBitsCleanup(IntegerType const& _typeOnStack)
{
	if (_typeOnStack.getNumBits() == 256)
		return;
	else if (_typeOnStack.isSigned())
		m_context << u256(_typeOnStack.getNumBits() / 8 - 1) << eth::Instruction::SIGNEXTEND;
	else
		m_context << ((u256(1) << _typeOnStack.getNumBits()) - 1) << eth::Instruction::AND;
}

void ExpressionCompiler::retrieveLValueValue(Expression const& _expression)
{
	switch (m_currentLValue.locationType)
	{
	case LValueLocation::CODE:
		// not stored on the stack
		break;
	case LValueLocation::STACK:
	{
		unsigned stackPos = m_context.baseToCurrentStackOffset(unsigned(m_currentLValue.location));
		if (stackPos >= 15) //@todo correct this by fetching earlier or moving to memory
			BOOST_THROW_EXCEPTION(CompilerError() << errinfo_sourceLocation(_expression.getLocation())
												  << errinfo_comment("Stack too deep."));
		m_context << eth::dupInstruction(stackPos + 1);
		break;
	}
	case LValueLocation::STORAGE:
		m_context << m_currentLValue.location << eth::Instruction::SLOAD;
		break;
	case LValueLocation::MEMORY:
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Location type not yet implemented."));
		break;
	default:
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Unsupported location type."));
		break;
	}
}

void ExpressionCompiler::storeInLValue(Expression const& _expression, bool _move)
{
	switch (m_currentLValue.locationType)
	{
	case LValueLocation::STACK:
	{
		unsigned stackPos = m_context.baseToCurrentStackOffset(unsigned(m_currentLValue.location));
		if (stackPos > 16)
			BOOST_THROW_EXCEPTION(CompilerError() << errinfo_sourceLocation(_expression.getLocation())
												  << errinfo_comment("Stack too deep."));
		else if (stackPos > 0)
			m_context << eth::swapInstruction(stackPos) << eth::Instruction::POP;
		if (!_move)
			retrieveLValueValue(_expression);
		break;
	}
	case LValueLocation::STORAGE:
		if (!_move)
			m_context << eth::Instruction::DUP1;
		m_context << m_currentLValue.location << eth::Instruction::SSTORE;
		break;
	case LValueLocation::CODE:
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Location type does not support assignment."));
		break;
	case LValueLocation::MEMORY:
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Location type not yet implemented."));
		break;
	default:
		BOOST_THROW_EXCEPTION(InternalCompilerError() << errinfo_comment("Unsupported location type."));
		break;
	}
}

}
}
