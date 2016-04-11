#pragma once

#include "preprocessor/llvm_includes_start.h"
#include <llvm/IR/IRBuilder.h>
#include "preprocessor/llvm_includes_end.h"


namespace dev
{
namespace eth
{
namespace jit
{
class RuntimeManager;

using IRBuilder = llvm::IRBuilder<>;

/// Base class for compiler helpers like Memory, GasMeter, etc.
class CompilerHelper
{
protected:
	CompilerHelper(IRBuilder& _builder);

	CompilerHelper(const CompilerHelper&) = delete;
	CompilerHelper& operator=(CompilerHelper) = delete;

	/// Reference to the IR module being compiled
	llvm::Module* getModule();

	/// Reference to the main module function
	llvm::Function* getMainFunction();

	/// Reference to parent compiler IR builder
	IRBuilder& m_builder;

	friend class RuntimeHelper;
};

/// Compiler helper that depends on runtime data
class RuntimeHelper : public CompilerHelper
{
protected:
	RuntimeHelper(RuntimeManager& _runtimeManager);

	RuntimeManager& getRuntimeManager() { return m_runtimeManager; }

private:
	RuntimeManager& m_runtimeManager;
};

struct InsertPointGuard
{
	explicit InsertPointGuard(llvm::IRBuilderBase& _builder): m_builder(_builder), m_insertPoint(_builder.saveIP()) {}
	~InsertPointGuard() { m_builder.restoreIP(m_insertPoint); }

private:
	llvm::IRBuilderBase& m_builder;
	llvm::IRBuilderBase::InsertPoint m_insertPoint;
};

}
}
}
