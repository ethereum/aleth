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


#include <libethereum/ExtVM.h>
#include "VMConfig.h"
#include "VM.h"
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
			InstructionInfo op = instructionInfo((Instruction)i);
			c_metrics[i].gasPriceTier = op.gasPriceTier;
			c_metrics[i].args = op.args;
			c_metrics[i].ret = op.ret;
		}
	}
	done = true;
}

/// Init interpreter on entry.
void VM::initEntry()
{
	m_bounce = &VM::interpretCases;

	// Copy and extend code by 32 zero bytes to allow reading virtual push data
	// at the end of the code without bounds checks.
	auto extendedSize = m_ext->code.size() + 32;
	m_codeSpace.reserve(extendedSize);
	m_codeSpace = m_ext->code;
	m_codeSpace.resize(extendedSize);
	m_code = m_codeSpace.data();

	interpretCases(); // first time initializes

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
	size_t nBytes = m_codeSpace.size();
	
	// build a table of jump destinations for use in verifyJumpDest
	for (size_t i = 0; i < nBytes; ++i)
	{
		Instruction op = Instruction(m_code[i]);
		TRACE_OP(2, i, op);
				
		// make synthetic ops in user code trigger invalid instruction if run
		if (op == Instruction::PUSHC ||
		    op == Instruction::JUMPV ||
		    op == Instruction::JUMPVI)
		{
			m_code[i] = (byte)Instruction::BAD;
		}

		if (op == Instruction::JUMPDEST)
		{
			TRACE_OP(1, i, op);
			m_jumpDests.push_back(i);
		}
		else if ((byte)Instruction::PUSH1 <= (byte)op &&
		         (byte)op <= (byte)Instruction::PUSH32)
		{
			i += (byte)op - (byte)Instruction::PUSH1 + 1;
		}
		
	}

#ifdef EVM_DO_FIRST_PASS_OPTIMIZATION

	for (size_t i = 0; i < nBytes; ++i)
	{
		Instruction op = Instruction(m_code[i]);

		if ((byte)Instruction::PUSH1 <= (byte)op && (byte)op <= (byte)Instruction::PUSH32)
		{
			byte n = (byte)op - (byte)Instruction::PUSH1 + 1;

			// decode pushed bytes to integral value
			u256 val = m_code[i+1];
			for (uint64_t j = i+2, m = n; --m; ++j)
				val = (val << 8) | m_code[j];

		#ifdef EVM_USE_CONSTANT_POOL
			TRACE_PRE_OPT(1, i, op);
	
			// add value to constant pool and replace PUSHn with PUSHC if room
			if (1 < n)
			{
				int pool_off = poolConstant(val);
				if (0 <= pool_off && pool_off < 256)
				{
					m_code[i] = byte(op = Instruction::PUSHC);
					m_code[i+1] = (byte)pool_off;
					m_code[i+2] = n;
				}
			}
			
			TRACE_POST_OPT(1, i, op);
		#endif

		#ifdef EVM_REPLACE_CONST_JUMP	
			// replace JUMP or JUMPI to constant location with JUMPV or JUMPVI
			// verifyJumpDest is M = log(number of jump destinations)
			// outer loop is N = number of bytes in code array
			// so complexity is N log M, worst case is N log N
			size_t ii = i + n + 1;
			op = Instruction(m_code[ii]);
			if (op == Instruction::JUMP)
			{
				TRACE_PRE_OPT(1, ii, op);
				
				if (0 <= verifyJumpDest(val, false))
					m_code[ii] = byte(op = Instruction::JUMPV);
				
				TRACE_POST_OPT(1, ii, op);
			}
			else if (op == Instruction::JUMPI)
			{
				TRACE_PRE_OPT(1, ii, op);
				
				if (0 <= verifyJumpDest(val, false))
					m_code[ii] = byte(op = Instruction::JUMPVI);
				
				TRACE_POST_OPT(1, ii, op);
			}

		#endif
			
			i += n;
		}
		
	}	
#endif
	
	m_pool = m_poolSpace.data();
	
}
