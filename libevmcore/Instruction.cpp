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
/** @file Instruction.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "Instruction.h"

#include <functional>
#include <libdevcore/Common.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Log.h>
using namespace std;
using namespace dev;
using namespace dev::eth;

static const std::map<Instruction,  InstructionInfo> c_instructionInfo =
{ //                                               Add,  Args,  Ret,  SideEffects, GasPriceTier
	{ Instruction::STOP,         { "STOP",           0,     0,    0,  true,        Tier::Zero } },
	{ Instruction::ADD,          { "ADD",            0,     2,    1,  false,       Tier::VeryLow } },
	{ Instruction::SUB,          { "SUB",            0,     2,    1,  false,       Tier::VeryLow } },
	{ Instruction::MUL,          { "MUL",            0,     2,    1,  false,       Tier::Low } },
	{ Instruction::DIV,          { "DIV",            0,     2,    1,  false,       Tier::Low } },
	{ Instruction::SDIV,         { "SDIV",           0,     2,    1,  false,       Tier::Low } },
	{ Instruction::MOD,          { "MOD",            0,     2,    1,  false,       Tier::Low } },
	{ Instruction::SMOD,         { "SMOD",           0,     2,    1,  false,       Tier::Low } },
	{ Instruction::EXP,          { "EXP",            0,     2,    1,  false,       Tier::Special } },
	{ Instruction::NOT,          { "NOT",            0,     1,    1,  false,       Tier::VeryLow } },
	{ Instruction::LT,           { "LT",             0,     2,    1,  false,       Tier::VeryLow } },
	{ Instruction::GT,           { "GT",             0,     2,    1,  false,       Tier::VeryLow } },
	{ Instruction::SLT,          { "SLT",            0,     2,    1,  false,       Tier::VeryLow } },
	{ Instruction::SGT,          { "SGT",            0,     2,    1,  false,       Tier::VeryLow } },
	{ Instruction::EQ,           { "EQ",             0,     2,    1,  false,       Tier::VeryLow } },
	{ Instruction::ISZERO,       { "ISZERO",         0,     1,    1,  false,       Tier::VeryLow } },
	{ Instruction::AND,          { "AND",            0,     2,    1,  false,       Tier::VeryLow } },
	{ Instruction::OR,           { "OR",             0,     2,    1,  false,       Tier::VeryLow } },
	{ Instruction::XOR,          { "XOR",            0,     2,    1,  false,       Tier::VeryLow } },
	{ Instruction::BYTE,         { "BYTE",           0,     2,    1,  false,       Tier::VeryLow } },
	{ Instruction::ADDMOD,       { "ADDMOD",         0,     3,    1,  false,       Tier::Mid } },
	{ Instruction::MULMOD,       { "MULMOD",         0,     3,    1,  false,       Tier::Mid } },
	{ Instruction::SIGNEXTEND,   { "SIGNEXTEND",     0,     2,    1,  false,       Tier::Low } },
	{ Instruction::SHA3,         { "SHA3",           0,     2,    1,  false,       Tier::Special } },
	{ Instruction::ADDRESS,      { "ADDRESS",        0,     0,    1,  false,       Tier::Base } },
	{ Instruction::BALANCE,      { "BALANCE",        0,     1,    1,  false,       Tier::Special } },
	{ Instruction::ORIGIN,       { "ORIGIN",         0,     0,    1,  false,       Tier::Base } },
	{ Instruction::CALLER,       { "CALLER",         0,     0,    1,  false,       Tier::Base } },
	{ Instruction::CALLVALUE,    { "CALLVALUE",      0,     0,    1,  false,       Tier::Base } },
	{ Instruction::CALLDATALOAD, { "CALLDATALOAD",   0,     1,    1,  false,       Tier::VeryLow } },
	{ Instruction::CALLDATASIZE, { "CALLDATASIZE",   0,     0,    1,  false,       Tier::Base } },
	{ Instruction::CALLDATACOPY, { "CALLDATACOPY",   0,     3,    0,  true,        Tier::VeryLow } },
	{ Instruction::CODESIZE,     { "CODESIZE",       0,     0,    1,  false,       Tier::Base } },
	{ Instruction::CODECOPY,     { "CODECOPY",       0,     3,    0,  true,        Tier::VeryLow } },
	{ Instruction::GASPRICE,     { "GASPRICE",       0,     0,    1,  false,       Tier::Base } },
	{ Instruction::EXTCODESIZE,  { "EXTCODESIZE",    0,     1,    1,  false,       Tier::Special } },
	{ Instruction::EXTCODECOPY,  { "EXTCODECOPY",    0,     4,    0,  true,        Tier::Special } },
	{ Instruction::RETURNDATASIZE,{"RETURNDATASIZE", 0,     0,    1,  false,       Tier::Base } },
	{ Instruction::RETURNDATACOPY,{"RETURNDATACOPY", 0,     3,    0,  true,        Tier::VeryLow } },
	{ Instruction::BLOCKHASH,    { "BLOCKHASH",      0,     1,    1,  false,       Tier::Special } },
	{ Instruction::COINBASE,     { "COINBASE",       0,     0,    1,  false,       Tier::Base } },
	{ Instruction::TIMESTAMP,    { "TIMESTAMP",      0,     0,    1,  false,       Tier::Base } },
	{ Instruction::NUMBER,       { "NUMBER",         0,     0,    1,  false,       Tier::Base } },
	{ Instruction::DIFFICULTY,   { "DIFFICULTY",     0,     0,    1,  false,       Tier::Base } },
	{ Instruction::GASLIMIT,     { "GASLIMIT",       0,     0,    1,  false,       Tier::Base } },
	{ Instruction::POP,          { "POP",            0,     1,    0,  false,       Tier::Base } },
	{ Instruction::MLOAD,        { "MLOAD",          0,     1,    1,  false,       Tier::VeryLow } },
	{ Instruction::MSTORE,       { "MSTORE",         0,     2,    0,  true,        Tier::VeryLow } },
	{ Instruction::MSTORE8,      { "MSTORE8",        0,     2,    0,  true,        Tier::VeryLow } },
	{ Instruction::SLOAD,        { "SLOAD",          0,     1,    1,  false,       Tier::Special } },
	{ Instruction::SSTORE,       { "SSTORE",         0,     2,    0,  true,        Tier::Special } },
	{ Instruction::JUMP,         { "JUMP",           0,     1,    0,  true,        Tier::Mid } },
	{ Instruction::JUMPI,        { "JUMPI",          0,     2,    0,  true,        Tier::High } },
	{ Instruction::PC,           { "PC",             0,     0,    1,  false,       Tier::Base } },
	{ Instruction::MSIZE,        { "MSIZE",          0,     0,    1,  false,       Tier::Base } },
	{ Instruction::GAS,          { "GAS",            0,     0,    1,  false,       Tier::Base } },
	{ Instruction::JUMPDEST,     { "JUMPDEST",       0,     0,    0,  true,        Tier::Special } },
	{ Instruction::BEGINDATA,    { "BEGINDATA",      0,     0,    0,  true,        Tier::Special } },
	{ Instruction::BEGINSUB,     { "BEGINSUB",       0,     0,    0,  true,        Tier::Special } },
	{ Instruction::PUSH1,        { "PUSH1",          1,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH2,        { "PUSH2",          2,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH3,        { "PUSH3",          3,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH4,        { "PUSH4",          4,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH5,        { "PUSH5",          5,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH6,        { "PUSH6",          6,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH7,        { "PUSH7",          7,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH8,        { "PUSH8",          8,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH9,        { "PUSH9",          9,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH10,       { "PUSH10",        10,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH11,       { "PUSH11",        11,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH12,       { "PUSH12",        12,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH13,       { "PUSH13",        13,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH14,       { "PUSH14",        14,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH15,       { "PUSH15",        15,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH16,       { "PUSH16",        16,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH17,       { "PUSH17",        17,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH18,       { "PUSH18",        18,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH19,       { "PUSH19",        19,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH20,       { "PUSH20",        20,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH21,       { "PUSH21",        21,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH22,       { "PUSH22",        22,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH23,       { "PUSH23",        23,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH24,       { "PUSH24",        24,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH25,       { "PUSH25",        25,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH26,       { "PUSH26",        26,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH27,       { "PUSH27",        27,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH28,       { "PUSH28",        28,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH29,       { "PUSH29",        29,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH30,       { "PUSH30",        30,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH31,       { "PUSH31",        31,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::PUSH32,       { "PUSH32",        32,     0,    1,  false,       Tier::VeryLow } },
	{ Instruction::DUP1,         { "DUP1",           0,     1,    2,  false,       Tier::VeryLow } },
	{ Instruction::DUP2,         { "DUP2",           0,     2,    3,  false,       Tier::VeryLow } },
	{ Instruction::DUP3,         { "DUP3",           0,     3,    4,  false,       Tier::VeryLow } },
	{ Instruction::DUP4,         { "DUP4",           0,     4,    5,  false,       Tier::VeryLow } },
	{ Instruction::DUP5,         { "DUP5",           0,     5,    6,  false,       Tier::VeryLow } },
	{ Instruction::DUP6,         { "DUP6",           0,     6,    7,  false,       Tier::VeryLow } },
	{ Instruction::DUP7,         { "DUP7",           0,     7,    8,  false,       Tier::VeryLow } },
	{ Instruction::DUP8,         { "DUP8",           0,     8,    9,  false,       Tier::VeryLow } },
	{ Instruction::DUP9,         { "DUP9",           0,     9,    10,  false,      Tier::VeryLow } },
	{ Instruction::DUP10,        { "DUP10",          0,    10,    11,  false,      Tier::VeryLow } },
	{ Instruction::DUP11,        { "DUP11",          0,    11,    12,  false,      Tier::VeryLow } },
	{ Instruction::DUP12,        { "DUP12",          0,    12,    13,  false,      Tier::VeryLow } },
	{ Instruction::DUP13,        { "DUP13",          0,    13,    14,  false,      Tier::VeryLow } },
	{ Instruction::DUP14,        { "DUP14",          0,    14,    15,  false,      Tier::VeryLow } },
	{ Instruction::DUP15,        { "DUP15",          0,    15,    16,  false,      Tier::VeryLow } },
	{ Instruction::DUP16,        { "DUP16",          0,    16,    17,  false,      Tier::VeryLow } },
	{ Instruction::SWAP1,        { "SWAP1",          0,     2,     2,  false,      Tier::VeryLow } },
	{ Instruction::SWAP2,        { "SWAP2",          0,     3,     3,  false,      Tier::VeryLow } },
	{ Instruction::SWAP3,        { "SWAP3",          0,     4,     4,  false,      Tier::VeryLow } },
	{ Instruction::SWAP4,        { "SWAP4",          0,     5,     5,  false,      Tier::VeryLow } },
	{ Instruction::SWAP5,        { "SWAP5",          0,     6,     6,  false,      Tier::VeryLow } },
	{ Instruction::SWAP6,        { "SWAP6",          0,     7,     7,  false,      Tier::VeryLow } },
	{ Instruction::SWAP7,        { "SWAP7",          0,     8,     8,  false,      Tier::VeryLow } },
	{ Instruction::SWAP8,        { "SWAP8",          0,     9,     9,  false,      Tier::VeryLow } },
	{ Instruction::SWAP9,        { "SWAP9",          0,    10,    10,  false,      Tier::VeryLow } },
	{ Instruction::SWAP10,       { "SWAP10",         0,    11,    11,  false,      Tier::VeryLow } },
	{ Instruction::SWAP11,       { "SWAP11",         0,    12,    12,  false,      Tier::VeryLow } },
	{ Instruction::SWAP12,       { "SWAP12",         0,    13,    13,  false,      Tier::VeryLow } },
	{ Instruction::SWAP13,       { "SWAP13",         0,    14,    14,  false,      Tier::VeryLow } },
	{ Instruction::SWAP14,       { "SWAP14",         0,    15,    15,  false,      Tier::VeryLow } },
	{ Instruction::SWAP15,       { "SWAP15",         0,    16,    16,  false,      Tier::VeryLow } },
	{ Instruction::SWAP16,       { "SWAP16",         0,    17,    17,  false,      Tier::VeryLow } },
	{ Instruction::LOG0,         { "LOG0",           0,     2,     0,  true,       Tier::Special } },
	{ Instruction::LOG1,         { "LOG1",           0,     3,     0,  true,       Tier::Special } },
	{ Instruction::LOG2,         { "LOG2",           0,     4,     0,  true,       Tier::Special } },
	{ Instruction::LOG3,         { "LOG3",           0,     5,     0,  true,       Tier::Special } },
	{ Instruction::LOG4,         { "LOG4",           0,     6,     0,  true,       Tier::Special } },

	{ Instruction::JUMPTO,       { "JUMPTO",         2,     1,    0,  true,        Tier::VeryLow } },
	{ Instruction::JUMPIF,       { "JUMPIF",         2,     2,    0,  true,        Tier::Low } },
	{ Instruction::JUMPV,        { "JUMPV",          2,     1,    0,  true,        Tier::Mid } },
	{ Instruction::JUMPSUB,      { "JUMPSUB",        2,     1,    0,  true,        Tier::Low } },
	{ Instruction::JUMPSUBV,     { "JUMPSUBV",       2,     1,    0,  true,        Tier::Mid } },
	{ Instruction::RETURNSUB,    { "RETURNSUB",      0,     1,    0,  true,        Tier::Mid } },
	{ Instruction::PUTLOCAL,     { "PUTLOCAL",       2,     1,    0,  true,        Tier::VeryLow } },
	{ Instruction::GETLOCAL,     { "GETLOCAL",       2,     0,    1,  true,        Tier::VeryLow } },

	{ Instruction::CREATE,       { "CREATE",         0,     3,     1,  true,       Tier::Special } },
	{ Instruction::CREATE2,      { "CREATE2",        0,     4,     1,  true,       Tier::Special } },
	{ Instruction::CALL,         { "CALL",           0,     7,     1,  true,       Tier::Special } },
	{ Instruction::CALLCODE,     { "CALLCODE",       0,     7,     1,  true,       Tier::Special } },
	{ Instruction::RETURN,       { "RETURN",         0,     2,     0,  true,       Tier::Zero } },
	{ Instruction::STATICCALL,   { "STATICCALL",     0,     6,     1,  true,       Tier::Special } },
	{ Instruction::DELEGATECALL, { "DELEGATECALL",   0,     6,     1,  true,       Tier::Special } },
	{ Instruction::REVERT,       { "REVERT",         0,     2,     0,  true,       Tier::Special } },
	{ Instruction::INVALID,      { "INVALID",        0,     0,     0,  true,       Tier::Zero    } },
	{ Instruction::SUICIDE,      { "SUICIDE",        0,     1,     0,  true,       Tier::Special } },
 
	// these are generated by the interpreter - should never be in user code
	{ Instruction::PUSHC,        { "PUSHC",          3,     0 ,    1,   false,     Tier::VeryLow } },
	{ Instruction::JUMPC,        { "JUMPC",          0,     1,     0,   true,      Tier::Mid } },
	{ Instruction::JUMPCI,       { "JUMPCI",         0,     2,     0,   true,      Tier::High } },
}; 
 
void dev::eth::eachInstruction( 
	bytes const& _mem, 
	function<void(Instruction,u256 const&)> const& _onInstruction
)
{
	for (auto it = _mem.begin(); it < _mem.end(); ++it)
	{
		Instruction instr = Instruction(*it);
		size_t additional = 0;
		if (isValidInstruction(instr))
		additional = instructionInfo(instr).additional;
		u256 data;
		for (size_t i = 0; i < additional; ++i)
		{
			data <<= 8;
			if (++it < _mem.end())
				data |= *it;
		}
		_onInstruction(instr, data);
	}
}

InstructionInfo dev::eth::instructionInfo(Instruction _inst)
{
	auto it = c_instructionInfo.find(_inst);
	if (it != c_instructionInfo.end())
		return it->second;
	return InstructionInfo({{}, 0, 0, 0, false, Tier::Invalid});
}

bool dev::eth::isValidInstruction(Instruction _inst)
{
	return !!c_instructionInfo.count(_inst);
}

