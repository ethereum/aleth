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
 * @file CommonSubexpressionEliminator.cpp
 * @author Christian <c@ethdev.com>
 * @date 2015
 * Optimizer step for common subexpression elimination and stack reorganisation.
 */

#include <functional>
#include <boost/range/adaptor/reversed.hpp>
#include <libevmcore/CommonSubexpressionEliminator.h>
#include <libevmcore/AssemblyItem.h>

using namespace std;
using namespace dev;
using namespace dev::eth;

vector<AssemblyItem> CommonSubexpressionEliminator::getOptimizedItems()
{
	optimizeBreakingItem();

	map<int, ExpressionClasses::Id> initialStackContents;
	map<int, ExpressionClasses::Id> targetStackContents;
	int minHeight = m_stackHeight + 1;
	if (!m_stackElements.empty())
		minHeight = min(minHeight, m_stackElements.begin()->first);
	for (int height = minHeight; height <= 0; ++height)
		initialStackContents[height] = initialStackElement(height);
	for (int height = minHeight; height <= m_stackHeight; ++height)
		targetStackContents[height] = stackElement(height);

	// Debug info:
	//stream(cout, initialStackContents, targetStackContents);

	AssemblyItems items = CSECodeGenerator(m_expressionClasses, m_storeOperations).generateCode(
		initialStackContents,
		targetStackContents
	);
	if (m_breakingItem)
		items.push_back(*m_breakingItem);
	return items;
}

ostream& CommonSubexpressionEliminator::stream(
	ostream& _out,
	map<int, ExpressionClasses::Id> _initialStack,
	map<int, ExpressionClasses::Id> _targetStack
) const
{
	auto streamExpressionClass = [this](ostream& _out, ExpressionClasses::Id _id)
	{
		auto const& expr = m_expressionClasses.representative(_id);
		_out << "  " << dec << _id << ": " << *expr.item;
		if (expr.sequenceNumber)
			_out << "@" << dec << expr.sequenceNumber;
		_out << "(";
		for (ExpressionClasses::Id arg: expr.arguments)
			_out << dec << arg << ",";
		_out << ")" << endl;
	};

	_out << "Optimizer analysis:" << endl;
	_out << "Final stack height: " << dec << m_stackHeight << endl;
	_out << "Equivalence classes: " << endl;
	for (ExpressionClasses::Id eqClass = 0; eqClass < m_expressionClasses.size(); ++eqClass)
		streamExpressionClass(_out, eqClass);

	_out << "Initial stack: " << endl;
	for (auto const& it: _initialStack)
	{
		_out << "  " << dec << it.first << ": ";
		streamExpressionClass(_out, it.second);
	}
	_out << "Target stack: " << endl;
	for (auto const& it: _targetStack)
	{
		_out << "  " << dec << it.first << ": ";
		streamExpressionClass(_out, it.second);
	}

	return _out;
}

void CommonSubexpressionEliminator::feedItem(AssemblyItem const& _item, bool _copyItem)
{
	if (_item.type() != Operation)
	{
		assertThrow(_item.deposit() == 1, InvalidDeposit, "");
		setStackElement(++m_stackHeight, m_expressionClasses.find(_item, {}, _copyItem));
	}
	else
	{
		Instruction instruction = _item.instruction();
		InstructionInfo info = instructionInfo(instruction);
		if (SemanticInformation::isDupInstruction(_item))
			setStackElement(
				m_stackHeight + 1,
				stackElement(m_stackHeight - int(instruction) + int(Instruction::DUP1))
			);
		else if (SemanticInformation::isSwapInstruction(_item))
			swapStackElements(
				m_stackHeight,
				m_stackHeight - 1 - int(instruction) + int(Instruction::SWAP1)
			);
		else if (instruction != Instruction::POP)
		{
			vector<ExpressionClasses::Id> arguments(info.args);
			for (int i = 0; i < info.args; ++i)
				arguments[i] = stackElement(m_stackHeight - i);
			if (_item.instruction() == Instruction::SSTORE)
				storeInStorage(arguments[0], arguments[1]);
			else if (_item.instruction() == Instruction::SLOAD)
				setStackElement(m_stackHeight + _item.deposit(), loadFromStorage(arguments[0]));
			else if (_item.instruction() == Instruction::MSTORE)
				storeInMemory(arguments[0], arguments[1]);
			else if (_item.instruction() == Instruction::MLOAD)
				setStackElement(m_stackHeight + _item.deposit(), loadFromMemory(arguments[0]));
			else
				setStackElement(m_stackHeight + _item.deposit(), m_expressionClasses.find(_item, arguments, _copyItem));
		}
		m_stackHeight += _item.deposit();
	}
}

