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

#include <cstdint>
#include <string>

namespace dev
{
namespace eth
{

/// Virtual machine bytecode instruction.
enum class Instruction: uint8_t
{
	STOP = 0x00,        ///< halts execution
	ADD,                ///< addition operation
	MUL,                ///< mulitplication operation
	SUB,                ///< subtraction operation
	DIV,                ///< integer division operation
	SDIV,               ///< signed integer division operation
	MOD,                ///< modulo remainder operation
	SMOD,               ///< signed modulo remainder operation
	ADDMOD,             ///< unsigned modular addition
	MULMOD,             ///< unsigned modular multiplication
	EXP,                ///< exponential operation
	SIGNEXTEND,         ///< extend length of signed integer

	LT = 0x10,          ///< less-than comparision
	GT,                 ///< greater-than comparision
	SLT,                ///< signed less-than comparision
	SGT,                ///< signed greater-than comparision
	EQ,                 ///< equality comparision
	ISZERO,             ///< simple not operator
	AND,                ///< bitwise AND operation
	OR,                 ///< bitwise OR operation
	XOR,                ///< bitwise XOR operation
	NOT,                ///< bitwise NOT opertation
	BYTE,               ///< retrieve single byte from word

	SHA3 = 0x20,        ///< compute SHA3-256 hash

	ADDRESS = 0x30,     ///< get address of currently executing account
	BALANCE,            ///< get balance of the given account
	ORIGIN,             ///< get execution origination address
	CALLER,             ///< get caller address
	CALLVALUE,          ///< get deposited value by the instruction/transaction responsible for this execution
	CALLDATALOAD,       ///< get input data of current environment
	CALLDATASIZE,       ///< get size of input data in current environment
	CALLDATACOPY,       ///< copy input data in current environment to memory
	CODESIZE,           ///< get size of code running in current environment
	CODECOPY,           ///< copy code running in current environment to memory
	GASPRICE,           ///< get price of gas in current environment
	EXTCODESIZE,        ///< get external code size (from another contract)
	EXTCODECOPY,        ///< copy external code (from another contract)
	RETURNDATASIZE = 0x3d,  ///< size of data returned from previous call
	RETURNDATACOPY = 0x3e,  ///< copy data returned from previous call to memory

	BLOCKHASH = 0x40,   ///< get hash of most recent complete block
	COINBASE,           ///< get the block's coinbase address
	TIMESTAMP,          ///< get the block's timestamp
	NUMBER,             ///< get the block's number
	DIFFICULTY,         ///< get the block's difficulty
	GASLIMIT,           ///< get the block's gas limit
	
	POP = 0x50,         ///< remove item from stack
	MLOAD,              ///< load word from memory
	MSTORE,             ///< save word to memory
	MSTORE8,            ///< save byte to memory
	SLOAD,              ///< load word from storage
	SSTORE,             ///< save word to storage
	JUMP,               ///< alter the program counter to a jumpdest
	JUMPI,              ///< conditionally alter the program counter
	PC,                 ///< get the program counter
	MSIZE,              ///< get the size of active memory
	GAS,                ///< get the amount of available gas
	JUMPDEST,           ///< set a potential jump destination
	
