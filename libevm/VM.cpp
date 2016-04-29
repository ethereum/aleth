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


// ethvm swallows exceptions
//#undef BOOST_THROW_EXCEPTION
//#define BOOST_THROW_EXCEPTION(X) (fprintf(stderr,"!!! %d EXCEPTION \n", __LINE__), exit(0))


// real machine word, virtual machine word, signed and unsigned overflow words
typedef uint64_t mw64;
typedef mw64 rmword;
typedef u256 vmword;
typedef s512 soword;
typedef u512 uoword;
	
// checked generic convertions
template<class T> static rmword to_rmword(T v) { if (rmword(v) != v) BOOST_THROW_EXCEPTION(OutOfGas()); return rmword(v); }
template<class T> static uoword to_uoword(T v) { if (uoword(v) != v) BOOST_THROW_EXCEPTION(OutOfGas()); return uoword(v); }
template<class T> static soword to_soword(T v) { if (soword(v) != v) BOOST_THROW_EXCEPTION(OutOfGas()); return soword(v); }


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


// build hash table of valid JUMPDESTs
// to verify static JUMP and JUMPI destinations
//

void VM::verifyJumpTable(ExtVMFace& _ext)
{
	// hash JUMPDESTs into table
	for (size_t i = 0; i < _ext.code.size(); ++i)
	{
		byte inst = _ext.code[i];
		if (inst == (byte)Instruction::JUMPDEST)
			m_jumpDests.insert(i);
		else if (inst >= (byte)Instruction::PUSH1 && inst <= (byte)Instruction::PUSH32)
			i += inst - (byte)Instruction::PUSH1 + 1;
	}
}

template <class S> S divWorkaround(S const& _a, S const& _b)
{
	return (S)(soword(_a) / soword(_b));
}

template <class S> S modWorkaround(S const& _a, S const& _b)
{
	return (S)(soword(_a) % soword(_b));
}


///////////////////////////////////////////////////////////////////////////////
//
// interpreter entry point

