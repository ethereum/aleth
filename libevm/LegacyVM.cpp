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

#include "LegacyVM.h"

using namespace std;
using namespace dev;
using namespace dev::eth;

uint64_t LegacyVM::memNeed(u256 _offset, u256 _size)
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

uint64_t LegacyVM::decodeJumpDest(const byte* const _code, uint64_t& _pc)
{
	// turn 2 MSB-first bytes in the code into a native-order integer
	uint64_t dest      = _code[_pc++];
	dest = (dest << 8) | _code[_pc++];
	return dest;
}

uint64_t LegacyVM::decodeJumpvDest(const byte* const _code, uint64_t& _pc, byte _voff)
{
	// Layout of jump table in bytecode...
	//     byte opcode
	//     byte n_jumps
	//     byte table[n_jumps][2]
	//
	uint64_t pc = _pc;
	byte n = _code[++pc];           // byte after opcode is number of jumps
	if (_voff >= n) _voff = n - 1;  // if offset overflows use default jump
	pc += _voff * 2;                // adjust inout pc before index destination in table

	uint64_t dest = decodeJumpDest(_code, pc);

	_pc += 1 + n * 2;               // adust inout _pc to opcode after table
	return dest;
}


//
// for tracing, checking, metering, measuring ...
//
void LegacyVM::onOperation()
{
	if (m_onOp)
		(m_onOp)(++m_nSteps, m_PC, m_OP,
			m_newMemSize > m_mem.size() ? (m_newMemSize - m_mem.size()) / 32 : uint64_t(0),
			m_runGas, m_io_gas, this, m_ext);
}

//
// set current SP to SP', adjust SP' per _removed and _added items
//
void LegacyVM::adjustStack(unsigned _removed, unsigned _added)
{
	m_SP = m_SPP;

	// adjust stack and check bounds
	m_SPP += _removed;
	if (m_stackEnd < m_SPP)
		throwBadStack(_removed, _added);
	m_SPP -= _added;
	if (m_SPP < m_stack)
		throwBadStack(_removed, _added);
}

void LegacyVM::updateSSGas()
{
	if (!m_ext->store(m_SP[0]) && m_SP[1])
		m_runGas = toInt63(m_schedule->sstoreSetGas);
	else if (m_ext->store(m_SP[0]) && !m_SP[1])
	{
		m_runGas = toInt63(m_schedule->sstoreResetGas);
		m_ext->sub.refunds += m_schedule->sstoreRefundGas;
	}
	else
		m_runGas = toInt63(m_schedule->sstoreResetGas);
}


uint64_t LegacyVM::gasForMem(u512 _size)
{
	u512 s = _size / 32;
	return toInt63((u512)m_schedule->memoryGas * s + s * s / m_schedule->quadCoeffDiv);
}

void LegacyVM::updateIOGas()
{
	if (m_io_gas < m_runGas)
		throwOutOfGas();
	m_io_gas -= m_runGas;
}

void LegacyVM::updateGas()
{
	if (m_newMemSize > m_mem.size())
		m_runGas += toInt63(gasForMem(m_newMemSize) - gasForMem(m_mem.size()));
	m_runGas += (m_schedule->copyGas * ((m_copyMemSize + 31) / 32));
	if (m_io_gas < m_runGas)
		throwOutOfGas();
}

void LegacyVM::updateMem(uint64_t _newMem)
{
	m_newMemSize = (_newMem + 31) / 32 * 32;
	updateGas();
	if (m_newMemSize > m_mem.size())
		m_mem.resize(m_newMemSize);
}

void LegacyVM::logGasMem()
{
	unsigned n = (unsigned)m_OP - (unsigned)Instruction::LOG0;
	m_runGas = toInt63(m_schedule->logGas + m_schedule->logTopicGas * n + u512(m_schedule->logDataGas) * m_SP[1]);
	updateMem(memNeed(m_SP[0], m_SP[1]));
}