void CommonSubexpressionEliminator::optimizeBreakingItem()
{
	if (!m_breakingItem || *m_breakingItem != AssemblyItem(Instruction::JUMPI))
		return;

	using Id = ExpressionClasses::Id;
	static AssemblyItem s_jump = Instruction::JUMP;

	Id condition = stackElement(m_stackHeight - 1);
	Id zero = m_expressionClasses.find(u256(0));
	if (m_expressionClasses.knownToBeDifferent(condition, zero))
	{
		feedItem(Instruction::SWAP1, true);
		feedItem(Instruction::POP, true);
		m_breakingItem = &s_jump;
		return;
	}
	Id negatedCondition = m_expressionClasses.find(Instruction::ISZERO, {condition});
	if (m_expressionClasses.knownToBeDifferent(negatedCondition, zero))
	{
		feedItem(Instruction::POP, true);
		feedItem(Instruction::POP, true);
		m_breakingItem = nullptr;
	}
}

void CommonSubexpressionEliminator::setStackElement(int _stackHeight, ExpressionClasses::Id _class)
{
	m_stackElements[_stackHeight] = _class;
}

void CommonSubexpressionEliminator::swapStackElements(int _stackHeightA, int _stackHeightB)
{
	assertThrow(_stackHeightA != _stackHeightB, OptimizerException, "Swap on same stack elements.");
	// ensure they are created
	stackElement(_stackHeightA);
	stackElement(_stackHeightB);

	swap(m_stackElements[_stackHeightA], m_stackElements[_stackHeightB]);
}

ExpressionClasses::Id CommonSubexpressionEliminator::stackElement(int _stackHeight)
{
	if (m_stackElements.count(_stackHeight))
		return m_stackElements.at(_stackHeight);
	// Stack element not found (not assigned yet), create new equivalence class.
	return m_stackElements[_stackHeight] = initialStackElement(_stackHeight);
}

ExpressionClasses::Id CommonSubexpressionEliminator::initialStackElement(int _stackHeight)
{
	assertThrow(_stackHeight <= 0, OptimizerException, "Initial stack element of positive height requested.");
	assertThrow(_stackHeight > -16, StackTooDeepException, "");
	// This is a special assembly item that refers to elements pre-existing on the initial stack.
	return m_expressionClasses.find(AssemblyItem(dupInstruction(1 - _stackHeight)));
}

void CommonSubexpressionEliminator::storeInStorage(ExpressionClasses::Id _slot, ExpressionClasses::Id _value)
{
	if (m_storageContent.count(_slot) && m_storageContent[_slot] == _value)
		// do not execute the storage if we know that the value is already there
		return;
	m_sequenceNumber++;
	decltype(m_storageContent) storageContents;
	// copy over values at points where we know that they are different from _slot
	for (auto const& storageItem: m_storageContent)
		if (m_expressionClasses.knownToBeDifferent(storageItem.first, _slot))
			storageContents.insert(storageItem);
	m_storageContent = move(storageContents);
	ExpressionClasses::Id id = m_expressionClasses.find(Instruction::SSTORE, {_slot, _value}, true, m_sequenceNumber);
	m_storeOperations.push_back(StoreOperation(StoreOperation::Storage, _slot, m_sequenceNumber, id));
	m_storageContent[_slot] = _value;
	// increment a second time so that we get unique sequence numbers for writes
	m_sequenceNumber++;
}

ExpressionClasses::Id CommonSubexpressionEliminator::loadFromStorage(ExpressionClasses::Id _slot)
{
	if (m_storageContent.count(_slot))
		return m_storageContent.at(_slot);
	else
		return m_storageContent[_slot] = m_expressionClasses.find(Instruction::SLOAD, {_slot}, true, m_sequenceNumber);
}

