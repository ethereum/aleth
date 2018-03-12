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

#include "Instruction.h"
#include "VMConfig.h"
#include "VMFace.h"
#include <evm.h>

namespace dev
{
namespace eth
{

class VM: public evm_instance
{
public:
    VM();

    owning_bytes_ref exec(u256& _io_gas, ExtVMFace& _ext);

#if EIP_615
    // invalid code will throw an exeption
    void validate(ExtVMFace& _ext);
    void validateSubroutine(uint64_t _PC, uint64_t* _rp, u256* _sp);
#endif

    bytes const& memory() const { return m_mem; }
    u256s stack() const {
        u256s stack(m_SP, m_stackEnd);
        reverse(stack.begin(), stack.end());
        return stack;
    };

private:

    u256* m_io_gas_p = 0;
    uint64_t m_io_gas = 0;
    ExtVMFace* m_ext = 0;

    static std::array<InstructionMetric, 256> c_metrics;
    static void initMetrics();
    static u256 exp256(u256 _base, u256 _exponent);
    void copyCode(int);
    typedef void (VM::*MemFnPtr)();
    MemFnPtr m_bounce = 0;
    MemFnPtr m_onFail = 0;
    uint64_t m_nSteps = 0;
    EVMSchedule const* m_schedule = nullptr;

    // return bytes
    owning_bytes_ref m_output;

    // space for memory
    bytes m_mem;

    // space for code
    bytes m_code;

    /// RETURNDATA buffer for memory returned from direct subcalls.
    bytes m_returnData;

    // space for data stack, grows towards smaller addresses from the end
    u256 m_stack[1024];
    u256 *m_stackEnd = &m_stack[1024];
    size_t stackSize() { return m_stackEnd - m_SP; }
    
#if EIP_615
    // space for return stack
    uint64_t m_return[1024];
    
    // mark PCs with frame size to detect cycles and stack mismatch
    std::vector<size_t> m_frameSize;
#endif

    // constant pool
    std::vector<u256> m_pool;

    // interpreter state
    Instruction m_OP;         // current operation
    uint64_t m_PC = 0;        // program counter
    u256* m_SP = m_stackEnd;  // stack pointer
    u256* m_SPP = m_SP;       // stack pointer prime (next SP)
#if EIP_615
    uint64_t* m_RP = m_return - 1;  // return pointer
#endif

    // metering and memory state
    uint64_t m_runGas = 0;
    uint64_t m_newMemSize = 0;
    uint64_t m_copyMemSize = 0;

    // initialize interpreter
    void initEntry();
    void optimize();

    // interpreter loop & switch
    void interpretCases();

    // interpreter cases that call out
    void caseCreate();
    bool caseCallSetup(CallParameters*, bytesRef& o_output);
    void caseCall();

    void copyDataToMemory(bytesConstRef _data, u256*_sp);
    uint64_t memNeed(u256 _offset, u256 _size);

    void throwOutOfGas();
    void throwBadInstruction();
    void throwBadJumpDestination();
    void throwBadStack(unsigned _removed, unsigned _added);
    void throwRevertInstruction(owning_bytes_ref&& _output);
    void throwDisallowedStateChange();
    void throwBufferOverrun(bigint const& _enfOfAccess);

    std::vector<uint64_t> m_beginSubs;
    std::vector<uint64_t> m_jumpDests;
    int64_t verifyJumpDest(u256 const& _dest, bool _throw = true);

    void onOperation() {}
    void adjustStack(unsigned _removed, unsigned _added);
    uint64_t gasForMem(u512 _size);
    void updateSSGas();
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
    
    //
    // implementations of simd opcodes
    //
    // input bytes are the inline simd type descriptors for the operand vectors on the stack
    //
#if EIP_616

    void xadd    (uint8_t);
    void xmul    (uint8_t);
    void xsub    (uint8_t);
    void xdiv    (uint8_t);
    void xsdiv   (uint8_t);
    void xmod    (uint8_t);
    void xsmod   (uint8_t);
    void xlt     (uint8_t);
    void xslt    (uint8_t);
    void xgt     (uint8_t);
    void xsgt    (uint8_t);
    void xeq     (uint8_t);
    void xzero   (uint8_t);
    void xand    (uint8_t);
    void xoor    (uint8_t);
    void xxor    (uint8_t);
    void xnot    (uint8_t);
    void xshr    (uint8_t);
    void xsar    (uint8_t);
    void xshl    (uint8_t);
    void xrol    (uint8_t);
    void xror    (uint8_t);
    void xmload  (uint8_t);
    void xmstore (uint8_t);
    void xsload  (uint8_t);
    void xsstore (uint8_t);
    void xvtowide(uint8_t);
    void xwidetov(uint8_t);
    void xpush   (uint8_t);
    void xput    (uint8_t, uint8_t);
    void xget    (uint8_t, uint8_t);
    void xswizzle(uint8_t);
    void xshuffle(uint8_t);
    
    u256 vtow(uint8_t _b, const u256& _in);
    void wtov(uint8_t _b, u256 _in, u256& _o_out);

    uint8_t simdType()
    {
        uint8_t nt = m_code[++m_PC];  // advance PC and get simd type from code
        ++m_PC;                       // advance PC to next opcode, ready to continue
        return nt;
    }

#endif
};

}
}
