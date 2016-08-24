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

#include "VM.h"
#include "VMConfig.h"
#include <libethereum/ExtVM.h>
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
// for tracing, checking, metering, measuring ...
//

void VM::onOperation()
{
	if (*m_onOp)
		(*m_onOp)(++m_nSteps, PC, INST,
			m_newMemSize > m_mem.size() ? (m_newMemSize - m_mem.size()) / 32 : uint64_t(0),
			m_runGas, *m_io_gas, this, m_ext);
}

void VM::checkStack(unsigned _removed, unsigned _added)
{
	int const size = 1 + SP - m_stack;
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
	if (*m_io_gas < m_runGas)
		throwOutOfGas();
	*m_io_gas -= m_runGas;
}

void VM::updateGas()
{
	if (m_newMemSize > m_mem.size())
		m_runGas += toUint64(gasForMem(m_newMemSize) - gasForMem(m_mem.size()));
	m_runGas += (m_schedule->copyGas * ((m_copyMemSize + 31) / 32));
	if (*m_io_gas < m_runGas)
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
	unsigned n = (unsigned)INST - (unsigned)Instruction::LOG0;
	m_runGas = toUint64(m_schedule->logGas + m_schedule->logTopicGas * n + u512(m_schedule->logDataGas) * *(SP - 1));
	m_newMemSize = memNeed(*SP, *(SP - 1));
	updateMem();
}

