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
#include "VMConfig.h"
#include <libethereum/ExtVM.h>
using namespace std;
using namespace dev;
using namespace dev::eth;

void VM::reportStackUse()
{
	static intptr_t p = 0;
	intptr_t q = intptr_t(&q);
	if (p)
		cerr << "STACK: " << p << " - " << q << " = " << (p - q) << endl;
	p = q;
}

std::array<InstructionMetric, 256> VM::c_metrics;
void VM::initMetrics()
{
	static bool done=false;
	if (!done)
	{
		for (unsigned i = 0; i < 256; ++i)
		{
			InstructionInfo inst = instructionInfo((Instruction)i);
			c_metrics[i].gasPriceTier = inst.gasPriceTier;
			c_metrics[i].args = inst.args;
			c_metrics[i].ret = inst.ret;
		}
	}
	done = true;
}

//
// init interpreter on entry
//

void VM::initEntry()
{
	m_bounce = &VM::interpretCases;	
	m_codeSpace = m_ext->code;
	m_code = m_codeSpace.data();
	
	initMetrics();	
	
	optimize();
}

int VM::poolConstant(const u256& _con)
{
	int i = 0, n = m_poolSpace.size();
	while (i < n)
	{
		if (m_poolSpace[i] == _con)
			return i;
	}
	if (i <= 255 )
	{
		m_poolSpace.push_back(_con);
		return i;
	}
	return -1;
}

void VM::optimize()
{	
	// build a table of jump destinations for use in verifyJumpDest
	for (size_t i = 0; i < m_codeSpace.size(); ++i)
	{
		byte inst = m_code[i];

		if (inst == (byte)Instruction::JUMPDEST)
		{
			m_jumpDests.push_back(i);
		}
		else if ((byte)Instruction::PUSH1 <= inst && inst <= (byte)Instruction::PUSH32)
		{
			byte n = inst - (byte)Instruction::PUSH1 + 1;
			i += n;
		}
	}

#ifdef EVM_DO_FIRST_PASS_OPTIMIZATION

	for (size_t i = 0; i < m_codeSpace.size(); ++i)
	{
		byte inst = m_code[i];

		if ((byte)Instruction::PUSH1 <= inst && inst <= (byte)Instruction::PUSH32)
		{
			byte n = inst - (byte)Instruction::PUSH1 + 1;

			// decode pushed bytes to integral value
			u256 val = m_code[i+1];
			for (uint64_t j = i+2, m = n; --m; ++j)
				val = (val << 8) | m_code[j];

	#ifdef EVM_USE_CONSTANT_POOL
	
			// add value to constant pool and replace PUSHn with PUSHC if room
			if (1 < n && VMJumpTable[i] == INVALID)
			{
				int pool_off = poolConstant(val);
				if (0 <= pool_off && pool_off < 256)
				{
					m_code[i] = (byte)Instruction::PUSHC;
					m_code[i+1] = (byte)pool_off;
					m_code[i+2] = n;
				}
			}

	#endif

	#ifdef EVM_REPLACE_CONST_JUMP
	
			// replace JUMP or JUMPI to constant location with JUMPV or JUMPVI
			// verifyJumpDest is M = log(number of jump destinations)
			// outer loop is N = number of bytes in code array
			// so complexity is N log (M)
			size_t ii = i + n + 1;
			byte op = m_code[ii];
			if (op == (byte)Instruction::JUMP)
			{
				if (verifyJumpDest(val))
					m_code[ii] = (byte)Instruction::JUMPV;
			}
			else if (op == (byte)Instruction::JUMPI)
			{
				if (verifyJumpDest(val))
					m_code[ii] = (byte)Instruction::JUMPV;
			}
	#endif
			
			i += n;
		}
	}	
#endif
	
	m_pool = m_poolSpace.data();
	
}

