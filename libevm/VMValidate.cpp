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
#include "VMConfig.h"
#include "VM.h"
using namespace std;
using namespace dev;
using namespace dev::eth;

#if EVM_JUMPS_AND_SUBS

///////////////////////////////////////////////////////////////////////////////
//
// invalid code will throw an exception
//

void VM::validate(ExtVMFace& _ext)
{
	m_ext = &_ext;
	initEntry();
	size_t PC;
	byte OP;
	for (PC = 0; (OP = m_code[PC]); ++PC)
		if  (OP == byte(Instruction::BEGINSUB))
			validateSubroutine(PC, m_return, m_stack);
		else if (OP == byte(Instruction::BEGINDATA))
			break;
}

// we validate each subroutine individually, as if at top level
// - PC is the offset in the code to start validating at
// - RP is the top PC on return stack that CASE_RETURNSUB returns to
// - SP = FP at the top level, so the stack size is also the frame size
void VM::validateSubroutine(uint64_t _PC, uint64_t* _RP, u256* _SP)
{
	// set current interpreter state
	m_pc = _PC, m_rp = _RP, m_sp = _SP;
	
	INIT_CASES
	DO_CASES
	{	
		CASE_BEGIN(JUMPDEST)
		{
			// if frame size is set then we have been here before
			ptrdiff_t frameSize = m_frameSize[m_pc];
			if (0 <= frameSize)
			{
				// check for constant frame size
				if (stackSize() != frameSize)
					throwBadStack(stackSize(), frameSize, 0);

				// return to break cycle in control flow graph
				return;
			}
			// set frame size to check later
			m_frameSize[m_pc] = stackSize();
			++m_pc;
		}
		CASE_END

		CASE_BEGIN(JUMPTO)
		{
			// extract jump destination from bytecode
			m_pc = decodeJumpDest(m_code, m_pc);
		}
		CASE_END

		CASE_BEGIN(JUMPIF)
		{
			// recurse to validate code to jump to, saving and restoring
			// interpreter state around call
			_PC = m_pc, _RP = m_rp, _SP = m_sp;
			validateSubroutine(decodeJumpDest(m_code, m_pc), _RP, _SP);
			m_pc = _PC, m_rp = _RP, m_sp = _SP;
			++m_pc;
		}
		CASE_END

		CASE_BEGIN(JUMPV)
		{
			// for every jump destination in jump vector
			for (size_t dest = 0, nDests = m_code[m_pc+1]; dest < nDests; ++dest)
			{
				// recurse to validate code to jump to, saving and 
				// restoring interpreter state around call
				_PC = m_pc, _RP = m_rp, _SP = m_sp;
				validateSubroutine(decodeJumpDest(m_code, m_pc), _RP, _SP);
				m_pc = _PC, m_rp = _RP, m_sp = _SP;
			}
		}
		CASE_RETURN

		CASE_BEGIN(JUMPSUB)
		{
			// check for enough arguments on stack
			size_t destPC = decodeJumpDest(m_code, m_pc);
			byte nArgs = m_code[destPC+1];
			if (stackSize() < nArgs) 
				throwBadStack(stackSize(), nArgs, 0);
		}
		CASE_NEXT

		CASE_BEGIN(JUMPSUBV)
		{
			// for every subroutine in jump vector
			for (size_t sub = 0, nSubs = m_code[m_PC+1]; sub < nSubs; ++sub)
			{
				// check for enough arguments on stack
				size_t destPC = decodeJumpvDest(m_code, m_PC, sub);
				byte nArgs = m_code[destPC+1];
				if (stackSize() < nArgs) 
					throwBadStack(stackSize(), , 0);
			}
		}
		CASE_END

		CASE_BEGIN(RETURNSUB)
		CASE_BEGIN(RETURN)
		CASE_BEGIN(SUICIDE)
		CASE_BEGIN(STOP)
		{
			// return to top level
		}
		CASE_RETURN;
		
		CASE_BEGIN(BEGINSUB)
		CASE_BEGIN(BEGINDATA)
		CASE_BEGIN(BAD)
		CASE_DEFAULT
		{
			throwBadInstruction();
		}
		CASE_END		
	}
	END_CASES
}

#endif