void LegacyVM::fetchInstruction()
{
	m_OP = Instruction(m_code[m_PC]);
	const InstructionMetric& metric = c_metrics[static_cast<size_t>(m_OP)];
	adjustStack(metric.args, metric.ret);

	// FEES...
	m_runGas = toInt63(m_schedule->tierStepGas[static_cast<unsigned>(metric.gasPriceTier)]);
	m_newMemSize = m_mem.size();
	m_copyMemSize = 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// interpreter entry point

owning_bytes_ref LegacyVM::exec(u256& _io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp)
{
	m_io_gas_p = &_io_gas;
	m_io_gas = uint64_t(_io_gas);
	m_ext = &_ext;
	m_schedule = &m_ext->evmSchedule();
	m_onOp = _onOp;
	m_onFail = &LegacyVM::onOperation; // this results in operations that fail being logged twice in the trace
	m_PC = 0;

	try
	{
		// trampoline to minimize depth of call stack when calling out
		m_bounce = &LegacyVM::initEntry;
		do
			(this->*m_bounce)();
		while (m_bounce);

	}
	catch (...)
	{
		*m_io_gas_p = m_io_gas;
		throw;
	}

	*m_io_gas_p = m_io_gas;
	return std::move(m_output);
}

//
// main interpreter loop and switch
//
void LegacyVM::interpretCases()
{
	INIT_CASES
	DO_CASES
	{
		//
		// Call-related instructions
		//

		CASE(CREATE2)
		{
			ON_OP();
			if (!m_schedule->haveCreate2)
				throwBadInstruction();
            if (m_ext->staticCall)
                throwDisallowedStateChange();

			m_bounce = &LegacyVM::caseCreate;
		}
		BREAK

		CASE(CREATE)
		{
			ON_OP();
			if (m_ext->staticCall)
				throwDisallowedStateChange();

			m_bounce = &LegacyVM::caseCreate;
		}
		BREAK

		CASE(DELEGATECALL)
		CASE(STATICCALL)
		CASE(CALL)
		CASE(CALLCODE)
		{
			ON_OP();
			if (m_OP == Instruction::DELEGATECALL && !m_schedule->haveDelegateCall)
				throwBadInstruction();
			if (m_OP == Instruction::STATICCALL && !m_schedule->haveStaticCall)
				throwBadInstruction();
			if (m_OP == Instruction::CALL && m_ext->staticCall && m_SP[2] != 0)
				throwDisallowedStateChange();
			m_bounce = &LegacyVM::caseCall;
		}
		BREAK

		CASE(RETURN)
		{
			ON_OP();
			m_copyMemSize = 0;
			updateMem(memNeed(m_SP[0], m_SP[1]));
			updateIOGas();

			uint64_t b = (uint64_t)m_SP[0];
			uint64_t s = (uint64_t)m_SP[1];
			m_output = owning_bytes_ref{std::move(m_mem), b, s};
			m_bounce = 0;
		}
		BREAK

		CASE(REVERT)
		{
			// Pre-byzantium
			if (!m_schedule->haveRevert)
				throwBadInstruction();

			ON_OP();
			m_copyMemSize = 0;
			updateMem(memNeed(m_SP[0], m_SP[1]));
			updateIOGas();

			uint64_t b = (uint64_t)m_SP[0];
			uint64_t s = (uint64_t)m_SP[1];
			owning_bytes_ref output{move(m_mem), b, s};
			throwRevertInstruction(move(output));
		}
		BREAK;

		CASE(SUICIDE)
		{
			ON_OP();
			if (m_ext->staticCall)
				throwDisallowedStateChange();

			m_runGas = toInt63(m_schedule->suicideGas);
			Address dest = asAddress(m_SP[0]);

			// After EIP158 zero-value suicides do not have to pay account creation gas.
			if (m_ext->balance(m_ext->myAddress) > 0 || m_schedule->zeroValueTransferChargesNewAccountGas())
				// After EIP150 hard fork charge additional cost of sending
				// ethers to non-existing account.
				if (m_schedule->suicideChargesNewAccountGas() && !m_ext->exists(dest))
					m_runGas += m_schedule->callNewAccountGas;

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
		BREAK


		//
		// instructions potentially expanding memory
		//

		CASE(MLOAD)
		{
			ON_OP();
			updateMem(toInt63(m_SP[0]) + 32);
			updateIOGas();

			m_SPP[0] = (u256)*(h256 const*)(m_mem.data() + (unsigned)m_SP[0]);
		}
		NEXT

		CASE(MSTORE)
		{
			ON_OP();
			updateMem(toInt63(m_SP[0]) + 32);
			updateIOGas();

			*(h256*)&m_mem[(unsigned)m_SP[0]] = (h256)m_SP[1];
		}
		NEXT

		CASE(MSTORE8)
		{
			ON_OP();
			updateMem(toInt63(m_SP[0]) + 1);
			updateIOGas();

			m_mem[(unsigned)m_SP[0]] = (byte)(m_SP[1] & 0xff);
		}
		NEXT

		CASE(SHA3)
		{
			ON_OP();
			m_runGas = toInt63(m_schedule->sha3Gas + (u512(m_SP[1]) + 31) / 32 * m_schedule->sha3WordGas);
			updateMem(memNeed(m_SP[0], m_SP[1]));
			updateIOGas();

			uint64_t inOff = (uint64_t)m_SP[0];
			uint64_t inSize = (uint64_t)m_SP[1];
			m_SPP[0] = (u256)sha3(bytesConstRef(m_mem.data() + inOff, inSize));
		}
		NEXT

		CASE(LOG0)
		{
			ON_OP();
			if (m_ext->staticCall)
				throwDisallowedStateChange();

			logGasMem();
			updateIOGas();

			m_ext->log({}, bytesConstRef(m_mem.data() + (uint64_t)m_SP[0], (uint64_t)m_SP[1]));
		}
		NEXT

		CASE(LOG1)
		{
			ON_OP();
			if (m_ext->staticCall)
				throwDisallowedStateChange();

			logGasMem();
			updateIOGas();

			m_ext->log({m_SP[2]}, bytesConstRef(m_mem.data() + (uint64_t)m_SP[0], (uint64_t)m_SP[1]));
		}
		NEXT

		CASE(LOG2)
		{
			ON_OP();
			if (m_ext->staticCall)
				throwDisallowedStateChange();

			logGasMem();
			updateIOGas();

			m_ext->log({m_SP[2], m_SP[3]}, bytesConstRef(m_mem.data() + (uint64_t)m_SP[0], (uint64_t)m_SP[1]));
		}
		NEXT

		CASE(LOG3)
		{
			ON_OP();
			if (m_ext->staticCall)
				throwDisallowedStateChange();

			logGasMem();
			updateIOGas();

			m_ext->log({m_SP[2], m_SP[3], m_SP[4]}, bytesConstRef(m_mem.data() + (uint64_t)m_SP[0], (uint64_t)m_SP[1]));
		}
		NEXT

		CASE(LOG4)
		{
			ON_OP();
			if (m_ext->staticCall)
				throwDisallowedStateChange();

			logGasMem();
			updateIOGas();

			m_ext->log({m_SP[2], m_SP[3], m_SP[4], m_SP[5]}, bytesConstRef(m_mem.data() + (uint64_t)m_SP[0], (uint64_t)m_SP[1]));
		}
		NEXT

		CASE(EXP)
		{
			u256 expon = m_SP[1];
			m_runGas = toInt63(m_schedule->expGas + m_schedule->expByteGas * (32 - (h256(expon).firstBitSet() / 8)));
			ON_OP();
			updateIOGas();

			u256 base = m_SP[0];
			m_SPP[0] = exp256(base, expon);
		}
		NEXT

		//
		// ordinary instructions
		//

		CASE(ADD)
		{
			ON_OP();
			updateIOGas();

			//pops two items and pushes their sum mod 2^256.
			m_SPP[0] = m_SP[0] + m_SP[1];
		}
		NEXT

		CASE(MUL)
		{
			ON_OP();
			updateIOGas();

			//pops two items and pushes their product mod 2^256.
			m_SPP[0] = m_SP[0] * m_SP[1];
		}
		NEXT

		CASE(SUB)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[0] - m_SP[1];
		}
		NEXT

		CASE(DIV)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[1] ? divWorkaround(m_SP[0], m_SP[1]) : 0;
		}
		NEXT

		CASE(SDIV)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[1] ? s2u(divWorkaround(u2s(m_SP[0]), u2s(m_SP[1]))) : 0;
			--m_SP;
		}
		NEXT

		CASE(MOD)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[1] ? modWorkaround(m_SP[0], m_SP[1]) : 0;
		}
		NEXT

		CASE(SMOD)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[1] ? s2u(modWorkaround(u2s(m_SP[0]), u2s(m_SP[1]))) : 0;
		}
		NEXT

		CASE(NOT)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = ~m_SP[0];
		}
		NEXT

		CASE(LT)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[0] < m_SP[1] ? 1 : 0;
		}
		NEXT

		CASE(GT)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[0] > m_SP[1] ? 1 : 0;
		}
		NEXT

		CASE(SLT)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = u2s(m_SP[0]) < u2s(m_SP[1]) ? 1 : 0;
		}
		NEXT

		CASE(SGT)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = u2s(m_SP[0]) > u2s(m_SP[1]) ? 1 : 0;
		}
		NEXT

		CASE(EQ)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[0] == m_SP[1] ? 1 : 0;
		}
		NEXT

		CASE(ISZERO)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[0] ? 0 : 1;
		}
		NEXT

		CASE(AND)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[0] & m_SP[1];
		}
		NEXT

		CASE(OR)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[0] | m_SP[1];
		}
		NEXT

		CASE(XOR)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[0] ^ m_SP[1];
		}
		NEXT

		CASE(BYTE)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[0] < 32 ? (m_SP[1] >> (unsigned)(8 * (31 - m_SP[0]))) & 0xff : 0;
		}
		NEXT

		CASE(SHL)
		{
			// Pre-constantinople
			if (!m_schedule->haveBitwiseShifting)
				throwBadInstruction();

			ON_OP();
			updateIOGas();

			if (m_SP[0] >= 256)
				m_SPP[0] = 0;
			else
				m_SPP[0] = m_SP[1] << unsigned(m_SP[0]);
		}
		NEXT

		CASE(SHR)
		{
			// Pre-constantinople
			if (!m_schedule->haveBitwiseShifting)
				throwBadInstruction();

			ON_OP();
			updateIOGas();

			if (m_SP[0] >= 256)
				m_SPP[0] = 0;
			else
				m_SPP[0] = m_SP[1] >> unsigned(m_SP[0]);
		}
		NEXT

		CASE(SAR)
		{
			// Pre-constantinople
			if (!m_schedule->haveBitwiseShifting)
				throwBadInstruction();

			ON_OP();
			updateIOGas();

			static u256 const hibit = u256(1) << 255;
			static u256 const allbits =
				u256("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

			u256 shiftee = m_SP[1];
			if (m_SP[0] >= 256)
			{
				if (shiftee & hibit)
					m_SPP[0] = allbits;
				else
					m_SPP[0] = 0;
			}
			else
			{
				unsigned amount = unsigned(m_SP[0]);
				m_SPP[0] = shiftee >> amount;
				if (shiftee & hibit)
					m_SPP[0] |= allbits << (256 - amount);
			}
		}
		NEXT

		CASE(ADDMOD)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[2] ? u256((u512(m_SP[0]) + u512(m_SP[1])) % m_SP[2]) : 0;
		}
		NEXT

		CASE(MULMOD)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_SP[2] ? u256((u512(m_SP[0]) * u512(m_SP[1])) % m_SP[2]) : 0;
		}
		NEXT

		CASE(SIGNEXTEND)
		{
			ON_OP();
			updateIOGas();

			if (m_SP[0] < 31)
			{
				unsigned testBit = static_cast<unsigned>(m_SP[0]) * 8 + 7;
				u256& number = m_SP[1];
				u256 mask = ((u256(1) << testBit) - 1);
				if (boost::multiprecision::bit_test(number, testBit))
					number |= ~mask;
				else
					number &= mask;
			}
		}
		NEXT

