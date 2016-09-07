#pragma once

#include <evmjit.h>
#include <libevm/VMFace.h>

namespace dev
{
namespace eth
{

class JitVM: public VMFace
{
public:
	virtual bytesConstRef execImpl(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp) override final;

	static bool isCodeReady(evm_mode _mode, h256 _codeHash);
	static void compile(evm_mode _mode, bytesConstRef _code, h256 _codeHash);

private:
	std::unique_ptr<VMFace> m_fallbackVM; ///< VM used in case of input data rejected by JIT
	bytes m_output;
};


}
}
