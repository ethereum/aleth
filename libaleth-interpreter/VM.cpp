// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
#include "interpreter.h"
#include "VM.h"

#include <aleth/version.h>

namespace
{
void destroy(evmc_vm* _instance)
{
    (void)_instance;
}

evmc_capabilities_flagset getCapabilities(evmc_vm* _instance) noexcept
{
    (void)_instance;
    return EVMC_CAPABILITY_EVM1;
}

void delete_output(const evmc_result* result)
{
    delete[] result->output_data;
}

evmc_result execute(evmc_vm* _instance, const evmc_host_interface* _host,
    evmc_host_context* _context, evmc_revision _rev, const evmc_message* _msg, uint8_t const* _code,
    size_t _codeSize) noexcept
{
    (void)_instance;
    std::unique_ptr<dev::eth::VM> vm{new dev::eth::VM};

    evmc_result result = {};
    dev::eth::owning_bytes_ref output;

    try
    {
        output = vm->exec(_host, _context, _rev, _msg, _code, _codeSize);
        result.status_code = EVMC_SUCCESS;
        result.gas_left = vm->m_io_gas;
        result.gas_refunded = vm->m_gas_refunded;
    }
    catch (dev::eth::RevertInstruction& ex)
    {
        result.status_code = EVMC_REVERT;
        result.gas_left = vm->m_io_gas;
        output = ex.output();  // This moves the output from the exception!
    }
    catch (dev::eth::InvalidInstruction const&)
    {
        result.status_code = EVMC_INVALID_INSTRUCTION;
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
}  // namespace

extern "C" evmc_vm* evmc_create_aleth_interpreter() noexcept
{
    // TODO: Allow creating multiple instances with different configurations.
    static evmc_vm s_vm{
        EVMC_ABI_VERSION, "interpreter", aleth_version, ::destroy, ::execute, getCapabilities,
        nullptr,  // set_option
    };
    static bool metricsInited = dev::eth::VM::initMetrics();
    (void)metricsInited;

    return &s_vm;
}


namespace dev
{
namespace eth
{
uint64_t VM::memNeed(intx::uint256 const& _offset, intx::uint256 const& _size)
{
    return toInt63(_size ? intx::uint512(_offset) + _size : intx::uint512(0));
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
void VM::adjustStack(int _required, int _change)
{
    m_SP = m_SPP;

    // adjust stack and check bounds
    if (m_stackEnd < m_SP + _required)
        throwBadStack(_required, _change);
    m_SPP -= _change;
    if (m_SPP < m_stack)
        throwBadStack(_required, _change);
}

uint64_t VM::gasForMem(intx::uint512 const& _size)
{
    constexpr int64_t memoryGas = VMSchedule::memoryGas;
    constexpr int64_t quadCoeffDiv = VMSchedule::quadCoeffDiv;
    intx::uint512 s = _size / 32;
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
        VMSchedule::logGas + VMSchedule::logTopicGas * n + logDataGas * intx::uint512(m_SP[1]));
    updateMem(memNeed(m_SP[0], m_SP[1]));
}

void VM::fetchInstruction()
{
    m_OP = Instruction(m_code[m_PC]);
    auto const metric = (*m_metrics)[static_cast<size_t>(m_OP)];
    adjustStack(metric.stack_height_required, metric.stack_height_change);

    // FEES...
    m_runGas = metric.gas_cost;
    m_newMemSize = m_mem.size();
    m_copyMemSize = 0;
}

evmc_tx_context const& VM::getTxContext()
{
    if (!m_tx_context)
        m_tx_context.emplace(m_host->get_tx_context(m_context));
    return m_tx_context.value();
}


///////////////////////////////////////////////////////////////////////////////
//
// interpreter entry point

owning_bytes_ref VM::exec(const evmc_host_interface* _host, evmc_host_context* _context,
    evmc_revision _rev, const evmc_message* _msg, uint8_t const* _code, size_t _codeSize)
{
    m_host = _host;
    m_context = _context;
    m_rev = _rev;
    m_metrics = &s_metrics[m_rev];
    m_message = _msg;
    m_io_gas = uint64_t(_msg->gas);
    m_PC = 0;
    m_pCode = _code;
    m_codeSize = _codeSize;

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

        CASE(SELFDESTRUCT)
        {
            ON_OP();
            if (m_message->flags & EVMC_STATIC)
                throwDisallowedStateChange();

            updateIOGas();

            auto const destination = intx::be::trunc<evmc::address>(m_SP[0]);

            // Starting with EIP150 (Tangerine Whistle), self-destructs need to pay account creation
            // gas. Starting with EIP158 (Spurious Dragon),
            // 0-value selfdestructs don't have to pay this charge.
            if (m_rev >= EVMC_TANGERINE_WHISTLE)
            {
                if (m_rev == EVMC_TANGERINE_WHISTLE ||
                    fromEvmC(m_host->get_balance(m_context, &m_message->destination)) > 0)
                {
                    if (!m_host->account_exists(m_context, &destination))
                    {
                        m_runGas = VMSchedule::callNewAccount;
                        updateIOGas();
                    }
                }
            }

            std::cerr << "SELFD " << fromEvmC(m_message->destination);
            if (m_host->selfdestruct(m_context, &m_message->destination, &destination))
            {
                m_gas_refunded += 15000;
                std::cerr << " REF";
            }
            std::cerr << "\n";
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

            m_SPP[0] = intx::be::unsafe::load<intx::uint256>(m_mem.data() + (unsigned)m_SP[0]);
        }
        NEXT

        CASE(MSTORE)
        {
            ON_OP();
            updateMem(toInt63(m_SP[0]) + 32);
            updateIOGas();

            intx::be::unsafe::store(&m_mem[(unsigned)m_SP[0]], m_SP[1]);
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
            m_runGas = toInt63(sha3Gas + (intx::uint512(m_SP[1]) + 31) / 32 * sha3WordGas);
            updateMem(memNeed(m_SP[0], m_SP[1]));
            updateIOGas();

            uint64_t inOff = (uint64_t)m_SP[0];
            uint64_t inSize = (uint64_t)m_SP[1];

            const auto h = ethash::keccak256(m_mem.data() + inOff, inSize);
            m_SPP[0] = intx::be::load<intx::uint256>(h);
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

            m_host->emit_log(m_context, &m_message->destination, data, dataSize, nullptr, 0);
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

            evmc_uint256be topics[] = {intx::be::store<evmc_uint256be>(m_SP[2])};
            size_t numTopics = sizeof(topics) / sizeof(topics[0]);

            m_host->emit_log(m_context, &m_message->destination, data, dataSize, topics, numTopics);
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

            evmc_uint256be topics[] = {
                intx::be::store<evmc_uint256be>(m_SP[2]),
                intx::be::store<evmc_uint256be>(m_SP[3])
            };
            size_t numTopics = sizeof(topics) / sizeof(topics[0]);

            m_host->emit_log(m_context, &m_message->destination, data, dataSize, topics, numTopics);
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

            evmc_uint256be topics[] = {
                intx::be::store<evmc_uint256be>(m_SP[2]),
                intx::be::store<evmc_uint256be>(m_SP[3]),
                intx::be::store<evmc_uint256be>(m_SP[4])
            };
            size_t numTopics = sizeof(topics) / sizeof(topics[0]);

            m_host->emit_log(m_context, &m_message->destination, data, dataSize, topics, numTopics);
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
                intx::be::store<evmc_uint256be>(m_SP[2]),
                intx::be::store<evmc_uint256be>(m_SP[3]),
                intx::be::store<evmc_uint256be>(m_SP[4]),
                intx::be::store<evmc_uint256be>(m_SP[5])
            };
            size_t numTopics = sizeof(topics) / sizeof(topics[0]);

            m_host->emit_log(m_context, &m_message->destination, data, dataSize, topics, numTopics);
        }
        NEXT

        CASE(EXP)
        {
            intx::uint256 expon = m_SP[1];
            const int64_t byteCost = m_rev >= EVMC_SPURIOUS_DRAGON ? 50 : 10;
            m_runGas = toInt63(VMSchedule::stepGas5 + byteCost * intx::count_significant_words<uint8_t>(expon));
            ON_OP();
            updateIOGas();

            intx::uint256 base = m_SP[0];
            m_SPP[0] = intx::exp(base, expon);
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

            m_SPP[0] = m_SP[1] ? m_SP[0] / m_SP[1] : 0;
        }
        NEXT

        CASE(SDIV)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_SP[1] ? intx::sdivrem(m_SP[0], m_SP[1]).quot : 0;
            --m_SP;
        }
        NEXT

        CASE(MOD)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_SP[1] ? m_SP[0] % m_SP[1] : 0;
        }
        NEXT

        CASE(SMOD)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_SP[1] ? intx::sdivrem(m_SP[0], m_SP[1]).rem : 0;
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

            bool const lhsNeg = static_cast<bool>(m_SP[0] >> 255);
            bool const rhsNeg = static_cast<bool>(m_SP[1] >> 255);
            m_SPP[0] = (lhsNeg != rhsNeg) ? lhsNeg : m_SP[0] < m_SP[1];
        }
        NEXT

