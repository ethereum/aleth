#pragma once

#include <array>

#include "CompilerHelper.h"
#include "Type.h"
#include "Instruction.h"

namespace dev
{
namespace eth
{
namespace jit
{
using namespace evmjit;

class RuntimeManager: public CompilerHelper
{
public:
	RuntimeManager(IRBuilder& _builder, code_iterator _codeBegin, code_iterator _codeEnd);

	llvm::Value* getRuntimePtr();
	llvm::Value* getDataPtr();
	llvm::Value* getEnvPtr();

	llvm::Value* get(RuntimeData::Index _index);
	llvm::Value* get(Instruction _inst);
	llvm::Value* getGas();
	llvm::Value* getGasPtr();
	llvm::Value* getCallData();
	llvm::Value* getCode();
	llvm::Value* getCodeSize();
	llvm::Value* getCallDataSize();
	llvm::Value* getJmpBuf() { return m_jmpBuf; }
	void setGas(llvm::Value* _gas);

	llvm::Value* getMem();

	void registerReturnData(llvm::Value* _index, llvm::Value* _size); // TODO: Move to Memory.
	void registerSuicide(llvm::Value* _balanceAddress);

	void exit(ReturnCode _returnCode);

	void abort(llvm::Value* _jmpBuf);

	llvm::Value* getStackBase() const { return m_stackBase; }
	llvm::Value* getStackSize() const { return m_stackSize; }

	void setJmpBuf(llvm::Value* _jmpBuf) { m_jmpBuf = _jmpBuf; }
	void setExitBB(llvm::BasicBlock* _bb) { m_exitBB = _bb; }

	static llvm::StructType* getRuntimeType();
	static llvm::StructType* getRuntimeDataType();

	//TODO Move to schedule
	static const size_t stackSizeLimit = 1024;

private:
	llvm::Value* getPtr(RuntimeData::Index _index);
	void set(RuntimeData::Index _index, llvm::Value* _value);

	llvm::Function* m_longjmp = nullptr;
	llvm::Value* m_jmpBuf = nullptr;
	llvm::Value* m_dataPtr = nullptr;
	llvm::Value* m_gasPtr = nullptr;
	llvm::Value* m_memPtr = nullptr;
	llvm::Value* m_envPtr = nullptr;

	std::array<llvm::Value*, RuntimeData::numElements> m_dataElts;

	llvm::Value* m_stackBase = nullptr;
	llvm::Value* m_stackSize = nullptr;

	llvm::BasicBlock* m_exitBB = nullptr;

	code_iterator m_codeBegin = {};
	code_iterator m_codeEnd = {};
	llvm::Value* m_codePtr = nullptr;
};

}
}
}
