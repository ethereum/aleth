#pragma once

#include "CompilerHelper.h"

namespace dev
{
namespace eth
{
namespace jit
{

class Arith256 : public CompilerHelper
{
public:
	Arith256(IRBuilder& _builder);

	llvm::Value* exp(llvm::Value* _arg1, llvm::Value* _arg2);

	static void debug(llvm::Value *_value, char _c, llvm::Module &_module, IRBuilder &_builder);

	static llvm::Function* getMulFunc(llvm::Module& _module);
	static llvm::Function* getMul512Func(llvm::Module& _module);
	static llvm::Function* getUDiv256Func(llvm::Module& _module);
	static llvm::Function* getURem256Func(llvm::Module& _module);
	static llvm::Function* getURem512Func(llvm::Module& _module);
	static llvm::Function* getUDivRem256Func(llvm::Module& _module);
	static llvm::Function* getSDiv256Func(llvm::Module& _module);
	static llvm::Function* getSRem256Func(llvm::Module& _module);
	static llvm::Function* getSDivRem256Func(llvm::Module& _module);
	static llvm::Function* getUDivRem512Func(llvm::Module& _module);

private:
	llvm::Function* getExpFunc();

	llvm::Function* m_exp = nullptr;
};


}
}
}
