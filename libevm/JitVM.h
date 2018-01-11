#pragma once

#include <evmjit.h>
#include <libevm/VMFace.h>

// FIXME: Move to EVM-C evm.h.
typedef struct evm_instance* (*evm_create_fn)(void);

namespace dev
{
namespace eth
{
class EVM;

/// Translate the EVMSchedule to EVM-C revision.
evm_revision toRevision(EVMSchedule const& _schedule);

template<evm_create_fn createFn>
class JitVM : public VMFace
{
public:
    owning_bytes_ref exec(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp) final;

    bool isCodeReady(evm_revision _mode, uint32_t _flags, h256 _codeHash);
    void compile(evm_revision _mode, uint32_t _flags, bytesConstRef _code, h256 _codeHash);

private:
    static EVM& instance();
};

using EVMJIT = JitVM<evmjit_create>;
}
}
