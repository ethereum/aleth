// Copyright 2018 cpp-ethereum Authors.
// Licensed under the GNU General Public License v3. See the LICENSE file.

#include "EVMC.h"

#include <evmc/instructions.h>
#include <libdevcore/Log.h>
#include <libevm/VMFactory.h>

namespace dev
{
namespace eth
{
EVM::EVM(evmc_instance* _instance) noexcept : m_instance(_instance)
{
    assert(m_instance != nullptr);
    assert(evmc_is_abi_compatible(m_instance));

    // Set the options.
    for (auto& pair : evmcOptions())
    {
        auto result = evmc_set_option(m_instance, pair.first.c_str(), pair.second.c_str());
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

/// Handy wrapper for evmc_execute().
EVM::Result EVM::execute(ExtVMFace& _ext, int64_t gas)
{
    auto mode = toRevision(_ext.evmSchedule());
    evmc_call_kind kind = _ext.isCreate ? EVMC_CREATE : EVMC_CALL;
    uint32_t flags = _ext.staticCall ? EVMC_STATIC : 0;
    assert(flags != EVMC_STATIC || kind == EVMC_CALL);  // STATIC implies a CALL.
    evmc_message msg = {kind, flags, static_cast<int32_t>(_ext.depth), gas, toEvmC(_ext.myAddress),
        toEvmC(_ext.caller), _ext.data.data(), _ext.data.size(), toEvmC(_ext.value),
        toEvmC(0x0_cppui256)};
    return EVM::Result{
        evmc_execute(m_instance, &_ext, mode, &msg, _ext.code.data(), _ext.code.size())};
}

static inline bool isCall(uint8_t _opcode) noexcept
{
    return _opcode == OP_CALL || _opcode == OP_CALLCODE || _opcode == OP_DELEGATECALL ||
           _opcode == OP_CREATE || _opcode == OP_CREATE2;
}

EVMC::EVMC(evmc_instance* _instance) : EVM(_instance)
{
    static const auto tracer = [](evmc_tracer_context * _context, size_t _codeOffset,
        evmc_status_code _statusCode, int64_t _gasLeft, size_t /*_stackNumItems*/,
        evmc_uint256be const* _pushedStackItem, size_t /*_memorySize*/,
        size_t /*changed_memory_offset*/, size_t /*changed_memory_size*/,
        uint8_t const* /*changed_memory*/) noexcept
    {
        EVMC* evmc = reinterpret_cast<EVMC*>(_context);
        boost::optional<evmc_uint256be> pushedStackItem;
        if (_pushedStackItem)
            pushedStackItem = *_pushedStackItem;

        auto opcode = evmc->m_code[_codeOffset];
        auto callIndex = isCall(opcode) ? evmc->m_prevCall : -1;
        evmc->m_calls[evmc->m_currentCall].trace.emplace_back(InstructionTrace{
            opcode, _codeOffset, _statusCode, _gasLeft, pushedStackItem, callIndex});
    };

    _instance->set_tracer(_instance, tracer, reinterpret_cast<evmc_tracer_context*>(this));
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

    auto prevCode = m_code;
    m_code = bytesConstRef{&_ext.code};

    auto gas = static_cast<int64_t>(io_gas);

    if (_ext.depth == 0)
        m_prevCall = -1;  // Reset prev call.

    m_calls.emplace_back(
        CallTrace{static_cast<int>(_ext.depth), _ext.isCreate ? EVMC_CREATE : EVMC_CALL,
            EVMC_SUCCESS, gas, -1, _ext.caller, _ext.myAddress, {}});
    auto parentCall = m_currentCall;
    m_currentCall = m_calls.size() - 1;

    EVM::Result r = execute(_ext, gas);

    m_calls[m_currentCall].status = r.status();
    m_calls[m_currentCall].gasLeft = r.gasLeft();
    m_prevCall = m_currentCall;
    m_currentCall = parentCall;

    m_code = prevCode;

    if (_ext.depth == 0)
        dumpTrace();

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
        if (r.status() <= EVMC_INTERNAL_ERROR)
            BOOST_THROW_EXCEPTION(InternalVMError{} << errinfo_evmcStatusCode(r.status()));
        else
            // These cases aren't really internal errors, just more specific
            // error codes returned by the VM. Map all of them to OOG.
            BOOST_THROW_EXCEPTION(OutOfGas());
    }
}

evmc_revision EVM::toRevision(EVMSchedule const& _schedule)
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

static char const* to_string(evmc_call_kind _kind)
{
    switch (_kind)
    {
    case EVMC_CALL:
        return "CALL";
    case EVMC_CALLCODE:
        return "CALLCODE";
    case EVMC_DELEGATECALL:
        return "DELEGATECALL";
    case EVMC_CREATE:
        return "CREATE";
    case EVMC_CREATE2:
        return "CREATE2";
    }
    return "<invalid call kind>";
}

static void dumpCallTrace(
    const std::vector<CallTrace>& calls, size_t index, char const* const* names)
{
    auto& c = calls[index];

    std::string indent;
    for (int i = 0; i <= c.depth; ++i)
        indent.push_back(' ');

    std::cout << ">>>>" << indent << "> " << to_string(c.kind) << " " << c.sender << " > "
              << c.destination << " gas: " << std::dec << c.gas << "\n";
    for (auto& trace : c.trace)
    {
        auto* name = names[trace.opcode];
        std::cout << std::hex << std::right << std::setfill('0') << std::setw(4) << trace.codeOffset
                  << indent << " ";
        if (name)
            std::cout << std::left << std::setfill(' ') << std::setw(7) << name;
        else
            std::cout << "<" << std::hex << std::right << std::setfill('0') << std::setw(2) << int(trace.opcode) << ">    ";
        if (trace.pushedStackItem)
            std::cout << " (" << fromEvmC(*trace.pushedStackItem) << ")";
        if (trace.statusCode != EVMC_SUCCESS)
            std::cout << " <" << trace.statusCode << ">";
        std::cout << "\n";

        if (trace.callIndex >= 0)
            dumpCallTrace(calls, trace.callIndex, names);
    }
    std::cout << "<<<<" << indent << "< <" << c.status << "> "
              << "gas left: " << std::dec << c.gasLeft << "\n";
}

void EVMC::dumpTrace()
{
    auto names = evmc_get_instruction_names_table(EVMC_LATEST_REVISION);

    for (size_t i = 0; i < m_calls.size(); ++i)
    {
        if (m_calls[i].depth == 0)
            dumpCallTrace(m_calls, i, names);
    }

    m_calls.clear();
}

}  // namespace eth
}  // namespace dev
