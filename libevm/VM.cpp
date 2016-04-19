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


struct InstructionMetric
{
	int gasPriceTier;
	int args;
	int ret;
};

static array<InstructionMetric, 256> metrics()
{
	array<InstructionMetric, 256> s_ret;
	for (unsigned i = 0; i < 256; ++i)
	{
		InstructionInfo inst = instructionInfo((Instruction)i);
		s_ret[i].gasPriceTier = inst.gasPriceTier;
		s_ret[i].args = inst.args;
		s_ret[i].ret = inst.ret;
	}
	return s_ret;
}


inline void VM::checkStack(unsigned _n, unsigned _d)
{
	if (m_stack.size() < _n)
	{
		if (m_onFail)
			m_onFail();
		BOOST_THROW_EXCEPTION(StackUnderflow() << RequirementError((bigint)_n, (bigint)m_stack.size()));
	}
	if (m_stack.size() - _n + _d > m_schedule->stackLimit)
	{
		if (m_onFail)
			m_onFail();
		BOOST_THROW_EXCEPTION(OutOfStack() << RequirementError((bigint)(_d - _n), (bigint)m_stack.size()));
	}
}



inline uint64_t VM::verifyJumpDest(vmword const& _dest)
{
	auto nextPC = static_cast<uint64_t>(_dest);
	if (!m_jumpDests.count(nextPC))
		BOOST_THROW_EXCEPTION(BadJumpDestination());
	return nextPC;
}

void VM::copyDataToMemory(bytesConstRef _data)
{
	auto offset = static_cast<size_t>(m_stack.back());
	m_stack.pop_back();
	soword bigIndex = m_stack.back();
	auto index = static_cast<size_t>(bigIndex);
	m_stack.pop_back();
	auto size = static_cast<size_t>(m_stack.back());
	m_stack.pop_back();

	size_t sizeToBeCopied = bigIndex + size > _data.size() ? _data.size() < bigIndex ? 0 : _data.size() - index : size;

	if (sizeToBeCopied > 0)
		std::memcpy(m_mem.data() + offset, _data.data() + index, sizeToBeCopied);
	if (size > sizeToBeCopied)
		std::memset(m_mem.data() + offset + sizeToBeCopied, 0, size - sizeToBeCopied);
}

template <class S> S divWorkaround(S const& _a, S const& _b)
{
	return (S)(VM::soword(_a) / VM::soword(_b));
}

template <class S> S modWorkaround(S const& _a, S const& _b)
{
	return (S)(VM::soword(_a) % VM::soword(_b));
}


///////////////////////////////////////////////////////////////////////////////
//
// interpreter entry point