void CommonSubexpressionEliminator::storeInMemory(ExpressionClasses::Id _slot, ExpressionClasses::Id _value)
{
	if (m_memoryContent.count(_slot) && m_memoryContent[_slot] == _value)
		// do not execute the store if we know that the value is already there
		return;
	m_sequenceNumber++;
	decltype(m_memoryContent) memoryContents;
	// copy over values at points where we know that they are different from _slot by at least 32
	for (auto const& memoryItem: m_memoryContent)
		if (m_expressionClasses.knownToBeDifferentBy32(memoryItem.first, _slot))
			memoryContents.insert(memoryItem);
	m_memoryContent = move(memoryContents);
	ExpressionClasses::Id id = m_expressionClasses.find(Instruction::MSTORE, {_slot, _value}, true, m_sequenceNumber);
	m_storeOperations.push_back(StoreOperation(StoreOperation::Memory, _slot, m_sequenceNumber, id));
	m_memoryContent[_slot] = _value;
	// increment a second time so that we get unique sequence numbers for writes
	m_sequenceNumber++;
}

ExpressionClasses::Id CommonSubexpressionEliminator::loadFromMemory(ExpressionClasses::Id _slot)
{
	if (m_memoryContent.count(_slot))
		return m_memoryContent.at(_slot);
	else
		return m_memoryContent[_slot] = m_expressionClasses.find(Instruction::MLOAD, {_slot}, true, m_sequenceNumber);
}

CSECodeGenerator::CSECodeGenerator(
	ExpressionClasses& _expressionClasses,
	vector<CSECodeGenerator::StoreOperation> const& _storeOperations
):
	m_expressionClasses(_expressionClasses)
{
	for (auto const& store: _storeOperations)
		m_storeOperations[make_pair(store.target, store.slot)].push_back(store);
}

AssemblyItems CSECodeGenerator::generateCode(
	map<int, ExpressionClasses::Id> const& _initialStack,
	map<int, ExpressionClasses::Id> const& _targetStackContents
)
{
	m_stack = _initialStack;
	for (auto const& item: m_stack)
		if (!m_classPositions.count(item.second))
			m_classPositions[item.second] = item.first;

	// @todo: provide information about the positions of copies of class elements

	// generate the dependency graph starting from final storage and memory writes and target stack contents
	for (auto const& p: m_storeOperations)
		addDependencies(p.second.back().expression);
	for (auto const& targetItem: _targetStackContents)
	{
		m_finalClasses.insert(targetItem.second);
		addDependencies(targetItem.second);
	}

	// store all needed sequenced expressions
	set<pair<unsigned, ExpressionClasses::Id>> sequencedExpressions;
	for (auto const& p: m_neededBy)
		for (auto id: {p.first, p.second})
			if (unsigned seqNr = m_expressionClasses.representative(id).sequenceNumber)
				sequencedExpressions.insert(make_pair(seqNr, id));

	// Perform all operations on storage and memory in order, if they are needed.
	for (auto const& seqAndId: sequencedExpressions)
		if (!m_classPositions.count(seqAndId.second))
			generateClassElement(seqAndId.second, true);

	// generate the target stack elements
	for (auto const& targetItem: _targetStackContents)
	{
		int position = generateClassElement(targetItem.second);
		assertThrow(position != c_invalidPosition, OptimizerException, "");
		if (position == targetItem.first)
			continue;
		if (position < targetItem.first)
			// it is already at its target, we need another copy
			appendDup(position);
		else
			appendOrRemoveSwap(position);
		appendOrRemoveSwap(targetItem.first);
	}

	// remove surplus elements
	while (removeStackTopIfPossible())
	{
		// no-op
	}

	// check validity
	int finalHeight = 0;
	if (!_targetStackContents.empty())
		// have target stack, so its height should be the final height
		finalHeight = (--_targetStackContents.end())->first;
	else if (!_initialStack.empty())
		// no target stack, only erase the initial stack
		finalHeight = _initialStack.begin()->first - 1;
	else
		// neither initial no target stack, no change in height
		finalHeight = 0;
	assertThrow(finalHeight == m_stackHeight, OptimizerException, "Incorrect final stack height.");
	return m_generatedItems;
}

