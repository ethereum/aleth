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
#include <libethereum/ExtVM.h>
using namespace std;
using namespace dev;
using namespace dev::eth;

// Convert from a 256-bit integer stack/memory entry into a 160-bit Address hash.
// Currently we just pull out the right (low-order in BE) 160-bits.
inline Address asAddress(u256 _item)
{
	return right160(h256(_item));
}

inline u256 fromAddress(Address _a)
{
	return (u160)_a;
}

template<class T> static uint64_t toUint64(T v)
{
	// check for overflow
	if (v > 0x7FFFFFFFFFFFFFFF)
		throwVMException(OutOfGas());
	uint64_t w = uint64_t(v);
	return w;
}


uint64_t VM::verifyJumpDest(u256 const& _dest)
{
	// check for overflow
	if (_dest > 0x7FFFFFFFFFFFFFFF)
		throwVMException(BadJumpDestination());
	uint64_t pc = uint64_t(_dest);
	if (!m_jumpDests.count(pc))
		throwVMException(BadJumpDestination());
	return pc;
}



static uint64_t memNeed(u256 _offset, u256 _size)
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
		(*m_onOp)(++m_nSteps, PC, inst,
			newMemSize > m_mem.size() ? (newMemSize - m_mem.size()) / 32 : uint64_t(0),
			runGas, *m_io_gas, this, m_ext);
}

void VM::checkStack(unsigned _removed, unsigned _added)
{
	int const size = 1 + SP - m_stack;
	int const usedSize = size - _removed;
	if (usedSize < 0 || usedSize + _added > 1024)
		throwVMStackException(size, _removed, _added);
}

uint64_t VM::gasForMem(u512 _size)
{
	u512 s = _size / 32;
	return toUint64((u512)m_schedule->memoryGas * s + s * s / m_schedule->quadCoeffDiv);
}

void VM::updateIOGas() {
	if (*m_io_gas < runGas)
		throwVMException(OutOfGas());
	*m_io_gas -= runGas;
}

void VM::updateGas() {
	if (newMemSize > m_mem.size())
		runGas += toUint64(gasForMem(newMemSize) - gasForMem(m_mem.size())) ;
	runGas += (m_schedule->copyGas * ((copyMemSize + 31) / 32));
	if (*m_io_gas < runGas)
		throwVMException(OutOfGas());
}

void VM::updateMem()
{
	newMemSize = (newMemSize + 31) / 32 * 32;
	updateGas();
	if (newMemSize > m_mem.size())
		m_mem.resize(newMemSize);
}

void VM::logGasMem(Instruction inst)
{
	unsigned n = (unsigned)inst - (unsigned)Instruction::LOG0;
	runGas = toUint64(m_schedule->logGas + m_schedule->logTopicGas * n + u512(m_schedule->logDataGas) * *(SP - 1));
	newMemSize = memNeed(*SP, *(SP - 1));
	updateMem();
}


///////////////////////////////////////////////////////////////////////////////
//
// interpreter entry point

bytesConstRef VM::execImpl(u256& _io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp)
{
	m_io_gas = &_io_gas;
	m_ext = &_ext;
	m_schedule = &m_ext->evmSchedule();
	m_onOp = &_onOp;
	m_onFail = &VM::onOperation;

	initMetrics();
	makeJumpDestTable(_ext);

	// trampoline to minimize depth of call stack when calling out
	m_bounce = &VM::interpretCases;
	do
		(this->*m_bounce)();
	while (m_bounce);

	return m_bytes;
}

//
// interpreter cases that call out
//

void VM::caseCreate()
{
	m_bounce = &VM::interpretCases;
	newMemSize = memNeed(*(SP - 1), *(SP - 2));
	runGas = toUint64(m_schedule->createGas);
	updateMem();
	onOperation();
	updateIOGas();
	
	auto const& endowment = *SP--;
	uint64_t initOff = (uint64_t)*SP--;
	uint64_t initSize = (uint64_t)*SP--;
	
	if (m_ext->balance(m_ext->myAddress) >= endowment && m_ext->depth < 1024)
		*++SP = (u160)m_ext->create(endowment, *m_io_gas, bytesConstRef(m_mem.data() + initOff, initSize), *m_onOp);
	else
		*++SP = 0;
}

