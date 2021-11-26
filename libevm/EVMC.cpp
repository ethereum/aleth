// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
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
    if (_schedule.haveChainID)
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

evmc_message createMessage(u256 const& _gas, Address const& _myAddress, Address const& _caller,
    u256 const& _value, bytesConstRef _data, unsigned _depth, bool _isCreate,
    bool _isStaticCall) noexcept
{
    // TODO: The following checks should be removed by changing the types
    //       used for gas, block number and timestamp.
    assert(_gas <= std::numeric_limits<int64_t>::max());
    assert(_depth <= static_cast<size_t>(std::numeric_limits<int32_t>::max()));

    auto gas = static_cast<int64_t>(_gas);

    evmc_call_kind kind = _isCreate ? EVMC_CREATE : EVMC_CALL;
    uint32_t flags = _isStaticCall ? EVMC_STATIC : 0;
    assert(flags != EVMC_STATIC || kind == EVMC_CALL);  // STATIC implies a CALL.
    return {kind, flags, static_cast<int32_t>(_depth), gas, toEvmC(_myAddress), toEvmC(_caller),
        _data.data(), _data.size(), toEvmC(_value), toEvmC(0x0_cppui256)};
}

owning_bytes_ref processResult(u256& o_gas, evmc::result const& _r)
{
    // FIXME: Copy the output for now, but copyless version possible.
    auto output =
        owning_bytes_ref{{&_r.output_data[0], &_r.output_data[_r.output_size]}, 0, _r.output_size};

    switch (_r.status_code)
    {
    case EVMC_SUCCESS:
        o_gas = _r.gas_left;
        return output;

    case EVMC_REVERT:
        o_gas = _r.gas_left;
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

    case EVMC_PRECOMPILE_FAILURE:
        BOOST_THROW_EXCEPTION(PrecompileFailure());

    case EVMC_INTERNAL_ERROR:
    default:
        if (_r.status_code <= EVMC_INTERNAL_ERROR)
            BOOST_THROW_EXCEPTION(InternalVMError{} << errinfo_evmcStatusCode(_r.status_code));
        else
            // These cases aren't really internal errors, just more specific
            // error codes returned by the VM. Map all of them to OOG.
            BOOST_THROW_EXCEPTION(OutOfGas());
    }
}

}  // namespace

EVMC::EVMC(evmc_vm* _vm, std::vector<std::pair<std::string, std::string>> const& _options) noexcept
  : evmc::VM(_vm)
{
    assert(_vm != nullptr);
    assert(is_abi_compatible());

    // Set the options.
    for (auto& pair : _options)
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
    assert(_ext.envInfo().gasLimit() <= std::numeric_limits<int64_t>::max());

    evmc_message const msg = createMessage(io_gas, _ext.myAddress, _ext.caller, _ext.value,
        _ext.data, _ext.depth, _ext.isCreate, _ext.staticCall);
    auto const mode = toRevision(_ext.evmSchedule());

    EvmCHost host{_ext};
    auto r = execute(host, mode, msg, _ext.code.data(), _ext.code.size());

    if (r.status_code == EVMC_REJECTED)
    {
        cwarn << "Execution rejected by EVMC, executing with default VM implementation";
        return VMFactory::create(VMKind::Legacy)->exec(io_gas, _ext, _onOp);
    }

    return processResult(io_gas, r);
}

owning_bytes_ref EVMC::exec(u256& io_gas, Address const& _myAddress, Address const& _caller,
    u256 const& _value, bytesConstRef _data, unsigned _depth, bool _isCreate, bool _isStaticCall,
    EVMSchedule const& _schedule)
{
    evmc_message const msg =
        createMessage(io_gas, _myAddress, _caller, _value, _data, _depth, _isCreate, _isStaticCall);
    auto const mode = toRevision(_schedule);

    auto r = execute(mode, msg, nullptr, 0);

    return processResult(io_gas, r);
}
}  // namespace eth
}  // namespace dev