#if EIP_615
		CASE(JUMPTO)
		{
			ON_OP();
			updateIOGas();

			m_PC = decodeJumpDest(m_code.data(), m_PC);
		}
		CONTINUE

		CASE(JUMPIF)
		{
			ON_OP();
			updateIOGas();

			if (m_SP[0])
				m_PC = decodeJumpDest(m_code.data(), m_PC);
			else
				++m_PC;
		}
		CONTINUE

		CASE(JUMPV)
		{
			ON_OP();
			updateIOGas();
			m_PC = decodeJumpvDest(m_code.data(), m_PC, byte(m_SP[0]));
		}
		CONTINUE

		CASE(JUMPSUB)
		{
			ON_OP();
			updateIOGas();
			*m_RP++ = m_PC++;
			m_PC = decodeJumpDest(m_code.data(), m_PC);
		}
		CONTINUE

		CASE(JUMPSUBV)
		{
			ON_OP();
			updateIOGas();
			*m_RP++ = m_PC;
			m_PC = decodeJumpvDest(m_code.data(), m_PC, byte(m_SP[0]));
		}
		CONTINUE

		CASE(RETURNSUB)
		{
			ON_OP();
			updateIOGas();

			m_PC = *m_RP--;
		}
		NEXT

		CASE(BEGINSUB)
		{
			ON_OP();
			updateIOGas();
		}
		NEXT


		CASE(BEGINDATA)
		{
			ON_OP();
			updateIOGas();
		}
		NEXT

		CASE(GETLOCAL)
		{
			ON_OP();
			updateIOGas();
		}
		NEXT

		CASE(PUTLOCAL)
		{
			ON_OP();
			updateIOGas();
		}
		NEXT