void CSECodeGenerator::addDependencies(ExpressionClasses::Id _c)
{
	if (m_neededBy.count(_c))
		return; // we already computed the dependencies for _c
	ExpressionClasses::Expression expr = m_expressionClasses.representative(_c);
	for (ExpressionClasses::Id argument: expr.arguments)
	{
		addDependencies(argument);
		m_neededBy.insert(make_pair(argument, _c));
	}
	if (expr.item->type() == Operation && (
		expr.item->instruction() == Instruction::SLOAD ||
		expr.item->instruction() == Instruction::MLOAD
	))
	{
		// this loads an unknown value from storage or memory and thus, in addition to its
		// arguments, depends on all store operations to addresses where we do not know that
		// they are different that occur before this load
		StoreOperation::Target target = expr.item->instruction() == Instruction::SLOAD ?
			StoreOperation::Storage : StoreOperation::Memory;
		ExpressionClasses::Id slotToLoadFrom = expr.arguments.at(0);
		for (auto const& p: m_storeOperations)
		{
			if (p.first.first != target)
				continue;
			ExpressionClasses::Id slot = p.first.second;
			StoreOperations const& storeOps = p.second;
			if (storeOps.front().sequenceNumber > expr.sequenceNumber)
				continue;
			if (
				(target == StoreOperation::Memory && m_expressionClasses.knownToBeDifferentBy32(slot, slotToLoadFrom)) ||
				(target == StoreOperation::Storage && m_expressionClasses.knownToBeDifferent(slot, slotToLoadFrom))
			)
				continue;
			// note that store and load never have the same sequence number
			ExpressionClasses::Id latestStore = storeOps.front().expression;
			for (auto it = ++storeOps.begin(); it != storeOps.end(); ++it)
				if (it->sequenceNumber < expr.sequenceNumber)
					latestStore = it->expression;
			addDependencies(latestStore);
			m_neededBy.insert(make_pair(latestStore, _c));
		}
	}
}

int CSECodeGenerator::generateClassElement(ExpressionClasses::Id _c, bool _allowSequenced)
{
	// do some cleanup
	removeStackTopIfPossible();

	if (m_classPositions.count(_c))
	{
		assertThrow(
			m_classPositions[_c] != c_invalidPosition,
			OptimizerException,
			"Element already removed but still needed."
		);
		return m_classPositions[_c];
	}
	ExpressionClasses::Expression const& expr = m_expressionClasses.representative(_c);
	assertThrow(
		_allowSequenced || expr.sequenceNumber == 0,
		OptimizerException,
		"Sequence constrained operation requested out of sequence."
	);
	ExpressionClasses::Ids const& arguments = expr.arguments;
	for (ExpressionClasses::Id arg: boost::adaptors::reverse(arguments))
		generateClassElement(arg);

	// The arguments are somewhere on the stack now, so it remains to move them at the correct place.
	// This is quite difficult as sometimes, the values also have to removed in this process
	// (if canBeRemoved() returns true) and the two arguments can be equal. For now, this is
	// implemented for every single case for combinations of up to two arguments manually.
	if (arguments.size() == 1)
	{
		if (canBeRemoved(arguments[0], _c))
			appendOrRemoveSwap(classElementPosition(arguments[0]));
		else
			appendDup(classElementPosition(arguments[0]));
	}
	else if (arguments.size() == 2)
	{
		if (canBeRemoved(arguments[1], _c))
		{
			appendOrRemoveSwap(classElementPosition(arguments[1]));
			if (arguments[0] == arguments[1])
				appendDup(m_stackHeight);
			else if (canBeRemoved(arguments[0], _c))
			{
				appendOrRemoveSwap(m_stackHeight - 1);
				appendOrRemoveSwap(classElementPosition(arguments[0]));
			}
			else
				appendDup(classElementPosition(arguments[0]));
		}
		else
		{
			if (arguments[0] == arguments[1])
			{
				appendDup(classElementPosition(arguments[0]));
				appendDup(m_stackHeight);
			}
			else if (canBeRemoved(arguments[0], _c))
			{
				appendOrRemoveSwap(classElementPosition(arguments[0]));
				appendDup(classElementPosition(arguments[1]));
				appendOrRemoveSwap(m_stackHeight - 1);
			}
			else
			{
				appendDup(classElementPosition(arguments[1]));
				appendDup(classElementPosition(arguments[0]));
			}
		}
	}
	else
		assertThrow(
			arguments.size() <= 2,
			OptimizerException,
			"Opcodes with more than two arguments not implemented yet."
		);
	for (size_t i = 0; i < arguments.size(); ++i)
		assertThrow(m_stack[m_stackHeight - i] == arguments[i], OptimizerException, "Expected arguments not present." );

	while (SemanticInformation::isCommutativeOperation(*expr.item) &&
			!m_generatedItems.empty() &&
			m_generatedItems.back() == AssemblyItem(Instruction::SWAP1))
		// this will not append a swap but remove the one that is already there
		appendOrRemoveSwap(m_stackHeight - 1);
	for (auto arg: arguments)
		if (canBeRemoved(arg, _c))
			m_classPositions[arg] = c_invalidPosition;
	for (size_t i = 0; i < arguments.size(); ++i)
		m_stack.erase(m_stackHeight - i);
	appendItem(*expr.item);
	if (expr.item->type() != Operation || instructionInfo(expr.item->instruction()).ret == 1)
	{
		m_stack[m_stackHeight] = _c;
		return m_classPositions[_c] = m_stackHeight;
	}
	else
	{
		assertThrow(
			instructionInfo(expr.item->instruction()).ret == 0,
			OptimizerException,
			"Invalid number of return values."
		);
		return m_classPositions[_c] = c_invalidPosition;
	}
}

