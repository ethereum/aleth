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

namespace dev
    {
        namespace eth
    {
///////////////////////////////////////////////////////////////////////////////
//
// interpreter configuration macros for development, optimizations and tracing
//
// EIP_615                - subroutines and static jumps
// EIP_616                - SIMD
//
// EVM_OPTIMIZE           - all optimizations off when false (TO DO - MAKE DYNAMIC)
//
// EVM_SWITCH_DISPATCH    - dispatch via loop and switch
// EVM_JUMP_DISPATCH      - dispatch via a jump table - available only on GCC
//
// EVM_USE_CONSTANT_POOL  - constants unpacked and ready to assign to stack
//
// EVM_REPLACE_CONST_JUMP - pre-verified jumps to save runtime lookup
//
// EVM_TRACE              - provides various levels of tracing

#ifndef EIP_615
#define EIP_615 false
#endif

#ifndef EIP_616
#define EIP_616 false
#endif

#ifndef EVM_JUMP_DISPATCH
#ifdef __GNUC__
#define EVM_JUMP_DISPATCH true
#else
#define EVM_JUMP_DISPATCH false
#endif
#endif
#if EVM_JUMP_DISPATCH
        #ifndef __GNUC__
#error "address of label extension available only on Gnu"
#endif
#else
#define EVM_SWITCH_DISPATCH true
#endif

#ifndef EVM_OPTIMIZE
#define EVM_OPTIMIZE false
#endif
#if EVM_OPTIMIZE
        #define EVM_REPLACE_CONST_JUMP true
#define EVM_USE_CONSTANT_POOL true
#define EVM_DO_FIRST_PASS_OPTIMIZATION (EVM_REPLACE_CONST_JUMP || EVM_USE_CONSTANT_POOL)
#endif


///////////////////////////////////////////////////////////////////////////////
//
// set EVM_TRACE to 3, 2, 1, or 0 for lots to no tracing to cerr
//
#ifndef EVM_TRACE
#define EVM_TRACE 0
#endif
#if EVM_TRACE > 0

        #undef ON_OP
#if EVM_TRACE > 2
#define ON_OP() \
    (cerr << "### " << ++m_nSteps << ": " << m_PC << " " << instructionInfo(m_OP).name << endl)
#else
#define ON_OP() onOperation()
#endif

#define TRACE_STR(level, str) \
    if ((level) <= EVM_TRACE) \
        cerr << "$$$ " << (str) << endl;

#define TRACE_VAL(level, name, val) \
    if ((level) <= EVM_TRACE)       \
        cerr << "=== " << (name) << " " << hex << (val) << endl;
#define TRACE_OP(level, pc, op) \
    if ((level) <= EVM_TRACE)   \
        cerr << "*** " << (pc) << " " << instructionInfo(op).name << endl;

#define TRACE_PRE_OPT(level, pc, op) \
    if ((level) <= EVM_TRACE)        \
        cerr << "<<< " << (pc) << " " << instructionInfo(op).name << endl;

#define TRACE_POST_OPT(level, pc, op) \
    if ((level) <= EVM_TRACE)         \
        cerr << ">>> " << (pc) << " " << instructionInfo(op).name << endl;
#else
#define TRACE_STR(level, str)
#define TRACE_VAL(level, name, val)
#define TRACE_OP(level, pc, op)
#define TRACE_PRE_OPT(level, pc, op)
#define TRACE_POST_OPT(level, pc, op)
#define ON_OP() onOperation()
#endif

// Executive swallows exceptions in some circumstances
#if 0
#define THROW_EXCEPTION(X) ((cerr << "!!! EVM EXCEPTION " << (X).what() << endl), abort())
#else
#if EVM_TRACE > 0
        #define THROW_EXCEPTION(X) \
    ((cerr << "!!! EVM EXCEPTION " << (X).what() << endl), BOOST_THROW_EXCEPTION(X))
#else
#define THROW_EXCEPTION(X) BOOST_THROW_EXCEPTION(X)
#endif
#endif


///////////////////////////////////////////////////////////////////////////////
//
// build a simple loop-and-switch interpreter
//
#if EVM_SWITCH_DISPATCH

        #define INIT_CASES
#define DO_CASES            \
    for (;;)                \
    {                       \
        fetchInstruction(); \
        switch (m_OP)       \
        {
#define CASE(name) case Instruction::name:
#define NEXT \
    ++m_PC;  \
    break;
#define CONTINUE continue;
#define BREAK return;
#define DEFAULT default:
#define WHILE_CASES \
    }               \
    }


///////////////////////////////////////////////////////////////////////////////
//
// build an indirect-threaded interpreter using a jump table of
// label addresses (a gcc extension)
//
#elif EVM_JUMP_DISPATCH

#define INIT_CASES                              \
                                                \
    static const void* const jumpTable[256] = { \
        &&STOP, /* 00 */                        \
        &&ADD,                                  \
        &&MUL,                                  \
        &&SUB,                                  \
        &&DIV,                                  \
        &&SDIV,                                 \
        &&MOD,                                  \
        &&SMOD,                                 \
        &&ADDMOD,                               \
        &&MULMOD,                               \
        &&EXP,                                  \
        &&SIGNEXTEND,                           \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&LT, /* 10, */                         \
        &&GT,                                   \
        &&SLT,                                  \
        &&SGT,                                  \
        &&EQ,                                   \
        &&ISZERO,                               \
        &&AND,                                  \
        &&OR,                                   \
        &&XOR,                                  \
        &&NOT,                                  \
        &&BYTE,                                 \
        &&SHL,                                  \
        &&SHR,                                  \
        &&SAR,                                  \
        &&INVALID,                              \
        &&INVALID,                              \
        &&SHA3, /* 20, */                       \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&ADDRESS, /* 30, */                    \
        &&BALANCE,                              \
        &&ORIGIN,                               \
        &&CALLER,                               \
        &&CALLVALUE,                            \
        &&CALLDATALOAD,                         \
        &&CALLDATASIZE,                         \
        &&CALLDATACOPY,                         \
        &&CODESIZE,                             \
        &&CODECOPY,                             \
        &&GASPRICE,                             \
        &&EXTCODESIZE,                          \
        &&EXTCODECOPY,                          \
        &&RETURNDATASIZE,                       \
        &&RETURNDATACOPY,                       \
        &&EXTCODEHASH,                          \
        &&BLOCKHASH, /* 40, */                  \
        &&COINBASE,                             \
        &&TIMESTAMP,                            \
        &&NUMBER,                               \
        &&DIFFICULTY,                           \
        &&GASLIMIT,                             \
        &&CHAINID,                              \
        &&SELFBALANCE,                          \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&POP, /* 50, */                        \
        &&MLOAD,                                \
        &&MSTORE,                               \
        &&MSTORE8,                              \
        &&SLOAD,                                \
        &&SSTORE,                               \
        &&JUMP,                                 \
        &&JUMPI,                                \
        &&PC,                                   \
        &&MSIZE,                                \
        &&GAS,                                  \
        &&JUMPDEST,                             \
        &&BEGINDATA,                            \
        &&BEGINSUB,                             \
        &&INVALID,                              \
        &&INVALID,                              \
        &&PUSH1, /* 60, */                      \
        &&PUSH2,                                \
        &&PUSH3,                                \
        &&PUSH4,                                \
        &&PUSH5,                                \
        &&PUSH6,                                \
        &&PUSH7,                                \
        &&PUSH8,                                \
        &&PUSH9,                                \
        &&PUSH10,                               \
        &&PUSH11,                               \
        &&PUSH12,                               \
        &&PUSH13,                               \
        &&PUSH14,                               \
        &&PUSH15,                               \
        &&PUSH16,                               \
        &&PUSH17, /* 70, */                     \
        &&PUSH18,                               \
        &&PUSH19,                               \
        &&PUSH20,                               \
        &&PUSH21,                               \
        &&PUSH22,                               \
        &&PUSH23,                               \
        &&PUSH24,                               \
        &&PUSH25,                               \
        &&PUSH26,                               \
        &&PUSH27,                               \
        &&PUSH28,                               \
        &&PUSH29,                               \
        &&PUSH30,                               \
        &&PUSH31,                               \
        &&PUSH32,                               \
        &&DUP1, /* 80, */                       \
        &&DUP2,                                 \
        &&DUP3,                                 \
        &&DUP4,                                 \
        &&DUP5,                                 \
        &&DUP6,                                 \
        &&DUP7,                                 \
        &&DUP8,                                 \
        &&DUP9,                                 \
        &&DUP10,                                \
        &&DUP11,                                \
        &&DUP12,                                \
        &&DUP13,                                \
        &&DUP14,                                \
        &&DUP15,                                \
        &&DUP16,                                \
        &&SWAP1, /* 90, */                      \
        &&SWAP2,                                \
        &&SWAP3,                                \
        &&SWAP4,                                \
        &&SWAP5,                                \
        &&SWAP6,                                \
        &&SWAP7,                                \
        &&SWAP8,                                \
        &&SWAP9,                                \
        &&SWAP10,                               \
        &&SWAP11,                               \
        &&SWAP12,                               \
        &&SWAP13,                               \
        &&SWAP14,                               \
        &&SWAP15,                               \
        &&SWAP16,                               \
        &&LOG0, /* A0, */                       \
        &&LOG1,                                 \
        &&LOG2,                                 \
        &&LOG3,                                 \
        &&LOG4,                                 \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&PUSHC,                                \
        &&JUMPC,                                \
        &&JUMPCI,                               \
        &&INVALID,                              \
        &&JUMPTO, /* B0, */                     \
        &&JUMPIF,                               \
        &&JUMPSUB,                              \
        &&JUMPV,                                \
        &&JUMPSUBV,                             \
        &&BEGINSUB,                             \
        &&BEGINDATA,                            \
        &&RETURNSUB,                            \
        &&PUTLOCAL,                             \
        &&GETLOCAL,                             \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID, /* C0, */                    \
        &&XADD,                                 \
        &&XMUL,                                 \
        &&XSUB,                                 \
        &&XDIV,                                 \
        &&XSDIV,                                \
        &&XMOD,                                 \
        &&XSMOD,                                \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&XLT, /* D0 */                         \
        &&XGT,                                  \
        &&XSLT,                                 \
        &&XSGT,                                 \
        &&XEQ,                                  \
        &&XISZERO,                              \
        &&XAND,                                 \
        &&XOOR,                                 \
        &&XXOR,                                 \
        &&XNOT,                                 \
        &&INVALID,                              \
        &&XSHL,                                 \
        &&XSHR,                                 \
        &&XSAR,                                 \
        &&XROL,                                 \
        &&XROR,                                 \
        &&XPUSH, /* E0, */                      \
        &&XMLOAD,                               \
        &&XMSTORE,                              \
        &&INVALID,                              \
        &&XSLOAD,                               \
        &&XSSTORE,                              \
        &&XVTOWIDE,                             \
        &&XWIDETOV,                             \
        &&XGET,                                 \
        &&XPUT,                                 \
        &&XSWIZZLE,                             \
        &&XSHUFFLE,                             \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&CREATE, /* F0, */                     \
        &&CALL,                                 \
        &&CALLCODE,                             \
        &&RETURN,                               \
        &&DELEGATECALL,                         \
        &&CREATE2,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&INVALID,                              \
        &&STATICCALL,                           \
        &&INVALID,                              \
        &&INVALID,                              \
        &&REVERT,                               \
        &&INVALID,                              \
        &&SELFDESTRUCT,                         \
    };

#define DO_CASES        \
    fetchInstruction(); \
    goto* jumpTable[(int)m_OP];
#define CASE(name) \
    name:
#define NEXT            \
    ++m_PC;             \
    fetchInstruction(); \
    goto* jumpTable[(int)m_OP];
#define CONTINUE        \
    fetchInstruction(); \
    goto* jumpTable[(int)m_OP];
#define BREAK return;
#define DEFAULT
#define WHILE_CASES

#else
#error No opcode dispatch configured
#endif
    }
    }