#else
		CASE(JUMPTO)
		CASE(JUMPIF)
		CASE(JUMPV)
		CASE(JUMPSUB)
		CASE(JUMPSUBV)
		CASE(RETURNSUB)
		CASE(BEGINSUB)
		CASE(BEGINDATA)
		CASE(GETLOCAL)
		CASE(PUTLOCAL)
		{
			throwBadInstruction();
		}
		CONTINUE
#endif

#if EIP_616

		CASE(XADD)
		{
			ON_OP();
			updateIOGas();

			xadd(simdType());
		}
		CONTINUE

		CASE(XMUL)
		{
			ON_OP();
			updateIOGas();

			xmul(simdType());
		}
		CONTINUE

		CASE(XSUB)
		{
			ON_OP();
			updateIOGas();

			xsub(simdType());
		}
		CONTINUE

		CASE(XDIV)
		{
			ON_OP();
			updateIOGas();

			xdiv(simdType());
		}
		CONTINUE

		CASE(XSDIV)
		{
			ON_OP();
			updateIOGas();

			xsdiv(simdType());
		}
		CONTINUE

		CASE(XMOD)
		{
			ON_OP();
			updateIOGas();

			xmod(simdType());
		}
		CONTINUE

		CASE(XSMOD)
		{
			ON_OP();
			updateIOGas();

			xsmod(simdType());
		}
		CONTINUE

		CASE(XLT)
		{
			ON_OP();
			updateIOGas();

			xlt(simdType());
		}
		CONTINUE

		CASE(XGT)
		{
			ON_OP();
			updateIOGas();

			xgt(simdType());
		}
		CONTINUE

		CASE(XSLT)
		{
			ON_OP();
			updateIOGas();

			xslt(simdType());
		}
		CONTINUE

		CASE(XSGT)
		{
			ON_OP();
			updateIOGas();

			xsgt(simdType());
		}
		CONTINUE

		CASE(XEQ)
		{
			ON_OP();
			updateIOGas();

			xeq(simdType());
		}
		CONTINUE

		CASE(XISZERO)
		{
			ON_OP();
			updateIOGas();

			xzero(simdType());
		}
		CONTINUE

		CASE(XAND)
		{
			ON_OP();
			updateIOGas();

			xand(simdType());
		}
		CONTINUE

		CASE(XOOR)
		{
			ON_OP();
			updateIOGas();

			xoor(simdType());
		}
		CONTINUE

		CASE(XXOR)
		{
			ON_OP();
			updateIOGas();

			xxor(simdType());
		}
		CONTINUE

		CASE(XNOT)
		{
			ON_OP();
			updateIOGas();

			xnot(simdType());
		}
		CONTINUE

		CASE(XSHL)
		{
			ON_OP();
			updateIOGas();

			xshl(simdType());
		}
		CONTINUE

		CASE(XSHR)
		{
			ON_OP();
			updateIOGas();

			xshr(simdType());
		}
		CONTINUE

		CASE(XSAR)
		{
			ON_OP();
			updateIOGas();

			xsar(simdType());
		}
		CONTINUE

		CASE(XROL)
		{
			ON_OP();
			updateIOGas();

			xrol(simdType());
		}
		CONTINUE

		CASE(XROR)
		{
			ON_OP();
			updateIOGas();

			xror(simdType());
		}
		CONTINUE

		CASE(XMLOAD)
		{
			updateMem(toInt63(m_SP[0]) + 32);
			ON_OP();
			updateIOGas();

			xmload(simdType());
		}
		CONTINUE

		CASE(XMSTORE)
		{
			updateMem(toInt63(m_SP[0]) + 32);
			ON_OP();
			updateIOGas();

			xmstore(simdType());
		}
		CONTINUE

		CASE(XSLOAD)
		{
			m_runGas = toInt63(m_schedule->sloadGas);
			ON_OP();
			updateIOGas();

			xsload(simdType());
		}
		CONTINUE

		CASE(XSSTORE)
		{
			if (m_ext->staticCall)
				throwDisallowedStateChange();

			updateSSGas();
			ON_OP();
			updateIOGas();

			xsstore(simdType());
		}
		CONTINUE

		CASE(XVTOWIDE)
		{
			ON_OP();
			updateIOGas();

			xvtowide(simdType());
		}
		CONTINUE

		CASE(XWIDETOV)
		{
			ON_OP();
			updateIOGas();

			xwidetov(simdType());
		}
		CONTINUE

		CASE(XPUSH)
		{
			ON_OP();
			updateIOGas();

			xpush(simdType());
		}
		CONTINUE

		CASE(XPUT)
		{
			ON_OP();
			updateIOGas();

			uint8_t b = ++m_PC;
			uint8_t c = ++m_PC;
			xput(m_code[b], m_code[c]);
			++m_PC;
		}
		CONTINUE

		CASE(XGET)
		{
			ON_OP();
			updateIOGas();

			uint8_t b = ++m_PC;
			uint8_t c = ++m_PC;
			xget(m_code[b], m_code[c]);
			++m_PC;
		}
		CONTINUE

		CASE(XSWIZZLE)
		{
			ON_OP();
			updateIOGas();

			xswizzle(simdType());
		}
		CONTINUE

		CASE(XSHUFFLE)
		{
			ON_OP();
			updateIOGas();

			xshuffle(simdType());
		}
		CONTINUE