void VM::caseCall()
{
	m_bounce = &VM::interpretCases;
	unique_ptr<CallParameters> callParams(new CallParameters());
	if (caseCallSetup(&*callParams))
		*++SP = m_ext->call(*callParams);
	else
		*++SP = 0;
	*m_io_gas += callParams->gas;
}

bool VM::caseCallSetup(CallParameters *callParams)
{
	runGas = toUint64(u512(*SP) + m_schedule->callGas);

	if (inst == Instruction::CALL && !m_ext->exists(asAddress(*(SP - 1))))
		runGas += toUint64(m_schedule->callNewAccountGas);

	if (inst != Instruction::DELEGATECALL && *(SP - 2) > 0)
		runGas += toUint64(m_schedule->callValueTransferGas);

	unsigned sizesOffset = inst == Instruction::DELEGATECALL ? 3 : 4;
	newMemSize = std::max(
		memNeed(m_stack[(1 + SP - m_stack) - sizesOffset - 2], m_stack[(1 + SP - m_stack) - sizesOffset - 3]),
		memNeed(m_stack[(1 + SP - m_stack) - sizesOffset], m_stack[(1 + SP - m_stack) - sizesOffset - 1])
	);
	updateMem();
	onOperation();
	updateIOGas();

	callParams->gas = *SP;
	if (inst != Instruction::DELEGATECALL && *(SP - 2) > 0)
		callParams->gas += m_schedule->callStipend;
	--SP;

	callParams->codeAddress = asAddress(*SP);
	--SP;

	if (inst == Instruction::DELEGATECALL)
	{
		callParams->apparentValue = m_ext->value;
		callParams->valueTransfer = 0;
	}
	else
	{
		callParams->apparentValue = callParams->valueTransfer = *SP;
		--SP;
	}

	uint64_t inOff = (uint64_t)*SP--;
	uint64_t inSize = (uint64_t)*SP--;
	uint64_t outOff = (uint64_t)*SP--;
	uint64_t outSize = (uint64_t)*SP--;

	if (m_ext->balance(m_ext->myAddress) >= callParams->valueTransfer && m_ext->depth < 1024)
	{
		callParams->onOp = *m_onOp;
		callParams->senderAddress = inst == Instruction::DELEGATECALL ? m_ext->caller : m_ext->myAddress;
		callParams->receiveAddress = inst == Instruction::CALL ? callParams->codeAddress : m_ext->myAddress;
		callParams->data = bytesConstRef(m_mem.data() + inOff, inSize);
		callParams->out = bytesRef(m_mem.data() + outOff, outSize);
		return true;
	}
	else
		return false;
}

