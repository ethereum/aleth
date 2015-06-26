#pragma once

#include <libevm/VMFace.h>
#include <evmjit/libevmjit/ExecutionEngine.h>

namespace dev
{
namespace eth
{

class JitVM: public VMFace
{
public:
	virtual bytesConstRef execImpl(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp) override final;

private:
	jit::RuntimeData m_data;
	jit::ExecutionEngine m_engine;
	std::unique_ptr<VMFace> m_fallbackVM; ///< VM used in case of input data rejected by JIT
};


}
}
