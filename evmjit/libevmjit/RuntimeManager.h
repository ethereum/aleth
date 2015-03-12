#pragma once

#include "CompilerHelper.h"
#include "Type.h"
#include "RuntimeData.h"
#include "Instruction.h"

namespace dev
{
namespace eth
{
namespace jit
{
class Stack;

class RuntimeManager: public CompilerHelper
{
public:
	RuntimeManager(llvm::IRBuilder<>& _builder, llvm::Value* _jmpBuf, code_iterator _codeBegin, code_iterator _codeEnd);

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

	void registerReturnData(llvm::Value* _index, llvm::Value* _size);
	void registerSuicide(llvm::Value* _balanceAddress);

	void exit(ReturnCode _returnCode);

	void abort(llvm::Value* _jmpBuf);
	void abort() { abort(getJmpBufExt()); }

	void setStack(Stack& _stack) { m_stack = &_stack; }

	static llvm::StructType* getRuntimeType();
	static llvm::StructType* getRuntimeDataType();

private:
	llvm::Value* getPtr(RuntimeData::Index _index);
	void set(RuntimeData::Index _index, llvm::Value* _value);
	llvm::Value* getJmpBufExt();

	llvm::Function* m_longjmp = nullptr;
	llvm::Value* const m_jmpBuf;
	llvm::Value* m_dataPtr = nullptr;
	llvm::Value* m_envPtr = nullptr;

	code_iterator m_codeBegin = {};
	code_iterator m_codeEnd = {};

	Stack* m_stack = nullptr;
};

}
}
}
