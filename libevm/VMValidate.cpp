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
/** @file
 */

#include <libethereum/ExtVM.h>

// validator is not a full interpreter, canot support optimized dispatch
#define EVM_JUMP_DISPATCH false
#include "VMConfig.h"

#include "VM.h"
using namespace std;
using namespace dev;
using namespace dev::eth;

#if EIP_615

///////////////////////////////////////////////////////////////////////////////
//
// invalid code will throw an exception
//

void VM::validate(ExtVMFace& _ext)
{
	m_ext = &_ext;
	initEntry();
	size_t PC;
	Instruction OP;
	for (PC = 0; bool(OP = Instruction(m_code[PC])); ++PC)
	{
		if  (OP == Instruction::BEGINSUB)
			validateSubroutine(PC, m_return, m_stack);
		else if (OP == Instruction::BEGINDATA)
			break;
		else if (
				Instruction::PUSH1 <= OP &&
				PC <= (size_t)Instruction::PUSH32)
			PC += (size_t)OP - (size_t)Instruction::PUSH1;
		else if (
				OP == Instruction::JUMPTO ||
				OP == Instruction::JUMPIF ||
				OP == Instruction::JUMPSUB)
			PC += 4;
		else if (OP == Instruction::JUMPV || OP == Instruction::JUMPSUBV)
			PC += 4 * m_code[PC];  // number of 4-byte dests followed by table
	}
}

// we validate each subroutine individually, as if at top level
// - PC is the offset in the code to start validating at
// - RP is the top PC on return stack that RETURNSUB returns to
// - SP = FP at the top level, so the stack size is also the frame size
void VM::validateSubroutine(uint64_t _pc, uint64_t* _rp, u256* _sp)
{
	// set current interpreter state
	m_PC = _pc, m_RP = _rp, m_SP = _sp;
	
	INIT_CASES
	DO_CASES
	{	
		CASE(JUMPDEST)
		{
			// if frame size is set then we have been here before
			size_t frameSize = m_frameSize[m_PC];
			if (0 != frameSize)
			{
				// check for constant frame size
				if (stackSize() != frameSize)
					throwBadStack(stackSize(), frameSize);

				// return to break cycle in control flow graph
				return;
			}
			// set frame size to check later
			m_frameSize[m_PC] = stackSize();
			++m_PC;
		}
		NEXT

		CASE(JUMPTO)
		{
			// extract jump destination from bytecode
			m_PC = decodeJumpDest(m_code.data(), m_PC);
		}
		NEXT

		CASE(JUMPIF)
		{
			// recurse to validate code to jump to, saving and restoring
			// interpreter state around call
			_pc = m_PC, _rp = m_RP, _sp = m_SP;
			validateSubroutine(decodeJumpvDest(m_code.data(), m_PC, byte(m_SP[0])), _rp, _sp);
			m_PC = _pc, m_RP = _rp, m_SP = _sp;
			++m_PC;
		}
		NEXT

		CASE(JUMPV)
		{
			// for every jump destination in jump vector
			for (size_t dest = 0, nDests = m_code[m_PC+1]; dest < nDests; ++dest)
			{
				// recurse to validate code to jump to, saving and 
				// restoring interpreter state around call
				_pc = m_PC, _rp = m_RP, _sp = m_SP;
				validateSubroutine(decodeJumpDest(m_code.data(), m_PC), _rp, _sp);
				m_PC = _pc, m_RP = _rp, m_SP = _sp;
			}
		}
		BREAK

		CASE(JUMPSUB)
		{
			// check for enough arguments on stack
			size_t destPC = decodeJumpDest(m_code.data(), m_PC);
			byte nArgs = m_code[destPC+1];
			if (stackSize() < nArgs) 
				throwBadStack(stackSize(), nArgs);
		}
		NEXT

		CASE(JUMPSUBV)
		{
			// for every subroutine in jump vector
			_pc = m_PC;
			for (size_t sub = 0, nSubs = m_code[m_PC+1]; sub < nSubs; ++sub)
			{
				// check for enough arguments on stack
				u256 slot = sub;
				_sp = &slot;
				size_t destPC = decodeJumpvDest(m_code.data(), _pc, byte(m_SP[0]));
				byte nArgs = m_code[destPC+1];
				if (stackSize() < nArgs) 
					throwBadStack(stackSize(), nArgs);
			}
			m_PC = _pc;
		}
		NEXT

		CASE(RETURNSUB)
		CASE(RETURN)
		CASE(SUICIDE)
		CASE(STOP)
		{
			// return to top level
		}
		BREAK;
		
		CASE(BEGINSUB)
		CASE(BEGINDATA)
		CASE(INVALID)
		DEFAULT
		{
			throwBadInstruction();
		}
	}
	WHILE_CASES
}

#endif