	PUSH1 = 0x60,       ///< place 1 byte item on stack
	PUSH2,              ///< place 2 byte item on stack
	PUSH3,              ///< place 3 byte item on stack
	PUSH4,              ///< place 4 byte item on stack
	PUSH5,              ///< place 5 byte item on stack
	PUSH6,              ///< place 6 byte item on stack
	PUSH7,              ///< place 7 byte item on stack
	PUSH8,              ///< place 8 byte item on stack
	PUSH9,              ///< place 9 byte item on stack
	PUSH10,             ///< place 10 byte item on stack
	PUSH11,             ///< place 11 byte item on stack
	PUSH12,             ///< place 12 byte item on stack
	PUSH13,             ///< place 13 byte item on stack
	PUSH14,             ///< place 14 byte item on stack
	PUSH15,             ///< place 15 byte item on stack
	PUSH16,             ///< place 16 byte item on stack
	PUSH17,             ///< place 17 byte item on stack
	PUSH18,             ///< place 18 byte item on stack
	PUSH19,             ///< place 19 byte item on stack
	PUSH20,             ///< place 20 byte item on stack
	PUSH21,             ///< place 21 byte item on stack
	PUSH22,             ///< place 22 byte item on stack
	PUSH23,             ///< place 23 byte item on stack
	PUSH24,             ///< place 24 byte item on stack
	PUSH25,             ///< place 25 byte item on stack
	PUSH26,             ///< place 26 byte item on stack
	PUSH27,             ///< place 27 byte item on stack
	PUSH28,             ///< place 28 byte item on stack
	PUSH29,             ///< place 29 byte item on stack
	PUSH30,             ///< place 30 byte item on stack
	PUSH31,             ///< place 31 byte item on stack
	PUSH32,             ///< place 32 byte item on stack
	
	DUP1 = 0x80,        ///< copies the highest item in the stack to the top of the stack
	DUP2,               ///< copies the second highest item in the stack to the top of the stack
	DUP3,               ///< copies the third highest item in the stack to the top of the stack
	DUP4,               ///< copies the 4th highest item in the stack to the top of the stack
	DUP5,               ///< copies the 5th highest item in the stack to the top of the stack
	DUP6,               ///< copies the 6th highest item in the stack to the top of the stack
	DUP7,               ///< copies the 7th highest item in the stack to the top of the stack
	DUP8,               ///< copies the 8th highest item in the stack to the top of the stack
	DUP9,               ///< copies the 9th highest item in the stack to the top of the stack
	DUP10,              ///< copies the 10th highest item in the stack to the top of the stack
	DUP11,              ///< copies the 11th highest item in the stack to the top of the stack
	DUP12,              ///< copies the 12th highest item in the stack to the top of the stack
	DUP13,              ///< copies the 13th highest item in the stack to the top of the stack
	DUP14,              ///< copies the 14th highest item in the stack to the top of the stack
	DUP15,              ///< copies the 15th highest item in the stack to the top of the stack
	DUP16,              ///< copies the 16th highest item in the stack to the top of the stack

	SWAP1 = 0x90,       ///< swaps the highest and second highest value on the stack
	SWAP2,              ///< swaps the highest and third highest value on the stack
	SWAP3,              ///< swaps the highest and 4th highest value on the stack
	SWAP4,              ///< swaps the highest and 5th highest value on the stack
	SWAP5,              ///< swaps the highest and 6th highest value on the stack
	SWAP6,              ///< swaps the highest and 7th highest value on the stack
	SWAP7,              ///< swaps the highest and 8th highest value on the stack
	SWAP8,              ///< swaps the highest and 9th highest value on the stack
	SWAP9,              ///< swaps the highest and 10th highest value on the stack
	SWAP10,             ///< swaps the highest and 11th highest value on the stack
	SWAP11,             ///< swaps the highest and 12th highest value on the stack
	SWAP12,             ///< swaps the highest and 13th highest value on the stack
	SWAP13,             ///< swaps the highest and 14th highest value on the stack
	SWAP14,             ///< swaps the highest and 15th highest value on the stack
	SWAP15,             ///< swaps the highest and 16th highest value on the stack
	SWAP16,             ///< swaps the highest and 17th highest value on the stack

	LOG0 = 0xa0,        ///< Makes a log entry; no topics.
	LOG1,               ///< Makes a log entry; 1 topic.
	LOG2,               ///< Makes a log entry; 2 topics.
	LOG3,               ///< Makes a log entry; 3 topics.
	LOG4,               ///< Makes a log entry; 4 topics.

	// these are generated by the interpreter - should never be in user code
	PUSHC = 0xac,       ///< push value from constant pool
	JUMPC,              ///< alter the program counter - pre-verified
	JUMPCI,             ///< conditionally alter the program counter - pre-verified

