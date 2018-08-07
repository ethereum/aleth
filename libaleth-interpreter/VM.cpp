/*
    This file is part of cpp-ethereum.

    cpp-ethereum is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    cpp-ethereum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "interpreter.h"
#include "VM.h"

#include <aleth/buildinfo.h>

namespace
{

evmc_capabilities_flagset getCapabilities(evmc_instance* _instance) noexcept
{
    (void)_instance;
    return EVMC_CAPABILITY_EVM1;
}

void delete_output(const evmc_result* result)
{
    delete[] result->output_data;
}

class InterpreterEvmcInstance : public evmc_instance
{
public:
    static InterpreterEvmcInstance* create() { return new InterpreterEvmcInstance{}; }

private:
    InterpreterEvmcInstance()
      : evmc_instance{
            EVMC_ABI_VERSION, "interpreter", aleth_get_buildinfo()->project_version, destroy,
            execute, getCapabilities, setTracer,
            nullptr,  // set_option
        }
    {}

    static void destroy(evmc_instance* _instance)
    {
        delete static_cast<InterpreterEvmcInstance*>(_instance);
    }

    static evmc_result execute(evmc_instance* _instance, evmc_context* _context, evmc_revision _rev,
        evmc_message const* _msg, uint8_t const* _code, size_t _codeSize) noexcept
    {
        std::unique_ptr<dev::eth::VM> vm{new dev::eth::VM};

        evmc_result result = {};
        dev::eth::owning_bytes_ref output;

        auto evmc = static_cast<InterpreterEvmcInstance*>(_instance);
        try
        {
            output = vm->exec(_context, _rev, _msg, _code, _codeSize, evmc->m_traceCallback,
                evmc->m_traceContext);
            result.status_code = EVMC_SUCCESS;
            result.gas_left = vm->m_io_gas;
        }
        catch (dev::eth::RevertInstruction& ex)
        {
            result.status_code = EVMC_REVERT;
            result.gas_left = vm->m_io_gas;
            output = ex.output();  // This moves the output from the exception!
        }
        catch (dev::eth::BadInstruction const&)
        {
            result.status_code = EVMC_UNDEFINED_INSTRUCTION;
        }
        catch (dev::eth::OutOfStack const&)
        {
            result.status_code = EVMC_STACK_OVERFLOW;
        }
        catch (dev::eth::StackUnderflow const&)
        {
            result.status_code = EVMC_STACK_UNDERFLOW;
        }
        catch (dev::eth::BufferOverrun const&)
        {
            result.status_code = EVMC_INVALID_MEMORY_ACCESS;
        }
        catch (dev::eth::OutOfGas const&)
        {
            result.status_code = EVMC_OUT_OF_GAS;
        }
        catch (dev::eth::BadJumpDestination const&)
        {
            result.status_code = EVMC_BAD_JUMP_DESTINATION;
        }
        catch (dev::eth::DisallowedStateChange const&)
        {
            result.status_code = EVMC_STATIC_MODE_VIOLATION;
        }
        catch (dev::eth::VMException const&)
        {
            result.status_code = EVMC_FAILURE;
        }
        catch (...)
        {
            result.status_code = EVMC_INTERNAL_ERROR;
        }

        if (!output.empty())
        {
            // Make a copy of the output.
            auto outputData = new uint8_t[output.size()];
            std::memcpy(outputData, output.data(), output.size());
            result.output_data = outputData;
            result.output_size = output.size();
            result.release = delete_output;
        }

        return result;
    }

    static void setTracer(evmc_instance* _instance, evmc_trace_callback _callback,
        evmc_tracer_context* _context) noexcept
    {
        auto evmc = static_cast<InterpreterEvmcInstance*>(_instance);

        evmc->m_traceCallback = _callback;
        evmc->m_traceContext = _context;
    }

    evmc_trace_callback m_traceCallback = nullptr;
    evmc_tracer_context* m_traceContext = nullptr;
};

}  // namespace

extern "C" evmc_instance* evmc_create_interpreter() noexcept
{
    return InterpreterEvmcInstance::create();
}

namespace dev
{
namespace eth
{
void VM::trace(uint64_t _pc) noexcept
{
    if (m_traceCallback)
    {
        auto const& metrics = c_metrics[static_cast<size_t>(m_OP)];
        evmc_uint256be topStackItem;
        evmc_uint256be const* pushedStackItem = nullptr;
        if (metrics.num_stack_returned_items >= 1)
        {
            topStackItem = toEvmC(m_SPP[0]);
            pushedStackItem = &topStackItem;
        }
        m_traceCallback(m_traceContext, _pc, EVMC_SUCCESS, m_io_gas, m_stackEnd - m_SPP,
            pushedStackItem, m_mem.size(), 0, 0, nullptr);
    }
}

uint64_t VM::memNeed(u256 _offset, u256 _size)
{
    return toInt63(_size ? u512(_offset) + _size : u512(0));
}

template <class S>
S divWorkaround(S const& _a, S const& _b)
{
    return (S)(s512(_a) / s512(_b));
}

template <class S>
S modWorkaround(S const& _a, S const& _b)
{
    return (S)(s512(_a) % s512(_b));
}


//
// for decoding destinations of JUMPTO, JUMPV, JUMPSUB and JUMPSUBV
//

uint64_t VM::decodeJumpDest(const byte* const _code, uint64_t& _pc)
{
    // turn 2 MSB-first bytes in the code into a native-order integer
    uint64_t dest      = _code[_pc++];
    dest = (dest << 8) | _code[_pc++];
    return dest;
}

uint64_t VM::decodeJumpvDest(const byte* const _code, uint64_t& _pc, byte _voff)
{
    // Layout of jump table in bytecode...
    //     byte opcode
    //     byte n_jumps
    //     byte table[n_jumps][2]
    //
    uint64_t pc = _pc;
    byte n = _code[++pc];           // byte after opcode is number of jumps
    if (_voff >= n) _voff = n - 1;  // if offset overflows use default jump
    pc += _voff * 2;                // adjust inout pc before index destination in table

    uint64_t dest = decodeJumpDest(_code, pc);

    _pc += 1 + n * 2;               // adust inout _pc to opcode after table
    return dest;
}


//
// set current SP to SP', adjust SP' per _removed and _added items
//
void VM::adjustStack(int _removed, int _added)
{
    m_SP = m_SPP;

    // adjust stack and check bounds
    m_SPP += _removed;
    if (m_stackEnd < m_SPP)
        throwBadStack(_removed, _added);
    m_SPP -= _added;
    if (m_SPP < m_stack)
        throwBadStack(_removed, _added);
}

uint64_t VM::gasForMem(u512 _size)
{
    constexpr int64_t memoryGas = VMSchedule::memoryGas;
    constexpr int64_t quadCoeffDiv = VMSchedule::quadCoeffDiv;
    u512 s = _size / 32;
    return toInt63(memoryGas * s + s * s / quadCoeffDiv);
}

void VM::updateIOGas()
{
    if (m_io_gas < m_runGas)
        throwOutOfGas();
    m_io_gas -= m_runGas;
}

void VM::updateGas()
{
    if (m_newMemSize > m_mem.size())
        m_runGas += toInt63(gasForMem(m_newMemSize) - gasForMem(m_mem.size()));
    m_runGas += (VMSchedule::copyGas * ((m_copyMemSize + 31) / 32));
    if (m_io_gas < m_runGas)
        throwOutOfGas();
}

void VM::updateMem(uint64_t _newMem)
{
    m_newMemSize = (_newMem + 31) / 32 * 32;
    updateGas();
    if (m_newMemSize > m_mem.size())
        m_mem.resize(m_newMemSize);
}

void VM::logGasMem()
{
    unsigned n = (unsigned) m_OP - (unsigned) Instruction::LOG0;
    constexpr int64_t logDataGas = VMSchedule::logDataGas;
    m_runGas = toInt63(
        VMSchedule::logGas + VMSchedule::logTopicGas * n + logDataGas * u512(m_SP[1]));
    updateMem(memNeed(m_SP[0], m_SP[1]));
}

void VM::fetchInstruction()
{
    m_OP = Instruction(m_code[m_PC]);
    auto const metric = c_metrics[static_cast<size_t>(m_OP)];
    adjustStack(metric.num_stack_arguments, metric.num_stack_returned_items);

    // FEES...
    m_runGas = metric.gas_cost;
    m_newMemSize = m_mem.size();
    m_copyMemSize = 0;
}

evmc_tx_context const& VM::getTxContext()
{
    if (!m_tx_context)
        m_tx_context.emplace(m_context->host->get_tx_context(m_context));
    return m_tx_context.value();
}


///////////////////////////////////////////////////////////////////////////////
//
// interpreter entry point

owning_bytes_ref VM::exec(evmc_context* _context, evmc_revision _rev, const evmc_message* _msg,
    uint8_t const* _code, size_t _codeSize, evmc_trace_callback _traceCallback,
    evmc_tracer_context* _traceContext)
{
    m_context = _context;
    m_rev = _rev;
    m_message = _msg;
    m_io_gas = uint64_t(_msg->gas);
    m_PC = 0;
    m_pCode = _code;
    m_codeSize = _codeSize;
    m_traceCallback = _traceCallback;
    m_traceContext = _traceContext;

    // trampoline to minimize depth of call stack when calling out
    m_bounce = &VM::initEntry;
    do
        (this->*m_bounce)();
    while (m_bounce);

    return std::move(m_output);
}

//
// main interpreter loop and switch
//
void VM::interpretCases()
{
    INIT_CASES
    DO_CASES
    {
        //
        // Call-related instructions
        //

        CASE(CREATE2)
        {
            ON_OP();
            if (m_rev < EVMC_CONSTANTINOPLE)
                throwBadInstruction();
            if (m_message->flags & EVMC_STATIC)
                throwDisallowedStateChange();

            m_bounce = &VM::caseCreate;
        }
        BREAK

        CASE(CREATE)
        {
            ON_OP();
            if (m_message->flags & EVMC_STATIC)
                throwDisallowedStateChange();

            m_bounce = &VM::caseCreate;
        }
        BREAK

        CASE(DELEGATECALL)
        CASE(STATICCALL)
        CASE(CALL)
        CASE(CALLCODE)
        {
            ON_OP();
            if (m_OP == Instruction::DELEGATECALL && m_rev < EVMC_HOMESTEAD)
                throwBadInstruction();
            if (m_OP == Instruction::STATICCALL && m_rev < EVMC_BYZANTIUM)
                throwBadInstruction();
            if (m_OP == Instruction::CALL && m_message->flags & EVMC_STATIC && m_SP[2] != 0)
                throwDisallowedStateChange();
            m_bounce = &VM::caseCall;
        }
        BREAK

        CASE(RETURN)
        {
            ON_OP();
            m_copyMemSize = 0;
            updateMem(memNeed(m_SP[0], m_SP[1]));
            updateIOGas();

            uint64_t b = (uint64_t)m_SP[0];
            uint64_t s = (uint64_t)m_SP[1];
            m_output = owning_bytes_ref{std::move(m_mem), b, s};
            m_bounce = 0;
        }
        BREAK

        CASE(REVERT)
        {
            // Pre-byzantium
            if (m_rev < EVMC_BYZANTIUM)
                throwBadInstruction();

            ON_OP();
            m_copyMemSize = 0;
            updateMem(memNeed(m_SP[0], m_SP[1]));
            updateIOGas();

            uint64_t b = (uint64_t)m_SP[0];
            uint64_t s = (uint64_t)m_SP[1];
            owning_bytes_ref output{std::move(m_mem), b, s};
            throwRevertInstruction(std::move(output));
        }
        BREAK;

        CASE(SUICIDE)
        {
            ON_OP();
            if (m_message->flags & EVMC_STATIC)
                throwDisallowedStateChange();

            m_runGas = m_rev >= EVMC_TANGERINE_WHISTLE ? 5000 : 0;
            evmc_address destination = toEvmC(asAddress(m_SP[0]));

            // After EIP158 zero-value suicides do not have to pay account creation gas.
            u256 const balance =
                fromEvmC(m_context->host->get_balance(m_context, &m_message->destination));
            if (balance > 0 || m_rev < EVMC_SPURIOUS_DRAGON)
            {
                // After EIP150 hard fork charge additional cost of sending
                // ethers to non-existing account.
                int destinationExists =
                    m_context->host->account_exists(m_context, &destination);
                if (m_rev >= EVMC_TANGERINE_WHISTLE && !destinationExists)
                    m_runGas += VMSchedule::callNewAccount;
            }

            updateIOGas();
            m_context->host->selfdestruct(m_context, &m_message->destination, &destination);
            m_bounce = nullptr;
        }
        BREAK

        CASE(STOP)
        {
            ON_OP();
            updateIOGas();
            m_bounce = 0;
        }
        BREAK


        //
        // instructions potentially expanding memory
        //

        CASE(MLOAD)
        {
            ON_OP();
            updateMem(toInt63(m_SP[0]) + 32);
            updateIOGas();

            m_SPP[0] = (u256)*(h256 const*)(m_mem.data() + (unsigned)m_SP[0]);
        }
        NEXT

        CASE(MSTORE)
        {
            ON_OP();
            updateMem(toInt63(m_SP[0]) + 32);
            updateIOGas();

            *(h256*)&m_mem[(unsigned)m_SP[0]] = (h256)m_SP[1];
        }
        NEXT

        CASE(MSTORE8)
        {
            ON_OP();
            updateMem(toInt63(m_SP[0]) + 1);
            updateIOGas();

            m_mem[(unsigned)m_SP[0]] = (byte)(m_SP[1] & 0xff);
        }
        NEXT

        CASE(SHA3)
        {
            ON_OP();
            constexpr int64_t sha3Gas = VMSchedule::sha3Gas;
            constexpr int64_t sha3WordGas = VMSchedule::sha3WordGas;
            m_runGas = toInt63(sha3Gas + (u512(m_SP[1]) + 31) / 32 * sha3WordGas);
            updateMem(memNeed(m_SP[0], m_SP[1]));
            updateIOGas();

            uint64_t inOff = (uint64_t)m_SP[0];
            uint64_t inSize = (uint64_t)m_SP[1];
            m_SPP[0] = (u256)sha3(bytesConstRef(m_mem.data() + inOff, inSize));
        }
        NEXT

        CASE(LOG0)
        {
            ON_OP();
            if (m_message->flags & EVMC_STATIC)
                throwDisallowedStateChange();

            logGasMem();
            updateIOGas();

            uint8_t const* data = m_mem.data() + size_t(m_SP[0]);
            size_t dataSize = size_t(m_SP[1]);

            m_context->host->emit_log(
                m_context, &m_message->destination, data, dataSize, nullptr, 0);
        }
        NEXT

        CASE(LOG1)
        {
            ON_OP();
            if (m_message->flags & EVMC_STATIC)
                throwDisallowedStateChange();

            logGasMem();
            updateIOGas();

            uint8_t const* data = m_mem.data() + size_t(m_SP[0]);
            size_t dataSize = size_t(m_SP[1]);

            evmc_uint256be topics[] = {toEvmC(m_SP[2])};
            size_t numTopics = sizeof(topics) / sizeof(topics[0]);

            m_context->host->emit_log(
                m_context, &m_message->destination, data, dataSize, topics, numTopics);
        }
        NEXT

        CASE(LOG2)
        {
            ON_OP();
            if (m_message->flags & EVMC_STATIC)
                throwDisallowedStateChange();

            logGasMem();
            updateIOGas();

            uint8_t const* data = m_mem.data() + size_t(m_SP[0]);
            size_t dataSize = size_t(m_SP[1]);

            evmc_uint256be topics[] = {toEvmC(m_SP[2]), toEvmC(m_SP[3])};
            size_t numTopics = sizeof(topics) / sizeof(topics[0]);

            m_context->host->emit_log(
                m_context, &m_message->destination, data, dataSize, topics, numTopics);
        }
        NEXT

        CASE(LOG3)
        {
            ON_OP();
            if (m_message->flags & EVMC_STATIC)
                throwDisallowedStateChange();

            logGasMem();
            updateIOGas();

            uint8_t const* data = m_mem.data() + size_t(m_SP[0]);
            size_t dataSize = size_t(m_SP[1]);

            evmc_uint256be topics[] = {toEvmC(m_SP[2]), toEvmC(m_SP[3]), toEvmC(m_SP[4])};
            size_t numTopics = sizeof(topics) / sizeof(topics[0]);

            m_context->host->emit_log(
                m_context, &m_message->destination, data, dataSize, topics, numTopics);
        }
        NEXT

        CASE(LOG4)
        {
            ON_OP();
            if (m_message->flags & EVMC_STATIC)
                throwDisallowedStateChange();

            logGasMem();
            updateIOGas();

            uint8_t const* data = m_mem.data() + size_t(m_SP[0]);
            size_t dataSize = size_t(m_SP[1]);

            evmc_uint256be topics[] = {
                toEvmC(m_SP[2]), toEvmC(m_SP[3]), toEvmC(m_SP[4]), toEvmC(m_SP[5])};
            size_t numTopics = sizeof(topics) / sizeof(topics[0]);

            m_context->host->emit_log(
                m_context, &m_message->destination, data, dataSize, topics, numTopics);
        }
        NEXT

        CASE(EXP)
        {
            u256 expon = m_SP[1];
            const int64_t byteCost = m_rev >= EVMC_SPURIOUS_DRAGON ? 50 : 10;
            m_runGas = toInt63(VMSchedule::stepGas5 + byteCost * (32 - (h256(expon).firstBitSet() / 8)));
            ON_OP();
            updateIOGas();

            u256 base = m_SP[0];
            m_SPP[0] = exp256(base, expon);
        }
        NEXT

        //
        // ordinary instructions
        //

        CASE(ADD)
        {
            ON_OP();
            updateIOGas();

            //pops two items and pushes their sum mod 2^256.
            m_SPP[0] = m_SP[0] + m_SP[1];
        }
        NEXT

        CASE(MUL)
        {
            ON_OP();
            updateIOGas();

            //pops two items and pushes their product mod 2^256.
            m_SPP[0] = m_SP[0] * m_SP[1];
        }
        NEXT

        CASE(SUB)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_SP[0] - m_SP[1];
        }
        NEXT

        CASE(DIV)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_SP[1] ? divWorkaround(m_SP[0], m_SP[1]) : 0;
        }
        NEXT

        CASE(SDIV)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_SP[1] ? s2u(divWorkaround(u2s(m_SP[0]), u2s(m_SP[1]))) : 0;
            --m_SP;
        }
        NEXT

        CASE(MOD)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_SP[1] ? modWorkaround(m_SP[0], m_SP[1]) : 0;
        }
        NEXT

        CASE(SMOD)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_SP[1] ? s2u(modWorkaround(u2s(m_SP[0]), u2s(m_SP[1]))) : 0;
        }
        NEXT

        CASE(NOT)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = ~m_SP[0];
        }
        NEXT

        CASE(LT)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_SP[0] < m_SP[1] ? 1 : 0;
        }
        NEXT

        CASE(GT)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_SP[0] > m_SP[1] ? 1 : 0;
        }
        NEXT

        CASE(SLT)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = u2s(m_SP[0]) < u2s(m_SP[1]) ? 1 : 0;
        }
        NEXT

        CASE(SGT)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = u2s(m_SP[0]) > u2s(m_SP[1]) ? 1 : 0;
        }
        NEXT

        CASE(EQ)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_SP[0] == m_SP[1] ? 1 : 0;
        }
        NEXT

        CASE(ISZERO)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_SP[0] ? 0 : 1;
        }
        NEXT

        CASE(AND)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_SP[0] & m_SP[1];
        }
        NEXT

        CASE(OR)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_SP[0] | m_SP[1];
        }
        NEXT

        CASE(XOR)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_SP[0] ^ m_SP[1];
        }
        NEXT

        CASE(BYTE)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_SP[0] < 32 ? (m_SP[1] >> (unsigned)(8 * (31 - m_SP[0]))) & 0xff : 0;
        }
        NEXT

        CASE(SHL)
        {
            // Pre-constantinople
            if (m_rev < EVMC_CONSTANTINOPLE)
                throwBadInstruction();

            ON_OP();
            updateIOGas();

            if (m_SP[0] >= 256)
                m_SPP[0] = 0;
            else
                m_SPP[0] = m_SP[1] << unsigned(m_SP[0]);
        }
        NEXT

        CASE(SHR)
        {
            // Pre-constantinople
            if (m_rev < EVMC_CONSTANTINOPLE)
                throwBadInstruction();

            ON_OP();
            updateIOGas();

            if (m_SP[0] >= 256)
                m_SPP[0] = 0;
            else
                m_SPP[0] = m_SP[1] >> unsigned(m_SP[0]);
        }
        NEXT

        CASE(SAR)
        {
            // Pre-constantinople
            if (m_rev < EVMC_CONSTANTINOPLE)
                throwBadInstruction();

            ON_OP();
            updateIOGas();

            static u256 const hibit = u256(1) << 255;
            static u256 const allbits =
                u256("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

            u256 shiftee = m_SP[1];
            if (m_SP[0] >= 256)
            {
                if (shiftee & hibit)
                    m_SPP[0] = allbits;
                else
                    m_SPP[0] = 0;
            }
            else
            {
                unsigned amount = unsigned(m_SP[0]);
                m_SPP[0] = shiftee >> amount;
                if (shiftee & hibit)
                    m_SPP[0] |= allbits << (256 - amount);
            }
        }
        NEXT

        CASE(ADDMOD)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_SP[2] ? u256((u512(m_SP[0]) + u512(m_SP[1])) % m_SP[2]) : 0;
        }
        NEXT

        CASE(MULMOD)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_SP[2] ? u256((u512(m_SP[0]) * u512(m_SP[1])) % m_SP[2]) : 0;
        }
        NEXT

        CASE(SIGNEXTEND)
        {
            ON_OP();
            updateIOGas();

            if (m_SP[0] < 31)
            {
                unsigned testBit = static_cast<unsigned>(m_SP[0]) * 8 + 7;
                u256& number = m_SP[1];
                u256 mask = ((u256(1) << testBit) - 1);
                if (boost::multiprecision::bit_test(number, testBit))
                    number |= ~mask;
                else
                    number &= mask;
            }
        }
        NEXT

        CASE(JUMPTO)
        CASE(JUMPIF)
        CASE(JUMPV)
        CASE(JUMPSUB)
        CASE(JUMPSUBV)
        CASE(RETURNSUB)
        CASE(BEGINSUB)
        CASE(BEGINDATA)
        CASE(GETLOCAL)
        CASE(PUTLOCAL)
        {
            throwBadInstruction();
        }
        CONTINUE

        CASE(XADD)
        CASE(XMUL)
        CASE(XSUB)
        CASE(XDIV)
        CASE(XSDIV)
        CASE(XMOD)
        CASE(XSMOD)
        CASE(XLT)
        CASE(XGT)
        CASE(XSLT)
        CASE(XSGT)
        CASE(XEQ)
        CASE(XISZERO)
        CASE(XAND)
        CASE(XOOR)
        CASE(XXOR)
        CASE(XNOT)
        CASE(XSHL)
        CASE(XSHR)
        CASE(XSAR)
        CASE(XROL)
        CASE(XROR)
        CASE(XMLOAD)
        CASE(XMSTORE)
        CASE(XSLOAD)
        CASE(XSSTORE)
        CASE(XVTOWIDE)
        CASE(XWIDETOV)
        CASE(XPUSH)
        CASE(XPUT)
        CASE(XGET)
        CASE(XSWIZZLE)
        CASE(XSHUFFLE)
        {
            throwBadInstruction();
        }
        CONTINUE

        CASE(ADDRESS)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = fromAddress(fromEvmC(m_message->destination));
        }
        NEXT

        CASE(ORIGIN)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = fromAddress(fromEvmC(getTxContext().tx_origin));
        }
        NEXT

        CASE(BALANCE)
        {
            m_runGas = m_rev >= EVMC_TANGERINE_WHISTLE ? 400 : 20;
            ON_OP();
            updateIOGas();

            evmc_address address = toEvmC(asAddress(m_SP[0]));
            m_SPP[0] = fromEvmC(m_context->host->get_balance(m_context, &address));
        }
        NEXT


        CASE(CALLER)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = fromAddress(fromEvmC(m_message->sender));
        }
        NEXT

        CASE(CALLVALUE)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = fromEvmC(m_message->value);
        }
        NEXT


        CASE(CALLDATALOAD)
        {
            ON_OP();
            updateIOGas();

            size_t const dataSize = m_message->input_size;
            uint8_t const* const data = m_message->input_data;

            if (u512(m_SP[0]) + 31 < dataSize)
                m_SP[0] = (u256)*(h256 const*)(data + (size_t)m_SP[0]);
            else if (m_SP[0] >= dataSize)
                m_SP[0] = u256(0);
            else
            {     h256 r;
                for (uint64_t i = (uint64_t)m_SP[0], e = (uint64_t)m_SP[0] + (uint64_t)32, j = 0; i < e; ++i, ++j)
                    r[j] = i < dataSize ? data[i] : 0;
                m_SP[0] = (u256)r;
            };
        }
        NEXT


        CASE(CALLDATASIZE)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_message->input_size;
        }
        NEXT

        CASE(RETURNDATASIZE)
        {
            if (m_rev < EVMC_BYZANTIUM)
                throwBadInstruction();

            ON_OP();
            updateIOGas();

            m_SPP[0] = m_returnData.size();
        }
        NEXT

        CASE(CODESIZE)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_codeSize;
        }
        NEXT

        CASE(EXTCODESIZE)
        {
            m_runGas = m_rev >= EVMC_TANGERINE_WHISTLE ? 700 : 20;
            ON_OP();
            updateIOGas();

            evmc_address address = toEvmC(asAddress(m_SP[0]));

            m_SPP[0] = m_context->host->get_code_size(m_context, &address);
        }
        NEXT

        CASE(CALLDATACOPY)
        {
            ON_OP();
            m_copyMemSize = toInt63(m_SP[2]);
            updateMem(memNeed(m_SP[0], m_SP[2]));
            updateIOGas();

            bytesConstRef data{m_message->input_data, m_message->input_size};
            copyDataToMemory(data, m_SP);
        }
        NEXT

        CASE(RETURNDATACOPY)
        {
            ON_OP();
            if (m_rev < EVMC_BYZANTIUM)
                throwBadInstruction();
            bigint const endOfAccess = bigint(m_SP[1]) + bigint(m_SP[2]);
            if (m_returnData.size() < endOfAccess)
                throwBufferOverrun(endOfAccess);

            m_copyMemSize = toInt63(m_SP[2]);
            updateMem(memNeed(m_SP[0], m_SP[2]));
            updateIOGas();

            copyDataToMemory(&m_returnData, m_SP);
        }
        NEXT

        CASE(EXTCODEHASH)
        {
            ON_OP();
            if (m_rev < EVMC_CONSTANTINOPLE)
                throwBadInstruction();

            updateIOGas();

            evmc_address address = toEvmC(asAddress(m_SP[0]));
            m_SPP[0] = fromEvmC(m_context->host->get_code_hash(m_context, &address));
        }
        NEXT

        CASE(CODECOPY)
        {
            ON_OP();
            m_copyMemSize = toInt63(m_SP[2]);
            updateMem(memNeed(m_SP[0], m_SP[2]));
            updateIOGas();

            copyDataToMemory({m_pCode, m_codeSize}, m_SP);
        }
        NEXT

        CASE(EXTCODECOPY)
        {
            ON_OP();
            m_runGas = m_rev >= EVMC_TANGERINE_WHISTLE ? 700 : 20;
            uint64_t copyMemSize = toInt63(m_SP[3]);
            m_copyMemSize = copyMemSize;
            updateMem(memNeed(m_SP[1], m_SP[3]));
            updateIOGas();

            evmc_address address = toEvmC(asAddress(m_SP[0]));

            size_t memoryOffset = static_cast<size_t>(m_SP[1]);
            constexpr size_t codeOffsetMax = std::numeric_limits<size_t>::max();
            size_t codeOffset =
                m_SP[2] > codeOffsetMax ? codeOffsetMax : static_cast<size_t>(m_SP[2]);
            size_t size = static_cast<size_t>(copyMemSize);

            size_t numCopied = m_context->host->copy_code(
                m_context, &address, codeOffset, &m_mem[memoryOffset], size);

            std::fill_n(&m_mem[memoryOffset + numCopied], size - numCopied, 0);
        }
        NEXT


        CASE(GASPRICE)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = fromEvmC(getTxContext().tx_gas_price);
        }
        NEXT

        CASE(BLOCKHASH)
        {
            ON_OP();
            m_runGas = VMSchedule::stepGas6;
            updateIOGas();

            const int64_t blockNumber = getTxContext().block_number;
            u256 number = m_SP[0];

            if (number < blockNumber && number >= std::max(int64_t(256), blockNumber) - 256)
            {
                m_SPP[0] = fromEvmC(m_context->host->get_block_hash(m_context, int64_t(number)));
            }
            else
                m_SPP[0] = 0;
        }
        NEXT

        CASE(COINBASE)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = fromAddress(fromEvmC(getTxContext().block_coinbase));
        }
        NEXT

        CASE(TIMESTAMP)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = getTxContext().block_timestamp;
        }
        NEXT

        CASE(NUMBER)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = getTxContext().block_number;
        }
        NEXT

        CASE(DIFFICULTY)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = fromEvmC(getTxContext().block_difficulty);
        }
        NEXT

        CASE(GASLIMIT)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = getTxContext().block_gas_limit;
        }
        NEXT

        CASE(POP)
        {
            ON_OP();
            updateIOGas();

            --m_SP;
        }
        NEXT

        CASE(PUSHC)
        {
#if EVM_USE_CONSTANT_POOL
            ON_OP();
            updateIOGas();

            // get val at two-byte offset into const pool and advance pc by one-byte remainder
            TRACE_OP(2, m_PC, m_OP);
            unsigned off;
            ++m_PC;
            off = m_code[m_PC++] << 8;
            off |= m_code[m_PC++];
            m_PC += m_code[m_PC];
            m_SPP[0] = m_pool[off];
            TRACE_VAL(2, "Retrieved pooled const", m_SPP[0]);
#else
            throwBadInstruction();
#endif
        }
        CONTINUE

        CASE(PUSH1)
        {
            ON_OP();
            updateIOGas();
            m_SPP[0] = m_code[m_PC + 1];
            m_PC += 2;
        }
        CONTINUE

        CASE(PUSH2)
        CASE(PUSH3)
        CASE(PUSH4)
        CASE(PUSH5)
        CASE(PUSH6)
        CASE(PUSH7)
        CASE(PUSH8)
        CASE(PUSH9)
        CASE(PUSH10)
        CASE(PUSH11)
        CASE(PUSH12)
        CASE(PUSH13)
        CASE(PUSH14)
        CASE(PUSH15)
        CASE(PUSH16)
        CASE(PUSH17)
        CASE(PUSH18)
        CASE(PUSH19)
        CASE(PUSH20)
        CASE(PUSH21)
        CASE(PUSH22)
        CASE(PUSH23)
        CASE(PUSH24)
        CASE(PUSH25)
        CASE(PUSH26)
        CASE(PUSH27)
        CASE(PUSH28)
        CASE(PUSH29)
        CASE(PUSH30)
        CASE(PUSH31)
        CASE(PUSH32)
        {
            ON_OP();
            updateIOGas();

            int numBytes = (int)m_OP - (int)Instruction::PUSH1 + 1;
            m_SPP[0] = 0;
            // Construct a number out of PUSH bytes.
            // This requires the code has been copied and extended by 32 zero
            // bytes to handle "out of code" push data here.
            uint64_t codeOffset = m_PC + 1;
            for (; numBytes--; ++codeOffset)
                m_SPP[0] = (m_SPP[0] << 8) | m_code[codeOffset];

            m_PC = codeOffset;
        }
        CONTINUE

        CASE(JUMP)
        {
            ON_OP();
            updateIOGas();
            m_PC = verifyJumpDest(m_SP[0]);
        }
        CONTINUE

        CASE(JUMPI)
        {
            ON_OP();
            updateIOGas();
            if (m_SP[1])
                m_PC = verifyJumpDest(m_SP[0]);
            else
                ++m_PC;
        }
        CONTINUE

        CASE(JUMPC)
        {
#if EVM_REPLACE_CONST_JUMP
            ON_OP();
            updateIOGas();

            m_PC = uint64_t(m_SP[0]);
#else
            throwBadInstruction();
#endif
        }
        CONTINUE

        CASE(JUMPCI)
        {
#if EVM_REPLACE_CONST_JUMP
            ON_OP();
            updateIOGas();

            if (m_SP[1])
                m_PC = uint64_t(m_SP[0]);
            else
                ++m_PC;
#else
            throwBadInstruction();
#endif
        }
        CONTINUE

        CASE(DUP1)
        CASE(DUP2)
        CASE(DUP3)
        CASE(DUP4)
        CASE(DUP5)
        CASE(DUP6)
        CASE(DUP7)
        CASE(DUP8)
        CASE(DUP9)
        CASE(DUP10)
        CASE(DUP11)
        CASE(DUP12)
        CASE(DUP13)
        CASE(DUP14)
        CASE(DUP15)
        CASE(DUP16)
        {
            ON_OP();
            updateIOGas();

            unsigned n = (unsigned)m_OP - (unsigned)Instruction::DUP1;
            *(uint64_t*)m_SPP = *(uint64_t*)(m_SP + n);

            // the stack slot being copied into may no longer hold a u256
            // so we construct a new one in the memory, rather than assign
            new(m_SPP) u256(m_SP[n]);
        }
        NEXT


        CASE(SWAP1)
        CASE(SWAP2)
        CASE(SWAP3)
        CASE(SWAP4)
        CASE(SWAP5)
        CASE(SWAP6)
        CASE(SWAP7)
        CASE(SWAP8)
        CASE(SWAP9)
        CASE(SWAP10)
        CASE(SWAP11)
        CASE(SWAP12)
        CASE(SWAP13)
        CASE(SWAP14)
        CASE(SWAP15)
        CASE(SWAP16)
        {
            ON_OP();
            updateIOGas();

            unsigned n = (unsigned)m_OP - (unsigned)Instruction::SWAP1 + 1;
            std::swap(m_SP[0], m_SP[n]);
        }
        NEXT


        CASE(SLOAD)
        {
            m_runGas = m_rev >= EVMC_TANGERINE_WHISTLE ? 200 : 50;
            ON_OP();
            updateIOGas();

            evmc_uint256be key = toEvmC(m_SP[0]);
            m_SPP[0] =
                fromEvmC(m_context->host->get_storage(m_context, &m_message->destination, &key));
        }
        NEXT

        CASE(SSTORE)
        {
            ON_OP();
            if (m_message->flags & EVMC_STATIC)
                throwDisallowedStateChange();

            evmc_uint256be const key = toEvmC(m_SP[0]);
            evmc_uint256be const value = toEvmC(m_SP[1]);
            auto const status =
                m_context->host->set_storage(m_context, &m_message->destination, &key, &value);

            if (status == EVMC_STORAGE_ADDED)
                m_runGas = VMSchedule::sstoreSetGas;
            else if (status == EVMC_STORAGE_MODIFIED || status == EVMC_STORAGE_DELETED)
                m_runGas = VMSchedule::sstoreResetGas;
            else if (status == EVMC_STORAGE_UNCHANGED && m_rev < EVMC_CONSTANTINOPLE)
                m_runGas = VMSchedule::sstoreResetGas;
            else
            {
                assert(status == EVMC_STORAGE_UNCHANGED || status == EVMC_STORAGE_MODIFIED_AGAIN);
                assert(m_rev >= EVMC_CONSTANTINOPLE);
                m_runGas = VMSchedule::sstoreUnchangedGas;
            }

            updateIOGas();
        }
        NEXT

        CASE(PC)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_PC;
        }
        NEXT

        CASE(MSIZE)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_mem.size();
        }
        NEXT

        CASE(GAS)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_io_gas;
        }
        NEXT

        CASE(JUMPDEST)
        {
            m_runGas = VMSchedule::jumpdestGas;
            ON_OP();
            updateIOGas();
        }
        NEXT

        CASE(INVALID)
        DEFAULT
        {
            throwBadInstruction();
        }
    }
    WHILE_CASES
}
}
}