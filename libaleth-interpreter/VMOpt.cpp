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

#include "VM.h"

namespace dev
{
namespace eth
{
std::array<evmc_instruction_metrics, 256> VM::c_metrics{{}};
void VM::initMetrics()
{
    static bool done = []() noexcept
    {
        // Copy the metrics of the top EVM revision.
        std::memcpy(&c_metrics[0], evmc_get_instruction_metrics_table(EVMC_LATEST_REVISION),
            c_metrics.size() * sizeof(c_metrics[0]));

        // Inject interpreter optimization opcodes.
        c_metrics[uint8_t(Instruction::PUSHC)] = c_metrics[uint8_t(Instruction::PUSH1)];
        c_metrics[uint8_t(Instruction::JUMPC)] = c_metrics[uint8_t(Instruction::JUMP)];
        c_metrics[uint8_t(Instruction::JUMPCI)] = c_metrics[uint8_t(Instruction::JUMPI)];
        return true;
    }();
    (void)done;
}

void VM::copyCode(int _extraBytes)
{
    // Copy code so that it can be safely modified and extend code by
    // _extraBytes zero bytes to allow reading virtual data at the end
    // of the code without bounds checks.
    auto extendedSize = m_codeSize + _extraBytes;
    m_code.reserve(extendedSize);
    m_code.assign(m_pCode, m_pCode + m_codeSize);
    m_code.resize(extendedSize);
}

void VM::optimize()
{
    copyCode(33);

    size_t const nBytes = m_codeSize;

    // build a table of jump destinations for use in verifyJumpDest
    
    TRACE_STR(1, "Build JUMPDEST table")
    for (size_t pc = 0; pc < nBytes; ++pc)
    {
        Instruction op = Instruction(m_code[pc]);
        TRACE_OP(2, pc, op);
                
        // make synthetic ops in user code trigger invalid instruction if run
        if (
            op == Instruction::PUSHC ||
            op == Instruction::JUMPC ||
            op == Instruction::JUMPCI
        )
        {
            TRACE_OP(1, pc, op);
            m_code[pc] = (byte)Instruction::INVALID;
        }

        if (op == Instruction::JUMPDEST)
        {
            m_jumpDests.push_back(pc);
        }
        else if (
            (byte)Instruction::PUSH1 <= (byte)op &&
            (byte)op <= (byte)Instruction::PUSH32
        )
        {
            pc += (byte)op - (byte)Instruction::PUSH1 + 1;
        }
    }
    
#ifdef EVM_DO_FIRST_PASS_OPTIMIZATION
    
    TRACE_STR(1, "Do first pass optimizations")
    for (size_t pc = 0; pc < nBytes; ++pc)
    {
        u256 val = 0;
        Instruction op = Instruction(m_code[pc]);

        if ((byte)Instruction::PUSH1 <= (byte)op && (byte)op <= (byte)Instruction::PUSH32)
        {
            byte nPush = (byte)op - (byte)Instruction::PUSH1 + 1;

            // decode pushed bytes to integral value
            val = m_code[pc+1];
            for (uint64_t i = pc+2, n = nPush; --n; ++i) {
                val = (val << 8) | m_code[i];
            }

        #if EVM_USE_CONSTANT_POOL

            // add value to constant pool and replace PUSHn with PUSHC
            // place offset in code as 2 bytes MSB-first
            // followed by one byte count of remaining pushed bytes
            if (5 < nPush)
            {
                uint16_t pool_off = m_pool.size();
                TRACE_VAL(1, "stash", val);
                TRACE_VAL(1, "... in pool at offset" , pool_off);
                m_pool.push_back(val);

                TRACE_PRE_OPT(1, pc, op);
                m_code[pc] = byte(op = Instruction::PUSHC);
                m_code[pc+3] = nPush - 2;
                m_code[pc+2] = pool_off & 0xff;
                m_code[pc+1] = pool_off >> 8;
                TRACE_POST_OPT(1, pc, op);
            }

        #endif

        #if EVM_REPLACE_CONST_JUMP    
            // replace JUMP or JUMPI to constant location with JUMPC or JUMPCI
            // verifyJumpDest is M = log(number of jump destinations)
            // outer loop is N = number of bytes in code array
            // so complexity is N log M, worst case is N log N
            size_t i = pc + nPush + 1;
            op = Instruction(m_code[i]);
            if (op == Instruction::JUMP)
            {
                TRACE_VAL(1, "Replace const JUMP with JUMPC to", val)
                TRACE_PRE_OPT(1, i, op);
                
                if (0 <= verifyJumpDest(val, false))
                    m_code[i] = byte(op = Instruction::JUMPC);
                
                TRACE_POST_OPT(1, i, op);
            }
            else if (op == Instruction::JUMPI)
            {
                TRACE_VAL(1, "Replace const JUMPI with JUMPCI to", val)
                TRACE_PRE_OPT(1, i, op);
                
                if (0 <= verifyJumpDest(val, false))
                    m_code[i] = byte(op = Instruction::JUMPCI);
                
                TRACE_POST_OPT(1, i, op);
            }
        #endif

            pc += nPush;
        }
    }
    TRACE_STR(1, "Finished optimizations")
#endif    
}


//
// Init interpreter on entry.
//
void VM::initEntry()
{
    m_bounce = &VM::interpretCases;     
    initMetrics();
    optimize();
}


// Implementation of EXP.
//
// This implements exponentiation by squaring algorithm.
// Is faster than boost::multiprecision::powm() because it avoids explicit
// mod operation.
// Do not inline it.
u256 VM::exp256(u256 _base, u256 _exponent)
{
    using boost::multiprecision::limb_type;
    u256 result = 1;
    while (_exponent)
    {
        if (static_cast<limb_type>(_exponent) & 1)    // If exponent is odd.
            result *= _base;
        _base *= _base;
        _exponent >>= 1;
    }
    return result;
}
}
}
