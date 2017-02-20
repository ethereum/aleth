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
	return toUint64(_size ? u512(_offset) + _size : u512(0));
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
	pc += 1 + i * 4;                // adjust pc to index destination in table
	
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
		(m_onOp)(++m_nSteps, m_pc, m_op,
			m_newMemSize > m_mem.size() ? (m_newMemSize - m_mem.size()) / 32 : uint64_t(0),
			m_runGas, m_io_gas, this, m_ext);
}

void VM::checkStack(unsigned _removed, unsigned _added)
{
	int const size = 1 + m_sp - m_stack;
	int const usedSize = size - _removed;
	if (usedSize < 0 || usedSize + _added > 1024)
		throwBadStack(size, _removed, _added);
}

uint64_t VM::gasForMem(u512 _size)
{
	u512 s = _size / 32;
	return toUint64((u512)m_schedule->memoryGas * s + s * s / m_schedule->quadCoeffDiv);
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
		m_runGas += toUint64(gasForMem(m_newMemSize) - gasForMem(m_mem.size()));
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
	unsigned n = (unsigned)m_op - (unsigned)Instruction::LOG0;
	m_runGas = toUint64(m_schedule->logGas + m_schedule->logTopicGas * n + u512(m_schedule->logDataGas) * *(m_sp - 1));
	m_newMemSize = memNeed(*m_sp, *(m_sp - 1));
	updateMem();
}

