#pragma once

#include <evmjit.h>
#include <libevm/VMFace.h>

namespace dev
{
namespace eth
{
/// Translate the EVMSchedule to EVM-C revision.
evm_revision toRevision(EVMSchedule const& _schedule);

/// The RAII wrapper for an EVM-C instance.
class EVM
{
public:
    explicit EVM(evm_instance* _instance) : m_instance(_instance)
    {
        assert(m_instance != nullptr);
        assert(m_instance->abi_version == EVM_ABI_VERSION);
    }

    ~EVM() { m_instance->destroy(m_instance); }

    EVM(EVM const&) = delete;
    EVM& operator=(EVM) = delete;

    class Result
    {
    public:
        explicit Result(evm_result const& _result):
            m_result(_result)
        {}

        ~Result()
        {
            if (m_result.release)
                m_result.release(&m_result);
        }

        Result(Result&& _other) noexcept:
            m_result(_other.m_result)
        {
            // Disable releaser of the rvalue object.
            _other.m_result.release = nullptr;
        }

        Result(Result const&) = delete;
        Result& operator=(Result const&) = delete;

        evm_status_code status() const
        {
            return m_result.status_code;
        }

        int64_t gasLeft() const
        {
            return m_result.gas_left;
        }

        bytesConstRef output() const
        {
            return {m_result.output_data, m_result.output_size};
        }

    private:
        evm_result m_result;
    };

    /// Handy wrapper for evm_execute().
    Result execute(ExtVMFace& _ext, int64_t gas)
    {
        auto mode = toRevision(_ext.evmSchedule());
        uint32_t flags = _ext.staticCall ? EVM_STATIC : 0;
        evm_message msg = {toEvmC(_ext.myAddress), toEvmC(_ext.caller),
                           toEvmC(_ext.value), _ext.data.data(),
                           _ext.data.size(), toEvmC(_ext.codeHash), gas,
                           static_cast<int32_t>(_ext.depth), EVM_CALL, flags};
        return Result{m_instance->execute(
            m_instance, &_ext, mode, &msg, _ext.code.data(), _ext.code.size()
        )};
    }

    bool isCodeReady(evm_revision _mode, uint32_t _flags, h256 _codeHash)
    {
        return m_instance->get_code_status(m_instance, _mode, _flags, toEvmC(_codeHash)) == EVM_READY;
    }

    void compile(evm_revision _mode, uint32_t _flags, bytesConstRef _code, h256 _codeHash)
    {
        m_instance->prepare_code(
            m_instance, _mode, _flags, toEvmC(_codeHash), _code.data(), _code.size()
        );
    }

private:
    /// The VM instance created with EVM-C <prefix>_create() function.
    evm_instance* m_instance = nullptr;
};


/// The wrapper implementing the VMFace interface with a EVM-C VM as a backend.
class EVMC : public EVM, public VMFace
{
public:
    explicit EVMC(evm_instance* _instance) : EVM(_instance) {}

    owning_bytes_ref exec(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp) final;
};
}
}
