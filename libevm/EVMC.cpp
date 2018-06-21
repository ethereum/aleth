// Copyright 2018 cpp-ethereum Authors.
// Licensed under the GNU General Public License v3. See the LICENSE file.

#include "EVMC.h"

#include <libdevcore/Log.h>
#include <libevm/VMFactory.h>

namespace dev
{
namespace eth
{
EVM::EVM(evmc_instance* _instance) noexcept : m_instance(_instance)
{
    assert(m_instance != nullptr);
    assert(m_instance->abi_version == EVMC_ABI_VERSION);

    // Set the options.
    for (auto& pair : evmcOptions())
        m_instance->set_option(m_instance, pair.first.c_str(), pair.second.c_str());
}

owning_bytes_ref EVMC::exec(u256& io_gas, ExtVMFace& _ext, const OnOpFunc& _onOp)
{
    assert(_ext.envInfo().number() >= 0);
    assert(_ext.envInfo().timestamp() >= 0);

    constexpr int64_t int64max = std::numeric_limits<int64_t>::max();

    // TODO: The following checks should be removed by changing the types
    //       used for gas, block number and timestamp.
    (void)int64max;
    assert(io_gas <= int64max);
    assert(_ext.envInfo().gasLimit() <= int64max);
    assert(_ext.depth <= static_cast<size_t>(std::numeric_limits<int32_t>::max()));

    auto gas = static_cast<int64_t>(io_gas);
    EVM::Result r = execute(_ext, gas);

    switch (r.status())
    {
    case EVMC_SUCCESS:
        io_gas = r.gasLeft();
        // FIXME: Copy the output for now, but copyless version possible.
        return {r.output().toVector(), 0, r.output().size()};

    case EVMC_REVERT:
        io_gas = r.gasLeft();
        // FIXME: Copy the output for now, but copyless version possible.
        throw RevertInstruction{{r.output().toVector(), 0, r.output().size()}};

    case EVMC_OUT_OF_GAS:
    case EVMC_FAILURE:
        BOOST_THROW_EXCEPTION(OutOfGas());

    case EVMC_UNDEFINED_INSTRUCTION:
        BOOST_THROW_EXCEPTION(BadInstruction());

    case EVMC_BAD_JUMP_DESTINATION:
        BOOST_THROW_EXCEPTION(BadJumpDestination());

    case EVMC_STACK_OVERFLOW:
        BOOST_THROW_EXCEPTION(OutOfStack());

    case EVMC_STACK_UNDERFLOW:
        BOOST_THROW_EXCEPTION(StackUnderflow());

    case EVMC_STATIC_MODE_VIOLATION:
        BOOST_THROW_EXCEPTION(DisallowedStateChange());

    case EVMC_REJECTED:
        cwarn << "Execution rejected by EVMC, executing with default VM implementation";
        return VMFactory::create(VMKind::Legacy)->exec(io_gas, _ext, _onOp);

    default:
        BOOST_THROW_EXCEPTION(InternalVMError{} << errinfo_evmcStatusCode(r.status()));
    }
}

evmc_revision toRevision(EVMSchedule const& _schedule)
{
    if (_schedule.haveCreate2)
        return EVMC_CONSTANTINOPLE;
    if (_schedule.haveRevert)
        return EVMC_BYZANTIUM;
    if (_schedule.eip158Mode)
        return EVMC_SPURIOUS_DRAGON;
    if (_schedule.eip150Mode)
        return EVMC_TANGERINE_WHISTLE;
    if (_schedule.haveDelegateCall)
        return EVMC_HOMESTEAD;
    return EVMC_FRONTIER;
}
}  // namespace eth
}  // namespace dev
