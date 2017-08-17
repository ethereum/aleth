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
	owning_bytes_ref exec(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp) override final;

	static evm_mode scheduleToMode(EVMSchedule const& _schedule);
	static bool isCodeReady(evm_mode _mode, uint32_t _flags, h256 _codeHash);
	static void compile(evm_mode _mode, uint32_t _flags, bytesConstRef _code, h256 _codeHash);
};


}
}