#else
		CASE(XADD)
		CASE(XMUL)
		CASE(XSUB)
		CASE(XDIV)
		CASE(XSDIV)
		CASE(XMOD)
		CASE(XSMOD)
		CASE(XLT)
		CASE(XGT)
		CASE(XSLT)
		CASE(XSGT)
		CASE(XEQ)
		CASE(XISZERO)
		CASE(XAND)
		CASE(XOOR)
		CASE(XXOR)
		CASE(XNOT)
		CASE(XSHL)
		CASE(XSHR)
		CASE(XSAR)
		CASE(XROL)
		CASE(XROR)
		CASE(XMLOAD)
		CASE(XMSTORE)
		CASE(XSLOAD)
		CASE(XSSTORE)
		CASE(XVTOWIDE)
		CASE(XWIDETOV)
		CASE(XPUSH)
		CASE(XPUT)
		CASE(XGET)
		CASE(XSWIZZLE)
		CASE(XSHUFFLE)
		{
			throwBadInstruction();
		}
		CONTINUE
#endif

		CASE(ADDRESS)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = fromAddress(m_ext->myAddress);
		}
		NEXT

		CASE(ORIGIN)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = fromAddress(m_ext->origin);
		}
		NEXT

		CASE(BALANCE)
		{
			m_runGas = toInt63(m_schedule->balanceGas);
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_ext->balance(asAddress(m_SP[0]));
		}
		NEXT


		CASE(CALLER)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = fromAddress(m_ext->caller);
		}
		NEXT

		CASE(CALLVALUE)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_ext->value;
		}
		NEXT


		CASE(CALLDATALOAD)
		{
			ON_OP();
			updateIOGas();

			if (u512(m_SP[0]) + 31 < m_ext->data.size())
				m_SP[0] = (u256)*(h256 const*)(m_ext->data.data() + (size_t)m_SP[0]);
			else if (m_SP[0] >= m_ext->data.size())
				m_SP[0] = u256(0);
			else
			{ 	h256 r;
				for (uint64_t i = (uint64_t)m_SP[0], e = (uint64_t)m_SP[0] + (uint64_t)32, j = 0; i < e; ++i, ++j)
					r[j] = i < m_ext->data.size() ? m_ext->data[i] : 0;
				m_SP[0] = (u256)r;
			};
		}
		NEXT


		CASE(CALLDATASIZE)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_ext->data.size();
		}
		NEXT

		CASE(RETURNDATASIZE)
		{
			if (!m_schedule->haveReturnData)
				throwBadInstruction();

			ON_OP();
			updateIOGas();

			m_SPP[0] = m_returnData.size();
		}
		NEXT

		CASE(CODESIZE)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_ext->code.size();
		}
		NEXT

		CASE(EXTCODESIZE)
		{
			m_runGas = toInt63(m_schedule->extcodesizeGas);
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_ext->codeSizeAt(asAddress(m_SP[0]));
		}
		NEXT

		CASE(CALLDATACOPY)
		{
			ON_OP();
			m_copyMemSize = toInt63(m_SP[2]);
			updateMem(memNeed(m_SP[0], m_SP[2]));
			updateIOGas();

			copyDataToMemory(m_ext->data, m_SP);
		}
		NEXT

		CASE(RETURNDATACOPY)
		{
			ON_OP();
			if (!m_schedule->haveReturnData)
				throwBadInstruction();
			bigint const endOfAccess = bigint(m_SP[1]) + bigint(m_SP[2]);
			if (m_returnData.size() < endOfAccess)
				throwBufferOverrun(endOfAccess);

			m_copyMemSize = toInt63(m_SP[2]);
			updateMem(memNeed(m_SP[0], m_SP[2]));
			updateIOGas();

			copyDataToMemory(&m_returnData, m_SP);
		}
		NEXT

		CASE(CODECOPY)
		{
			ON_OP();
			m_copyMemSize = toInt63(m_SP[2]);
			updateMem(memNeed(m_SP[0], m_SP[2]));
			updateIOGas();

			copyDataToMemory(&m_ext->code, m_SP);
		}
		NEXT

		CASE(EXTCODECOPY)
		{
			ON_OP();
			m_runGas = toInt63(m_schedule->extcodecopyGas);
			m_copyMemSize = toInt63(m_SP[3]);
			updateMem(memNeed(m_SP[1], m_SP[3]));
			updateIOGas();

			Address a = asAddress(m_SP[0]);
			copyDataToMemory(&m_ext->codeAt(a), m_SP + 1);
		}
		NEXT


		CASE(GASPRICE)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_ext->gasPrice;
		}
		NEXT

		CASE(BLOCKHASH)
		{
			ON_OP();
			m_runGas = toInt63(m_schedule->blockhashGas);
			updateIOGas();

			m_SPP[0] = (u256)m_ext->blockHash(m_SP[0]);
		}
		NEXT

		CASE(COINBASE)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = (u160)m_ext->envInfo().author();
		}
		NEXT

		CASE(TIMESTAMP)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_ext->envInfo().timestamp();
		}
		NEXT

		CASE(NUMBER)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_ext->envInfo().number();
		}
		NEXT

		CASE(DIFFICULTY)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_ext->envInfo().difficulty();
		}
		NEXT

		CASE(GASLIMIT)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_ext->envInfo().gasLimit();
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
#if EVM_USE_CONSTANT_POOL
			ON_OP();
			updateIOGas();

			// get val at two-byte offset into const pool and advance pc by one-byte remainder
			TRACE_OP(2, m_PC, m_OP);
			unsigned off;
			++m_PC;
			off = m_code[m_PC++] << 8;
			off |= m_code[m_PC++];
			m_PC += m_code[m_PC];
			m_SPP[0] = m_pool[off];
			TRACE_VAL(2, "Retrieved pooled const", m_SPP[0]);
