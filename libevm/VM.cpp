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
/** @file VM.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include <libethereum/ExtVM.h>
#include "VMConfig.h"
#include "VM.h"
using namespace std;
using namespace dev;
using namespace dev::eth;


uint64_t VM::memNeed(u256 _offset, u256 _size)
{
	return toInt63(_size ? u512(_offset) + _size : u512(0));
}

template <class S> S divWorkaround(S const& _a, S const& _b)
{
	return (S)(s512(_a) / s512(_b));
}

template <class S> S modWorkaround(S const& _a, S const& _b)
{
	return (S)(s512(_a) % s512(_b));
}


//
// for decoding destinations of JUMPTO, JUMPV, JUMPSUB and JUMPSUBV
//

uint64_t VM::decodeJumpDest(const byte* const _code, uint64_t& _pc)
{
	// turn 4 MSB-first bytes in the code into a native-order integer
	uint64_t dest      = _code[_pc++];
	dest = (dest << 8) | _code[_pc++];
	dest = (dest << 8) | _code[_pc++];
	dest = (dest << 8) | _code[_pc++];
	return dest;
}

uint64_t VM::decodeJumpvDest(const byte* const _code, uint64_t& _pc, u256*& _sp)
{
	// Layout of jump table in bytecode...
	//     byte opcode
	//     byte n_jumps
	//     byte table[n_jumps][4]
	//	
	uint64_t i = uint64_t(*_sp--);  // byte on stack indexes into jump table
	uint64_t pc = _pc;
	byte n = _code[++pc];           // byte after opcode is number of jumps
	if (i >= n) i = n - 1;          // if index overflow use default jump
	pc += i * 4;                    // adjust pc to index destination in table
	
	uint64_t dest = decodeJumpDest(_code, pc);
	
	_pc += 1 + n * 4;               // adust input _pc to opcode after table 
	return dest;
}


//
// for tracing, checking, metering, measuring ...
//
void VM::onOperation()
{
	if (m_onOp)
		(m_onOp)(++m_nSteps, m_PC, m_OP,
			m_newMemSize > m_mem.size() ? (m_newMemSize - m_mem.size()) / 32 : uint64_t(0),
			m_runGas, m_io_gas, this, m_ext);
}

void VM::checkStack(unsigned _removed, unsigned _added)
{
	int const size = 1 + m_SP - m_stack;
	int const usedSize = size - _removed;
	if (usedSize < 0 || usedSize + _added > 1024)
		throwBadStack(size, _removed, _added);
}

uint64_t VM::gasForMem(u512 _size)
{
	u512 s = _size / 32;
	return toInt63((u512)m_schedule->memoryGas * s + s * s / m_schedule->quadCoeffDiv);
}

void VM::updateIOGas()
{
	if (m_io_gas < m_runGas)
		throwOutOfGas();
	m_io_gas -= m_runGas;
}

void VM::updateGas()
{
	if (m_newMemSize > m_mem.size())
		m_runGas += toInt63(gasForMem(m_newMemSize) - gasForMem(m_mem.size()));
	m_runGas += (m_schedule->copyGas * ((m_copyMemSize + 31) / 32));
	if (m_io_gas < m_runGas)
		throwOutOfGas();
}

void VM::updateMem()
{
	m_newMemSize = (m_newMemSize + 31) / 32 * 32;
	updateGas();
	if (m_newMemSize > m_mem.size())
		m_mem.resize(m_newMemSize);
}

void VM::logGasMem()
{
	unsigned n = (unsigned)m_OP - (unsigned)Instruction::LOG0;
	m_runGas = toInt63(m_schedule->logGas + m_schedule->logTopicGas * n + u512(m_schedule->logDataGas) * *(m_SP - 1));
	m_newMemSize = memNeed(*m_SP, *(m_SP - 1));
	updateMem();
}

void VM::fetchInstruction()
{
	m_OP = Instruction(m_code[m_PC]);
	const InstructionMetric& metric = c_metrics[static_cast<size_t>(m_OP)];
	checkStack(metric.args, metric.ret);

	// FEES...
	m_runGas = toInt63(m_schedule->tierStepGas[static_cast<unsigned>(metric.gasPriceTier)]);
	m_newMemSize = m_mem.size();
	m_copyMemSize = 0;
}

#if EVM_HACK_ON_OPERATION
	#define onOperation()
#endif
#if EVM_HACK_UPDATE_IO_GAS
	#define updateIOGas()
#endif
#if EVM_HACK_STACK
	#define checkStack(r,a)
#endif

///////////////////////////////////////////////////////////////////////////////
//
// interpreter entry point

owning_bytes_ref VM::exec(u256& _io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp)
{
	io_gas = &_io_gas;
	m_io_gas = uint64_t(_io_gas);
	m_ext = &_ext;
	m_schedule = &m_ext->evmSchedule();
	m_onOp = _onOp;
	m_onFail = &VM::onOperation;
	
	try
	{
		// trampoline to minimize depth of call stack when calling out
		m_bounce = &VM::initEntry;
		do
			(this->*m_bounce)();
		while (m_bounce);
		
	}
	catch (...)
	{
		*io_gas = m_io_gas;
		throw;
	}

	*io_gas = m_io_gas;
	return std::move(m_output);
}

//
// main interpreter loop and switch
//
void VM::interpretCases()
{
	INIT_CASES
	DO_CASES
	{	
		//
		// Call-related instructions
		//
		
		CASE(CREATE)
		{
			m_bounce = &VM::caseCreate;
		}
		BREAK;

		CASE(DELEGATECALL)

			// Pre-homestead
			if (!m_schedule->haveDelegateCall)
				throwBadInstruction();

		CASE(CALL)
		CASE(CALLCODE)
		{
			m_bounce = &VM::caseCall;
		}
		BREAK

		CASE(RETURN)
		{
			m_newMemSize = memNeed(*m_SP, *(m_SP - 1));
			updateMem();
			ON_OP();
			updateIOGas();

			uint64_t b = (uint64_t)*m_SP--;
			uint64_t s = (uint64_t)*m_SP--;
			m_output = owning_bytes_ref{std::move(m_mem), b, s};
			m_bounce = 0;
		}
		BREAK

		CASE(SUICIDE)
		{
			m_runGas = toInt63(m_schedule->suicideGas);
			Address dest = asAddress(*m_SP);

			// After EIP158 zero-value suicides do not have to pay account creation gas.
			if (m_ext->balance(m_ext->myAddress) > 0 || m_schedule->zeroValueTransferChargesNewAccountGas())
				// After EIP150 hard fork charge additional cost of sending
				// ethers to non-existing account.
				if (m_schedule->suicideChargesNewAccountGas() && !m_ext->exists(dest))
					m_runGas += m_schedule->callNewAccountGas;

			ON_OP();
			updateIOGas();
			m_ext->suicide(dest);
			m_bounce = 0;
		}
		BREAK

		CASE(STOP)
		{
			ON_OP();
			updateIOGas();
			m_bounce = 0;
		}
		BREAK;
			
			
		//
		// instructions potentially expanding memory
		//
		
		CASE(MLOAD)
		{
			m_newMemSize = toInt63(*m_SP) + 32;
			updateMem();
			ON_OP();
			updateIOGas();

			*m_SP = (u256)*(h256 const*)(m_mem.data() + (unsigned)*m_SP);
		}
		NEXT

		CASE(MSTORE)
		{
			m_newMemSize = toInt63(*m_SP) + 32;
			updateMem();
			ON_OP();
			updateIOGas();

			*(h256*)&m_mem[(unsigned)*m_SP] = (h256)*(m_SP - 1);
			m_SP -= 2;
		}
		NEXT

		CASE(MSTORE8)
		{
			m_newMemSize = toInt63(*m_SP) + 1;
			updateMem();
			ON_OP();
			updateIOGas();

			m_mem[(unsigned)*m_SP] = (byte)(*(m_SP - 1) & 0xff);
			m_SP -= 2;
		}
		NEXT

		CASE(SHA3)
		{
			m_runGas = toInt63(m_schedule->sha3Gas + (u512(*(m_SP - 1)) + 31) / 32 * m_schedule->sha3WordGas);
			m_newMemSize = memNeed(*m_SP, *(m_SP - 1));
			updateMem();
			ON_OP();
			updateIOGas();

			uint64_t inOff = (uint64_t)*m_SP--;
			uint64_t inSize = (uint64_t)*m_SP--;
			*++m_SP = (u256)sha3(bytesConstRef(m_mem.data() + inOff, inSize));
		}
		NEXT

		CASE(LOG0)
		{
			logGasMem();
			ON_OP();
			updateIOGas();

			m_ext->log({}, bytesConstRef(m_mem.data() + (uint64_t)*m_SP, (uint64_t)*(m_SP - 1)));
			m_SP -= 2;
		}
		NEXT

		CASE(LOG1)
		{
			logGasMem();
			ON_OP();
			updateIOGas();

			m_ext->log({*(m_SP - 2)}, bytesConstRef(m_mem.data() + (uint64_t)*m_SP, (uint64_t)*(m_SP - 1)));
			m_SP -= 3;
		}
		NEXT

		CASE(LOG2)
		{
			logGasMem();
			ON_OP();
			updateIOGas();

			m_ext->log({*(m_SP - 2), *(m_SP-3)}, bytesConstRef(m_mem.data() + (uint64_t)*m_SP, (uint64_t)*(m_SP - 1)));
			m_SP -= 4;
		}
		NEXT

		CASE(LOG3)
		{
			logGasMem();
			ON_OP();
			updateIOGas();

			m_ext->log({*(m_SP - 2), *(m_SP-3), *(m_SP-4)}, bytesConstRef(m_mem.data() + (uint64_t)*m_SP, (uint64_t)*(m_SP - 1)));
			m_SP -= 5;
		}
		NEXT

		CASE(LOG4)
		{
			logGasMem();
			ON_OP();
			updateIOGas();

			m_ext->log({*(m_SP - 2), *(m_SP-3), *(m_SP-4), *(m_SP-5)}, bytesConstRef(m_mem.data() + (uint64_t)*m_SP, (uint64_t)*(m_SP - 1)));
			m_SP -= 6;
		}
		NEXT	

		CASE(EXP)
		{
			u256 expon = *(m_SP - 1);
			m_runGas = toInt63(m_schedule->expGas + m_schedule->expByteGas * (32 - (h256(expon).firstBitSet() / 8)));
			ON_OP();
			updateIOGas();

			u256 base = *m_SP--;
			*m_SP = exp256(base, expon);
		}
		NEXT

		//
		// ordinary instructions
		//

		CASE(ADD)
		{
			ON_OP();
			updateIOGas();

			//pops two items and pushes S[-1] + S[-2] mod 2^256.
			*(m_SP - 1) += *m_SP;
			--m_SP;
		}
		NEXT

		CASE(MUL)
		{
			ON_OP();
			updateIOGas();

			//pops two items and pushes S[-1] * S[-2] mod 2^256.
#if EVM_HACK_MUL_64
			*(uint64_t*)(m_SP - 1) *= *(uint64_t*)m_SP;
#else
			*(m_SP - 1) *= *m_SP;
#endif
			--m_SP;
		}
		NEXT

		CASE(SUB)
		{
			ON_OP();
			updateIOGas();

			*(m_SP - 1) = *m_SP - *(m_SP - 1);
			--m_SP;
		}
		NEXT

		CASE(DIV)
		{
			ON_OP();
			updateIOGas();

			*(m_SP - 1) = *(m_SP - 1) ? divWorkaround(*m_SP, *(m_SP - 1)) : 0;
			--m_SP;
		}
		NEXT

		CASE(SDIV)
		{
			ON_OP();
			updateIOGas();

			*(m_SP - 1) = *(m_SP - 1) ? s2u(divWorkaround(u2s(*m_SP), u2s(*(m_SP - 1)))) : 0;
			--m_SP;
		}
		NEXT

		CASE(MOD)
		{
			ON_OP();
			updateIOGas();

			*(m_SP - 1) = *(m_SP - 1) ? modWorkaround(*m_SP, *(m_SP - 1)) : 0;
			--m_SP;
		}
		NEXT

		CASE(SMOD)
		{
			ON_OP();
			updateIOGas();

			*(m_SP - 1) = *(m_SP - 1) ? s2u(modWorkaround(u2s(*m_SP), u2s(*(m_SP - 1)))) : 0;
			--m_SP;
		}
		NEXT

		CASE(NOT)
		{
			ON_OP();
			updateIOGas();

			*m_SP = ~*m_SP;
		}
		NEXT

		CASE(LT)
		{
			ON_OP();
			updateIOGas();

			*(m_SP - 1) = *m_SP < *(m_SP - 1) ? 1 : 0;
			--m_SP;
		}
		NEXT

		CASE(GT)
		{
			ON_OP();
			updateIOGas();

			*(m_SP - 1) = *m_SP > *(m_SP - 1) ? 1 : 0;
			--m_SP;
		}
		NEXT

		CASE(SLT)
		{
			ON_OP();
			updateIOGas();

			*(m_SP - 1) = u2s(*m_SP) < u2s(*(m_SP - 1)) ? 1 : 0;
			--m_SP;
		}
		NEXT

		CASE(SGT)
		{
			ON_OP();
			updateIOGas();

			*(m_SP - 1) = u2s(*m_SP) > u2s(*(m_SP - 1)) ? 1 : 0;
			--m_SP;
		}
		NEXT

		CASE(EQ)
		{
			ON_OP();
			updateIOGas();

			*(m_SP - 1) = *m_SP == *(m_SP - 1) ? 1 : 0;
			--m_SP;
		}
		NEXT

		CASE(ISZERO)
		{
			ON_OP();
			updateIOGas();

			*m_SP = *m_SP ? 0 : 1;
		}
		NEXT

		CASE(AND)
		{
			ON_OP();
			updateIOGas();

			*(m_SP - 1) = *m_SP & *(m_SP - 1);
			--m_SP;
		}
		NEXT

		CASE(OR)
		{
			ON_OP();
			updateIOGas();

			*(m_SP - 1) = *m_SP | *(m_SP - 1);
			--m_SP;
		}
		NEXT

		CASE(XOR)
		{
			ON_OP();
			updateIOGas();

			*(m_SP - 1) = *m_SP ^ *(m_SP - 1);
			--m_SP;
		}
		NEXT

		CASE(BYTE)
		{
			ON_OP();
			updateIOGas();

			*(m_SP - 1) = *m_SP < 32 ? (*(m_SP - 1) >> (unsigned)(8 * (31 - *m_SP))) & 0xff : 0;
			--m_SP;
		}
		NEXT

		CASE(ADDMOD)
		{
			ON_OP();
			updateIOGas();

			*(m_SP - 2) = *(m_SP - 2) ? u256((u512(*m_SP) + u512(*(m_SP - 1))) % *(m_SP - 2)) : 0;
			m_SP -= 2;
		}
		NEXT

		CASE(MULMOD)
		{
			ON_OP();
			updateIOGas();

			*(m_SP - 2) = *(m_SP - 2) ? u256((u512(*m_SP) * u512(*(m_SP - 1))) % *(m_SP - 2)) : 0;
			m_SP -= 2;
		}
		NEXT

		CASE(SIGNEXTEND)
		{
			ON_OP();
			updateIOGas();

			if (*m_SP < 31)
			{
				unsigned testBit = static_cast<unsigned>(*m_SP) * 8 + 7;
				u256& number = *(m_SP - 1);
				u256 mask = ((u256(1) << testBit) - 1);
				if (boost::multiprecision::bit_test(number, testBit))
					number |= ~mask;
				else
					number &= mask;
			}
			--m_SP;
		}
		NEXT

		CASE(ADDRESS)
		{
			ON_OP();
			updateIOGas();

			*++m_SP = fromAddress(m_ext->myAddress);
		}
		NEXT

		CASE(ORIGIN)
		{
			ON_OP();
			updateIOGas();

			*++m_SP = fromAddress(m_ext->origin);
		}
		NEXT

		CASE(BALANCE)
		{
			m_runGas = toInt63(m_schedule->balanceGas);
			ON_OP();
			updateIOGas();

			*m_SP = m_ext->balance(asAddress(*m_SP));
		}
		NEXT


		CASE(CALLER)
		{
			ON_OP();
			updateIOGas();

			*++m_SP = fromAddress(m_ext->caller);
		}
		NEXT

		CASE(CALLVALUE)
		{
			ON_OP();
			updateIOGas();

			*++m_SP = m_ext->value;
		}
		NEXT


		CASE(CALLDATALOAD)
		{
			ON_OP();
			updateIOGas();

			if (u512(*m_SP) + 31 < m_ext->data.size())
				*m_SP = (u256)*(h256 const*)(m_ext->data.data() + (size_t)*m_SP);
			else if (*m_SP >= m_ext->data.size())
				*m_SP = u256(0);
			else
			{
				h256 r;
				for (uint64_t i = (uint64_t)*m_SP, e = (uint64_t)*m_SP + (uint64_t)32, j = 0; i < e; ++i, ++j)
					r[j] = i < m_ext->data.size() ? m_ext->data[i] : 0;
				*m_SP = (u256)r;
			}
		}
		NEXT


		CASE(CALLDATASIZE)
		{
			ON_OP();
			updateIOGas();

			*++m_SP = m_ext->data.size();
		}
		NEXT

		CASE(CODESIZE)
		{
			ON_OP();
			updateIOGas();

			*++m_SP = m_ext->code.size();
		}
		NEXT

		CASE(EXTCODESIZE)
		{
			m_runGas = toInt63(m_schedule->extcodesizeGas);
			ON_OP();
			updateIOGas();

			*m_SP = m_ext->codeSizeAt(asAddress(*m_SP));
		}
		NEXT

		CASE(CALLDATACOPY)
		{
			m_copyMemSize = toInt63(*(m_SP - 2));
			m_newMemSize = memNeed(*m_SP, *(m_SP - 2));
			updateMem();
			ON_OP();
			updateIOGas();

			copyDataToMemory(m_ext->data, m_SP);
		}
		NEXT

		CASE(CODECOPY)
		{
			m_copyMemSize = toInt63(*(m_SP - 2));
			m_newMemSize = memNeed(*m_SP, *(m_SP - 2));
			updateMem();
			ON_OP();
			updateIOGas();

			copyDataToMemory(&m_ext->code, m_SP);
		}
		NEXT

		CASE(EXTCODECOPY)
		{
			m_runGas = toInt63(m_schedule->extcodecopyGas);
			m_copyMemSize = toInt63(*(m_SP - 3));
			m_newMemSize = memNeed(*(m_SP - 1), *(m_SP - 3));
			updateMem();
			ON_OP();
			updateIOGas();

			Address a = asAddress(*m_SP);
			--m_SP;
			copyDataToMemory(&m_ext->codeAt(a), m_SP);
		}
		NEXT


		CASE(GASPRICE)
		{
			ON_OP();
			updateIOGas();

			*++m_SP = m_ext->gasPrice;
		}
		NEXT

		CASE(BLOCKHASH)
		{
			ON_OP();
			updateIOGas();

			*m_SP = (u256)m_ext->blockHash(*m_SP);
		}
		NEXT

		CASE(COINBASE)
		{
			ON_OP();
			updateIOGas();

			*++m_SP = (u160)m_ext->envInfo().author();
		}
		NEXT

		CASE(TIMESTAMP)
		{
			ON_OP();
			updateIOGas();

			*++m_SP = m_ext->envInfo().timestamp();
		}
		NEXT

		CASE(NUMBER)
		{
			ON_OP();
			updateIOGas();

			*++m_SP = m_ext->envInfo().number();
		}
		NEXT

		CASE(DIFFICULTY)
		{
			ON_OP();
			updateIOGas();

			*++m_SP = m_ext->envInfo().difficulty();
		}
		NEXT

		CASE(GASLIMIT)
		{
			ON_OP();
			updateIOGas();

			*++m_SP = m_ext->envInfo().gasLimit();
		}
		NEXT

		CASE(POP)
		{
			ON_OP();
			updateIOGas();

			--m_SP;
		}
		NEXT

		CASE(PUSHC)
		{
#ifdef EVM_USE_CONSTANT_POOL
			ON_OP();
			updateIOGas();

			++m_PC;
			*++m_SP = m_pool[m_code[m_PC]];
			++m_PC;
			m_PC += m_code[m_PC];
#else
			throwBadInstruction();
#endif
		}
		CONTINUE

		CASE(PUSH1)
		{
			ON_OP();
			updateIOGas();
			*++m_SP = m_code[++m_PC];
			++m_PC;
		}
		CONTINUE

		CASE(PUSH2)
		CASE(PUSH3)
		CASE(PUSH4)
		CASE(PUSH5)
		CASE(PUSH6)
		CASE(PUSH7)
		CASE(PUSH8)
		CASE(PUSH9)
		CASE(PUSH10)
		CASE(PUSH11)
		CASE(PUSH12)
		CASE(PUSH13)
		CASE(PUSH14)
		CASE(PUSH15)
		CASE(PUSH16)
		CASE(PUSH17)
		CASE(PUSH18)
		CASE(PUSH19)
		CASE(PUSH20)
		CASE(PUSH21)
		CASE(PUSH22)
		CASE(PUSH23)
		CASE(PUSH24)
		CASE(PUSH25)
		CASE(PUSH26)
		CASE(PUSH27)
		CASE(PUSH28)
		CASE(PUSH29)
		CASE(PUSH30)
		CASE(PUSH31)
		CASE(PUSH32)
		{
			ON_OP();
			updateIOGas();

			int numBytes = (int)m_OP - (int)Instruction::PUSH1 + 1;
			*++m_SP = 0;
			// Construct a number out of PUSH bytes.
			// This requires the code has been copied and extended by 32 zero
			// bytes to handle "out of code" push data here.
			for (++m_PC; numBytes--; ++m_PC)
				*m_SP = (*m_SP << 8) | m_code[m_PC];
		}
		CONTINUE

		CASE(JUMP)
		{
			ON_OP();
			updateIOGas();

			m_PC = verifyJumpDest(*m_SP);
			--m_SP;
		}
		CONTINUE

		CASE(JUMPI)
		{
			ON_OP();
			updateIOGas();
			if (*(m_SP - 1))
				m_PC = verifyJumpDest(*m_SP);
			else
				++m_PC;
			m_SP -= 2;
		}
		CONTINUE

#if EVM_JUMPS_AND_SUBS
		CASE(JUMPTO)
		{
			ON_OP();
			updateIOGas();
			m_PC = decodeJumpDest(m_code, m_PC);
		}
		CONTINUE

		CASE(JUMPIF)
		{
			ON_OP();
			updateIOGas();
			if (*m_SP)
				m_PC = decodeJumpDest(m_code, m_PC);
			else
				++m_PC;
			--m_SP;
		}
		CONTINUE

		CASE(JUMPV)
		{
			ON_OP();
			updateIOGas();			
			m_PC = decodeJumpvDest(m_code, m_PC, m_SP);
		}
		CONTINUE

		CASE(JUMPSUB)
		{
			ON_OP();
			updateIOGas();
			{
				*++m_RP = m_PC;
				m_PC = decodeJumpDest(m_code, m_PC);
			}
		}
		CONTINUE

		CASE(JUMPSUBV)
		{
			ON_OP();
			updateIOGas();
			{
				*++m_RP = m_PC;
				m_PC = decodeJumpDest(m_code, m_PC);
			}
		}
		CONTINUE

		CASE(RETURNSUB)
		{
			ON_OP();
			updateIOGas();
			
			m_PC = *m_RP--;
		}
		NEXT
#else
		CASE(JUMPTO)
		CASE(JUMPIF)
		CASE(JUMPV)
		CASE(JUMPSUB)
		CASE(JUMPSUBV)
		CASE(RETURNSUB)
		{
			throwBadInstruction();
		}
		CONTINUE
#endif

		CASE(JUMPC)
		{
#ifdef EVM_REPLACE_CONST_JUMP
			ON_OP();
			updateIOGas();

			m_PC = uint64_t(*m_SP);
			--m_SP;
#else
			throwBadInstruction();
#endif
		}
		CONTINUE

		CASE(JUMPCI)
		{
#ifdef EVM_REPLACE_CONST_JUMP
			ON_OP();
			updateIOGas();

			if (*(m_SP - 1))
				m_PC = uint64_t(*m_SP);
			else
				++m_PC;
			m_SP -= 2;
#else
			throwBadInstruction();
#endif
		}
		CONTINUE

		CASE(DUP1)
		CASE(DUP2)
		CASE(DUP3)
		CASE(DUP4)
		CASE(DUP5)
		CASE(DUP6)
		CASE(DUP7)
		CASE(DUP8)
		CASE(DUP9)
		CASE(DUP10)
		CASE(DUP11)
		CASE(DUP12)
		CASE(DUP13)
		CASE(DUP14)
		CASE(DUP15)
		CASE(DUP16)
		{
			ON_OP();
			updateIOGas();

			unsigned n = 1 + (unsigned)m_OP - (unsigned)Instruction::DUP1;
#if EVM_HACK_DUP_64
			*(uint64_t*)(m_SP+1) = *(uint64_t*)&m_stack[(1 + m_SP - m_stack) - n];
#else
			*(m_SP+1) = m_stack[(1 + m_SP - m_stack) - n];
#endif
			++m_SP;
		}
		NEXT


		CASE(SWAP1)
		CASE(SWAP2)
		CASE(SWAP3)
		CASE(SWAP4)
		CASE(SWAP5)
		CASE(SWAP6)
		CASE(SWAP7)
		CASE(SWAP8)
		CASE(SWAP9)
		CASE(SWAP10)
		CASE(SWAP11)
		CASE(SWAP12)
		CASE(SWAP13)
		CASE(SWAP14)
		CASE(SWAP15)
		CASE(SWAP16)
		{
			ON_OP();
			updateIOGas();

			unsigned n = (unsigned)m_OP - (unsigned)Instruction::SWAP1 + 2;
			u256 d = *m_SP;
			*m_SP = m_stack[(1 + m_SP - m_stack) - n];
			m_stack[(1 + m_SP - m_stack) - n] = d;
		}
		NEXT


		CASE(SLOAD)
		{
			m_runGas = toInt63(m_schedule->sloadGas);
			ON_OP();
			updateIOGas();

			*m_SP = m_ext->store(*m_SP);
		}
		NEXT

		CASE(SSTORE)
		{
			if (!m_ext->store(*m_SP) && *(m_SP - 1))
				m_runGas = toInt63(m_schedule->sstoreSetGas);
			else if (m_ext->store(*m_SP) && !*(m_SP - 1))
			{
				m_runGas = toInt63(m_schedule->sstoreResetGas);
				m_ext->sub.refunds += m_schedule->sstoreRefundGas;
			}
			else
				m_runGas = toInt63(m_schedule->sstoreResetGas);
			ON_OP();
			updateIOGas();
	
			m_ext->setStore(*m_SP, *(m_SP - 1));
			m_SP -= 2;
		}
		NEXT

		CASE(PC)
		{
			ON_OP();
			updateIOGas();

			*++m_SP = m_PC;
		}
		NEXT

		CASE(MSIZE)
		{
			ON_OP();
			updateIOGas();

			*++m_SP = m_mem.size();
		}
		NEXT

		CASE(GAS)
		{
			ON_OP();
			updateIOGas();

			*++m_SP = m_io_gas;
		}
		NEXT

		CASE(JUMPDEST)
		{
			m_runGas = 1;
			ON_OP();
			updateIOGas();
		}
		NEXT

#if EVM_JUMPS_AND_SUBS
		CASE(BEGINSUB)
		{
			m_runGas = 1;
			ON_OP();
			updateIOGas();
		}
		NEXT
#else
		CASE(BEGINSUB)
#endif
		CASE(BEGINDATA)
		CASE(BAD)
		DEFAULT
			throwBadInstruction();
	}	
	WHILE_CASES
}