	JUMPTO = 0xb0,      ///< alter the program counter to a jumpdest
	JUMPIF,             ///< conditionally alter the program counter
	JUMPSUB,            ///< alter the program counter to a beginsub
	JUMPV,              ///< alter the program counter to a jumpdest
	JUMPSUBV,           ///< alter the program counter to a beginsub
	BEGINSUB,           ///< set a potential jumpsub destination
	BEGINDATA,          ///< begine the data section
	RETURNSUB,          ///< return to subroutine jumped from
	PUTLOCAL,           ///< pop top of stack to local variable
	GETLOCAL,           ///< push local variable to top of stack

	XADD = 0xc1,        ///< addition operation
	XMUL,               ///< mulitplication operation
	XSUB,               ///< subtraction operation
	XDIV,               ///< integer division operation
	XSDIV,              ///< signed integer division operation
	XMOD,               ///< modulo remainder operation
	XSMOD,              ///< signed modulo remainder operation
	XLT = 0xd0,         ///< less-than comparision
	XGT,                ///< greater-than comparision
	XSLT,               ///< signed less-than comparision
	XSGT,               ///< signed greater-than comparision
	XEQ,                ///< equality comparision
	XISZERO,            ///< simple not operator
	XAND,               ///< bitwise AND operation
	XOOR,               ///< bitwise OR operation
	XXOR,               ///< bitwise XOR operation
	XNOT,               ///< bitwise NOT opertation
	XSHL = 0xdb,        ///< shift left opertation
	XSHR,               ///< shift right opertation
	XSAR,               ///< shift arithmetic right opertation
	XROL,               ///< rotate left opertation
	XROR,               ///< rotate right opertation
	XPUSH = 0xe0,       ///< push vector to stack
	XMLOAD,             ///< load vector from memory
	XMSTORE,            ///< save vector to memory
	XSLOAD = 0xe4,      ///< load vector from storage
	XSSTORE,            ///< save vector to storage
	XVTOWIDE,           ///< convert vector to wide integer
	XWIDETOV,           ///< convert wide integer to vector
	XGET,               ///< get data from vector
	XPUT,               ///< put data in vector
	XSWIZZLE,           ///< permute data in vector
	XSHUFFLE,           ///< permute data in two vectors

	CREATE = 0xf0,      ///< create a new account with associated code
	CALL,               ///< message-call into an account
	CALLCODE,           ///< message-call with another account's code only
	RETURN,             ///< halt execution returning output data
	DELEGATECALL,       ///< like CALLCODE but keeps caller's value and sender
	STATICCALL = 0xfa,	///< like CALL except state changing operation are not permitted (will throw)
	CREATE2 = 0xfb,		///< create a new account with associated code. sha3((sender + salt + sha3(code))
	REVERT = 0xfd,      ///< stop execution and revert state changes, without consuming all provided gas
	INVALID = 0xfe,     ///< dedicated invalid instruction
	SUICIDE = 0xff      ///< halt execution and register account for later deletion
};

enum class Tier : unsigned
{
	Zero = 0,   // 0, Zero
	Base,       // 2, Quick
	VeryLow,    // 3, Fastest
	Low,        // 5, Fast
	Mid,        // 8, Mid
	High,       // 10, Slow
	Ext,        // 20, Ext
	Special,    // multiparam or otherwise special
	Invalid     // Invalid.
};

/// Information structure for a particular instruction.
struct InstructionInfo
{
	std::string name;   ///< The name of the instruction.
	int additional;     ///< Additional items required in memory for this instructions (only for PUSH).
	int args;           ///< Number of items required on the stack for this instruction (and, for the purposes of ret, the number taken from the stack).
	int ret;            ///< Number of items placed (back) on the stack by this instruction, assuming args items were removed.
	Tier gasPriceTier;   ///< Tier for gas pricing.
};

/// Information on all the instructions.
InstructionInfo instructionInfo(Instruction _inst);

}
}
