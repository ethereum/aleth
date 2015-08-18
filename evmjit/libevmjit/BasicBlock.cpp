#include "BasicBlock.h"

#include <iostream>

#include "preprocessor/llvm_includes_start.h"
#include <llvm/IR/CFG.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/raw_os_ostream.h>
#include "preprocessor/llvm_includes_end.h"

#include "RuntimeManager.h"
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

LocalStack::LocalStack(IRBuilder& _builder, RuntimeManager& _runtimeManager):
	CompilerHelper(_builder)
{
	// Call stack.prepare. min, max, size args will be filled up in finalize().
	auto undef = llvm::UndefValue::get(Type::Size);
	m_sp = m_builder.CreateCall(getStackPrepareFunc(),
			{_runtimeManager.getStackBase(), _runtimeManager.getStackSize(), undef, undef, undef, _runtimeManager.getJmpBuf()},
			{"sp", m_builder.GetInsertBlock()->getName()});
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
		ssize_t globalIdx = -idx - 1;
		auto slot = m_builder.CreateConstGEP1_64(m_sp, globalIdx);
		item = m_builder.CreateAlignedLoad(slot, 16); // TODO: Handle malloc alignment. Also for 32-bit systems.
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


void LocalStack::finalize()
{
	m_sp->setArgOperand(2, m_builder.getInt64(minSize()));
	m_sp->setArgOperand(3, m_builder.getInt64(maxSize()));
	m_sp->setArgOperand(4, m_builder.getInt64(size()));

	auto blockTerminator = m_builder.GetInsertBlock()->getTerminator();
	if (!blockTerminator || blockTerminator->getOpcode() != llvm::Instruction::Ret) // TODO: Always exit through exit block
	{
		// Not needed in case of ret instruction. Ret invalidates the stack.
		if (blockTerminator)
			m_builder.SetInsertPoint(blockTerminator); // Insert before terminator

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

			auto slot = m_builder.CreateConstGEP1_64(m_sp, globalIdx);
			m_builder.CreateAlignedStore(item, slot, 16); // TODO: Handle malloc alignment. Also for 32-bit systems.
		}
	}
}


llvm::Function* LocalStack::getStackPrepareFunc()
{
	static const auto c_funcName = "stack.prepare";
	if (auto func = getModule()->getFunction(c_funcName))
		return func;

	llvm::Type* argsTys[] = {Type::WordPtr, Type::Size->getPointerTo(), Type::Size, Type::Size, Type::Size, Type::BytePtr};
	auto func = llvm::Function::Create(llvm::FunctionType::get(Type::WordPtr, argsTys, false), llvm::Function::PrivateLinkage, c_funcName, getModule());
	func->setDoesNotThrow();
	func->setDoesNotAccessMemory(1);
	func->setDoesNotAlias(2);
	func->setDoesNotCapture(2);

	auto checkBB = llvm::BasicBlock::Create(func->getContext(), "Check", func);
	auto updateBB = llvm::BasicBlock::Create(func->getContext(), "Update", func);
	auto outOfStackBB = llvm::BasicBlock::Create(func->getContext(), "OutOfStack", func);

	auto base = &func->getArgumentList().front();
	base->setName("base");
	auto sizePtr = base->getNextNode();
	sizePtr->setName("size.ptr");
	auto min = sizePtr->getNextNode();
	min->setName("min");
	auto max = min->getNextNode();
	max->setName("max");
	auto diff = max->getNextNode();
	diff->setName("diff");
	auto jmpBuf = diff->getNextNode();
	jmpBuf->setName("jmpBuf");

	InsertPointGuard guard{m_builder};
	m_builder.SetInsertPoint(checkBB);
	auto sizeAlignment = getModule()->getDataLayout().getABITypeAlignment(Type::Size);
	auto size = m_builder.CreateAlignedLoad(sizePtr, sizeAlignment, "size");
	auto minSize = m_builder.CreateAdd(size, min, "size.min", false, true);
	auto maxSize = m_builder.CreateAdd(size, max, "size.max", true, true);
	auto minOk = m_builder.CreateICmpSGE(minSize, m_builder.getInt64(0), "ok.min");
	auto maxOk = m_builder.CreateICmpULE(maxSize, m_builder.getInt64(1024), "ok.max"); // FIXME: Extract constants
	auto ok = m_builder.CreateAnd(minOk, maxOk, "ok");
	m_builder.CreateCondBr(ok, updateBB, outOfStackBB, Type::expectTrue);

	m_builder.SetInsertPoint(updateBB);
	auto newSize = m_builder.CreateNSWAdd(size, diff, "size.next");
	m_builder.CreateAlignedStore(newSize, sizePtr, sizeAlignment);
	auto sp = m_builder.CreateGEP(base, size, "sp");
	m_builder.CreateRet(sp);

	m_builder.SetInsertPoint(outOfStackBB);
	auto longjmp = llvm::Intrinsic::getDeclaration(getModule(), llvm::Intrinsic::eh_sjlj_longjmp);
	m_builder.CreateCall(longjmp, {jmpBuf});
	m_builder.CreateUnreachable();

	return func;
}


}
}
}