void VM::fetchInstruction()
{
	m_op = Instruction(m_code[m_pc]);
	const InstructionMetric& metric = c_metrics[static_cast<size_t>(m_op)];
	checkStack(metric.args, metric.ret);

	// FEES...
	m_runGas = toUint64(m_schedule->tierStepGas[static_cast<unsigned>(metric.gasPriceTier)]);
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
		
		//
		// Call-related instructions
		//
		
		CASE_BEGIN(CREATE)
			m_bounce = &VM::caseCreate;
		CASE_RETURN;

		CASE_BEGIN(DELEGATECALL)

			// Pre-homestead
			if (!m_schedule->haveDelegateCall)
				throwBadInstruction();

		CASE_BEGIN(CALL)
		CASE_BEGIN(CALLCODE)
			m_bounce = &VM::caseCall;
		CASE_RETURN

		CASE_BEGIN(RETURN)
		{
			m_newMemSize = memNeed(*m_sp, *(m_sp - 1));
			updateMem();
			ON_OP();
			updateIOGas();

			uint64_t b = (uint64_t)*m_sp--;
			uint64_t s = (uint64_t)*m_sp--;
			m_output = owning_bytes_ref{std::move(m_mem), b, s};
			m_bounce = 0;
		}
		CASE_RETURN

		CASE_BEGIN(SUICIDE)
		{
			m_runGas = toUint64(m_schedule->suicideGas);
			Address dest = asAddress(*m_sp);

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
		CASE_RETURN

		CASE_BEGIN(STOP)
			ON_OP();
			updateIOGas();
			m_bounce = 0;
		CASE_RETURN;
			
			
		//
		// instructions potentially expanding memory
		//
		
		CASE_BEGIN(MLOAD)
		{
			m_newMemSize = toUint64(*m_sp) + 32;
			updateMem();
			ON_OP();
			updateIOGas();

			*m_sp = (u256)*(h256 const*)(m_mem.data() + (unsigned)*m_sp);
			++m_pc;
		}
		CASE_END

		CASE_BEGIN(MSTORE)
		{
			m_newMemSize = toUint64(*m_sp) + 32;
			updateMem();
			ON_OP();
			updateIOGas();

			*(h256*)&m_mem[(unsigned)*m_sp] = (h256)*(m_sp - 1);
			m_sp -= 2;
			++m_pc;
		}
		CASE_END

		CASE_BEGIN(MSTORE8)
		{
			m_newMemSize = toUint64(*m_sp) + 1;
			updateMem();
			ON_OP();
			updateIOGas();

			m_mem[(unsigned)*m_sp] = (byte)(*(m_sp - 1) & 0xff);
			m_sp -= 2;
			++m_pc;
		}
		CASE_END

		CASE_BEGIN(SHA3)
		{
			m_runGas = toUint64(m_schedule->sha3Gas + (u512(*(m_sp - 1)) + 31) / 32 * m_schedule->sha3WordGas);
			m_newMemSize = memNeed(*m_sp, *(m_sp - 1));
			updateMem();
			ON_OP();
			updateIOGas();

			uint64_t inOff = (uint64_t)*m_sp--;
			uint64_t inSize = (uint64_t)*m_sp--;
			*++m_sp = (u256)sha3(bytesConstRef(m_mem.data() + inOff, inSize));
			++m_pc;
		}
		CASE_END

		CASE_BEGIN(LOG0)
			logGasMem();
			ON_OP();
			updateIOGas();

			m_ext->log({}, bytesConstRef(m_mem.data() + (uint64_t)*m_sp, (uint64_t)*(m_sp - 1)));
			m_sp -= 2;
			++m_pc;
		CASE_END

		CASE_BEGIN(LOG1)
			logGasMem();
			ON_OP();
			updateIOGas();

			m_ext->log({*(m_sp - 2)}, bytesConstRef(m_mem.data() + (uint64_t)*m_sp, (uint64_t)*(m_sp - 1)));
			m_sp -= 3;
			++m_pc;
		CASE_END

		CASE_BEGIN(LOG2)
			logGasMem();
			ON_OP();
			updateIOGas();

			m_ext->log({*(m_sp - 2), *(m_sp-3)}, bytesConstRef(m_mem.data() + (uint64_t)*m_sp, (uint64_t)*(m_sp - 1)));
			m_sp -= 4;
			++m_pc;
		CASE_END

		CASE_BEGIN(LOG3)
			logGasMem();
			ON_OP();
			updateIOGas();

			m_ext->log({*(m_sp - 2), *(m_sp-3), *(m_sp-4)}, bytesConstRef(m_mem.data() + (uint64_t)*m_sp, (uint64_t)*(m_sp - 1)));
			m_sp -= 5;
			++m_pc;
		CASE_END

		CASE_BEGIN(LOG4)
			logGasMem();
			ON_OP();
			updateIOGas();

			m_ext->log({*(m_sp - 2), *(m_sp-3), *(m_sp-4), *(m_sp-5)}, bytesConstRef(m_mem.data() + (uint64_t)*m_sp, (uint64_t)*(m_sp - 1)));
			m_sp -= 6;
			++m_pc;
		CASE_END	

		CASE_BEGIN(EXP)
		{
			u256 expon = *(m_sp - 1);
			m_runGas = toUint64(m_schedule->expGas + m_schedule->expByteGas * (32 - (h256(expon).firstBitSet() / 8)));
			ON_OP();
			updateIOGas();

			u256 base = *m_sp--;
			*m_sp = exp256(base, expon);
			++m_pc;
		}
		CASE_END

		//
		// ordinary instructions
		//

		CASE_BEGIN(ADD)
			ON_OP();
			updateIOGas();

			//pops two items and pushes S[-1] + S[-2] mod 2^256.
			*(m_sp - 1) += *m_sp;
			--m_sp;
			++m_pc;
		CASE_END

		CASE_BEGIN(MUL)
			ON_OP();
			updateIOGas();

			//pops two items and pushes S[-1] * S[-2] mod 2^256.
#if EVM_HACK_MUL_64
			*(uint64_t*)(m_sp - 1) *= *(uint64_t*)m_sp;
#else
			*(m_sp - 1) *= *m_sp;
#endif
			--m_sp;
			++m_pc;
		CASE_END

		CASE_BEGIN(SUB)
			ON_OP();
			updateIOGas();

			*(m_sp - 1) = *m_sp - *(m_sp - 1);
			--m_sp;
			++m_pc;
		CASE_END

		CASE_BEGIN(DIV)
			ON_OP();
			updateIOGas();

			*(m_sp - 1) = *(m_sp - 1) ? divWorkaround(*m_sp, *(m_sp - 1)) : 0;
			--m_sp;
			++m_pc;
		CASE_END

		CASE_BEGIN(SDIV)
			ON_OP();
			updateIOGas();

			*(m_sp - 1) = *(m_sp - 1) ? s2u(divWorkaround(u2s(*m_sp), u2s(*(m_sp - 1)))) : 0;
			--m_sp;
			++m_pc;
		CASE_END

		CASE_BEGIN(MOD)
			ON_OP();
			updateIOGas();

			*(m_sp - 1) = *(m_sp - 1) ? modWorkaround(*m_sp, *(m_sp - 1)) : 0;
			--m_sp;
			++m_pc;
		CASE_END

		CASE_BEGIN(SMOD)
			ON_OP();
			updateIOGas();

			*(m_sp - 1) = *(m_sp - 1) ? s2u(modWorkaround(u2s(*m_sp), u2s(*(m_sp - 1)))) : 0;
			--m_sp;
			++m_pc;
		CASE_END

		CASE_BEGIN(NOT)
			ON_OP();
			updateIOGas();

			*m_sp = ~*m_sp;
			++m_pc;
		CASE_END

		CASE_BEGIN(LT)
			ON_OP();
			updateIOGas();

			*(m_sp - 1) = *m_sp < *(m_sp - 1) ? 1 : 0;
			--m_sp;
			++m_pc;
		CASE_END

		CASE_BEGIN(GT)
			ON_OP();
			updateIOGas();

			*(m_sp - 1) = *m_sp > *(m_sp - 1) ? 1 : 0;
			--m_sp;
			++m_pc;
		CASE_END

		CASE_BEGIN(SLT)
			ON_OP();
			updateIOGas();

			*(m_sp - 1) = u2s(*m_sp) < u2s(*(m_sp - 1)) ? 1 : 0;
			--m_sp;
			++m_pc;
		CASE_END

		CASE_BEGIN(SGT)
			ON_OP();
			updateIOGas();

			*(m_sp - 1) = u2s(*m_sp) > u2s(*(m_sp - 1)) ? 1 : 0;
			--m_sp;
			++m_pc;
		CASE_END

		CASE_BEGIN(EQ)
			ON_OP();
			updateIOGas();

			*(m_sp - 1) = *m_sp == *(m_sp - 1) ? 1 : 0;
			--m_sp;
			++m_pc;
		CASE_END

		CASE_BEGIN(ISZERO)
			ON_OP();
			updateIOGas();

			*m_sp = *m_sp ? 0 : 1;
			++m_pc;
		CASE_END

		CASE_BEGIN(AND)
			ON_OP();
			updateIOGas();

			*(m_sp - 1) = *m_sp & *(m_sp - 1);
			--m_sp;
			++m_pc;
		CASE_END

		CASE_BEGIN(OR)
			ON_OP();
			updateIOGas();

			*(m_sp - 1) = *m_sp | *(m_sp - 1);
			--m_sp;
			++m_pc;
		CASE_END

		CASE_BEGIN(XOR)
			ON_OP();
			updateIOGas();

			*(m_sp - 1) = *m_sp ^ *(m_sp - 1);
			--m_sp;
			++m_pc;
		CASE_END

		CASE_BEGIN(BYTE)
			ON_OP();
			updateIOGas();

			*(m_sp - 1) = *m_sp < 32 ? (*(m_sp - 1) >> (unsigned)(8 * (31 - *m_sp))) & 0xff : 0;
			--m_sp;
			++m_pc;
		CASE_END

		CASE_BEGIN(ADDMOD)
			ON_OP();
			updateIOGas();

			*(m_sp - 2) = *(m_sp - 2) ? u256((u512(*m_sp) + u512(*(m_sp - 1))) % *(m_sp - 2)) : 0;
			m_sp -= 2;
			++m_pc;
		CASE_END

		CASE_BEGIN(MULMOD)
			ON_OP();
			updateIOGas();

			*(m_sp - 2) = *(m_sp - 2) ? u256((u512(*m_sp) * u512(*(m_sp - 1))) % *(m_sp - 2)) : 0;
			m_sp -= 2;
			++m_pc;
		CASE_END

		CASE_BEGIN(SIGNEXTEND)
			ON_OP();
			updateIOGas();

			if (*m_sp < 31)
			{
				unsigned testBit = static_cast<unsigned>(*m_sp) * 8 + 7;
				u256& number = *(m_sp - 1);
				u256 mask = ((u256(1) << testBit) - 1);
				if (boost::multiprecision::bit_test(number, testBit))
					number |= ~mask;
				else
					number &= mask;
			}
			--m_sp;
			++m_pc;
		CASE_END

		CASE_BEGIN(ADDRESS)
			ON_OP();
			updateIOGas();

			*++m_sp = fromAddress(m_ext->myAddress);
			++m_pc;
		CASE_END

		CASE_BEGIN(ORIGIN)
			ON_OP();
			updateIOGas();

			*++m_sp = fromAddress(m_ext->origin);
			++m_pc;
		CASE_END

		CASE_BEGIN(BALANCE)
		{
			m_runGas = toUint64(m_schedule->balanceGas);
			ON_OP();
			updateIOGas();

			*m_sp = m_ext->balance(asAddress(*m_sp));
			++m_pc;
		}
		CASE_END


		CASE_BEGIN(CALLER)
			ON_OP();
			updateIOGas();

			*++m_sp = fromAddress(m_ext->caller);
			++m_pc;
		CASE_END

		CASE_BEGIN(CALLVALUE)
			ON_OP();
			updateIOGas();

			*++m_sp = m_ext->value;
			++m_pc;
		CASE_END


		CASE_BEGIN(CALLDATALOAD)
		{
			ON_OP();
			updateIOGas();

			if (u512(*m_sp) + 31 < m_ext->data.size())
				*m_sp = (u256)*(h256 const*)(m_ext->data.data() + (size_t)*m_sp);
			else if (*m_sp >= m_ext->data.size())
				*m_sp = u256(0);
			else
			{
				h256 r;
				for (uint64_t i = (uint64_t)*m_sp, e = (uint64_t)*m_sp + (uint64_t)32, j = 0; i < e; ++i, ++j)
					r[j] = i < m_ext->data.size() ? m_ext->data[i] : 0;
				*m_sp = (u256)r;
			}
			++m_pc;
		}
		CASE_END


		CASE_BEGIN(CALLDATASIZE)
			ON_OP();
			updateIOGas();

			*++m_sp = m_ext->data.size();
			++m_pc;
		CASE_END

		CASE_BEGIN(CODESIZE)
			ON_OP();
			updateIOGas();

			*++m_sp = m_ext->code.size();
			++m_pc;
		CASE_END

		CASE_BEGIN(EXTCODESIZE)
			m_runGas = toUint64(m_schedule->extcodesizeGas);
			ON_OP();
			updateIOGas();

			*m_sp = m_ext->codeSizeAt(asAddress(*m_sp));
			++m_pc;
		CASE_END

		CASE_BEGIN(CALLDATACOPY)
			m_copyMemSize = toUint64(*(m_sp - 2));
			m_newMemSize = memNeed(*m_sp, *(m_sp - 2));
			updateMem();
			ON_OP();
			updateIOGas();

			copyDataToMemory(m_ext->data, m_sp);
			++m_pc;
		CASE_END

		CASE_BEGIN(CODECOPY)
			m_copyMemSize = toUint64(*(m_sp - 2));
			m_newMemSize = memNeed(*m_sp, *(m_sp - 2));
			updateMem();
			ON_OP();
			updateIOGas();

			copyDataToMemory(&m_ext->code, m_sp);
			++m_pc;
		CASE_END

		CASE_BEGIN(EXTCODECOPY)
		{
			m_runGas = toUint64(m_schedule->extcodecopyGas);
			m_copyMemSize = toUint64(*(m_sp - 3));
			m_newMemSize = memNeed(*(m_sp - 1), *(m_sp - 3));
			updateMem();
			ON_OP();
			updateIOGas();

			Address a = asAddress(*m_sp);
			--m_sp;
			copyDataToMemory(&m_ext->codeAt(a), m_sp);
			++m_pc;
		}
		CASE_END


		CASE_BEGIN(GASPRICE)
			ON_OP();
			updateIOGas();

			*++m_sp = m_ext->gasPrice;
			++m_pc;
		CASE_END

		CASE_BEGIN(BLOCKHASH)
			ON_OP();
			updateIOGas();

			*m_sp = (u256)m_ext->blockHash(*m_sp);
			++m_pc;
		CASE_END

		CASE_BEGIN(COINBASE)
			ON_OP();
			updateIOGas();

			*++m_sp = (u160)m_ext->envInfo().author();
			++m_pc;
		CASE_END

		CASE_BEGIN(TIMESTAMP)
			ON_OP();
			updateIOGas();

			*++m_sp = m_ext->envInfo().timestamp();
			++m_pc;
		CASE_END

		CASE_BEGIN(NUMBER)
			ON_OP();
			updateIOGas();

			*++m_sp = m_ext->envInfo().number();
			++m_pc;
		CASE_END

		CASE_BEGIN(DIFFICULTY)
			ON_OP();
			updateIOGas();

			*++m_sp = m_ext->envInfo().difficulty();
			++m_pc;
		CASE_END

		CASE_BEGIN(GASLIMIT)
			ON_OP();
			updateIOGas();

			*++m_sp = m_ext->envInfo().gasLimit();
			++m_pc;
		CASE_END

		CASE_BEGIN(POP)
			ON_OP();
			updateIOGas();

			--m_sp;
			++m_pc;
		CASE_END

		CASE_BEGIN(PUSHC)
#ifdef EVM_USE_CONSTANT_POOL
		{
			ON_OP();
			updateIOGas();

			++m_pc;
			*++m_sp = m_pool[m_code[m_pc]];
			++m_pc;
			m_pc += m_code[m_pc];
		}
#else
			throwBadInstruction();
#endif
		CASE_END

		CASE_BEGIN(PUSH1)
			ON_OP();
			updateIOGas();
			*++m_sp = m_code[++m_pc];
			++m_pc;
		CASE_END

		CASE_BEGIN(PUSH2)
		CASE_BEGIN(PUSH3)
		CASE_BEGIN(PUSH4)
		CASE_BEGIN(PUSH5)
		CASE_BEGIN(PUSH6)
		CASE_BEGIN(PUSH7)
		CASE_BEGIN(PUSH8)
		CASE_BEGIN(PUSH9)
		CASE_BEGIN(PUSH10)
		CASE_BEGIN(PUSH11)
		CASE_BEGIN(PUSH12)
		CASE_BEGIN(PUSH13)
		CASE_BEGIN(PUSH14)
		CASE_BEGIN(PUSH15)
		CASE_BEGIN(PUSH16)
		CASE_BEGIN(PUSH17)
		CASE_BEGIN(PUSH18)
		CASE_BEGIN(PUSH19)
		CASE_BEGIN(PUSH20)
		CASE_BEGIN(PUSH21)
		CASE_BEGIN(PUSH22)
		CASE_BEGIN(PUSH23)
		CASE_BEGIN(PUSH24)
		CASE_BEGIN(PUSH25)
		CASE_BEGIN(PUSH26)
		CASE_BEGIN(PUSH27)
		CASE_BEGIN(PUSH28)
		CASE_BEGIN(PUSH29)
		CASE_BEGIN(PUSH30)
		CASE_BEGIN(PUSH31)
		CASE_BEGIN(PUSH32)
		{
			ON_OP();
			updateIOGas();

			int numBytes = (int)m_op - (int)Instruction::PUSH1 + 1;
			*++m_sp = 0;
			// Construct a number out of PUSH bytes.
			// This requires the code has been copied and extended by 32 zero
			// bytes to handle "out of code" push data here.
			for (++m_pc; numBytes--; ++m_pc)
				*m_sp = (*m_sp << 8) | m_code[m_pc];
		}
		CASE_END

		CASE_BEGIN(JUMP)
			ON_OP();
			updateIOGas();

			m_pc = verifyJumpDest(*m_sp);
			--m_sp;
		CASE_END

		CASE_BEGIN(JUMPI)
			ON_OP();
			updateIOGas();
			if (*(m_sp - 1))
				m_pc = verifyJumpDest(*m_sp);
			else
				++m_pc;
			m_sp -= 2;
		CASE_END

#if EVM_JUMPS_AND_SUBS
		CASE_BEGIN(JUMPTO)
			ON_OP();
			updateIOGas();
			m_pc = decodeJumpDest(m_code, m_pc);
		CASE_END

		CASE_BEGIN(JUMPIF)
			ON_OP();
			updateIOGas();
			if (*m_sp)
				m_pc = decodeJumpDest(m_code, m_pc);
			else
				++m_pc;
			--m_sp;
		CASE_END

		CASE_BEGIN(JUMPV)
			ON_OP();
			updateIOGas();			
			m_pc = decodeJumpvDest(m_code, m_pc, m_sp);
		CASE_END

		CASE_BEGIN(JUMPSUB)
			ON_OP();
			updateIOGas();
			{
				*++m_rp = m_pc++;
				m_pc = decodeJumpDest(m_code, m_pc);
			}
		CASE_END

		CASE_BEGIN(JUMPSUBV)
			ON_OP();
			updateIOGas();
			{
				*++m_rp = m_pc;
				m_pc = 
				jumpvDest(m_code, m_pc, m_sp);
			}
		CASE_END

		CASE_BEGIN(RETURNSUB)
			ON_OP();
			updateIOGas();
			
			m_pc = *m_rp--;
			++m_pc;
		CASE_END
#else
		CASE_BEGIN(JUMPTO)
		CASE_BEGIN(JUMPIF)
		CASE_BEGIN(JUMPV)
		CASE_BEGIN(JUMPSUB)
		CASE_BEGIN(JUMPSUBV)
		CASE_BEGIN(RETURNSUB)
			throwBadInstruction();
		CASE_END
#endif

		CASE_BEGIN(JUMPC)
#ifdef EVM_REPLACE_CONST_JUMP
			ON_OP();
			updateIOGas();

			m_pc = uint64_t(*m_sp);
			--m_sp;
#else
			throwBadInstruction();
#endif
		CASE_END

		CASE_BEGIN(JUMPCI)
#ifdef EVM_REPLACE_CONST_JUMP
			ON_OP();
			updateIOGas();

			if (*(m_sp - 1))
				m_pc = uint64_t(*m_sp);
			else
				++m_pc;
			m_sp -= 2;
#else
			throwBadInstruction();
#endif
		CASE_END

		CASE_BEGIN(DUP1)
		CASE_BEGIN(DUP2)
		CASE_BEGIN(DUP3)
		CASE_BEGIN(DUP4)
		CASE_BEGIN(DUP5)
		CASE_BEGIN(DUP6)
		CASE_BEGIN(DUP7)
		CASE_BEGIN(DUP8)
		CASE_BEGIN(DUP9)
		CASE_BEGIN(DUP10)
		CASE_BEGIN(DUP11)
		CASE_BEGIN(DUP12)
		CASE_BEGIN(DUP13)
		CASE_BEGIN(DUP14)
		CASE_BEGIN(DUP15)
		CASE_BEGIN(DUP16)
		{
			ON_OP();
			updateIOGas();

			unsigned n = 1 + (unsigned)m_op - (unsigned)Instruction::DUP1;
#if EVM_HACK_DUP_64
			*(uint64_t*)(m_sp+1) = *(uint64_t*)&m_stack[(1 + m_sp - m_stack) - n];
#else
			*(m_sp+1) = m_stack[(1 + m_sp - m_stack) - n];
#endif
			++m_sp;
			++m_pc;
		}
		CASE_END


		CASE_BEGIN(SWAP1)
		CASE_BEGIN(SWAP2)
		CASE_BEGIN(SWAP3)
		CASE_BEGIN(SWAP4)
		CASE_BEGIN(SWAP5)
		CASE_BEGIN(SWAP6)
		CASE_BEGIN(SWAP7)
		CASE_BEGIN(SWAP8)
		CASE_BEGIN(SWAP9)
		CASE_BEGIN(SWAP10)
		CASE_BEGIN(SWAP11)
		CASE_BEGIN(SWAP12)
		CASE_BEGIN(SWAP13)
		CASE_BEGIN(SWAP14)
		CASE_BEGIN(SWAP15)
		CASE_BEGIN(SWAP16)
		{
			ON_OP();
			updateIOGas();

			unsigned n = (unsigned)m_op - (unsigned)Instruction::SWAP1 + 2;
			u256 d = *m_sp;
			*m_sp = m_stack[(1 + m_sp - m_stack) - n];
			m_stack[(1 + m_sp - m_stack) - n] = d;
			++m_pc;
		}
		CASE_END


		CASE_BEGIN(SLOAD)
			m_runGas = toUint64(m_schedule->sloadGas);
			ON_OP();
			updateIOGas();

			*m_sp = m_ext->store(*m_sp);
			++m_pc;
		CASE_END

		CASE_BEGIN(SSTORE)
			if (!m_ext->store(*m_sp) && *(m_sp - 1))
				m_runGas = toUint64(m_schedule->sstoreSetGas);
			else if (m_ext->store(*m_sp) && !*(m_sp - 1))
			{
				m_runGas = toUint64(m_schedule->sstoreResetGas);
				m_ext->sub.refunds += m_schedule->sstoreRefundGas;
			}
			else
				m_runGas = toUint64(m_schedule->sstoreResetGas);
			ON_OP();
			updateIOGas();
	
			m_ext->setStore(*m_sp, *(m_sp - 1));
			m_sp -= 2;
			++m_pc;
		CASE_END

		CASE_BEGIN(PC)
			ON_OP();
			updateIOGas();

			*++m_sp = m_pc;
			++m_pc;
		CASE_END

		CASE_BEGIN(MSIZE)
			ON_OP();
			updateIOGas();

			*++m_sp = m_mem.size();
			++m_pc;
		CASE_END

		CASE_BEGIN(GAS)
			ON_OP();
			updateIOGas();

			*++m_sp = m_io_gas;
			++m_pc;
		CASE_END

		CASE_BEGIN(JUMPDEST)
			m_runGas = 1;
			ON_OP();
			updateIOGas();
			++m_pc;
		CASE_END

#if EVM_JUMPS_AND_SUBS
		CASE_BEGIN(BEGINSUB)
			m_runGas = 1;
			ON_OP();
			updateIOGas();
			++m_pc;
		CASE_END

		CASE_BEGIN(BEGINDATA)
			throwBadInstruction();
		CASE_END
#else
		CASE_BEGIN(BEGINSUB)
		CASE_BEGIN(BEGINDATA)
			throwBadInstruction();
		CASE_END
#endif

		CASE_BEGIN(BAD)
			throwBadInstruction();
		CASE_END

		CASE_DEFAULT
			throwBadInstruction();
		CASE_END
		
	END_CASES
}