bytesConstRef VM::execImpl(vmword& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp)
{
	Instruction inst;
	uint64_t PC = 0;
	u256* SP = m_stack - 1;
	
	m_schedule = &_ext.evmSchedule();
	rmword runGas = 0, newTempSize = 0, copySize = 0;
	uint64_t m_steps = 0;

	// hash JUMPDESTs into table to verify static JUMP and JUMPI destinations
	for (size_t i = 0; i < _ext.code.size(); ++i)
	{
		byte inst = _ext.code[i];
		if (inst == (byte)Instruction::JUMPDEST)
			m_jumpDests.insert(i);
		else if (inst >= (byte)Instruction::PUSH1 && inst <= (byte)Instruction::PUSH32)
			i += inst - (byte)Instruction::PUSH1 + 1;
	}

	//
	// closures for tracing, checking, metering, measuring ...
	//
	
	auto onOperation = [&]()
	{
		if (_onOp)
			_onOp(++m_steps, inst, newTempSize > m_mem.size() ? (newTempSize - m_mem.size()) / 32 : rmword(0), runGas, io_gas, this, &_ext);
	};
	m_onFail = std::function<void()>(onOperation);
	
	auto checkStack = [&](unsigned _n, unsigned _d)
	{
		const size_t size = 1 + SP - m_stack;
		if ((size) < _n)
		{
			if (m_onFail)
				m_onFail();
			BOOST_THROW_EXCEPTION(StackUnderflow() << RequirementError((bigint)_n, (bigint)size));
		}
		if ((size) - _n + _d > 1024)
		{
			if (m_onFail)
				m_onFail();
			BOOST_THROW_EXCEPTION(OutOfStack() << RequirementError((bigint)(_d - _n), (bigint)size));
		}
	};
	
	auto verifyJumpDest = [&](u256 const& _dest) -> uint64_t
	{
		uint64_t pc = uint64_t(_dest);
		if (!m_jumpDests.count(pc))
			BOOST_THROW_EXCEPTION(BadJumpDestination());
		return pc;
	};

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
		runGas = to_rmword(m_schedule->logGas + m_schedule->logTopicGas * n + to_uoword(m_schedule->logDataGas) * *(SP-1));
		newTempSize = memNeed(*SP, *(SP-1));
		resetMem();
	};

	auto copyDataToMemory= [&](bytesConstRef _data)
	{
		auto offset = static_cast<size_t>(*SP);
		--SP;
		soword bigIndex = *SP;
		auto index = static_cast<size_t>(bigIndex);
		--SP;
		auto size = static_cast<size_t>(*SP);
		--SP;
	
		size_t sizeToBeCopied = bigIndex + size > _data.size() ? _data.size() < bigIndex ? 0 : _data.size() - index : size;
	
		if (sizeToBeCopied > 0)
			std::memcpy(m_mem.data() + offset, _data.data() + index, sizeToBeCopied);
		if (size > sizeToBeCopied)
			std::memset(m_mem.data() + offset + sizeToBeCopied, 0, size - sizeToBeCopied);
	};

	//
	// the interpreter, itself
	//

	static const auto c_metrics = metrics();
	m_steps = 0;	
	for (;;)
	{
		Instruction inst = (Instruction)_ext.getCode(PC);	
		auto& metric = c_metrics[static_cast<size_t>(inst)];

//		if (metric.gasPriceTier == InvalidTier)
//			BOOST_THROW_EXCEPTION(BadInstruction());
		
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
			newTempSize = memNeed(*(SP-1), *(SP-2));
			runGas = to_rmword(m_schedule->createGas);
			resetMem();
			onOperation();
			resetIOGas();
		
			auto const& endowment = *SP;
			--SP;
			unsigned initOff = (unsigned)*SP;
			--SP;
			unsigned initSize = (unsigned)*SP;
			--SP;

			if (_ext.balance(_ext.myAddress) >= endowment && _ext.depth < 1024)
				*++SP = (u160)_ext.create(endowment, io_gas, bytesConstRef(m_mem.data() + initOff, initSize), _onOp);
			else
				*++SP = 0;
			break;
		}

		case Instruction::DELEGATECALL:

			// Pre-homestead
			if (!m_schedule->haveDelegateCall && inst == Instruction::DELEGATECALL)
				BOOST_THROW_EXCEPTION(BadInstruction());

		case Instruction::CALL:
		case Instruction::CALLCODE:		
		{
			runGas = to_rmword(to_uoword(*SP) + m_schedule->callGas);
		
			if (inst == Instruction::CALL && !_ext.exists(asAddress(*(SP-1))))
				runGas += to_rmword(m_schedule->callNewAccountGas);
		
			if (inst != Instruction::DELEGATECALL && *(SP-2) > 0)
				runGas += to_rmword(m_schedule->callValueTransferGas);
		
			unsigned sizesOffset = inst == Instruction::DELEGATECALL ? 3 : 4;
			newTempSize = std::max(
				memNeed(m_stack[(1 + SP - m_stack) - sizesOffset - 2], m_stack[(1 + SP - m_stack) - sizesOffset - 3]),
				memNeed(m_stack[(1 + SP - m_stack) - sizesOffset], m_stack[(1 + SP - m_stack) - sizesOffset - 1])
			);
			resetMem();
			onOperation();
			resetIOGas();

			unique_ptr<CallParameters> callParams(new CallParameters());

			callParams->gas = *SP;
			if (inst != Instruction::DELEGATECALL && *(SP-2) > 0)
				callParams->gas += m_schedule->callStipend;
			--SP;

			callParams->codeAddress = asAddress(*SP);
			--SP;

			if (inst == Instruction::DELEGATECALL)
			{
				callParams->apparentValue = _ext.value;
				callParams->valueTransfer = 0;
			}
			else
			{
				callParams->apparentValue = callParams->valueTransfer = *SP;
				--SP;
			}

			unsigned inOff = (unsigned)*SP;
			--SP;
			unsigned inSize = (unsigned)*SP;
			--SP;
			unsigned outOff = (unsigned)*SP;
			--SP;
			unsigned outSize = (unsigned)*SP;
			--SP;

			if (_ext.balance(_ext.myAddress) >= callParams->valueTransfer && _ext.depth < 1024)
			{
				callParams->onOp = _onOp;
				callParams->senderAddress = inst == Instruction::DELEGATECALL ? _ext.caller : _ext.myAddress;
				callParams->receiveAddress = inst == Instruction::CALL ? callParams->codeAddress : _ext.myAddress;
				callParams->data = bytesConstRef(m_mem.data() + inOff, inSize);
				callParams->out = bytesRef(m_mem.data() + outOff, outSize);
				*++SP = _ext.call(*callParams);
			}
			else
				*++SP = 0;

			io_gas += callParams->gas;
			break;
		}

		case Instruction::RETURN:
		{
			newTempSize = memNeed(*SP, *(SP-1));
			resetMem();
			onOperation();
			resetIOGas();

			unsigned b = (unsigned)*SP;
			--SP;
			unsigned s = (unsigned)*SP;
			--SP;
			return bytesConstRef(m_mem.data() + b, s);
		}

		case Instruction::SUICIDE:
		{
			onOperation();
			resetIOGas();

			Address dest = asAddress(*SP);
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
			newTempSize = to_rmword(*SP) + 32;
			resetMem();
			onOperation();
			resetIOGas();

			*SP = (vmword)*(h256 const*)(m_mem.data() + (unsigned)*SP);
			break;
		}

		case Instruction::MSTORE:
		{
			newTempSize = to_rmword(*SP) + 32;
			resetMem();
			onOperation();
			resetIOGas();

			*(h256*)&m_mem[(unsigned)*SP] = (h256)*(SP-1);
			SP -= 2;
			break;
		}

		case Instruction::MSTORE8:
		{
			newTempSize = to_rmword(*SP) + 1;
			resetMem();
			onOperation();
			resetIOGas();

			m_mem[(unsigned)*SP] = (byte)(*(SP-1) & 0xff);
			SP -= 2;
			break;
		}

		case Instruction::SHA3:
		{
			runGas = to_rmword(m_schedule->sha3Gas + (to_uoword(*(SP-1)) + 31) / 32 * m_schedule->sha3WordGas);
			newTempSize = memNeed(*SP, *(SP-1));
			resetMem();
			onOperation();
			resetIOGas();

			unsigned inOff = (unsigned)*SP;
			--SP;
			unsigned inSize = (unsigned)*SP;
			--SP;
			*++SP = (u256)sha3(bytesConstRef(m_mem.data() + inOff, inSize));
			break;
		}

	   case Instruction::LOG0:
			logGasMem(inst);
			onOperation();
			resetIOGas();

			_ext.log({}, bytesConstRef(m_mem.data() + (unsigned)*SP, (unsigned)*(SP-1)));
			SP -= 2;
			break;
		case Instruction::LOG1:

			logGasMem(inst);
			onOperation();
			resetIOGas();

			_ext.log({*(SP-2)}, bytesConstRef(m_mem.data() + (unsigned)*SP, (unsigned)*(SP-1)));
			SP -= 3;
			break;

		case Instruction::LOG2:
			logGasMem(inst);
			onOperation();
			resetIOGas();

			_ext.log({*(SP-2), *(SP-3)}, bytesConstRef(m_mem.data() + (unsigned)*SP, (unsigned)*(SP-1)));
			SP -= 4;
			break;

		case Instruction::LOG3:
			logGasMem(inst);
			onOperation();
			resetIOGas();

			_ext.log({*(SP-2), *(SP-3), *(SP-4)}, bytesConstRef(m_mem.data() + (unsigned)*SP, (unsigned)*(SP-1)));
			SP -= 5;
			break;
		case Instruction::LOG4:
			logGasMem(inst);
			onOperation();
			resetIOGas();

			_ext.log({*(SP-2), *(SP-3), *(SP-4), *(SP-5)}, bytesConstRef(m_mem.data() + (unsigned)*SP, (unsigned)*(SP-1)));
			SP -= 6;
			break;	

		case Instruction::EXP:
		{
			auto expon = *(SP-1);
			runGas = to_rmword(m_schedule->expGas + m_schedule->expByteGas * (32 - (h256(expon).firstBitSet() / 8)));
			resetMem();
			onOperation();
			resetIOGas();

			auto base = *SP;
			--SP;
			*SP = (vmword)boost::multiprecision::powm((bigint)base, (bigint)expon, bigint(1) << 256);
			break;
		}


		//
		// ordinary instructions
		//

		case Instruction::ADD:
			onOperation();
			resetIOGas();

			//pops two items and pushes S[-1] + S[-2] mod 2^256.
			*(SP-1) += *SP;
			--SP;
			break;

		case Instruction::MUL:
			onOperation();
			resetIOGas();

			//pops two items and pushes S[-1] * S[-2] mod 2^256.
			*(SP-1) *= *SP;
			--SP;
			break;

		case Instruction::SUB:
			onOperation();
			resetIOGas();

			*(SP-1) = *SP - *(SP-1);
			--SP;
			break;

		case Instruction::DIV:
			onOperation();
			resetIOGas();

			*(SP-1) = *(SP-1) ? divWorkaround(*SP, *(SP-1)) : 0;
			--SP;
			break;

		case Instruction::SDIV:
			onOperation();
			resetIOGas();

			*(SP-1) = *(SP-1) ? s2u(divWorkaround(u2s(*SP), u2s(*(SP-1)))) : 0;
			--SP;
			break;

		case Instruction::MOD:
			onOperation();
			resetIOGas();

			*(SP-1) = *(SP-1) ? modWorkaround(*SP, *(SP-1)) : 0;
			--SP;
			break;

		case Instruction::SMOD:
			onOperation();
			resetIOGas();

			*(SP-1) = *(SP-1) ? s2u(modWorkaround(u2s(*SP), u2s(*(SP-1)))) : 0;
			--SP;
			break;

		case Instruction::NOT:
			onOperation();
			resetIOGas();

			*SP = ~*SP;
			break;

		case Instruction::LT:
			onOperation();
			resetIOGas();

			*(SP-1) = *SP < *(SP-1) ? 1 : 0;
			--SP;
			break;

		case Instruction::GT:
			onOperation();
			resetIOGas();

			*(SP-1) = *SP > *(SP-1) ? 1 : 0;
			--SP;
			break;

		case Instruction::SLT:
			onOperation();
			resetIOGas();

			*(SP-1) = u2s(*SP) < u2s(*(SP-1)) ? 1 : 0;
			--SP;
			break;

		case Instruction::SGT:
			onOperation();
			resetIOGas();

			*(SP-1) = u2s(*SP) > u2s(*(SP-1)) ? 1 : 0;
			--SP;
			break;

		case Instruction::EQ:
			onOperation();
			resetIOGas();

			*(SP-1) = *SP == *(SP-1) ? 1 : 0;
			--SP;
			break;

		case Instruction::ISZERO:
			onOperation();
			resetIOGas();

			*SP = *SP ? 0 : 1;
			break;

		case Instruction::AND:
			onOperation();
			resetIOGas();

			*(SP-1) = *SP & *(SP-1);
			--SP;
			break;

		case Instruction::OR:
			onOperation();
			resetIOGas();

			*(SP-1) = *SP | *(SP-1);
			--SP;
			break;

		case Instruction::XOR:
			onOperation();
			resetIOGas();

			*(SP-1) = *SP ^ *(SP-1);
			--SP;
			break;

		case Instruction::BYTE:
			onOperation();
			resetIOGas();

			*(SP-1) = *SP < 32 ? (*(SP-1) >> (unsigned)(8 * (31 - *SP))) & 0xff : 0;
			--SP;
			break;

		case Instruction::ADDMOD:
			onOperation();
			resetIOGas();

			*(SP-2) = *(SP-2) ? vmword((to_uoword(*SP) + to_uoword(*(SP-1))) % *(SP-2)) : 0;
			SP -= 2;
			break;

		case Instruction::MULMOD:
			onOperation();
			resetIOGas();

			*(SP-2) = *(SP-2) ? vmword((to_uoword(*SP) * to_uoword(*(SP-1))) % *(SP-2)) : 0;
			SP -= 2;
			break;

		case Instruction::SIGNEXTEND:
			onOperation();
			resetIOGas();

			if (*SP < 31)
			{
				auto testBit = static_cast<unsigned>(*SP) * 8 + 7;
				vmword& number = *(SP-1);
				vmword mask = ((vmword(1) << testBit) - 1);
				if (boost::multiprecision::bit_test(number, testBit))
					number |= ~mask;
				else
					number &= mask;
			}
			--SP;
			break;

		case Instruction::ADDRESS:
			onOperation();
			resetIOGas();

			*++SP = fromAddress(_ext.myAddress);
			break;

		case Instruction::ORIGIN:
			onOperation();
			resetIOGas();

			*++SP = fromAddress(_ext.origin);
			break;

		case Instruction::BALANCE:
		{
			onOperation();
			resetIOGas();

			*SP = _ext.balance(asAddress(*SP));
			break;
		}

		case Instruction::CALLER:
			onOperation();
			resetIOGas();

			*++SP = fromAddress(_ext.caller);
			break;

		case Instruction::CALLVALUE:
			onOperation();
			resetIOGas();

			*++SP = _ext.value;
			break;


		case Instruction::CALLDATALOAD:
		{
			onOperation();
			resetIOGas();

			if (to_uoword(*SP) + 31 < _ext.data.size())
				*SP = (vmword)*(h256 const*)(_ext.data.data() + (size_t)*SP);
			else if (*SP >= _ext.data.size())
				*SP = vmword(0);
			else
			{
				h256 r;
				for (uint64_t i = (unsigned)*SP, e = (unsigned)*SP + (uint64_t)32, j = 0; i < e; ++i, ++j)
					r[j] = i < _ext.data.size() ? _ext.data[i] : 0;
				*SP = (vmword)r;
			}
			break;
		}

		case Instruction::CALLDATASIZE:
			onOperation();
			resetIOGas();

			*++SP = _ext.data.size();
			break;

		case Instruction::CODESIZE:
			onOperation();
			resetIOGas();

			*++SP = _ext.code.size();
			break;

		case Instruction::EXTCODESIZE:
			onOperation();
			resetIOGas();

			*SP = _ext.codeAt(asAddress(*SP)).size();
			break;

		case Instruction::CALLDATACOPY:
			copySize = to_rmword(*(SP-2));
			newTempSize = memNeed(*SP, *(SP-2));
			resetMem();
			onOperation();
			resetIOGas();

			copyDataToMemory(_ext.data);
			break;

		case Instruction::CODECOPY:
			copySize = to_rmword(*(SP-2));
			newTempSize = memNeed(*SP, *(SP-2));
			resetMem();
			onOperation();
			resetIOGas();

			copyDataToMemory(&_ext.code);
			break;

		case Instruction::EXTCODECOPY:
		{
			copySize = to_rmword(*(SP-3));
			newTempSize = memNeed(*(SP-1), *(SP-3));
			resetMem();
			onOperation();
			resetIOGas();

			auto a = asAddress(*SP);
			--SP;
			copyDataToMemory(&_ext.codeAt(a));
			break;
		}

		case Instruction::GASPRICE:
			onOperation();
			resetIOGas();

			*++SP = _ext.gasPrice;
			break;

		case Instruction::BLOCKHASH:
			onOperation();
			resetIOGas();

			*SP = (vmword)_ext.blockHash(*SP);
			break;

		case Instruction::COINBASE:
			onOperation();
			resetIOGas();

			*++SP = (u160)_ext.envInfo().author();
			break;

		case Instruction::TIMESTAMP:
			onOperation();
			resetIOGas();

			*++SP = _ext.envInfo().timestamp();
			break;

		case Instruction::NUMBER:
			onOperation();
			resetIOGas();

			*++SP = _ext.envInfo().number();
			break;

		case Instruction::DIFFICULTY:
			onOperation();
			resetIOGas();

			*++SP = _ext.envInfo().difficulty();
			break;

		case Instruction::GASLIMIT:
			onOperation();
			resetIOGas();

			*++SP = _ext.envInfo().gasLimit();
			break;

		case Instruction::POP:
			onOperation();
			resetIOGas();

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
			resetIOGas();

			int i = (int)inst - (int)Instruction::PUSH1 + 1;
			*++SP = 0;
			for (++PC; i--; ++PC)
				*SP = (*SP << 8) | _ext.getCode(PC);
			continue;
		}

		case Instruction::JUMP:
			onOperation();
			resetIOGas();

			PC = verifyJumpDest(*SP);
			--SP;
			continue;

		case Instruction::JUMPI:
			onOperation();
			resetIOGas();

			if (*(SP-1))
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
			resetIOGas();

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
			resetIOGas();

			auto n = (unsigned)inst - (unsigned)Instruction::SWAP1 + 2;
			auto d = *SP;
			*SP = m_stack[(1 + SP - m_stack) - n];
			m_stack[(1 + SP - m_stack) - n] = d;
			break;
		}

		case Instruction::SLOAD:
			runGas = to_rmword(m_schedule->sloadGas);
			onOperation();
			resetIOGas();

			*SP = _ext.store(*SP);
			break;

		case Instruction::SSTORE:
			if (!_ext.store(*SP) && *(SP-1))
				runGas = to_rmword(m_schedule->sstoreSetGas);
			else if (_ext.store(*SP) && !*(SP-1))
			{
				runGas = to_rmword(m_schedule->sstoreResetGas);
				_ext.sub.refunds += m_schedule->sstoreRefundGas;
			}
			else
				runGas = to_rmword(m_schedule->sstoreResetGas);
			onOperation();
			resetIOGas();
	
			_ext.setStore(*SP, *(SP-1));
			SP -= 2;
			break;

		case Instruction::PC:
			onOperation();
			resetIOGas();

			*++SP = PC;
			break;

		case Instruction::MSIZE:
			onOperation();
			resetIOGas();

			*++SP = m_mem.size();
			break;

		case Instruction::GAS:
			onOperation();
			resetIOGas();

			*++SP = io_gas;
			break;

		case Instruction::JUMPDEST:
			runGas = 1;
			onOperation();
			resetIOGas();
			break;

		default:
			BOOST_THROW_EXCEPTION(BadInstruction());
		}
		
		++PC;
	}

	return bytesConstRef();
}