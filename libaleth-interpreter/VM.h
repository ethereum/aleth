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

#pragma once

#include "VMConfig.h"

#include <libevm/VMFace.h>

#include <evmc/evmc.h>
#include <evmc/instructions.h>

#include <boost/optional.hpp>

namespace dev
{
namespace eth
{

struct VMSchedule
{
    static constexpr int64_t stackLimit = 1024;
    static constexpr int64_t stepGas0 = 0;
    static constexpr int64_t stepGas1 = 2;
    static constexpr int64_t stepGas2 = 3;
    static constexpr int64_t stepGas3 = 5;
    static constexpr int64_t stepGas4 = 8;
    static constexpr int64_t stepGas5 = 10;
    static constexpr int64_t stepGas6 = 20;
    static constexpr int64_t sha3Gas = 30;
    static constexpr int64_t sha3WordGas = 6;
    static constexpr int64_t sloadGas = 50;
    static constexpr int64_t sstoreSetGas = 20000;
    static constexpr int64_t sstoreResetGas = 5000;
    static constexpr int64_t sstoreUnchangedGas = 200;
    static constexpr int64_t jumpdestGas = 1;
    static constexpr int64_t logGas = 375;
    static constexpr int64_t logDataGas = 8;
    static constexpr int64_t logTopicGas = 375;
    static constexpr int64_t createGas = 32000;
    static constexpr int64_t memoryGas = 3;
    static constexpr int64_t quadCoeffDiv = 512;
    static constexpr int64_t copyGas = 3;
    static constexpr int64_t valueTransferGas = 9000;
    static constexpr int64_t callStipend = 2300;
    static constexpr int64_t callNewAccount = 25000;
};

class VM
{
public:
    VM() = default;

    owning_bytes_ref exec(evmc_context* _context, evmc_revision _rev, const evmc_message* _msg,
        uint8_t const* _code, size_t _codeSize, evmc_trace_callback _traceCallback,
        evmc_tracer_context* _traceContext);

    uint64_t m_io_gas = 0;
private:
    evmc_context* m_context = nullptr;
    evmc_revision m_rev = EVMC_FRONTIER;
    evmc_message const* m_message = nullptr;
    boost::optional<evmc_tx_context> m_tx_context;

    static std::array<evmc_instruction_metrics, 256> c_metrics;
    static void initMetrics();
    static u256 exp256(u256 _base, u256 _exponent);
    void copyCode(int);
    typedef void (VM::*MemFnPtr)();
    MemFnPtr m_bounce = nullptr;

    // return bytes
    owning_bytes_ref m_output;

    // space for memory
    bytes m_mem;

    uint8_t const* m_pCode = nullptr;
    size_t m_codeSize = 0;
    // space for code
    bytes m_code;

    /// RETURNDATA buffer for memory returned from direct subcalls.
    bytes m_returnData;

    // space for data stack, grows towards smaller addresses from the end
    u256 m_stack[VMSchedule::stackLimit];
    u256 *m_stackEnd = &m_stack[VMSchedule::stackLimit];
    size_t stackSize() { return m_stackEnd - m_SP; }
    
    // constant pool
    std::vector<u256> m_pool;

    // interpreter state
    Instruction m_OP;         // current operation
    uint64_t m_PC = 0;        // program counter
    u256* m_SP = m_stackEnd;  // stack pointer
    u256* m_SPP = m_SP;       // stack pointer prime (next SP)

    // metering and memory state
    uint64_t m_runGas = 0;
    uint64_t m_newMemSize = 0;
    uint64_t m_copyMemSize = 0;

    evmc_trace_callback m_traceCallback = nullptr;
    evmc_tracer_context* m_traceContext = nullptr;
    void trace(uint64_t _pc) noexcept;

    // initialize interpreter
    void initEntry();
    void optimize();

    // interpreter loop & switch
    void interpretCases();

    // interpreter cases that call out
    void caseCreate();
    bool caseCallSetup(evmc_message& _msg, bytesRef& o_output);
    void caseCall();

    void copyDataToMemory(bytesConstRef _data, u256*_sp);
    uint64_t memNeed(u256 _offset, u256 _size);

    const evmc_tx_context& getTxContext();

    void throwOutOfGas();
    void throwBadInstruction();
    void throwBadJumpDestination();
    void throwBadStack(int _removed, int _added);
    void throwRevertInstruction(owning_bytes_ref&& _output);
    void throwDisallowedStateChange();
    void throwBufferOverrun(bigint const& _enfOfAccess);

    std::vector<uint64_t> m_beginSubs;
    std::vector<uint64_t> m_jumpDests;
    int64_t verifyJumpDest(u256 const& _dest, bool _throw = true);

    void onOperation() {}
    void adjustStack(int _removed, int _added);
    uint64_t gasForMem(u512 _size);
    void updateIOGas();
    void updateGas();
    void updateMem(uint64_t _newMem);
    void logGasMem();
    void fetchInstruction();
    
    uint64_t decodeJumpDest(const byte* const _code, uint64_t& _pc);
    uint64_t decodeJumpvDest(const byte* const _code, uint64_t& _pc, byte _voff);

    template<class T> uint64_t toInt63(T v)
    {
        // check for overflow
        if (v > 0x7FFFFFFFFFFFFFFF)
            throwOutOfGas();
        uint64_t w = uint64_t(v);
        return w;
    }
    
    template<class T> uint64_t toInt15(T v)
    {
        // check for overflow
        if (v > 0x7FFF)
            throwOutOfGas();
        uint64_t w = uint64_t(v);
        return w;
    }
};

}
}