        CASE(SGT)
        {
            ON_OP();
            updateIOGas();

            bool const lhsNeg = static_cast<bool>(m_SP[0] >> 255);
            bool const rhsNeg = static_cast<bool>(m_SP[1] >> 255);
            m_SPP[0] = (lhsNeg != rhsNeg) ? rhsNeg : m_SP[0] > m_SP[1];
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

            using namespace intx;
            static constexpr uint256 hibit = 1_u256 << 255;
            static constexpr uint256 allbits = ~0_u256;

            uint256 shiftee = m_SP[1];
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

            m_SPP[0] = m_SP[2] ? intx::addmod(m_SP[0], m_SP[1], m_SP[2]) : 0;
        }
        NEXT

        CASE(MULMOD)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = m_SP[2] ? intx::mulmod(m_SP[0], m_SP[1], m_SP[2]) : 0;
        }
        NEXT

        CASE(SIGNEXTEND)
        {
            ON_OP();
            updateIOGas();

            if (m_SP[0] < 31)
            {
                using namespace intx;

                unsigned testBit = static_cast<unsigned>(m_SP[0]) * 8 + 7;
                uint256& number = m_SP[1];
                uint256 mask = ((1_u256 << testBit) - 1);
                if (number & (1_u256 << testBit))
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

            m_SPP[0] = intx::be::load<intx::uint256>(m_message->destination);
        }
        NEXT

        CASE(ORIGIN)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = intx::be::load<intx::uint256>(getTxContext().tx_origin);
        }
        NEXT

        CASE(BALANCE)
        {
            ON_OP();
            updateIOGas();

            auto const address = intx::be::trunc<evmc::address>(m_SP[0]);
            m_SPP[0] = intx::be::load<intx::uint256>(m_host->get_balance(m_context, &address));
        }
        NEXT


        CASE(CALLER)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = intx::be::load<intx::uint256>(m_message->sender);
        }
        NEXT

        CASE(CALLVALUE)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = intx::be::load<intx::uint256>(m_message->value);
        }
        NEXT


        CASE(CALLDATALOAD)
        {
            ON_OP();
            updateIOGas();

            size_t const dataSize = m_message->input_size;
            uint8_t const* const data = m_message->input_data;

            if (intx::uint512(m_SP[0]) + 31 < dataSize)
                m_SP[0] = intx::be::unsafe::load<intx::uint256>(data + (size_t)m_SP[0]);
            else if (m_SP[0] >= dataSize)
                m_SP[0] = 0;
            else
            {
                uint8_t r[32];
                for (uint64_t i = (uint64_t)m_SP[0], e = (uint64_t)m_SP[0] + (uint64_t)32, j = 0; i < e; ++i, ++j)
                    r[j] = i < dataSize ? data[i] : 0;
                m_SP[0] = intx::be::load<intx::uint256>(r);
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
            ON_OP();
            updateIOGas();

            auto const address = intx::be::trunc<evmc::address>(m_SP[0]);

            m_SPP[0] = m_host->get_code_size(m_context, &address);
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
            intx::uint512 const endOfAccess = intx::uint512(m_SP[1]) + intx::uint512(m_SP[2]);
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

            auto const address = intx::be::trunc<evmc::address>(m_SP[0]);
            m_SPP[0] = intx::be::load<intx::uint256>(m_host->get_code_hash(m_context, &address));
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
            uint64_t copyMemSize = toInt63(m_SP[3]);
            m_copyMemSize = copyMemSize;
            updateMem(memNeed(m_SP[1], m_SP[3]));
            updateIOGas();

            auto const address = intx::be::trunc<evmc::address>(m_SP[0]);

            size_t memoryOffset = static_cast<size_t>(m_SP[1]);
            constexpr size_t codeOffsetMax = std::numeric_limits<size_t>::max();
            size_t codeOffset =
                m_SP[2] > codeOffsetMax ? codeOffsetMax : static_cast<size_t>(m_SP[2]);
            size_t size = static_cast<size_t>(copyMemSize);

            size_t numCopied =
                m_host->copy_code(m_context, &address, codeOffset, &m_mem[memoryOffset], size);

            std::fill_n(&m_mem[memoryOffset + numCopied], size - numCopied, 0);
        }
        NEXT


        CASE(GASPRICE)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = intx::be::load<intx::uint256>(getTxContext().tx_gas_price);
        }
        NEXT

        CASE(BLOCKHASH)
        {
            ON_OP();
            m_runGas = VMSchedule::stepGas6;
            updateIOGas();

            const int64_t blockNumber = getTxContext().block_number;
            intx::uint256 number = m_SP[0];

            if (number < blockNumber && number >= std::max(int64_t(256), blockNumber) - 256)
            {
                m_SPP[0] = intx::be::load<intx::uint256>(m_host->get_block_hash(m_context, int64_t(number)));
            }
            else
                m_SPP[0] = 0;
        }
        NEXT

        CASE(COINBASE)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = intx::be::load<intx::uint256>(getTxContext().block_coinbase);
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

            m_SPP[0] = intx::be::load<intx::uint256>(getTxContext().block_difficulty);
        }
        NEXT

        CASE(GASLIMIT)
        {
            ON_OP();
            updateIOGas();

            m_SPP[0] = getTxContext().block_gas_limit;
        }
        NEXT


        CASE(CHAINID)
        {
            ON_OP();

            if (m_rev < EVMC_ISTANBUL)
                throwBadInstruction();

            updateIOGas();

            m_SPP[0] = intx::be::load<intx::uint256>(getTxContext().chain_id);
        }
        NEXT

        CASE(SELFBALANCE)
        {
            ON_OP();

            if (m_rev < EVMC_ISTANBUL)
                throwBadInstruction();

            updateIOGas();

            m_SPP[0] = intx::be::load<intx::uint256>(m_host->get_balance(m_context, &m_message->destination));
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
            ++m_PC;
            m_SPP[0] = m_code[m_PC];
            ++m_PC;
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
            for (++m_PC; numBytes--; ++m_PC)
                m_SPP[0] = (m_SPP[0] << 8) | m_code[m_PC];
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

            // the stack slot being copied into may no longer hold a uint256
            // so we construct a new one in the memory, rather than assign
            new(m_SPP) intx::uint256(m_SP[n]);
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
            ON_OP();
            updateIOGas();

            auto const key = intx::be::store<evmc_uint256be>(m_SP[0]);
            m_SPP[0] =
                intx::be::load<intx::uint256>(m_host->get_storage(m_context, &m_message->destination, &key));
        }
        NEXT

        CASE(SSTORE)
        {
            ON_OP();
            if (m_message->flags & EVMC_STATIC)
                throwDisallowedStateChange();

            if (m_rev >= EVMC_ISTANBUL && m_io_gas <= VMSchedule::callStipend)
                throwOutOfGas();

            auto const key = intx::be::store<evmc_uint256be>(m_SP[0]);
            auto const value = intx::be::store<evmc_uint256be>(m_SP[1]);
            auto const status =
                m_host->set_storage(m_context, &m_message->destination, &key, &value);

            const auto netStorageCost = (m_rev == EVMC_CONSTANTINOPLE || m_rev >= EVMC_ISTANBUL);
            if (!netStorageCost)
            {
                switch (status)
                {
                case EVMC_STORAGE_ADDED:
                    m_runGas = VMSchedule::sstoreSetGas;
                    break;
                case EVMC_STORAGE_DELETED:
                    m_runGas = VMSchedule::sstoreResetGas;
                    m_gas_refunded += VMSchedule::sstoreRefundGas;
                    break;
                default:
                    m_runGas = VMSchedule::sstoreResetGas;
                    break;
                }
            }
            else
            {
                const auto sstoreUnchangedGas = (*m_metrics)[OP_SLOAD].gas_cost;
                switch (status)
                {
                case EVMC_STORAGE_UNCHANGED:
                    m_runGas = sstoreUnchangedGas;
                    break;
                case EVMC_STORAGE_ADDED:
                    m_runGas = VMSchedule::sstoreSetGas;
                    break;
                case EVMC_STORAGE_MODIFIED:
                    m_runGas = VMSchedule::sstoreResetGas;
                    break;
                case EVMC_STORAGE_DELETED:
                    m_runGas = VMSchedule::sstoreResetGas;
                    m_gas_refunded += VMSchedule::sstoreRefundGas;
                    break;
                case EVMC_DIRTY_ADDED_TO_DELETED:
                    m_runGas = sstoreUnchangedGas;
                    m_gas_refunded = VMSchedule::sstoreSetGas - sstoreUnchangedGas;
                    break;
                case EVMC_DIRTY_DELETED_REVERTED:
                    m_runGas = sstoreUnchangedGas;
                    m_gas_refunded += VMSchedule::sstoreResetGas - sstoreUnchangedGas -
                                      VMSchedule::sstoreRefundGas;
                    break;
                case EVMC_DIRTY_DELETED_TO_ADDED:
                    m_runGas = sstoreUnchangedGas;
                    m_gas_refunded += -VMSchedule::sstoreRefundGas;
                    break;
                case EVMC_DIRTY_MODIFIED_TO_DELETED:
                    m_runGas = sstoreUnchangedGas;
                    m_gas_refunded += VMSchedule::sstoreRefundGas;
                    break;
                case EVMC_DIRTY_MODIFIED_REVERTED:
                    m_runGas = sstoreUnchangedGas;
                    m_gas_refunded += VMSchedule::sstoreResetGas - sstoreUnchangedGas;
                    break;
                case EVMC_DIRTY_MODIFIED_AGAIN:
                    m_runGas = sstoreUnchangedGas;
                    break;
                }
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
            if (m_OP == Instruction::INVALID)
                throwInvalidInstruction();
            else
                throwBadInstruction();
        }
    }
    WHILE_CASES
}
}
}