int CSECodeGenerator::classElementPosition(ExpressionClasses::Id _id) const
{
	assertThrow(
		m_classPositions.count(_id) && m_classPositions.at(_id) != c_invalidPosition,
		OptimizerException,
		"Element requested but is not present."
	);
	return m_classPositions.at(_id);
}

bool CSECodeGenerator::canBeRemoved(ExpressionClasses::Id _element, ExpressionClasses::Id _result)
{
	// Returns false if _element is finally needed or is needed by a class that has not been
	// computed yet. Note that m_classPositions also includes classes that were deleted in the meantime.
	if (m_finalClasses.count(_element))
		return false;

	auto range = m_neededBy.equal_range(_element);
	for (auto it = range.first; it != range.second; ++it)
		if (it->second != _result && !m_classPositions.count(it->second))
			return false;
	return true;
}

bool CSECodeGenerator::removeStackTopIfPossible()
{
	if (m_stack.empty())
		return false;
	assertThrow(m_stack.count(m_stackHeight), OptimizerException, "");
	ExpressionClasses::Id top = m_stack[m_stackHeight];
	if (!canBeRemoved(top))
		return false;
	m_generatedItems.push_back(AssemblyItem(Instruction::POP));
	m_stack.erase(m_stackHeight);
	m_stackHeight--;
	return true;
}

void CSECodeGenerator::appendDup(int _fromPosition)
{
	assertThrow(_fromPosition != c_invalidPosition, OptimizerException, "");
	int instructionNum = 1 + m_stackHeight - _fromPosition;
	assertThrow(instructionNum <= 16, StackTooDeepException, "Stack too deep.");
	assertThrow(1 <= instructionNum, OptimizerException, "Invalid stack access.");
	appendItem(AssemblyItem(dupInstruction(instructionNum)));
	m_stack[m_stackHeight] = m_stack[_fromPosition];
}

void CSECodeGenerator::appendOrRemoveSwap(int _fromPosition)
{
	assertThrow(_fromPosition != c_invalidPosition, OptimizerException, "");
	if (_fromPosition == m_stackHeight)
		return;
	int instructionNum = m_stackHeight - _fromPosition;
	assertThrow(instructionNum <= 16, StackTooDeepException, "Stack too deep.");
	assertThrow(1 <= instructionNum, OptimizerException, "Invalid stack access.");
	appendItem(AssemblyItem(swapInstruction(instructionNum)));
	// The value of a class can be present in multiple locations on the stack. We only update the
	// "canonical" one that is tracked by m_classPositions
	if (m_classPositions[m_stack[m_stackHeight]] == m_stackHeight)
		m_classPositions[m_stack[m_stackHeight]] = _fromPosition;
	if (m_classPositions[m_stack[_fromPosition]] == _fromPosition)
		m_classPositions[m_stack[_fromPosition]] = m_stackHeight;
	swap(m_stack[m_stackHeight], m_stack[_fromPosition]);
	if (m_generatedItems.size() >= 2 &&
		SemanticInformation::isSwapInstruction(m_generatedItems.back()) &&
		*(m_generatedItems.end() - 2) == m_generatedItems.back())
	{
		m_generatedItems.pop_back();
		m_generatedItems.pop_back();
	}
}

void CSECodeGenerator::appendItem(AssemblyItem const& _item)
{
	m_generatedItems.push_back(_item);
	m_stackHeight += _item.deposit();
}