bytesConstRef VM::execImpl(vmword& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp)
{
	m_schedule = &_ext.evmSchedule();
	m_stack.reserve((size_t)m_schedule->stackLimit);
	rmword runGas = 0, newTempSize = 0, copySize = 0;

	//
	// build hash table of valid JUMPDESTs
	// and verify static JUMP and JUMPI destinations
	//
	for (size_t i = 0; i < _ext.code.size(); ++i)
	{
		if (_ext.code[i] == (byte)Instruction::JUMPDEST)
			m_jumpDests.insert(i);
		else if (_ext.code[i] >= (byte)Instruction::PUSH1 && _ext.code[i] <= (byte)Instruction::PUSH32)
			i += _ext.code[i] - (size_t)Instruction::PUSH1 + 1;
	}
	for (size_t i = 0, j = 0; i < _ext.code.size(); ++i)
	{
		if (_ext.code[i] >= (byte)Instruction::PUSH1 && _ext.code[i] <= (byte)Instruction::PUSH32)
		{
			size_t n = _ext.code[i] - (size_t)Instruction::PUSH1 + 1;
			j = i;
			if (_ext.code[i+1] == (byte)Instruction::JUMP || _ext.code[i+1] == (byte)Instruction::JUMPI) 
			{
				// read address out of the code
				uint64_t dest = static_cast<uint64_t>(m_stack.back());
				for (n += j; j < n; ++j)
					dest = (dest << 8) | _ext.getCode(j);
					
				// if the destination is bad, bail now
//				if (!m_jumpDests.count(dest))
//					BOOST_THROW_EXCEPTION(BadJumpDestination());
					
				// replace preceding PUSH with verified JUMPV or JUMPVI
				// overwrite stack slot with aligned 64-bit dest
				// *** doable, but not done ***                                                                     
			}
			i += n;
		}
	}

	//
	// closures for metering gas and memory
	//
	
	auto onOperation = [&]()
	{
		if (_onOp)
			_onOp(m_steps, inst, newTempSize > m_mem.size() ? (newTempSize - m_mem.size()) / 32 : rmword(0), runGas, io_gas, this, &_ext);
	};
	m_onFail = std::function<void()>(onOperation);
	
	auto memNeed = [](vmword _offset, vmword _size)
	{
		return to_rmword(_size ? (uoword)_offset + _size : uoword(0));
	};
	
	auto gasForMem = [=](uoword _size) -> uoword
	{
		uoword s = _size / 32;
		return to_rmword((uoword)m_schedule->memoryGas * s + s * s / m_schedule->quadCoeffDiv);
	};
	
	auto resetIOGas = [&]() {
		if (io_gas < runGas)
			BOOST_THROW_EXCEPTION(OutOfGas());
		io_gas -= runGas;
	};
	
	auto resetGas = [&]() {
		if (newTempSize > m_mem.size())
			runGas += to_rmword(gasForMem(newTempSize) - gasForMem(m_mem.size())) ;
		runGas += to_rmword(m_schedule->copyGas * ((copySize + 31) / 32));
		if (io_gas < runGas)
			BOOST_THROW_EXCEPTION(OutOfGas());
	};
	
	auto resetMem = [&]()
	{
		newTempSize = (newTempSize + 31) / 32 * 32;
		resetGas();
		if (newTempSize > m_mem.size())
			m_mem.resize((size_t)newTempSize);
	};

	auto logGasMem = [&](Instruction inst)
	{
		unsigned n = (unsigned)inst - (unsigned)Instruction::LOG0;
		runGas = to_rmword(m_schedule->logGas + m_schedule->logTopicGas * n + to_uoword(m_schedule->logDataGas) * m_stack[m_stack.size() - 2]);
		newTempSize = memNeed(m_stack[m_stack.size() - 1], m_stack[m_stack.size() - 2]);
		resetMem();
	};


	//
	// the interpreter proper
	//

	static const auto c_metrics = metrics();
	m_steps = 0;	
	for (auto nextPC = m_curPC + 1; true; m_curPC = nextPC, nextPC = m_curPC + 1, ++m_steps)
	{
		Instruction inst = (Instruction)_ext.getCode(m_curPC);	
		auto& metric = c_metrics[static_cast<size_t>(inst)];
		
		if (metric.gasPriceTier == InvalidTier)
			BOOST_THROW_EXCEPTION(BadInstruction());
		
		checkStack(metric.args, metric.ret);
		
		// FEES...
		runGas = to_rmword(m_schedule->tierStepGas[metric.gasPriceTier]);
		newTempSize = m_mem.size();
		copySize = 0;
		
		switch (inst)
		{
		
		//
		// Call-related instructions
		//
		
		case Instruction::CREATE:
		{
			newTempSize = memNeed(m_stack[m_stack.size() - 2], m_stack[m_stack.size() - 3]);
			runGas = to_rmword(m_schedule->createGas);
			resetMem();
			onOperation();
			resetIOGas();
		
			auto const& endowment = m_stack.back();
			m_stack.pop_back();
			unsigned initOff = (unsigned)m_stack.back();
			m_stack.pop_back();
			unsigned initSize = (unsigned)m_stack.back();
			m_stack.pop_back();

			if (_ext.balance(_ext.myAddress) >= endowment && _ext.depth < 1024)
				m_stack.push_back((u160)_ext.create(endowment, io_gas, bytesConstRef(m_mem.data() + initOff, initSize), _onOp));
			else
				m_stack.push_back(0);
			break;
		}

		case Instruction::DELEGATECALL:

			// Pre-homestead
			if (!m_schedule->haveDelegateCall && inst == Instruction::DELEGATECALL)
				BOOST_THROW_EXCEPTION(BadInstruction());

		case Instruction::CALL:
		case Instruction::CALLCODE:		
		{
			runGas = to_rmword(to_uoword(m_stack[m_stack.size() - 1]) + m_schedule->callGas);
		
			if (inst == Instruction::CALL && !_ext.exists(asAddress(m_stack[m_stack.size() - 2])))
				runGas += to_rmword(m_schedule->callNewAccountGas);
		
			if (inst != Instruction::DELEGATECALL && m_stack[m_stack.size() - 3] > 0)
				runGas += to_rmword(m_schedule->callValueTransferGas);
		
			unsigned sizesOffset = inst == Instruction::DELEGATECALL ? 3 : 4;
			newTempSize = std::max(
				memNeed(m_stack[m_stack.size() - sizesOffset - 2], m_stack[m_stack.size() - sizesOffset - 3]),
				memNeed(m_stack[m_stack.size() - sizesOffset], m_stack[m_stack.size() - sizesOffset - 1])
			);
			resetMem();
			onOperation();
			resetIOGas();

			unique_ptr<CallParameters> callParams(new CallParameters());

			callParams->gas = m_stack.back();
			if (inst != Instruction::DELEGATECALL && m_stack[m_stack.size() - 3] > 0)
				callParams->gas += m_schedule->callStipend;
			m_stack.pop_back();

			callParams->codeAddress = asAddress(m_stack.back());
			m_stack.pop_back();

			if (inst == Instruction::DELEGATECALL)
			{
				callParams->apparentValue = _ext.value;
				callParams->valueTransfer = 0;
			}
			else
			{
				callParams->apparentValue = callParams->valueTransfer = m_stack.back();
				m_stack.pop_back();
			}

			unsigned inOff = (unsigned)m_stack.back();
			m_stack.pop_back();
			unsigned inSize = (unsigned)m_stack.back();
			m_stack.pop_back();
			unsigned outOff = (unsigned)m_stack.back();
			m_stack.pop_back();
			unsigned outSize = (unsigned)m_stack.back();
			m_stack.pop_back();

			if (_ext.balance(_ext.myAddress) >= callParams->valueTransfer && _ext.depth < 1024)
			{
				callParams->onOp = _onOp;
				callParams->senderAddress = inst == Instruction::DELEGATECALL ? _ext.caller : _ext.myAddress;
				callParams->receiveAddress = inst == Instruction::CALL ? callParams->codeAddress : _ext.myAddress;
				callParams->data = bytesConstRef(m_mem.data() + inOff, inSize);
				callParams->out = bytesRef(m_mem.data() + outOff, outSize);
				m_stack.push_back(_ext.call(*callParams));
			}
			else
				m_stack.push_back(0);

			io_gas += callParams->gas;
			break;
		}

		case Instruction::RETURN:
		{
			newTempSize = memNeed(m_stack.back(), m_stack[m_stack.size() - 2]);
			resetMem();
			onOperation();
			resetIOGas();

			unsigned b = (unsigned)m_stack.back();
			m_stack.pop_back();
			unsigned s = (unsigned)m_stack.back();
			m_stack.pop_back();
			return bytesConstRef(m_mem.data() + b, s);
		}

		case Instruction::SUICIDE:
		{
			onOperation();
			resetIOGas();

			Address dest = asAddress(m_stack.back());
			_ext.suicide(dest);
			return bytesConstRef();
		}

		case Instruction::STOP:
			onOperation();
			resetIOGas();

			return bytesConstRef();
			
			
		//
		// instructions potentially expanding memory
		//
		
		case Instruction::MLOAD:
		{
			newTempSize = to_rmword(m_stack.back()) + 32;
			resetMem();
			onOperation();
			resetIOGas();

			m_stack.back() = (vmword)*(h256 const*)(m_mem.data() + (unsigned)m_stack.back());
			break;
		}

		case Instruction::MSTORE:
		{
			newTempSize = to_rmword(m_stack.back()) + 32;
			resetMem();
			onOperation();
			resetIOGas();

			*(h256*)&m_mem[(unsigned)m_stack.back()] = (h256)m_stack[m_stack.size() - 2];
			m_stack.pop_back();
			m_stack.pop_back();
			break;
		}

		case Instruction::MSTORE8:
		{
			newTempSize = to_rmword(m_stack.back()) + 1;
			resetMem();
			onOperation();
			resetIOGas();

			m_mem[(unsigned)m_stack.back()] = (byte)(m_stack[m_stack.size() - 2] & 0xff);
			m_stack.pop_back();
			m_stack.pop_back();
			break;
		}

		case Instruction::SHA3:
		{
			runGas = to_rmword(m_schedule->sha3Gas + (to_uoword(m_stack[m_stack.size() - 2]) + 31) / 32 * m_schedule->sha3WordGas);
			newTempSize = memNeed(m_stack.back(), m_stack[m_stack.size() - 2]);
			resetMem();
			onOperation();
			resetIOGas();

			unsigned inOff = (unsigned)m_stack.back();
			m_stack.pop_back();
			unsigned inSize = (unsigned)m_stack.back();
			m_stack.pop_back();
			m_stack.push_back(sha3(bytesConstRef(m_mem.data() + inOff, inSize)));
			break;
		}

	   case Instruction::LOG0:
			logGasMem(inst);
			onOperation();
			resetIOGas();

			_ext.log({}, bytesConstRef(m_mem.data() + (unsigned)m_stack[m_stack.size() - 1], (unsigned)m_stack[m_stack.size() - 2]));
			m_stack.pop_back();
			m_stack.pop_back();
			break;
		case Instruction::LOG1:

			logGasMem(inst);
			onOperation();
			resetIOGas();

			_ext.log({m_stack[m_stack.size() - 3]}, bytesConstRef(m_mem.data() + (unsigned)m_stack[m_stack.size() - 1], (unsigned)m_stack[m_stack.size() - 2]));
			m_stack.pop_back();
			m_stack.pop_back();
			m_stack.pop_back();
			break;

		case Instruction::LOG2:
			logGasMem(inst);
			onOperation();
			resetIOGas();

			_ext.log({m_stack[m_stack.size() - 3], m_stack[m_stack.size() - 4]}, bytesConstRef(m_mem.data() + (unsigned)m_stack[m_stack.size() - 1], (unsigned)m_stack[m_stack.size() - 2]));
			m_stack.pop_back();
			m_stack.pop_back();
			m_stack.pop_back();
			m_stack.pop_back();
			break;

		case Instruction::LOG3:
			logGasMem(inst);
			onOperation();
			resetIOGas();

			_ext.log({m_stack[m_stack.size() - 3], m_stack[m_stack.size() - 4], m_stack[m_stack.size() - 5]}, bytesConstRef(m_mem.data() + (unsigned)m_stack[m_stack.size() - 1], (unsigned)m_stack[m_stack.size() - 2]));
			m_stack.pop_back();
			m_stack.pop_back();
			m_stack.pop_back();
			m_stack.pop_back();
			m_stack.pop_back();
			break;
		case Instruction::LOG4:
			logGasMem(inst);
			onOperation();
			resetIOGas();

			_ext.log({m_stack[m_stack.size() - 3], m_stack[m_stack.size() - 4], m_stack[m_stack.size() - 5], m_stack[m_stack.size() - 6]}, bytesConstRef(m_mem.data() + (unsigned)m_stack[m_stack.size() - 1], (unsigned)m_stack[m_stack.size() - 2]));
			m_stack.pop_back();
			m_stack.pop_back();
			m_stack.pop_back();
			m_stack.pop_back();
			m_stack.pop_back();
			m_stack.pop_back();
			break;	

		case Instruction::EXP:
		{
			auto expon = m_stack[m_stack.size() - 2];
			runGas = to_rmword(m_schedule->expGas + m_schedule->expByteGas * (32 - (h256(expon).firstBitSet() / 8)));
			resetMem();
			onOperation();
			resetIOGas();

			auto base = m_stack.back();
			m_stack.pop_back();
			m_stack.back() = (vmword)boost::multiprecision::powm((bigint)base, (bigint)expon, bigint(1) << 256);
			break;
		}


		//
		// ordinary instructions
		//

		case Instruction::ADD:
			onOperation();
			resetIOGas();

			//pops two items and pushes S[-1] + S[-2] mod 2^256.
			m_stack[m_stack.size() - 2] += m_stack.back();
			m_stack.pop_back();
			break;

		case Instruction::MUL:
			onOperation();
			resetIOGas();

			//pops two items and pushes S[-1] * S[-2] mod 2^256.
			m_stack[m_stack.size() - 2] *= m_stack.back();
			m_stack.pop_back();
			break;

		case Instruction::SUB:
			onOperation();
			resetIOGas();

			m_stack[m_stack.size() - 2] = m_stack.back() - m_stack[m_stack.size() - 2];
			m_stack.pop_back();
			break;

		case Instruction::DIV:
			onOperation();
			resetIOGas();

			m_stack[m_stack.size() - 2] = m_stack[m_stack.size() - 2] ? divWorkaround(m_stack.back(), m_stack[m_stack.size() - 2]) : 0;
			m_stack.pop_back();
			break;

		case Instruction::SDIV:
			onOperation();
			resetIOGas();

			m_stack[m_stack.size() - 2] = m_stack[m_stack.size() - 2] ? s2u(divWorkaround(u2s(m_stack.back()), u2s(m_stack[m_stack.size() - 2]))) : 0;
			m_stack.pop_back();
			break;

		case Instruction::MOD:
			onOperation();
			resetIOGas();

			m_stack[m_stack.size() - 2] = m_stack[m_stack.size() - 2] ? modWorkaround(m_stack.back(), m_stack[m_stack.size() - 2]) : 0;
			m_stack.pop_back();
			break;

		case Instruction::SMOD:
			onOperation();
			resetIOGas();

			m_stack[m_stack.size() - 2] = m_stack[m_stack.size() - 2] ? s2u(modWorkaround(u2s(m_stack.back()), u2s(m_stack[m_stack.size() - 2]))) : 0;
			m_stack.pop_back();
			break;

		case Instruction::NOT:
			onOperation();
			resetIOGas();

			m_stack.back() = ~m_stack.back();
			break;

		case Instruction::LT:
			onOperation();
			resetIOGas();

			m_stack[m_stack.size() - 2] = m_stack.back() < m_stack[m_stack.size() - 2] ? 1 : 0;
			m_stack.pop_back();
			break;

		case Instruction::GT:
			onOperation();
			resetIOGas();

			m_stack[m_stack.size() - 2] = m_stack.back() > m_stack[m_stack.size() - 2] ? 1 : 0;
			m_stack.pop_back();
			break;

		case Instruction::SLT:
			onOperation();
			resetIOGas();

			m_stack[m_stack.size() - 2] = u2s(m_stack.back()) < u2s(m_stack[m_stack.size() - 2]) ? 1 : 0;
			m_stack.pop_back();
			break;

		case Instruction::SGT:
			onOperation();
			resetIOGas();

			m_stack[m_stack.size() - 2] = u2s(m_stack.back()) > u2s(m_stack[m_stack.size() - 2]) ? 1 : 0;
			m_stack.pop_back();
			break;

		case Instruction::EQ:
			onOperation();
			resetIOGas();

			m_stack[m_stack.size() - 2] = m_stack.back() == m_stack[m_stack.size() - 2] ? 1 : 0;
			m_stack.pop_back();
			break;

		case Instruction::ISZERO:
			onOperation();
			resetIOGas();

			m_stack.back() = m_stack.back() ? 0 : 1;
			break;

		case Instruction::AND:
			onOperation();
			resetIOGas();

			m_stack[m_stack.size() - 2] = m_stack.back() & m_stack[m_stack.size() - 2];
			m_stack.pop_back();
			break;

		case Instruction::OR:
			onOperation();
			resetIOGas();

			m_stack[m_stack.size() - 2] = m_stack.back() | m_stack[m_stack.size() - 2];
			m_stack.pop_back();
			break;

		case Instruction::XOR:
			onOperation();
			resetIOGas();

			m_stack[m_stack.size() - 2] = m_stack.back() ^ m_stack[m_stack.size() - 2];
			m_stack.pop_back();
			break;

		case Instruction::BYTE:
			onOperation();
			resetIOGas();

			m_stack[m_stack.size() - 2] = m_stack.back() < 32 ? (m_stack[m_stack.size() - 2] >> (unsigned)(8 * (31 - m_stack.back()))) & 0xff : 0;
			m_stack.pop_back();
			break;

		case Instruction::ADDMOD:
			onOperation();
			resetIOGas();

			m_stack[m_stack.size() - 3] = m_stack[m_stack.size() - 3] ? vmword((to_uoword(m_stack.back()) + to_uoword(m_stack[m_stack.size() - 2])) % m_stack[m_stack.size() - 3]) : 0;
			m_stack.pop_back();
			m_stack.pop_back();
			break;

		case Instruction::MULMOD:
			onOperation();
			resetIOGas();

			m_stack[m_stack.size() - 3] = m_stack[m_stack.size() - 3] ? vmword((to_uoword(m_stack.back()) * to_uoword(m_stack[m_stack.size() - 2])) % m_stack[m_stack.size() - 3]) : 0;
			m_stack.pop_back();
			m_stack.pop_back();
			break;

		case Instruction::SIGNEXTEND:
			onOperation();
			resetIOGas();

			if (m_stack.back() < 31)
			{
				auto testBit = static_cast<unsigned>(m_stack.back()) * 8 + 7;
				vmword& number = m_stack[m_stack.size() - 2];
				vmword mask = ((vmword(1) << testBit) - 1);
				if (boost::multiprecision::bit_test(number, testBit))
					number |= ~mask;
				else
					number &= mask;
			}
			m_stack.pop_back();
			break;

		case Instruction::ADDRESS:
			onOperation();
			resetIOGas();

			m_stack.push_back(fromAddress(_ext.myAddress));
			break;

		case Instruction::ORIGIN:
			onOperation();
			resetIOGas();

			m_stack.push_back(fromAddress(_ext.origin));
			break;

		case Instruction::BALANCE:
		{
			onOperation();
			resetIOGas();

			m_stack.back() = _ext.balance(asAddress(m_stack.back()));
			break;
		}

		case Instruction::CALLER:
			onOperation();
			resetIOGas();

			m_stack.push_back(fromAddress(_ext.caller));
			break;

		case Instruction::CALLVALUE:
			onOperation();
			resetIOGas();

			m_stack.push_back(_ext.value);
			break;


		case Instruction::CALLDATALOAD:
		{
			onOperation();
			resetIOGas();

			if (to_uoword(m_stack.back()) + 31 < _ext.data.size())
				m_stack.back() = (vmword)*(h256 const*)(_ext.data.data() + (size_t)m_stack.back());
			else if (m_stack.back() >= _ext.data.size())
				m_stack.back() = vmword(0);
			else
			{
				h256 r;
				for (uint64_t i = (unsigned)m_stack.back(), e = (unsigned)m_stack.back() + (uint64_t)32, j = 0; i < e; ++i, ++j)
					r[j] = i < _ext.data.size() ? _ext.data[i] : 0;
				m_stack.back() = (vmword)r;
			}
			break;
		}

		case Instruction::CALLDATASIZE:
			onOperation();
			resetIOGas();

			m_stack.push_back(_ext.data.size());
			break;

		case Instruction::CODESIZE:
			onOperation();
			resetIOGas();

			m_stack.push_back(_ext.code.size());
			break;

		case Instruction::EXTCODESIZE:
			onOperation();
			resetIOGas();

			m_stack.back() = _ext.codeAt(asAddress(m_stack.back())).size();
			break;

		case Instruction::CALLDATACOPY:
			copySize = to_rmword(m_stack[m_stack.size() - 3]);
			newTempSize = memNeed(m_stack.back(), m_stack[m_stack.size() - 3]);
			resetMem();
			onOperation();
			resetIOGas();

			copyDataToMemory(_ext.data);
			break;

		case Instruction::CODECOPY:
			copySize = to_rmword(m_stack[m_stack.size() - 3]);
			newTempSize = memNeed(m_stack.back(), m_stack[m_stack.size() - 3]);
			resetMem();
			onOperation();
			resetIOGas();

			copyDataToMemory(&_ext.code);
			break;

		case Instruction::EXTCODECOPY:
		{
			copySize = to_rmword(m_stack[m_stack.size() - 4]);
			newTempSize = memNeed(m_stack[m_stack.size() - 2], m_stack[m_stack.size() - 4]);
			resetMem();
			onOperation();
			resetIOGas();

			auto a = asAddress(m_stack.back());
			m_stack.pop_back();
			copyDataToMemory(&_ext.codeAt(a));
			break;
		}

		case Instruction::GASPRICE:
			onOperation();
			resetIOGas();

			m_stack.push_back(_ext.gasPrice);
			break;

		case Instruction::BLOCKHASH:
			onOperation();
			resetIOGas();

			m_stack.back() = (vmword)_ext.blockHash(m_stack.back());
			break;

		case Instruction::COINBASE:
			onOperation();
			resetIOGas();

			m_stack.push_back((u160)_ext.envInfo().author());
			break;

		case Instruction::TIMESTAMP:
			onOperation();
			resetIOGas();

			m_stack.push_back(_ext.envInfo().timestamp());
			break;

		case Instruction::NUMBER:
			onOperation();
			resetIOGas();

			m_stack.push_back(_ext.envInfo().number());
			break;

		case Instruction::DIFFICULTY:
			onOperation();
			resetIOGas();

			m_stack.push_back(_ext.envInfo().difficulty());
			break;

		case Instruction::GASLIMIT:
			onOperation();
			resetIOGas();

			m_stack.push_back(_ext.envInfo().gasLimit());
			break;

		case Instruction::POP:
			onOperation();
			resetIOGas();

			m_stack.pop_back();
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
			resetIOGas();

			int i = (int)inst - (int)Instruction::PUSH1 + 1;
			nextPC = m_curPC + 1;
			m_stack.push_back(0);
			for (; i--; nextPC++)
				m_stack.back() = (m_stack.back() << 8) | _ext.getCode(nextPC);
			break;
		}

		case Instruction::JUMP:
			onOperation();
			resetIOGas();

			nextPC = verifyJumpDest(m_stack.back());
			m_stack.pop_back();
			break;

		case Instruction::JUMPI:
			onOperation();
			resetIOGas();

			if (m_stack[m_stack.size() - 2])
				nextPC = verifyJumpDest(m_stack.back());
			m_stack.pop_back();
			m_stack.pop_back();
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
			resetIOGas();

			auto n = 1 + (unsigned)inst - (unsigned)Instruction::DUP1;
			m_stack.push_back(m_stack[m_stack.size() - n]);
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
			resetIOGas();

			auto n = (unsigned)inst - (unsigned)Instruction::SWAP1 + 2;
			auto d = m_stack.back();
			m_stack.back() = m_stack[m_stack.size() - n];
			m_stack[m_stack.size() - n] = d;
			break;
		}

		case Instruction::SLOAD:
			runGas = to_rmword(m_schedule->sloadGas);
			onOperation();
			resetIOGas();

			m_stack.back() = _ext.store(m_stack.back());
			break;

		case Instruction::SSTORE:
			if (!_ext.store(m_stack.back()) && m_stack[m_stack.size() - 2])
				runGas = to_rmword(m_schedule->sstoreSetGas);
			else if (_ext.store(m_stack.back()) && !m_stack[m_stack.size() - 2])
			{
				runGas = to_rmword(m_schedule->sstoreResetGas);
				_ext.sub.refunds += m_schedule->sstoreRefundGas;
			}
			else
				runGas = to_rmword(m_schedule->sstoreResetGas);
			onOperation();
			resetIOGas();
	
			_ext.setStore(m_stack.back(), m_stack[m_stack.size() - 2]);
			m_stack.pop_back();
			m_stack.pop_back();
			break;

		case Instruction::PC:
			onOperation();
			resetIOGas();

			m_stack.push_back(m_curPC);
			break;

		case Instruction::MSIZE:
			onOperation();
			resetIOGas();

			m_stack.push_back(m_mem.size());
			break;

		case Instruction::GAS:
			onOperation();
			resetIOGas();

			m_stack.push_back(io_gas);
			break;

		case Instruction::JUMPDEST:
			runGas = 1;
			onOperation();
			resetIOGas();
			break;

		default:
			BOOST_THROW_EXCEPTION(BadInstruction());
		}
	}

	return bytesConstRef();
}
