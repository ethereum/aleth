#include "RuntimeManager.h"

#include "preprocessor/llvm_includes_start.h"
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Module.h>
#include "preprocessor/llvm_includes_end.h"

#include "Array.h"
#include "Utils.h"

namespace dev
{
namespace eth
{
namespace jit
{

llvm::StructType* RuntimeManager::getRuntimeDataType()
{
	static llvm::StructType* type = nullptr;
	if (!type)
	{
		llvm::Type* elems[] =
		{
			Type::Size,		// gas
			Type::Size,		// gasPrice
			Type::BytePtr,	// callData
			Type::Size,		// callDataSize
			Type::Word,		// address
			Type::Word,		// caller
			Type::Word,		// origin
			Type::Word,		// callValue
			Type::Word,		// coinBase
			Type::Word,		// difficulty
			Type::Word,		// gasLimit
			Type::Size,		// blockNumber
			Type::Size,		// blockTimestamp
			Type::BytePtr,	// code
			Type::Size,		// codeSize
		};
		type = llvm::StructType::create(elems, "RuntimeData");
	}
	return type;
}

llvm::StructType* RuntimeManager::getRuntimeType()
{
	static llvm::StructType* type = nullptr;
	if (!type)
	{
		llvm::Type* elems[] =
		{
			Type::RuntimeDataPtr,	// data
			Type::EnvPtr,			// Env*
			Array::getType()		// memory
		};
		type = llvm::StructType::create(elems, "Runtime");
	}
	return type;
}

namespace
{
llvm::Twine getName(RuntimeData::Index _index)
{
	switch (_index)
	{
	default:						return "";
	case RuntimeData::Gas:			return "msg.gas";
	case RuntimeData::GasPrice:		return "tx.gasprice";
	case RuntimeData::CallData:		return "msg.data.ptr";
	case RuntimeData::CallDataSize:	return "msg.data.size";
	case RuntimeData::Address:		return "this.address";
	case RuntimeData::Caller:		return "msg.caller";
	case RuntimeData::Origin:		return "tx.origin";
	case RuntimeData::CallValue:	return "msg.value";
	case RuntimeData::CoinBase:		return "block.coinbase";
	case RuntimeData::Difficulty:	return "block.difficulty";
	case RuntimeData::GasLimit:		return "block.gaslimit";
	case RuntimeData::Number:		return "block.number";
	case RuntimeData::Timestamp:	return "block.timestamp";
	case RuntimeData::Code:			return "code.ptr";
	case RuntimeData::CodeSize:		return "code.size";
	}
}
}

RuntimeManager::RuntimeManager(IRBuilder& _builder, code_iterator _codeBegin, code_iterator _codeEnd):
	CompilerHelper(_builder),
	m_codeBegin(_codeBegin),
	m_codeEnd(_codeEnd)
{
	m_longjmp = llvm::Intrinsic::getDeclaration(getModule(), llvm::Intrinsic::eh_sjlj_longjmp);

	// Unpack data
	auto rtPtr = getRuntimePtr();
	m_dataPtr = m_builder.CreateLoad(m_builder.CreateStructGEP(getRuntimeType(), rtPtr, 0), "dataPtr");
	assert(m_dataPtr->getType() == Type::RuntimeDataPtr);
	m_memPtr = m_builder.CreateStructGEP(getRuntimeType(), rtPtr, 2, "mem");
	assert(m_memPtr->getType() == Array::getType()->getPointerTo());
	m_envPtr = m_builder.CreateLoad(m_builder.CreateStructGEP(getRuntimeType(), rtPtr, 1), "env");
	assert(m_envPtr->getType() == Type::EnvPtr);

	auto mallocFunc = llvm::Function::Create(llvm::FunctionType::get(Type::WordPtr, {Type::Size}, false), llvm::Function::ExternalLinkage, "malloc", getModule());
	mallocFunc->setDoesNotThrow();
	mallocFunc->setDoesNotAlias(0);

	m_stackBase = m_builder.CreateCall(mallocFunc, m_builder.getInt64(Type::Word->getPrimitiveSizeInBits() / 8 * stackSizeLimit), "stack.base"); // TODO: Use Type::SizeT type
	m_stackSize = m_builder.CreateAlloca(Type::Size, nullptr, "stack.size");
	m_builder.CreateStore(m_builder.getInt64(0), m_stackSize);

	auto data = m_builder.CreateLoad(m_dataPtr, "data");
	for (unsigned i = 0; i < m_dataElts.size(); ++i)
		m_dataElts[i] = m_builder.CreateExtractValue(data, i, getName(RuntimeData::Index(i)));

	m_gasPtr = m_builder.CreateAlloca(Type::Gas, nullptr, "gas.ptr");
	m_builder.CreateStore(m_dataElts[RuntimeData::Index::Gas], m_gasPtr);

	m_exitBB = llvm::BasicBlock::Create(m_builder.getContext(), "Exit", getMainFunction());
	InsertPointGuard guard{m_builder};
	m_builder.SetInsertPoint(m_exitBB);
	auto retPhi = m_builder.CreatePHI(Type::MainReturn, 16, "ret");
	auto freeFunc = getModule()->getFunction("free");
	if (!freeFunc)
	{
		freeFunc = llvm::Function::Create(llvm::FunctionType::get(Type::Void, Type::WordPtr, false), llvm::Function::ExternalLinkage, "free", getModule());
		freeFunc->setDoesNotThrow();
		freeFunc->setDoesNotCapture(1);
	}
	m_builder.CreateCall(freeFunc, {m_stackBase});
	auto extGasPtr = m_builder.CreateStructGEP(getRuntimeDataType(), getDataPtr(), RuntimeData::Index::Gas, "msg.gas.ptr");
	m_builder.CreateStore(getGas(), extGasPtr);
	m_builder.CreateRet(retPhi);
}

llvm::Value* RuntimeManager::getRuntimePtr()
{
	// Expect first argument of a function to be a pointer to Runtime
	auto func = m_builder.GetInsertBlock()->getParent();
	auto rtPtr = &func->getArgumentList().front();
	assert(rtPtr->getType() == Type::RuntimePtr);
	return rtPtr;
}

llvm::Value* RuntimeManager::getDataPtr()
{
	if (getMainFunction())
		return m_dataPtr;

	auto rtPtr = getRuntimePtr();
	auto dataPtr = m_builder.CreateLoad(m_builder.CreateStructGEP(getRuntimeType(), rtPtr, 0), "data");
	assert(dataPtr->getType() == getRuntimeDataType()->getPointerTo());
	return dataPtr;
}

llvm::Value* RuntimeManager::getEnvPtr()
{
	assert(getMainFunction());	// Available only in main function
	return m_envPtr;
}

llvm::Value* RuntimeManager::getPtr(RuntimeData::Index _index)
{
	auto ptr = m_builder.CreateStructGEP(getRuntimeDataType(), getDataPtr(), _index);
	assert(getRuntimeDataType()->getElementType(_index)->getPointerTo() == ptr->getType());
	return ptr;
}

llvm::Value* RuntimeManager::get(RuntimeData::Index _index)
{
	return m_dataElts[_index];
}

void RuntimeManager::set(RuntimeData::Index _index, llvm::Value* _value)
{
	auto ptr = getPtr(_index);
	assert(ptr->getType() == _value->getType()->getPointerTo());
	m_builder.CreateStore(_value, ptr);
}

void RuntimeManager::registerReturnData(llvm::Value* _offset, llvm::Value* _size)
{
	auto memPtr = m_builder.CreateBitCast(getMem(), Type::BytePtr->getPointerTo());
	auto mem = m_builder.CreateLoad(memPtr, "memory");
	auto returnDataPtr = m_builder.CreateGEP(mem, _offset);
	set(RuntimeData::ReturnData, returnDataPtr);

	auto size64 = m_builder.CreateTrunc(_size, Type::Size);
	set(RuntimeData::ReturnDataSize, size64);
}

void RuntimeManager::registerSuicide(llvm::Value* _balanceAddress)
{
	set(RuntimeData::SuicideDestAddress, _balanceAddress);
}

void RuntimeManager::exit(ReturnCode _returnCode)
{
	m_builder.CreateBr(m_exitBB);
	auto retPhi = llvm::cast<llvm::PHINode>(&m_exitBB->front());
	retPhi->addIncoming(Constant::get(_returnCode), m_builder.GetInsertBlock());
}

void RuntimeManager::abort(llvm::Value* _jmpBuf)
{
	auto longjmp = llvm::Intrinsic::getDeclaration(getModule(), llvm::Intrinsic::eh_sjlj_longjmp);
	m_builder.CreateCall(longjmp, {_jmpBuf});
}

llvm::Value* RuntimeManager::get(Instruction _inst)
{
	switch (_inst)
	{
	default: assert(false); return nullptr;
	case Instruction::ADDRESS:		return get(RuntimeData::Address);
	case Instruction::CALLER:		return get(RuntimeData::Caller);
	case Instruction::ORIGIN:		return get(RuntimeData::Origin);
	case Instruction::CALLVALUE:	return get(RuntimeData::CallValue);
	case Instruction::GASPRICE:		return get(RuntimeData::GasPrice);
	case Instruction::COINBASE:		return get(RuntimeData::CoinBase);
	case Instruction::DIFFICULTY:	return get(RuntimeData::Difficulty);
	case Instruction::GASLIMIT:		return get(RuntimeData::GasLimit);
	case Instruction::NUMBER:		return get(RuntimeData::Number);
	case Instruction::TIMESTAMP:	return get(RuntimeData::Timestamp);
	}
}

llvm::Value* RuntimeManager::getCallData()
{
	return get(RuntimeData::CallData);
}

llvm::Value* RuntimeManager::getCode()
{
	// OPT Check what is faster
	//return get(RuntimeData::Code);
	return m_builder.CreateGlobalStringPtr({reinterpret_cast<char const*>(m_codeBegin), static_cast<size_t>(m_codeEnd - m_codeBegin)}, "code");
}

llvm::Value* RuntimeManager::getCodeSize()
{
	return Constant::get(m_codeEnd - m_codeBegin);
}

llvm::Value* RuntimeManager::getCallDataSize()
{
	auto value = get(RuntimeData::CallDataSize);
	assert(value->getType() == Type::Size);
	return m_builder.CreateZExt(value, Type::Word);
}

llvm::Value* RuntimeManager::getGas()
{
	return m_builder.CreateLoad(getGasPtr(), "gas");
}

llvm::Value* RuntimeManager::getGasPtr()
{
	assert(getMainFunction());
	return m_gasPtr;
}

llvm::Value* RuntimeManager::getMem()
{
	assert(getMainFunction());
	return m_memPtr;
}

void RuntimeManager::setGas(llvm::Value* _gas)
{
	assert(_gas->getType() == Type::Gas);
	m_builder.CreateStore(_gas, getGasPtr());
}

}
}
}
