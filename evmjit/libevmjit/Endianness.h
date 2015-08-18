#pragma once

#include "CompilerHelper.h"

namespace dev
{
namespace eth
{
namespace jit
{

struct Endianness
{
	static llvm::Value* toBE(IRBuilder& _builder, llvm::Value* _word) { return bswapIfLE(_builder, _word); }
	static llvm::Value* toNative(IRBuilder& _builder, llvm::Value* _word) { return bswapIfLE(_builder, _word); }

private:
	static llvm::Value* bswapIfLE(IRBuilder& _builder, llvm::Value* _word);
};

}
}
}
