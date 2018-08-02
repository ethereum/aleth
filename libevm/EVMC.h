// Copyright 2018 cpp-ethereum Authors.
// Licensed under the GNU General Public License v3. See the LICENSE file.

#pragma once

#include <libevm/VMFace.h>

#include <evmc/evmc.h>
#include <evmc/helpers.h>

namespace dev
{
namespace eth
{

/// The RAII wrapper for an EVMC instance.
class EVM
{
public:
    explicit EVM(evmc_instance* _instance) noexcept;

    ~EVM() { evmc_destroy(m_instance); }

    EVM(EVM const&) = delete;
    EVM& operator=(EVM) = delete;

    char const* name() const noexcept { return evmc_vm_name(m_instance); }

    char const* version() const noexcept { return evmc_vm_version(m_instance); }

    class Result
    {
    public:
        explicit Result(evmc_result const& _result):
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

        evmc_status_code status() const
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
        evmc_result m_result;
    };

    /// Handy wrapper for evmc_execute().
    Result execute(ExtVMFace& _ext, int64_t gas);

    /// Translate the EVMSchedule to EVMC revision.
    static evmc_revision toRevision(EVMSchedule const& _schedule);

private:
    /// The VM instance created with EVMC <prefix>_create() function.
    evmc_instance* m_instance = nullptr;
};


/// The wrapper implementing the VMFace interface with a EVMC VM as a backend.
class EVMC : public EVM, public VMFace
{
public:
    explicit EVMC(evmc_instance* _instance);

    owning_bytes_ref exec(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp) final;

private:
    bytesConstRef m_code;
    int m_step = 0;
    char const* const* m_instructionNames = nullptr;

    Logger m_vmTraceLogger{createLogger(VerbosityTrace, "vmtrace")};
};
}
}
