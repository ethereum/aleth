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
 * Solidity compiler.
 */

#include <algorithm>
#include <libevmcore/Instruction.h>
#include <libevmcore/Assembly.h>
#include <libsolidity/AST.h>
#include <libsolidity/Compiler.h>
#include <libsolidity/ExpressionCompiler.h>
#include <libsolidity/CompilerUtils.h>
#include <libsolidity/CallGraph.h>

using namespace std;

namespace dev {
namespace solidity {

void Compiler::compileContract(ContractDefinition const& _contract, vector<MagicVariableDeclaration const*> const& _magicGlobals,
							   map<ContractDefinition const*, bytes const*> const& _contracts)
{
	m_context = CompilerContext(); // clear it just in case
	initializeContext(_contract, _magicGlobals, _contracts);

	for (ASTPointer<FunctionDefinition> const& function: _contract.getDefinedFunctions())
		if (function->getName() != _contract.getName()) // don't add the constructor here
			m_context.addFunction(*function);

	appendFunctionSelector(_contract);
	for (ASTPointer<FunctionDefinition> const& function: _contract.getDefinedFunctions())
		if (function->getName() != _contract.getName()) // don't add the constructor here
			function->accept(*this);

	// Swap the runtime context with the creation-time context
	CompilerContext runtimeContext;
	swap(m_context, runtimeContext);
	initializeContext(_contract, _magicGlobals, _contracts);
	packIntoContractCreator(_contract, runtimeContext);
}

void Compiler::initializeContext(ContractDefinition const& _contract, vector<MagicVariableDeclaration const*> const& _magicGlobals,
								 map<ContractDefinition const*, bytes const*> const& _contracts)
{
	m_context.setCompiledContracts(_contracts);
	for (MagicVariableDeclaration const* variable: _magicGlobals)
		m_context.addMagicGlobal(*variable);
	registerStateVariables(_contract);
}

void Compiler::packIntoContractCreator(ContractDefinition const& _contract, CompilerContext const& _runtimeContext)
{
	set<FunctionDefinition const*> neededFunctions;
	FunctionDefinition const* constructor = _contract.getConstructor();
	if (constructor)
		neededFunctions = getFunctionsNeededByConstructor(*constructor);

	for (FunctionDefinition const* fun: neededFunctions)
		m_context.addFunction(*fun);

	if (constructor)
		appendConstructorCall(*constructor);

	eth::AssemblyItem sub = m_context.addSubroutine(_runtimeContext.getAssembly());
	// stack contains sub size
	m_context << eth::Instruction::DUP1 << sub << u256(0) << eth::Instruction::CODECOPY;
	m_context << u256(0) << eth::Instruction::RETURN;

	// note that we have to explicitly include all used functions because of absolute jump
	// labels
	for (FunctionDefinition const* fun: neededFunctions)
		fun->accept(*this);
}

void Compiler::appendConstructorCall(FunctionDefinition const& _constructor)
{
	eth::AssemblyItem returnTag = m_context.pushNewTag();
	// copy constructor arguments from code to memory and then to stack, they are supplied after the actual program
	unsigned argumentSize = 0;
	for (ASTPointer<VariableDeclaration> const& var: _constructor.getParameters())
		argumentSize += var->getType()->getCalldataEncodedSize();
	if (argumentSize > 0)
	{
		m_context << u256(argumentSize);
		m_context.appendProgramSize();
		m_context << u256(1); // copy it to byte one as expected for ABI calls
		m_context << eth::Instruction::CODECOPY;
		appendCalldataUnpacker(_constructor, true);
	}
	m_context.appendJumpTo(m_context.getFunctionEntryLabel(_constructor));
	m_context << returnTag;
}

set<FunctionDefinition const*> Compiler::getFunctionsNeededByConstructor(FunctionDefinition const& _constructor)
{
	CallGraph callgraph;
	callgraph.addFunction(_constructor);
	callgraph.computeCallGraph();
	return callgraph.getCalls();
}

void Compiler::appendFunctionSelector(ContractDefinition const& _contract)
{
	vector<FunctionDefinition const*> interfaceFunctions = _contract.getInterfaceFunctions();
	vector<eth::AssemblyItem> callDataUnpackerEntryPoints;

	if (interfaceFunctions.size() > 255)
		BOOST_THROW_EXCEPTION(CompilerError() << errinfo_comment("More than 255 public functions for contract."));

	// retrieve the first byte of the call data, which determines the called function
	// @todo This code had a jump table in a previous version which was more efficient but also
	// error prone (due to the optimizer and variable length tag addresses)
	m_context << u256(1) << u256(0) // some constants
			  << eth::dupInstruction(1) << eth::Instruction::CALLDATALOAD
			  << eth::dupInstruction(2) << eth::Instruction::BYTE
			  << eth::dupInstruction(2);

	// stack here: 1 0 <funid> 0, stack top will be counted up until it matches funid
	for (unsigned funid = 0; funid < interfaceFunctions.size(); ++funid)
	{
		callDataUnpackerEntryPoints.push_back(m_context.newTag());
		m_context << eth::dupInstruction(2) << eth::dupInstruction(2) << eth::Instruction::EQ;
		m_context.appendConditionalJumpTo(callDataUnpackerEntryPoints.back());
		if (funid < interfaceFunctions.size() - 1)
			m_context << eth::dupInstruction(4) << eth::Instruction::ADD;
	}
	m_context << eth::Instruction::STOP; // function not found

	for (unsigned funid = 0; funid < interfaceFunctions.size(); ++funid)
	{
		FunctionDefinition const& function = *interfaceFunctions[funid];
		m_context << callDataUnpackerEntryPoints[funid];
		eth::AssemblyItem returnTag = m_context.pushNewTag();
		appendCalldataUnpacker(function);
		m_context.appendJumpTo(m_context.getFunctionEntryLabel(function));
		m_context << returnTag;
		appendReturnValuePacker(function);
	}
}

unsigned Compiler::appendCalldataUnpacker(FunctionDefinition const& _function, bool _fromMemory)
{
	// We do not check the calldata size, everything is zero-padded.
	unsigned dataOffset = 1;
	//@todo this can be done more efficiently, saving some CALLDATALOAD calls
	for (ASTPointer<VariableDeclaration> const& var: _function.getParameters())
	{
		unsigned const numBytes = var->getType()->getCalldataEncodedSize();
		if (numBytes > 32)
			BOOST_THROW_EXCEPTION(CompilerError()
								  << errinfo_sourceLocation(var->getLocation())
								  << errinfo_comment("Type " + var->getType()->toString() + " not yet supported."));
		bool leftAligned = var->getType()->getCategory() == Type::Category::STRING;
		CompilerUtils(m_context).loadFromMemory(dataOffset, numBytes, leftAligned, !_fromMemory);
		dataOffset += numBytes;
	}
	return dataOffset;
}

void Compiler::appendReturnValuePacker(FunctionDefinition const& _function)
{
	//@todo this can be also done more efficiently
	unsigned dataOffset = 0;
	vector<ASTPointer<VariableDeclaration>> const& parameters = _function.getReturnParameters();
	unsigned stackDepth = CompilerUtils(m_context).getSizeOnStack(parameters);
	for (unsigned i = 0; i < parameters.size(); ++i)
	{
		Type const& paramType = *parameters[i]->getType();
		unsigned numBytes = paramType.getCalldataEncodedSize();
		if (numBytes > 32)
			BOOST_THROW_EXCEPTION(CompilerError()
								  << errinfo_sourceLocation(parameters[i]->getLocation())
								  << errinfo_comment("Type " + paramType.toString() + " not yet supported."));
		CompilerUtils(m_context).copyToStackTop(stackDepth, paramType);
		bool const leftAligned = paramType.getCategory() == Type::Category::STRING;
		CompilerUtils(m_context).storeInMemory(dataOffset, numBytes, leftAligned);
		stackDepth -= paramType.getSizeOnStack();
		dataOffset += numBytes;
	}
	// note that the stack is not cleaned up here
	m_context << u256(dataOffset) << u256(0) << eth::Instruction::RETURN;
}

void Compiler::registerStateVariables(ContractDefinition const& _contract)
{
	//@todo sort them?
	for (ASTPointer<VariableDeclaration> const& variable: _contract.getStateVariables())
		m_context.addStateVariable(*variable);
}

bool Compiler::visit(FunctionDefinition const& _function)
{
	//@todo to simplify this, the calling convention could by changed such that
	// caller puts: [retarg0] ... [retargm] [return address] [arg0] ... [argn]
	// although note that this reduces the size of the visible stack

	m_context.startNewFunction();
	m_returnTag = m_context.newTag();
	m_breakTags.clear();
	m_continueTags.clear();

	m_context << m_context.getFunctionEntryLabel(_function);

	// stack upon entry: [return address] [arg0] [arg1] ... [argn]
	// reserve additional slots: [retarg0] ... [retargm] [localvar0] ... [localvarp]

	for (ASTPointer<VariableDeclaration const> const& variable: _function.getParameters())
		m_context.addVariable(*variable);
	for (ASTPointer<VariableDeclaration const> const& variable: _function.getReturnParameters())
		m_context.addAndInitializeVariable(*variable);
	for (VariableDeclaration const* localVariable: _function.getLocalVariables())
		m_context.addAndInitializeVariable(*localVariable);

	_function.getBody().accept(*this);

	m_context << m_returnTag;

	// Now we need to re-shuffle the stack. For this we keep a record of the stack layout
	// that shows the target positions of the elements, where "-1" denotes that this element needs
	// to be removed from the stack.
	// Note that the fact that the return arguments are of increasing index is vital for this
	// algorithm to work.

	unsigned const argumentsSize = CompilerUtils::getSizeOnStack(_function.getParameters());
	unsigned const returnValuesSize = CompilerUtils::getSizeOnStack(_function.getReturnParameters());
	unsigned const localVariablesSize = CompilerUtils::getSizeOnStack(_function.getLocalVariables());

	vector<int> stackLayout;
	stackLayout.push_back(returnValuesSize); // target of return address
	stackLayout += vector<int>(argumentsSize, -1); // discard all arguments
	for (unsigned i = 0; i < returnValuesSize; ++i)
		stackLayout.push_back(i);
	stackLayout += vector<int>(localVariablesSize, -1);

	while (stackLayout.back() != int(stackLayout.size() - 1))
		if (stackLayout.back() < 0)
		{
			m_context << eth::Instruction::POP;
			stackLayout.pop_back();
		}
		else
		{
			m_context << eth::swapInstruction(stackLayout.size() - stackLayout.back() - 1);
			swap(stackLayout[stackLayout.back()], stackLayout.back());
		}
	//@todo assert that everything is in place now

	m_context << eth::Instruction::JUMP;

	return false;
}

bool Compiler::visit(IfStatement const& _ifStatement)
{
	compileExpression(_ifStatement.getCondition());
	eth::AssemblyItem trueTag = m_context.appendConditionalJump();
	if (_ifStatement.getFalseStatement())
		_ifStatement.getFalseStatement()->accept(*this);
	eth::AssemblyItem endTag = m_context.appendJumpToNew();
	m_context << trueTag;
	_ifStatement.getTrueStatement().accept(*this);
	m_context << endTag;
	return false;
}

bool Compiler::visit(WhileStatement const& _whileStatement)
{
	eth::AssemblyItem loopStart = m_context.newTag();
	eth::AssemblyItem loopEnd = m_context.newTag();
	m_continueTags.push_back(loopStart);
	m_breakTags.push_back(loopEnd);

	m_context << loopStart;
	compileExpression(_whileStatement.getCondition());
	m_context << eth::Instruction::ISZERO;
	m_context.appendConditionalJumpTo(loopEnd);

	_whileStatement.getBody().accept(*this);

	m_context.appendJumpTo(loopStart);
	m_context << loopEnd;

	m_continueTags.pop_back();
	m_breakTags.pop_back();
	return false;
}

bool Compiler::visit(ForStatement const& _forStatement)
{
	eth::AssemblyItem loopStart = m_context.newTag();
	eth::AssemblyItem loopEnd = m_context.newTag();
	m_continueTags.push_back(loopStart);
	m_breakTags.push_back(loopEnd);

	if (_forStatement.getInitializationExpression())
		_forStatement.getInitializationExpression()->accept(*this);

	m_context << loopStart;

	// if there is no terminating condition in for, default is to always be true
	if (_forStatement.getCondition())
	{
		compileExpression(*_forStatement.getCondition());
		m_context << eth::Instruction::ISZERO;
		m_context.appendConditionalJumpTo(loopEnd);
	}

	_forStatement.getBody().accept(*this);

	// for's loop expression if existing
	if (_forStatement.getLoopExpression())
		_forStatement.getLoopExpression()->accept(*this);

	m_context.appendJumpTo(loopStart);
	m_context << loopEnd;

	m_continueTags.pop_back();
	m_breakTags.pop_back();
	return false;
}

bool Compiler::visit(Continue const&)
{
	if (!m_continueTags.empty())
		m_context.appendJumpTo(m_continueTags.back());
	return false;
}

bool Compiler::visit(Break const&)
{
	if (!m_breakTags.empty())
		m_context.appendJumpTo(m_breakTags.back());
	return false;
}

bool Compiler::visit(Return const& _return)
{
	//@todo modifications are needed to make this work with functions returning multiple values
	if (Expression const* expression = _return.getExpression())
	{
		compileExpression(*expression);
		VariableDeclaration const& firstVariable = *_return.getFunctionReturnParameters().getParameters().front();
		ExpressionCompiler::appendTypeConversion(m_context, *expression->getType(), *firstVariable.getType());

		CompilerUtils(m_context).moveToStackVariable(firstVariable);
	}
	m_context.appendJumpTo(m_returnTag);
	return false;
}

bool Compiler::visit(VariableDefinition const& _variableDefinition)
{
	if (Expression const* expression = _variableDefinition.getExpression())
	{
		compileExpression(*expression);
		ExpressionCompiler::appendTypeConversion(m_context,
												 *expression->getType(),
												 *_variableDefinition.getDeclaration().getType());
		CompilerUtils(m_context).moveToStackVariable(_variableDefinition.getDeclaration());
	}
	return false;
}

bool Compiler::visit(ExpressionStatement const& _expressionStatement)
{
	Expression const& expression = _expressionStatement.getExpression();
	compileExpression(expression);
	CompilerUtils(m_context).popStackElement(*expression.getType());
	return false;
}

void Compiler::compileExpression(Expression const& _expression)
{
	ExpressionCompiler::compileExpression(m_context, _expression, m_optimize);
}

}
}