void VM::fetchInstruction()
{
		INST =  Instruction(PC < m_code_vector.size() ? m_code[PC] : 0);
		const InstructionMetric& metric = c_metrics[static_cast<size_t>(INST)];
		checkStack(metric.args, metric.ret);

		// FEES...
		m_runGas = toUint64(m_schedule->tierStepGas[metric.gasPriceTier]);
		m_newMemSize = m_mem.size();
		m_copyMemSize = 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// interpreter entry point

bytesConstRef VM::execImpl(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp)
{
	m_io_gas = &io_gas;
	m_ext = &_ext;
   m_schedule = &m_ext->evmSchedule();
	m_onOp = &_onOp;
	m_onFail = &VM::onOperation;

	// trampoline to minimize depth of call stack when calling out
	m_bounce = &VM::initEntry;
	do
		(this->*m_bounce)();
	while (m_bounce);

	return m_bytes;
}

//
// main interpreter loop and switch
//
void VM::interpretCases()
{
	DO_CASES
		
		//
		// Call-related instructions
		//
		
		CASE_BEGIN(CREATE)
			m_bounce = &VM::caseCreate;
			++PC;
		CASE_RETURN;

		CASE_BEGIN(DELEGATECALL)

			// Pre-homestead
			if (!m_schedule->haveDelegateCall)
				throwBadInstruction();

		CASE_BEGIN(CALL)
		CASE_BEGIN(CALLCODE)
			m_bounce = &VM::caseCall;
			++PC;
		CASE_RETURN

		CASE_BEGIN(RETURN)
		{
			m_newMemSize = memNeed(*SP, *(SP - 1));
			updateMem();
			onOperation();
			updateIOGas();

			uint64_t b = (uint64_t)*SP--;
			uint64_t s = (uint64_t)*SP--;
			m_bytes = bytesConstRef(m_mem.data() + b, s);
			m_bounce = 0;
		}
		CASE_RETURN

		CASE_BEGIN(SUICIDE)
		{
			onOperation();
			updateIOGas();

			Address dest = asAddress(*SP);
			m_ext->suicide(dest);
			m_bounce = 0;
		}
		CASE_RETURN

		CASE_BEGIN(STOP)
			onOperation();
			updateIOGas();
			m_bounce = 0;
		CASE_RETURN;
			
			
		//
		// instructions potentially expanding memory
		//
		
		CASE_BEGIN(MLOAD)
		{
			m_newMemSize = toUint64(*SP) + 32;
			updateMem();
			onOperation();
			updateIOGas();

			*SP = (u256)*(h256 const*)(m_mem.data() + (unsigned)*SP);
			++PC;
		}
		CASE_END

		CASE_BEGIN(MSTORE)
		{
			m_newMemSize = toUint64(*SP) + 32;
			updateMem();
			onOperation();
			updateIOGas();

			*(h256*)&m_mem[(unsigned)*SP] = (h256)*(SP - 1);
			SP -= 2;
			++PC;
		}
		CASE_END

		CASE_BEGIN(MSTORE8)
		{
			m_newMemSize = toUint64(*SP) + 1;
			updateMem();
			onOperation();
			updateIOGas();

			m_mem[(unsigned)*SP] = (byte)(*(SP - 1) & 0xff);
			SP -= 2;
			++PC;
		}
		CASE_END

		CASE_BEGIN(SHA3)
		{
			m_runGas = toUint64(m_schedule->sha3Gas + (u512(*(SP - 1)) + 31) / 32 * m_schedule->sha3WordGas);
			m_newMemSize = memNeed(*SP, *(SP - 1));
			updateMem();
			onOperation();
			updateIOGas();

			uint64_t inOff = (uint64_t)*SP--;
			uint64_t inSize = (uint64_t)*SP--;
			*++SP = (u256)sha3(bytesConstRef(m_mem.data() + inOff, inSize));
			++PC;
		}
		CASE_END

		CASE_BEGIN(LOG0)
			logGasMem();
			onOperation();
			updateIOGas();

			m_ext->log({}, bytesConstRef(m_mem.data() + (uint64_t)*SP, (uint64_t)*(SP - 1)));
			SP -= 2;
			++PC;
		CASE_END

		CASE_BEGIN(LOG1)
			logGasMem();
			onOperation();
			updateIOGas();

			m_ext->log({*(SP - 2)}, bytesConstRef(m_mem.data() + (uint64_t)*SP, (uint64_t)*(SP - 1)));
			SP -= 3;
			++PC;
		CASE_END

		CASE_BEGIN(LOG2)
			logGasMem();
			onOperation();
			updateIOGas();

			m_ext->log({*(SP - 2), *(SP-3)}, bytesConstRef(m_mem.data() + (uint64_t)*SP, (uint64_t)*(SP - 1)));
			SP -= 4;
			++PC;
		CASE_END

		CASE_BEGIN(LOG3)
			logGasMem();
			onOperation();
			updateIOGas();

			m_ext->log({*(SP - 2), *(SP-3), *(SP-4)}, bytesConstRef(m_mem.data() + (uint64_t)*SP, (uint64_t)*(SP - 1)));
			SP -= 5;
			++PC;
		CASE_END

		CASE_BEGIN(LOG4)
			logGasMem();
			onOperation();
			updateIOGas();

			m_ext->log({*(SP - 2), *(SP-3), *(SP-4), *(SP-5)}, bytesConstRef(m_mem.data() + (uint64_t)*SP, (uint64_t)*(SP - 1)));
			SP -= 6;
			++PC;
		CASE_END	

		CASE_BEGIN(EXP)
		{
			auto expon = *(SP - 1);
			m_runGas = toUint64(m_schedule->expGas + m_schedule->expByteGas * (32 - (h256(expon).firstBitSet() / 8)));
			updateMem();
			onOperation();
			updateIOGas();

			auto base = *SP--;
			*SP = (u256)boost::multiprecision::powm((bigint)base, (bigint)expon, bigint(1) << 256);
			++PC;
		}
		CASE_END

		//
		// ordinary instructions
		//

		CASE_BEGIN(ADD)
			onOperation();
			updateIOGas();

			//pops two items and pushes S[-1] + S[-2] mod 2^256.
			*(SP - 1) += *SP;
			--SP;
			++PC;
		CASE_END

		CASE_BEGIN(MUL)
			onOperation();
			updateIOGas();

			//pops two items and pushes S[-1] * S[-2] mod 2^256.
			*(SP - 1) *= *SP;
			--SP;
			++PC;
		CASE_END

		CASE_BEGIN(SUB)
			onOperation();
			updateIOGas();

			*(SP - 1) = *SP - *(SP - 1);
			--SP;
			++PC;
		CASE_END

		CASE_BEGIN(DIV)
			onOperation();
			updateIOGas();

			*(SP - 1) = *(SP - 1) ? divWorkaround(*SP, *(SP - 1)) : 0;
			--SP;
			++PC;
		CASE_END

		CASE_BEGIN(SDIV)
			onOperation();
			updateIOGas();

			*(SP - 1) = *(SP - 1) ? s2u(divWorkaround(u2s(*SP), u2s(*(SP - 1)))) : 0;
			--SP;
			++PC;
		CASE_END

		CASE_BEGIN(MOD)
			onOperation();
			updateIOGas();

			*(SP - 1) = *(SP - 1) ? modWorkaround(*SP, *(SP - 1)) : 0;
			--SP;
			++PC;
		CASE_END

		CASE_BEGIN(SMOD)
			onOperation();
			updateIOGas();

			*(SP - 1) = *(SP - 1) ? s2u(modWorkaround(u2s(*SP), u2s(*(SP - 1)))) : 0;
			--SP;
			++PC;
		CASE_END

		CASE_BEGIN(NOT)
			onOperation();
			updateIOGas();

			*SP = ~*SP;
			++PC;
		CASE_END

		CASE_BEGIN(LT)
			onOperation();
			updateIOGas();

			*(SP - 1) = *SP < *(SP - 1) ? 1 : 0;
			--SP;
			++PC;
		CASE_END

		CASE_BEGIN(GT)
			onOperation();
			updateIOGas();

			*(SP - 1) = *SP > *(SP - 1) ? 1 : 0;
			--SP;
			++PC;
		CASE_END

		CASE_BEGIN(SLT)
			onOperation();
			updateIOGas();

			*(SP - 1) = u2s(*SP) < u2s(*(SP - 1)) ? 1 : 0;
			--SP;
			++PC;
		CASE_END

		CASE_BEGIN(SGT)
			onOperation();
			updateIOGas();

			*(SP - 1) = u2s(*SP) > u2s(*(SP - 1)) ? 1 : 0;
			--SP;
			++PC;
		CASE_END

		CASE_BEGIN(EQ)
			onOperation();
			updateIOGas();

			*(SP - 1) = *SP == *(SP - 1) ? 1 : 0;
			--SP;
			++PC;
		CASE_END

		CASE_BEGIN(ISZERO)
			onOperation();
			updateIOGas();

			*SP = *SP ? 0 : 1;
			++PC;
		CASE_END

		CASE_BEGIN(AND)
			onOperation();
			updateIOGas();

			*(SP - 1) = *SP & *(SP - 1);
			--SP;
			++PC;
		CASE_END

		CASE_BEGIN(OR)
			onOperation();
			updateIOGas();

			*(SP - 1) = *SP | *(SP - 1);
			--SP;
			++PC;
		CASE_END

		CASE_BEGIN(XOR)
			onOperation();
			updateIOGas();

			*(SP - 1) = *SP ^ *(SP - 1);
			--SP;
			++PC;
		CASE_END

		CASE_BEGIN(BYTE)
			onOperation();
			updateIOGas();

			*(SP - 1) = *SP < 32 ? (*(SP - 1) >> (unsigned)(8 * (31 - *SP))) & 0xff : 0;
			--SP;
			++PC;
		CASE_END

		CASE_BEGIN(ADDMOD)
			onOperation();
			updateIOGas();

			*(SP - 2) = *(SP - 2) ? u256((u512(*SP) + u512(*(SP - 1))) % *(SP - 2)) : 0;
			SP -= 2;
			++PC;
		CASE_END

		CASE_BEGIN(MULMOD)
			onOperation();
			updateIOGas();

			*(SP - 2) = *(SP - 2) ? u256((u512(*SP) * u512(*(SP - 1))) % *(SP - 2)) : 0;
			SP -= 2;
			++PC;
		CASE_END

		CASE_BEGIN(SIGNEXTEND)
			onOperation();
			updateIOGas();

			if (*SP < 31)
			{
				auto testBit = static_cast<unsigned>(*SP) * 8 + 7;
				u256& number = *(SP - 1);
				u256 mask = ((u256(1) << testBit) - 1);
				if (boost::multiprecision::bit_test(number, testBit))
					number |= ~mask;
				else
					number &= mask;
			}
			--SP;
			++PC;
		CASE_END

		CASE_BEGIN(ADDRESS)
			onOperation();
			updateIOGas();

			*++SP = fromAddress(m_ext->myAddress);
			++PC;
		CASE_END

		CASE_BEGIN(ORIGIN)
			onOperation();
			updateIOGas();

			*++SP = fromAddress(m_ext->origin);
			++PC;
		CASE_END

		CASE_BEGIN(BALANCE)
		{
			onOperation();
			updateIOGas();

			*SP = m_ext->balance(asAddress(*SP));
			++PC;
		}
		CASE_END


		CASE_BEGIN(CALLER)
			onOperation();
			updateIOGas();

			*++SP = fromAddress(m_ext->caller);
			++PC;
		CASE_END

		CASE_BEGIN(CALLVALUE)
			onOperation();
			updateIOGas();

			*++SP = m_ext->value;
			++PC;
		CASE_END


		CASE_BEGIN(CALLDATALOAD)
		{
			onOperation();
			updateIOGas();

			if (u512(*SP) + 31 < m_ext->data.size())
				*SP = (u256)*(h256 const*)(m_ext->data.data() + (size_t)*SP);
			else if (*SP >= m_ext->data.size())
				*SP = u256(0);
			else
			{
				h256 r;
				for (uint64_t i = (uint64_t)*SP, e = (uint64_t)*SP + (uint64_t)32, j = 0; i < e; ++i, ++j)
					r[j] = i < m_ext->data.size() ? m_ext->data[i] : 0;
				*SP = (u256)r;
			}
			++PC;
		}
		CASE_END


		CASE_BEGIN(CALLDATASIZE)
			onOperation();
			updateIOGas();

			*++SP = m_ext->data.size();
			++PC;
		CASE_END

		CASE_BEGIN(CODESIZE)
			onOperation();
			updateIOGas();

			*++SP = m_ext->code.size();
			++PC;
		CASE_END

		CASE_BEGIN(EXTCODESIZE)
			onOperation();
			updateIOGas();

			*SP = m_ext->codeAt(asAddress(*SP)).size();
			++PC;
		CASE_END

		CASE_BEGIN(CALLDATACOPY)
			m_copyMemSize = toUint64(*(SP - 2));
			m_newMemSize = memNeed(*SP, *(SP - 2));
			updateMem();
			onOperation();
			updateIOGas();

			copyDataToMemory(m_ext->data, SP);
			++PC;
		CASE_END

		CASE_BEGIN(CODECOPY)
			m_copyMemSize = toUint64(*(SP - 2));
			m_newMemSize = memNeed(*SP, *(SP - 2));
			updateMem();
			onOperation();
			updateIOGas();

			copyDataToMemory(&m_ext->code, SP);
			++PC;
		CASE_END

		CASE_BEGIN(EXTCODECOPY)
		{
			m_copyMemSize = toUint64(*(SP - 3));
			m_newMemSize = memNeed(*(SP - 1), *(SP - 3));
			updateMem();
			onOperation();
			updateIOGas();

			auto a = asAddress(*SP);
			--SP;
			copyDataToMemory(&m_ext->codeAt(a), SP);
			++PC;
		}
		CASE_END


		CASE_BEGIN(GASPRICE)
			onOperation();
			updateIOGas();

			*++SP = m_ext->gasPrice;
			++PC;
		CASE_END

		CASE_BEGIN(BLOCKHASH)
			onOperation();
			updateIOGas();

			*SP = (u256)m_ext->blockHash(*SP);
			++PC;
		CASE_END

		CASE_BEGIN(COINBASE)
			onOperation();
			updateIOGas();

			*++SP = (u160)m_ext->envInfo().author();
			++PC;
		CASE_END

		CASE_BEGIN(TIMESTAMP)
			onOperation();
			updateIOGas();

			*++SP = m_ext->envInfo().timestamp();
			++PC;
		CASE_END

		CASE_BEGIN(NUMBER)
			onOperation();
			updateIOGas();

			*++SP = m_ext->envInfo().number();
			++PC;
		CASE_END

		CASE_BEGIN(DIFFICULTY)
			onOperation();
			updateIOGas();

			*++SP = m_ext->envInfo().difficulty();
			++PC;
		CASE_END

		CASE_BEGIN(GASLIMIT)
			onOperation();
			updateIOGas();

			*++SP = m_ext->envInfo().gasLimit();
			++PC;
		CASE_END

		CASE_BEGIN(POP)
			onOperation();
			updateIOGas();

			--SP;
			++PC;
		CASE_END

		CASE_BEGIN(PUSHC)
			onOperation();
			updateIOGas();

			*++SP = m_pool[m_code[++PC]];
			++PC;
			PC += m_code[PC];
		CASE_END

		CASE_BEGIN(PUSH1)
			onOperation();
			updateIOGas();
			*++SP = *(uint8_t*)(m_code + ++PC);
			PC += 1;
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
			onOperation();
			updateIOGas();

			int i = (int)INST - (int)Instruction::PUSH1 + 1;
			*++SP = 0;
			for (++PC; i--; ++PC)
				*SP = (*SP << 8) | m_code[PC];
		}
		CASE_END

		CASE_BEGIN(JUMP)
			onOperation();
			updateIOGas();

			PC = verifyJumpDest(*SP);
			--SP;
		CASE_END

		CASE_BEGIN(JUMPI)
			onOperation();
			updateIOGas();

			if (*(SP - 1))
				PC = verifyJumpDest(*SP);
			else
				++PC;
			SP -= 2;
		CASE_END

		CASE_BEGIN(JUMPV)
			onOperation();
			updateIOGas();

			PC = uint64_t(*SP);
			--SP;
		CASE_END

		CASE_BEGIN(JUMPVI)
			onOperation();
			updateIOGas();

			if (*(SP - 1))
				PC = uint64_t(*SP);
			else
				++PC;
			SP -= 2;
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
			onOperation();
			updateIOGas();

			unsigned n = 1 + (unsigned)INST - (unsigned)Instruction::DUP1;
			*(SP+1) = m_stack[(1 + SP - m_stack) - n];
			++SP;
			++PC;
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
			onOperation();
			updateIOGas();

			auto n = (unsigned)INST - (unsigned)Instruction::SWAP1 + 2;
			auto d = *SP;
			*SP = m_stack[(1 + SP - m_stack) - n];
			m_stack[(1 + SP - m_stack) - n] = d;
			++PC;
		}
		CASE_END


		CASE_BEGIN(SLOAD)
			m_runGas = toUint64(m_schedule->sloadGas);
			onOperation();
			updateIOGas();

			*SP = m_ext->store(*SP);
			++PC;
		CASE_END

		CASE_BEGIN(SSTORE)
			if (!m_ext->store(*SP) && *(SP - 1))
				m_runGas = toUint64(m_schedule->sstoreSetGas);
			else if (m_ext->store(*SP) && !*(SP - 1))
			{
				m_runGas = toUint64(m_schedule->sstoreResetGas);
				m_ext->sub.refunds += m_schedule->sstoreRefundGas;
			}
			else
				m_runGas = toUint64(m_schedule->sstoreResetGas);
			onOperation();
			updateIOGas();
	
			m_ext->setStore(*SP, *(SP - 1));
			SP -= 2;
			++PC;
		CASE_END

		CASE_BEGIN(PC)
			onOperation();
			updateIOGas();

			*++SP = PC;
			++PC;
		CASE_END

		CASE_BEGIN(MSIZE)
			onOperation();
			updateIOGas();

			*++SP = m_mem.size();
			++PC;
		CASE_END

		CASE_BEGIN(GAS)
			onOperation();
			updateIOGas();

			*++SP = *m_io_gas;
			++PC;
		CASE_END

		CASE_BEGIN(JUMPDEST)
			m_runGas = 1;
			onOperation();
			updateIOGas();
			++PC;
		CASE_END

		CASE_DEFAULT
			throwBadInstruction();
		CASE_END
		
	END_CASES
}
