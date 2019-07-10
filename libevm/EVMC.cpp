// Copyright 2018 cpp-ethereum Authors.
// Licensed under the GNU General Public License v3. See the LICENSE file.

#include "EVMC.h"

#include <libdevcore/Log.h>
#include <libevm/VMFactory.h>

namespace dev
{
namespace eth
{
namespace
{
evmc_revision toRevision(EVMSchedule const& _schedule) noexcept
{
    if (_schedule.accountVersion == IstanbulSchedule.accountVersion)
        return EVMC_ISTANBUL;
    if (_schedule.haveCreate2 && !_schedule.eip1283Mode)
        return EVMC_PETERSBURG;
    if (_schedule.haveCreate2 && _schedule.eip1283Mode)
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
}  // namespace

EVMC::EVMC(evmc_instance* _instance) noexcept : evmc::vm(_instance)
{
    assert(_instance != nullptr);
    assert(is_abi_compatible());

    // Set the options.
    for (auto& pair : evmcOptions())
    {
        auto result = set_option(pair.first.c_str(), pair.second.c_str());
        switch (result)
        {
        case EVMC_SET_OPTION_SUCCESS:
            break;
        case EVMC_SET_OPTION_INVALID_NAME:
            cwarn << "Unknown EVMC option '" << pair.first << "'";
            break;
        case EVMC_SET_OPTION_INVALID_VALUE:
            cwarn << "Invalid value '" << pair.second << "' for EVMC option '" << pair.first << "'";
            break;
        default:
            cwarn << "Unknown error when setting EVMC option '" << pair.first << "'";
        }
    }
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

    auto mode = toRevision(_ext.evmSchedule());
    evmc_call_kind kind = _ext.isCreate ? EVMC_CREATE : EVMC_CALL;
    uint32_t flags = _ext.staticCall ? EVMC_STATIC : 0;
    assert(flags != EVMC_STATIC || kind == EVMC_CALL);  // STATIC implies a CALL.
    evmc_message msg = {kind, flags, static_cast<int32_t>(_ext.depth), gas, toEvmC(_ext.myAddress),
        toEvmC(_ext.caller), _ext.data.data(), _ext.data.size(), toEvmC(_ext.value),
        toEvmC(0x0_cppui256)};
    EvmCHost host{_ext};
    auto r = execute(host, mode, msg, _ext.code.data(), _ext.code.size());
    // FIXME: Copy the output for now, but copyless version possible.
    auto output = owning_bytes_ref{{&r.output_data[0], &r.output_data[r.output_size]}, 0, r.output_size};

    switch (r.status_code)
    {
    case EVMC_SUCCESS:
        io_gas = r.gas_left;
        return output;

    case EVMC_REVERT:
        io_gas = r.gas_left;
        throw RevertInstruction{std::move(output)};

    case EVMC_OUT_OF_GAS:
    case EVMC_FAILURE:
        BOOST_THROW_EXCEPTION(OutOfGas());

    case EVMC_INVALID_INSTRUCTION:  // NOTE: this could have its own exception
    case EVMC_UNDEFINED_INSTRUCTION:
        BOOST_THROW_EXCEPTION(BadInstruction());

    case EVMC_BAD_JUMP_DESTINATION:
        BOOST_THROW_EXCEPTION(BadJumpDestination());

    case EVMC_STACK_OVERFLOW:
        BOOST_THROW_EXCEPTION(OutOfStack());

    case EVMC_STACK_UNDERFLOW:
        BOOST_THROW_EXCEPTION(StackUnderflow());

    case EVMC_INVALID_MEMORY_ACCESS:
        BOOST_THROW_EXCEPTION(BufferOverrun());

    case EVMC_STATIC_MODE_VIOLATION:
        BOOST_THROW_EXCEPTION(DisallowedStateChange());

    case EVMC_REJECTED:
        cwarn << "Execution rejected by EVMC, executing with default VM implementation";
        return VMFactory::create(VMKind::Legacy)->exec(io_gas, _ext, _onOp);

    case EVMC_INTERNAL_ERROR:
    default:
        if (r.status_code <= EVMC_INTERNAL_ERROR)
            BOOST_THROW_EXCEPTION(InternalVMError{} << errinfo_evmcStatusCode(r.status_code));
        else
            // These cases aren't really internal errors, just more specific
            // error codes returned by the VM. Map all of them to OOG.
            BOOST_THROW_EXCEPTION(OutOfGas());
    }
}
}  // namespace eth
}  // namespace dev