//
// main interpreter loop and switch
//
void VM::interpretCases()
{
	for (;;)
	{
		inst = (Instruction)m_ext->getCode(PC);
		InstructionMetric metric = c_metrics[static_cast<size_t>(inst)];
		
		checkStack(metric.args, metric.ret);
		
		// FEES...
		runGas = toUint64(m_schedule->tierStepGas[metric.gasPriceTier]);
		newMemSize = m_mem.size();
		copyMemSize = 0;
		
		switch (inst)
		{
		
		//
		// Call-related instructions
		//
		
		case Instruction::CREATE:
//			caseCreate();
//			break;
			m_bounce = &VM::caseCreate;
			++PC;
			return;

		case Instruction::DELEGATECALL:

			// Pre-homestead
			if (!m_schedule->haveDelegateCall)
				throwVMException(BadInstruction());

		case Instruction::CALL:
		case Instruction::CALLCODE:
		{
//			caseCall();
//			break;
			m_bounce = &VM::caseCall;
			++PC;
			return;
		}

		case Instruction::RETURN:
		{
			newMemSize = memNeed(*SP, *(SP - 1));
			updateMem();
			onOperation();
			updateIOGas();

			uint64_t b = (uint64_t)*SP--;
			uint64_t s = (uint64_t)*SP--;
			m_bytes = bytesConstRef(m_mem.data() + b, s);
			m_bounce = 0;
			return;
		}

		case Instruction::SUICIDE:
		{
			onOperation();
			updateIOGas();

			Address dest = asAddress(*SP);
			m_ext->suicide(dest);
			m_bounce = 0;
			return;
		}

		case Instruction::STOP:
			onOperation();
			updateIOGas();

			m_bounce = 0;
			return;
			
			
		//
		// instructions potentially expanding memory
		//
		
		case Instruction::MLOAD:
		{
			newMemSize = toUint64(*SP) + 32;
			updateMem();
			onOperation();
			updateIOGas();

			*SP = (u256)*(h256 const*)(m_mem.data() + (unsigned)*SP);
			break;
		}

		case Instruction::MSTORE:
		{
			newMemSize = toUint64(*SP) + 32;
			updateMem();
			onOperation();
			updateIOGas();

			*(h256*)&m_mem[(unsigned)*SP] = (h256)*(SP - 1);
			SP -= 2;
			break;
		}

		case Instruction::MSTORE8:
		{
			newMemSize = toUint64(*SP) + 1;
			updateMem();
			onOperation();
			updateIOGas();

			m_mem[(unsigned)*SP] = (byte)(*(SP - 1) & 0xff);
			SP -= 2;
			break;
		}

		case Instruction::SHA3:
		{
			runGas = toUint64(m_schedule->sha3Gas + (u512(*(SP - 1)) + 31) / 32 * m_schedule->sha3WordGas);
			newMemSize = memNeed(*SP, *(SP - 1));
			updateMem();
			onOperation();
			updateIOGas();

			uint64_t inOff = (uint64_t)*SP--;
			uint64_t inSize = (uint64_t)*SP--;
			*++SP = (u256)sha3(bytesConstRef(m_mem.data() + inOff, inSize));
			break;
		}

		case Instruction::LOG0:
			logGasMem(inst);
			onOperation();
			updateIOGas();

			m_ext->log({}, bytesConstRef(m_mem.data() + (uint64_t)*SP, (uint64_t)*(SP - 1)));
			SP -= 2;
			break;

		case Instruction::LOG1:
			logGasMem(inst);
			onOperation();
			updateIOGas();

			m_ext->log({*(SP - 2)}, bytesConstRef(m_mem.data() + (uint64_t)*SP, (uint64_t)*(SP - 1)));
			SP -= 3;
			break;

		case Instruction::LOG2:
			logGasMem(inst);
			onOperation();
			updateIOGas();

			m_ext->log({*(SP - 2), *(SP-3)}, bytesConstRef(m_mem.data() + (uint64_t)*SP, (uint64_t)*(SP - 1)));
			SP -= 4;
			break;

		case Instruction::LOG3:
			logGasMem(inst);
			onOperation();
			updateIOGas();

			m_ext->log({*(SP - 2), *(SP-3), *(SP-4)}, bytesConstRef(m_mem.data() + (uint64_t)*SP, (uint64_t)*(SP - 1)));
			SP -= 5;
			break;
		case Instruction::LOG4:
			logGasMem(inst);
			onOperation();
			updateIOGas();

			m_ext->log({*(SP - 2), *(SP-3), *(SP-4), *(SP-5)}, bytesConstRef(m_mem.data() + (uint64_t)*SP, (uint64_t)*(SP - 1)));
			SP -= 6;
			break;	

		case Instruction::EXP:
		{
			auto expon = *(SP - 1);
			runGas = toUint64(m_schedule->expGas + m_schedule->expByteGas * (32 - (h256(expon).firstBitSet() / 8)));
			updateMem();
			onOperation();
			updateIOGas();

			auto base = *SP--;
			*SP = (u256)boost::multiprecision::powm((bigint)base, (bigint)expon, bigint(1) << 256);
			break;
		}


		//
		// ordinary instructions
		//

		case Instruction::ADD:
			onOperation();
			updateIOGas();

			//pops two items and pushes S[-1] + S[-2] mod 2^256.
			*(SP - 1) += *SP;
			--SP;
			break;

		case Instruction::MUL:
			onOperation();
			updateIOGas();

			//pops two items and pushes S[-1] * S[-2] mod 2^256.
			*(SP - 1) *= *SP;
			--SP;
			break;

		case Instruction::SUB:
			onOperation();
			updateIOGas();

			*(SP - 1) = *SP - *(SP - 1);
			--SP;
			break;

		case Instruction::DIV:
			onOperation();
			updateIOGas();

			*(SP - 1) = *(SP - 1) ? divWorkaround(*SP, *(SP - 1)) : 0;
			--SP;
			break;

		case Instruction::SDIV:
			onOperation();
			updateIOGas();

			*(SP - 1) = *(SP - 1) ? s2u(divWorkaround(u2s(*SP), u2s(*(SP - 1)))) : 0;
			--SP;
			break;

		case Instruction::MOD:
			onOperation();
			updateIOGas();

			*(SP - 1) = *(SP - 1) ? modWorkaround(*SP, *(SP - 1)) : 0;
			--SP;
			break;

		case Instruction::SMOD:
			onOperation();
			updateIOGas();

			*(SP - 1) = *(SP - 1) ? s2u(modWorkaround(u2s(*SP), u2s(*(SP - 1)))) : 0;
			--SP;
			break;

		case Instruction::NOT:
			onOperation();
			updateIOGas();

			*SP = ~*SP;
			break;

		case Instruction::LT:
			onOperation();
			updateIOGas();

			*(SP - 1) = *SP < *(SP - 1) ? 1 : 0;
			--SP;
			break;

		case Instruction::GT:
			onOperation();
			updateIOGas();

			*(SP - 1) = *SP > *(SP - 1) ? 1 : 0;
			--SP;
			break;

		case Instruction::SLT:
			onOperation();
			updateIOGas();

			*(SP - 1) = u2s(*SP) < u2s(*(SP - 1)) ? 1 : 0;
			--SP;
			break;

		case Instruction::SGT:
			onOperation();
			updateIOGas();

			*(SP - 1) = u2s(*SP) > u2s(*(SP - 1)) ? 1 : 0;
			--SP;
			break;

		case Instruction::EQ:
			onOperation();
			updateIOGas();

			*(SP - 1) = *SP == *(SP - 1) ? 1 : 0;
			--SP;
			break;

		case Instruction::ISZERO:
			onOperation();
			updateIOGas();

			*SP = *SP ? 0 : 1;
			break;

		case Instruction::AND:
			onOperation();
			updateIOGas();

			*(SP - 1) = *SP & *(SP - 1);
			--SP;
			break;

		case Instruction::OR:
			onOperation();
			updateIOGas();

			*(SP - 1) = *SP | *(SP - 1);
			--SP;
			break;

		case Instruction::XOR:
			onOperation();
			updateIOGas();

			*(SP - 1) = *SP ^ *(SP - 1);
			--SP;
			break;

		case Instruction::BYTE:
			onOperation();
			updateIOGas();

			*(SP - 1) = *SP < 32 ? (*(SP - 1) >> (unsigned)(8 * (31 - *SP))) & 0xff : 0;
			--SP;
			break;

		case Instruction::ADDMOD:
			onOperation();
			updateIOGas();

			*(SP - 2) = *(SP - 2) ? u256((u512(*SP) + u512(*(SP - 1))) % *(SP - 2)) : 0;
			SP -= 2;
			break;

		case Instruction::MULMOD:
			onOperation();
			updateIOGas();

			*(SP - 2) = *(SP - 2) ? u256((u512(*SP) * u512(*(SP - 1))) % *(SP - 2)) : 0;
			SP -= 2;
			break;

		case Instruction::SIGNEXTEND:
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
			break;

		case Instruction::ADDRESS:
			onOperation();
			updateIOGas();

			*++SP = fromAddress(m_ext->myAddress);
			break;

		case Instruction::ORIGIN:
			onOperation();
			updateIOGas();

			*++SP = fromAddress(m_ext->origin);
			break;

		case Instruction::BALANCE:
		{
			onOperation();
			updateIOGas();

			*SP = m_ext->balance(asAddress(*SP));
			break;
		}

		case Instruction::CALLER:
			onOperation();
			updateIOGas();

			*++SP = fromAddress(m_ext->caller);
			break;

		case Instruction::CALLVALUE:
			onOperation();
			updateIOGas();

			*++SP = m_ext->value;
			break;


		case Instruction::CALLDATALOAD:
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
			break;
		}

		case Instruction::CALLDATASIZE:
			onOperation();
			updateIOGas();

			*++SP = m_ext->data.size();
			break;

		case Instruction::CODESIZE:
			onOperation();
			updateIOGas();

			*++SP = m_ext->code.size();
			break;

		case Instruction::EXTCODESIZE:
			onOperation();
			updateIOGas();

			*SP = m_ext->codeAt(asAddress(*SP)).size();
			break;

		case Instruction::CALLDATACOPY:
			copyMemSize = toUint64(*(SP - 2));
			newMemSize = memNeed(*SP, *(SP - 2));
			updateMem();
			onOperation();
			updateIOGas();

			copyDataToMemory(m_ext->data, SP);
			break;

		case Instruction::CODECOPY:
			copyMemSize = toUint64(*(SP - 2));
			newMemSize = memNeed(*SP, *(SP - 2));
			updateMem();
			onOperation();
			updateIOGas();

			copyDataToMemory(&m_ext->code, SP);
			break;

		case Instruction::EXTCODECOPY:
		{
			copyMemSize = toUint64(*(SP - 3));
			newMemSize = memNeed(*(SP - 1), *(SP - 3));
			updateMem();
			onOperation();
			updateIOGas();

			auto a = asAddress(*SP);
			--SP;
			copyDataToMemory(&m_ext->codeAt(a), SP);
			break;
		}

		case Instruction::GASPRICE:
			onOperation();
			updateIOGas();

			*++SP = m_ext->gasPrice;
			break;

		case Instruction::BLOCKHASH:
			onOperation();
			updateIOGas();

			*SP = (u256)m_ext->blockHash(*SP);
			break;

		case Instruction::COINBASE:
			onOperation();
			updateIOGas();

			*++SP = (u160)m_ext->envInfo().author();
			break;

		case Instruction::TIMESTAMP:
			onOperation();
			updateIOGas();

			*++SP = m_ext->envInfo().timestamp();
			break;

		case Instruction::NUMBER:
			onOperation();
			updateIOGas();

			*++SP = m_ext->envInfo().number();
			break;

		case Instruction::DIFFICULTY:
			onOperation();
			updateIOGas();

			*++SP = m_ext->envInfo().difficulty();
			break;

		case Instruction::GASLIMIT:
			onOperation();
			updateIOGas();

			*++SP = m_ext->envInfo().gasLimit();
			break;

		case Instruction::POP:
			onOperation();
			updateIOGas();

			--SP;
			break;

		case Instruction::PUSH1:
		case Instruction::PUSH2:
		case Instruction::PUSH3:
		case Instruction::PUSH4:
		case Instruction::PUSH5:
		case Instruction::PUSH6:
		case Instruction::PUSH7:
		case Instruction::PUSH8:
		case Instruction::PUSH9:
		case Instruction::PUSH10:
		case Instruction::PUSH11:
		case Instruction::PUSH12:
		case Instruction::PUSH13:
		case Instruction::PUSH14:
		case Instruction::PUSH15:
		case Instruction::PUSH16:
		case Instruction::PUSH17:
		case Instruction::PUSH18:
		case Instruction::PUSH19:
		case Instruction::PUSH20:
		case Instruction::PUSH21:
		case Instruction::PUSH22:
		case Instruction::PUSH23:
		case Instruction::PUSH24:
		case Instruction::PUSH25:
		case Instruction::PUSH26:
		case Instruction::PUSH27:
		case Instruction::PUSH28:
		case Instruction::PUSH29:
		case Instruction::PUSH30:
		case Instruction::PUSH31:
		case Instruction::PUSH32:
		{
			onOperation();
			updateIOGas();

			int i = (int)inst - (int)Instruction::PUSH1 + 1;
			*++SP = 0;
			for (++PC; i--; ++PC)
				*SP = (*SP << 8) | m_ext->getCode(PC);
			continue;
		}

		case Instruction::JUMP:
			onOperation();
			updateIOGas();

			PC = verifyJumpDest(*SP);
			--SP;
			continue;

		case Instruction::JUMPI:
			onOperation();
			updateIOGas();

			if (*(SP - 1))
			{
				PC = verifyJumpDest(*SP);
				SP -= 2;
				continue;
			}
			SP -= 2;
			break;

		case Instruction::DUP1:
		case Instruction::DUP2:
		case Instruction::DUP3:
		case Instruction::DUP4:
		case Instruction::DUP5:
		case Instruction::DUP6:
		case Instruction::DUP7:
		case Instruction::DUP8:
		case Instruction::DUP9:
		case Instruction::DUP10:
		case Instruction::DUP11:
		case Instruction::DUP12:
		case Instruction::DUP13:
		case Instruction::DUP14:
		case Instruction::DUP15:
		case Instruction::DUP16:
		{
			onOperation();
			updateIOGas();

			unsigned n = 1 + (unsigned)inst - (unsigned)Instruction::DUP1;
			*(SP+1) = m_stack[(1 + SP - m_stack) - n];
			++SP;
			break;
		}

		case Instruction::SWAP1:
		case Instruction::SWAP2:
		case Instruction::SWAP3:
		case Instruction::SWAP4:
		case Instruction::SWAP5:
		case Instruction::SWAP6:
		case Instruction::SWAP7:
		case Instruction::SWAP8:
		case Instruction::SWAP9:
		case Instruction::SWAP10:
		case Instruction::SWAP11:
		case Instruction::SWAP12:
		case Instruction::SWAP13:
		case Instruction::SWAP14:
		case Instruction::SWAP15:
		case Instruction::SWAP16:
		{
			onOperation();
			updateIOGas();

			auto n = (unsigned)inst - (unsigned)Instruction::SWAP1 + 2;
			auto d = *SP;
			*SP = m_stack[(1 + SP - m_stack) - n];
			m_stack[(1 + SP - m_stack) - n] = d;
			break;
		}

		case Instruction::SLOAD:
			runGas = toUint64(m_schedule->sloadGas);
			onOperation();
			updateIOGas();

			*SP = m_ext->store(*SP);
			break;

		case Instruction::SSTORE:
			if (!m_ext->store(*SP) && *(SP - 1))
				runGas = toUint64(m_schedule->sstoreSetGas);
			else if (m_ext->store(*SP) && !*(SP - 1))
			{
				runGas = toUint64(m_schedule->sstoreResetGas);
				m_ext->sub.refunds += m_schedule->sstoreRefundGas;
			}
			else
				runGas = toUint64(m_schedule->sstoreResetGas);
			onOperation();
			updateIOGas();
	
			m_ext->setStore(*SP, *(SP - 1));
			SP -= 2;
			break;

		case Instruction::PC:
			onOperation();
			updateIOGas();

			*++SP = PC;
			break;

		case Instruction::MSIZE:
			onOperation();
			updateIOGas();

			*++SP = m_mem.size();
			break;

		case Instruction::GAS:
			onOperation();
			updateIOGas();

			*++SP = *m_io_gas;
			break;

		case Instruction::JUMPDEST:
			runGas = 1;
			onOperation();
			updateIOGas();
			break;

		default:
			throwVMException(BadInstruction());
		}
		
		++PC;
	}

}
