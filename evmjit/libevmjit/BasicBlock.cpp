#include "BasicBlock.h"

#include <iostream>

#include "preprocessor/llvm_includes_start.h"
#include <llvm/IR/CFG.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/raw_os_ostream.h>
#include "preprocessor/llvm_includes_end.h"

#include "Type.h"
#include "Utils.h"

namespace dev
{
namespace eth
{
namespace jit
{

BasicBlock::BasicBlock(instr_idx _firstInstrIdx, code_iterator _begin, code_iterator _end, llvm::Function* _mainFunc):
	m_firstInstrIdx{_firstInstrIdx},
	m_begin(_begin),
	m_end(_end),
	m_llvmBB(llvm::BasicBlock::Create(_mainFunc->getContext(), {".", std::to_string(_firstInstrIdx)}, _mainFunc))
{}

LocalStack::LocalStack(RuntimeManager& _runtimeManager, Stack& _globalStack):
	m_global(_globalStack)
{
	m_sp = _runtimeManager.prepareStack();
}

void LocalStack::push(llvm::Value* _value)
{
	assert(_value->getType() == Type::Word);
	m_local.push_back(_value);
	m_maxSize = std::max(m_maxSize, size());
}

llvm::Value* LocalStack::pop()
{
	auto item = get(0);
	assert(!m_local.empty() || !m_input.empty());

	if (m_local.size() > 0)
		m_local.pop_back();
	else
		++m_globalPops;

	m_minSize = std::min(m_minSize, size());
	return item;
}

/**
 *  Pushes a copy of _index-th element (tos is 0-th elem).
 */
void LocalStack::dup(size_t _index)
{
	auto val = get(_index);
	push(val);
}

/**
 *  Swaps tos with _index-th element (tos is 0-th elem).
 *  _index must be > 0.
 */
void LocalStack::swap(size_t _index)
{
	assert(_index > 0);
	auto val = get(_index);
	auto tos = get(0);
	set(_index, tos);
	set(0, val);
}

llvm::Value* LocalStack::get(size_t _index)
{
	if (_index < m_local.size())
		return *(m_local.rbegin() + _index); // count from back

	auto idx = _index - m_local.size() + m_globalPops;
	if (idx >= m_input.size())
		m_input.resize(idx + 1);
	auto& item = m_input[idx];

	if (!item)
	{
		// Fetch an item from global stack
		auto& builder = m_global.getBuilder();
		ssize_t globalIdx = -idx - 1;
		auto slot = builder.CreateConstGEP1_64(m_sp, globalIdx);
		item = builder.CreateLoad(slot);
		m_minSize = std::min(m_minSize, globalIdx); 	// remember required stack size
	}

	return item;
}

void LocalStack::set(size_t _index, llvm::Value* _word)
{
	if (_index < m_local.size())
	{
		*(m_local.rbegin() + _index) = _word;
		return;
	}

	auto idx = _index - m_local.size() + m_globalPops;
	assert(idx < m_input.size());
	m_input[idx] = _word;
}


void LocalStack::finalize(IRBuilder& _builder, llvm::BasicBlock& _bb)
{
	m_sp->setArgOperand(2, _builder.getInt64(minSize()));
	m_sp->setArgOperand(3, _builder.getInt64(maxSize()));
	m_sp->setArgOperand(4, _builder.getInt64(size()));

	auto blockTerminator = _bb.getTerminator();
	if (!blockTerminator || blockTerminator->getOpcode() != llvm::Instruction::Ret) // TODO: Always exit through exit block
	{
		// Not needed in case of ret instruction. Ret invalidates the stack.
		if (blockTerminator)
			_builder.SetInsertPoint(blockTerminator);
		else
			_builder.SetInsertPoint(&_bb);

		auto inputIt = m_input.rbegin();
		auto localIt = m_local.begin();
		for (ssize_t globalIdx = -m_input.size(); globalIdx < size(); ++globalIdx)
		{
			llvm::Value* item = nullptr;
			if (globalIdx < -m_globalPops)
			{
				item = *inputIt++;	// update input items (might contain original value)
				if (!item)			// some items are skipped
					continue;
			}
			else
				item = *localIt++;	// store new items

			auto slot = _builder.CreateConstGEP1_64(m_sp, globalIdx);
			_builder.CreateStore(item, slot); // FIXME: Set alignment
		}
	}
}


}
}
}