#else
			throwBadInstruction();
#endif
		}
		CONTINUE

		CASE(PUSH1)
		{
			ON_OP();
			updateIOGas();
			++m_PC;
			m_SPP[0] = m_code[m_PC];
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
			m_SPP[0] = 0;
			// Construct a number out of PUSH bytes.
			// This requires the code has been copied and extended by 32 zero
			// bytes to handle "out of code" push data here.
			for (++m_PC; numBytes--; ++m_PC)
				m_SPP[0] = (m_SPP[0] << 8) | m_code[m_PC];
		}
		CONTINUE

		CASE(JUMP)
		{
			ON_OP();
			updateIOGas();
			m_PC = verifyJumpDest(m_SP[0]);
		}
		CONTINUE

		CASE(JUMPI)
		{
			ON_OP();
			updateIOGas();
			if (m_SP[1])
				m_PC = verifyJumpDest(m_SP[0]);
			else
				++m_PC;
		}
		CONTINUE

		CASE(JUMPC)
		{
#if EVM_REPLACE_CONST_JUMP
			ON_OP();
			updateIOGas();

			m_PC = uint64_t(m_SP[0]);
#else
			throwBadInstruction();
#endif
		}
		CONTINUE

		CASE(JUMPCI)
		{
#if EVM_REPLACE_CONST_JUMP
			ON_OP();
			updateIOGas();

			if (m_SP[1])
				m_PC = uint64_t(m_SP[0]);
			else
				++m_PC;
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

			unsigned n = (unsigned)m_OP - (unsigned)Instruction::DUP1;
			*(uint64_t*)m_SPP = *(uint64_t*)(m_SP + n);

			// the stack slot being copied into may no longer hold a u256
			// so we construct a new one in the memory, rather than assign
			new(m_SPP) u256(m_SP[n]);
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

			unsigned n = (unsigned)m_OP - (unsigned)Instruction::SWAP1 + 1;
			std::swap(m_SP[0], m_SP[n]);
		}
		NEXT


		CASE(SLOAD)
		{
			m_runGas = toInt63(m_schedule->sloadGas);
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_ext->store(m_SP[0]);
		}
		NEXT

		CASE(SSTORE)
		{
			ON_OP();
			if (m_ext->staticCall)
				throwDisallowedStateChange();

			updateSSGas();
			updateIOGas();

			m_ext->setStore(m_SP[0], m_SP[1]);
		}
		NEXT

		CASE(PC)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_PC;
		}
		NEXT

		CASE(MSIZE)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_mem.size();
		}
		NEXT

		CASE(GAS)
		{
			ON_OP();
			updateIOGas();

			m_SPP[0] = m_io_gas;
		}
		NEXT

		CASE(JUMPDEST)
		{
			m_runGas = 1;
			ON_OP();
			updateIOGas();
		}
		NEXT

		CASE(INVALID)
		DEFAULT
		{
			throwBadInstruction();
		}
	}
	WHILE_CASES
}
