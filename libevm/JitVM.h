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

	static evm_revision toRevision(EVMSchedule const& _schedule);
	static bool isCodeReady(evm_revision _mode, uint32_t _flags, h256 _codeHash);
	static void compile(evm_revision _mode, uint32_t _flags, bytesConstRef _code, h256 _codeHash);
};


}
}
