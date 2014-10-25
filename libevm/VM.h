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
/** @file VM.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <unordered_map>
#include <boost/exception/all.hpp>
#include <libdevcore/Exceptions.h>
#include <libethcore/CommonEth.h>
#include <libevmface/Instruction.h>
#include <libdevcrypto/SHA3.h>
#include <libethcore/BlockInfo.h>
#include "FeeStructure.h"
#include "ExtVMFace.h"

namespace dev
{
namespace eth
{

struct VMException: virtual Exception {};
struct StepsDone: virtual VMException {};
struct BreakPointHit: virtual VMException {};
struct BadInstruction: virtual VMException {};
struct BadJumpDestination: virtual VMException {};
struct OutOfGas: virtual VMException {};
struct StackTooSmall: virtual VMException {};

// Convert from a 256-bit integer stack/memory entry into a 160-bit Address hash.
// Currently we just pull out the right (low-order in BE) 160-bits.
inline Address asAddress(u256 _item)
{
	return right160(h256(_item));
}

inline u256 fromAddress(Address _a)
{
	return (u160)_a;
//	h256 ret;
//	memcpy(&ret, &_a, sizeof(_a));
//	return ret;
}

/**
 */
class VM
{
public:
	/// Construct VM object.
	explicit VM(u256 _gas = 0) { reset(_gas); }

	void reset(u256 _gas = 0);

	template <class Ext>
	bytesConstRef go(Ext& _ext, OnOpFunc const& _onOp = OnOpFunc(), uint64_t _steps = (uint64_t)-1);

	void require(u256 _n) { if (m_stack.size() < _n) BOOST_THROW_EXCEPTION(StackTooSmall() << RequirementNotMetError(_n, m_stack.size())); }
	void requireMem(unsigned _n) { if (m_temp.size() < _n) { m_temp.resize(_n); } }
	u256 gas() const { return m_gas; }
	u256 curPC() const { return m_curPC; }

	bytes const& memory() const { return m_temp; }
	u256s const& stack() const { return m_stack; }

private:
	u256 m_gas = 0;
	u256 m_curPC = 0;
	bytes m_temp;
	u256s m_stack;
};

}

// INLINE:
template <class Ext> dev::bytesConstRef dev::eth::VM::go(Ext& _ext, OnOpFunc const& _onOp, uint64_t _steps)
{
	auto memNeed = [](dev::u256 _offset, dev::u256 _size) { return _size ? _offset + _size : 0; };

	u256 nextPC = m_curPC + 1;
	auto osteps = _steps;
	for (bool stopped = false; !stopped && _steps--; m_curPC = nextPC, nextPC = m_curPC + 1)
	{
		// INSTRUCTION...
		Instruction inst = (Instruction)_ext.getCode(m_curPC);

		// FEES...
		bigint runGas = c_stepGas;
		bigint newTempSize = m_temp.size();
		switch (inst)
		{
		case Instruction::STOP:
			runGas = 0;
			break;

		case Instruction::SUICIDE:
			require(1);
			runGas = 0;
			break;

		case Instruction::SSTORE:
			require(2);
			if (!_ext.store(m_stack.back()) && m_stack[m_stack.size() - 2])
				runGas = c_sstoreGas * 2;
			else if (_ext.store(m_stack.back()) && !m_stack[m_stack.size() - 2])
				runGas = 0;
			else
				runGas = c_sstoreGas;
			break;

		case Instruction::SLOAD:
			require(1);
            runGas = c_sloadGas;
			break;

		// These all operate on memory and therefore potentially expand it:
		case Instruction::MSTORE:
			require(2);
			newTempSize = m_stack.back() + 32;
			break;
		case Instruction::MSTORE8:
			require(2);
			newTempSize = m_stack.back() + 1;
			break;
		case Instruction::MLOAD:
			require(1);
			newTempSize = m_stack.back() + 32;
			break;
		case Instruction::RETURN:
			require(2);
			newTempSize = memNeed(m_stack.back(), m_stack[m_stack.size() - 2]);
			break;
		case Instruction::SHA3:
			require(2);
			runGas = c_sha3Gas;
			newTempSize = memNeed(m_stack.back(), m_stack[m_stack.size() - 2]);
			break;
		case Instruction::CALLDATACOPY:
			require(3);
			newTempSize = memNeed(m_stack.back(), m_stack[m_stack.size() - 3]);
			break;
		case Instruction::CODECOPY:
			require(3);
			newTempSize = memNeed(m_stack.back(), m_stack[m_stack.size() - 3]);
			break;
		case Instruction::EXTCODECOPY:
			require(4);
			newTempSize = memNeed(m_stack[m_stack.size() - 2], m_stack[m_stack.size() - 4]);
			break;
			
		case Instruction::BALANCE:
			require(1);
			runGas = c_balanceGas;
			break;

		case Instruction::CALL:
		case Instruction::CALLCODE:
			require(7);
			runGas = c_callGas + m_stack[m_stack.size() - 1];
			newTempSize = std::max(memNeed(m_stack[m_stack.size() - 6], m_stack[m_stack.size() - 7]), memNeed(m_stack[m_stack.size() - 4], m_stack[m_stack.size() - 5]));
			break;

		case Instruction::CREATE:
		{
			require(3);
			auto inOff = m_stack[m_stack.size() - 2];
			auto inSize = m_stack[m_stack.size() - 3];
			newTempSize = inOff + inSize;
            runGas = c_createGas;
			break;
		}

		case Instruction::PC:
		case Instruction::MSIZE:
		case Instruction::GAS:
		case Instruction::JUMPDEST:
		case Instruction::ADDRESS:
		case Instruction::ORIGIN:
		case Instruction::CALLER:
		case Instruction::CALLVALUE:
		case Instruction::CALLDATASIZE:
		case Instruction::CODESIZE:
		case Instruction::GASPRICE:
		case Instruction::PREVHASH:
		case Instruction::COINBASE:
		case Instruction::TIMESTAMP:
		case Instruction::NUMBER:
		case Instruction::DIFFICULTY:
		case Instruction::GASLIMIT:
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
			break;
		case Instruction::NEG:
		case Instruction::NOT:
		case Instruction::CALLDATALOAD:
		case Instruction::EXTCODESIZE:
		case Instruction::POP:
		case Instruction::JUMP:
			require(1);
			break;
		case Instruction::ADD:
		case Instruction::MUL:
		case Instruction::SUB:
		case Instruction::DIV:
		case Instruction::SDIV:
		case Instruction::MOD:
		case Instruction::SMOD:
		case Instruction::EXP:
		case Instruction::LT:
		case Instruction::GT:
		case Instruction::SLT:
		case Instruction::SGT:
		case Instruction::EQ:
		case Instruction::AND:
		case Instruction::OR:
		case Instruction::XOR:
		case Instruction::BYTE:
		case Instruction::JUMPI:
			require(2);
			break;
		case Instruction::ADDMOD:
		case Instruction::MULMOD:
			require(3);
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
			require(1 + (int)inst - (int)Instruction::DUP1);
			break;
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
			require((int)inst - (int)Instruction::SWAP1 + 2);
			break;
		default:
			BOOST_THROW_EXCEPTION(BadInstruction());
		}

		newTempSize = (newTempSize + 31) / 32 * 32;
		if (newTempSize > m_temp.size())
			runGas += c_memoryGas * (newTempSize - m_temp.size()) / 32;

		if (_onOp)
			_onOp(osteps - _steps - 1, inst, newTempSize > m_temp.size() ? (newTempSize - m_temp.size()) / 32 : bigint(0), runGas, this, &_ext);

		if (m_gas < runGas)
		{
			// Out of gas!
			m_gas = 0;
			BOOST_THROW_EXCEPTION(OutOfGas());
		}

		m_gas = (u256)((bigint)m_gas - runGas);

		if (newTempSize > m_temp.size())
			m_temp.resize((size_t)newTempSize);

		// EXECUTE...
		switch (inst)
		{
		case Instruction::ADD:
			//pops two items and pushes S[-1] + S[-2] mod 2^256.
			m_stack[m_stack.size() - 2] += m_stack.back();
			m_stack.pop_back();
			break;
		case Instruction::MUL:
			//pops two items and pushes S[-1] * S[-2] mod 2^256.
			m_stack[m_stack.size() - 2] *= m_stack.back();
			m_stack.pop_back();
			break;
		case Instruction::SUB:
			m_stack[m_stack.size() - 2] = m_stack.back() - m_stack[m_stack.size() - 2];
			m_stack.pop_back();
			break;
		case Instruction::DIV:
			m_stack[m_stack.size() - 2] = m_stack[m_stack.size() - 2] ? m_stack.back() / m_stack[m_stack.size() - 2] : 0;
			m_stack.pop_back();
			break;
		case Instruction::SDIV:
            m_stack[m_stack.size() - 2] = m_stack[m_stack.size() - 2] ? s2u(u2s(m_stack.back()) / u2s(m_stack[m_stack.size() - 2])) : 0;
			m_stack.pop_back();
			break;
		case Instruction::MOD:
			m_stack[m_stack.size() - 2] = m_stack[m_stack.size() - 2] ? m_stack.back() % m_stack[m_stack.size() - 2] : 0;
			m_stack.pop_back();
			break;
		case Instruction::SMOD:
            m_stack[m_stack.size() - 2] = m_stack[m_stack.size() - 2] ? s2u(u2s(m_stack.back()) % u2s(m_stack[m_stack.size() - 2])) : 0;
			m_stack.pop_back();
			break;
		case Instruction::EXP:
		{
			auto base = m_stack.back();
			auto expon = m_stack[m_stack.size() - 2];
			m_stack.pop_back();
			m_stack.back() = (u256)boost::multiprecision::powm((bigint)base, (bigint)expon, bigint(2) << 256);
			break;
		}
		case Instruction::NEG:
			m_stack.back() = ~(m_stack.back() - 1);
			break;
		case Instruction::LT:
			m_stack[m_stack.size() - 2] = m_stack.back() < m_stack[m_stack.size() - 2] ? 1 : 0;
			m_stack.pop_back();
			break;
		case Instruction::GT:
			m_stack[m_stack.size() - 2] = m_stack.back() > m_stack[m_stack.size() - 2] ? 1 : 0;
			m_stack.pop_back();
			break;
		case Instruction::SLT:
            m_stack[m_stack.size() - 2] = u2s(m_stack.back()) < u2s(m_stack[m_stack.size() - 2]) ? 1 : 0;
			m_stack.pop_back();
			break;
		case Instruction::SGT:
            m_stack[m_stack.size() - 2] = u2s(m_stack.back()) > u2s(m_stack[m_stack.size() - 2]) ? 1 : 0;
			m_stack.pop_back();
			break;
		case Instruction::EQ:
			m_stack[m_stack.size() - 2] = m_stack.back() == m_stack[m_stack.size() - 2] ? 1 : 0;
			m_stack.pop_back();
			break;
		case Instruction::NOT:
			m_stack.back() = m_stack.back() ? 0 : 1;
			break;
		case Instruction::AND:
			m_stack[m_stack.size() - 2] = m_stack.back() & m_stack[m_stack.size() - 2];
			m_stack.pop_back();
			break;
		case Instruction::OR:
			m_stack[m_stack.size() - 2] = m_stack.back() | m_stack[m_stack.size() - 2];
			m_stack.pop_back();
			break;
		case Instruction::XOR:
			m_stack[m_stack.size() - 2] = m_stack.back() ^ m_stack[m_stack.size() - 2];
			m_stack.pop_back();
			break;
		case Instruction::BYTE:
			m_stack[m_stack.size() - 2] = m_stack.back() < 32 ? (m_stack[m_stack.size() - 2] >> (unsigned)(8 * (31 - m_stack.back()))) & 0xff : 0;
			m_stack.pop_back();
			break;
		case Instruction::ADDMOD:
			m_stack[m_stack.size() - 3] = u256((bigint(m_stack.back()) + bigint(m_stack[m_stack.size() - 2])) % m_stack[m_stack.size() - 3]);
			m_stack.pop_back();
			m_stack.pop_back();
			break;
		case Instruction::MULMOD:
			m_stack[m_stack.size() - 3] = u256((bigint(m_stack.back()) * bigint(m_stack[m_stack.size() - 2])) % m_stack[m_stack.size() - 3]);
			m_stack.pop_back();
			m_stack.pop_back();
			break;
		case Instruction::SHA3:
		{
			unsigned inOff = (unsigned)m_stack.back();
			m_stack.pop_back();
			unsigned inSize = (unsigned)m_stack.back();
			m_stack.pop_back();
			m_stack.push_back(sha3(bytesConstRef(m_temp.data() + inOff, inSize)));
			break;
		}
		case Instruction::ADDRESS:
			m_stack.push_back(fromAddress(_ext.myAddress));
			break;
		case Instruction::ORIGIN:
			m_stack.push_back(fromAddress(_ext.origin));
			break;
		case Instruction::BALANCE:
		{
			m_stack.back() = _ext.balance(asAddress(m_stack.back()));
			break;
		}
		case Instruction::CALLER:
			m_stack.push_back(fromAddress(_ext.caller));
			break;
		case Instruction::CALLVALUE:
			m_stack.push_back(_ext.value);
			break;
		case Instruction::CALLDATALOAD:
		{
			if ((unsigned)m_stack.back() + 31 < _ext.data.size())
				m_stack.back() = (u256)*(h256 const*)(_ext.data.data() + (unsigned)m_stack.back());
			else
			{
				h256 r;
				for (unsigned i = (unsigned)m_stack.back(), e = (unsigned)m_stack.back() + 32, j = 0; i < e; ++i, ++j)
					r[j] = i < _ext.data.size() ? _ext.data[i] : 0;
				m_stack.back() = (u256)r;
			}
			break;
		}
		case Instruction::CALLDATASIZE:
			m_stack.push_back(_ext.data.size());
			break;
		case Instruction::CALLDATACOPY:
		{
			unsigned mf = (unsigned)m_stack.back();
			m_stack.pop_back();
			unsigned cf = (unsigned)m_stack.back();
			m_stack.pop_back();
			unsigned l = (unsigned)m_stack.back();
			m_stack.pop_back();
			unsigned el = cf + l > _ext.data.size() ? _ext.data.size() < cf ? 0 : _ext.data.size() - cf : l;
			memcpy(m_temp.data() + mf, _ext.data.data() + cf, el);
			memset(m_temp.data() + mf + el, 0, l - el);
			break;
		}
		case Instruction::CODESIZE:
			m_stack.push_back(_ext.code.size());
			break;
		case Instruction::CODECOPY:
		{
			unsigned mf = (unsigned)m_stack.back();
			m_stack.pop_back();
			unsigned cf = (unsigned)m_stack.back();
			m_stack.pop_back();
			unsigned l = (unsigned)m_stack.back();
			m_stack.pop_back();
			unsigned el = cf + l > _ext.code.size() ? _ext.code.size() < cf ? 0 : _ext.code.size() - cf : l;
			memcpy(m_temp.data() + mf, _ext.code.data() + cf, el);
			memset(m_temp.data() + mf + el, 0, l - el);
			break;
		}
		case Instruction::EXTCODESIZE:
			m_stack.back() = _ext.codeAt(asAddress(m_stack.back())).size();
			break;
		case Instruction::EXTCODECOPY:
		{
			Address a = asAddress(m_stack.back());
			m_stack.pop_back();
			unsigned mf = (unsigned)m_stack.back();
			m_stack.pop_back();
			unsigned cf = (unsigned)m_stack.back();
			m_stack.pop_back();
			unsigned l = (unsigned)m_stack.back();
			m_stack.pop_back();
			unsigned el = cf + l > _ext.codeAt(a).size() ? _ext.codeAt(a).size() < cf ? 0 : _ext.codeAt(a).size() - cf : l;
			memcpy(m_temp.data() + mf, _ext.codeAt(a).data() + cf, el);
			memset(m_temp.data() + mf + el, 0, l - el);
			break;
		}
		case Instruction::GASPRICE:
			m_stack.push_back(_ext.gasPrice);
			break;
		case Instruction::PREVHASH:
			m_stack.push_back(_ext.previousBlock.hash);
			break;
		case Instruction::COINBASE:
			m_stack.push_back((u160)_ext.currentBlock.coinbaseAddress);
			break;
		case Instruction::TIMESTAMP:
			m_stack.push_back(_ext.currentBlock.timestamp);
			break;
		case Instruction::NUMBER:
			m_stack.push_back(_ext.currentBlock.number);
			break;
		case Instruction::DIFFICULTY:
			m_stack.push_back(_ext.currentBlock.difficulty);
			break;
		case Instruction::GASLIMIT:
			m_stack.push_back(1000000);
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
			int i = (int)inst - (int)Instruction::PUSH1 + 1;
			nextPC = m_curPC + 1;
			m_stack.push_back(0);
			for (; i--; nextPC++)
				m_stack.back() = (m_stack.back() << 8) | _ext.getCode(nextPC);
			break;
		}
		case Instruction::POP:
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
			auto n = 1 + (int)inst - (int)Instruction::DUP1;
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
			unsigned n = (int)inst - (int)Instruction::SWAP1 + 2;
			auto d = m_stack.back();
			m_stack.back() = m_stack[m_stack.size() - n];
			m_stack[m_stack.size() - n] = d;
			break;
		}
		case Instruction::MLOAD:
		{
			m_stack.back() = (u256)*(h256 const*)(m_temp.data() + (unsigned)m_stack.back());
			break;
		}
		case Instruction::MSTORE:
		{
			*(h256*)&m_temp[(unsigned)m_stack.back()] = (h256)m_stack[m_stack.size() - 2];
			m_stack.pop_back();
			m_stack.pop_back();
			break;
		}
		case Instruction::MSTORE8:
		{
			m_temp[(unsigned)m_stack.back()] = (byte)(m_stack[m_stack.size() - 2] & 0xff);
			m_stack.pop_back();
			m_stack.pop_back();
			break;
		}
		case Instruction::SLOAD:
			m_stack.back() = _ext.store(m_stack.back());
			break;
		case Instruction::SSTORE:
			_ext.setStore(m_stack.back(), m_stack[m_stack.size() - 2]);
			m_stack.pop_back();
			m_stack.pop_back();
			break;
		case Instruction::JUMP:
			nextPC = m_stack.back();
			if (nextPC && (Instruction)_ext.getCode(nextPC - 1) != Instruction::JUMPDEST)
				BOOST_THROW_EXCEPTION(BadJumpDestination());
			m_stack.pop_back();
			break;
		case Instruction::JUMPI:
			if (m_stack[m_stack.size() - 2])
			{
				nextPC = m_stack.back();
				if (nextPC && (Instruction)_ext.getCode(nextPC - 1) != Instruction::JUMPDEST)
					BOOST_THROW_EXCEPTION(BadJumpDestination());
			}
			m_stack.pop_back();
			m_stack.pop_back();
			break;
		case Instruction::PC:
			m_stack.push_back(m_curPC);
			break;
		case Instruction::MSIZE:
			m_stack.push_back(m_temp.size());
			break;
		case Instruction::GAS:
			m_stack.push_back(m_gas);
			break;
		case Instruction::JUMPDEST:
			break;
		case Instruction::CREATE:
		{
			u256 endowment = m_stack.back();
			m_stack.pop_back();
			unsigned initOff = (unsigned)m_stack.back();
			m_stack.pop_back();
			unsigned initSize = (unsigned)m_stack.back();
			m_stack.pop_back();

			if (_ext.balance(_ext.myAddress) >= endowment)
			{
				if (_ext.depth == 1024)
					BOOST_THROW_EXCEPTION(OutOfGas());
				_ext.subBalance(endowment);
				m_stack.push_back((u160)_ext.create(endowment, &m_gas, bytesConstRef(m_temp.data() + initOff, initSize), _onOp));
			}
			else
				m_stack.push_back(0);
			break;
		}
		case Instruction::CALL:
		case Instruction::CALLCODE:
		{
			u256 gas = m_stack.back();
			m_stack.pop_back();
			Address receiveAddress = asAddress(m_stack.back());
			m_stack.pop_back();
			u256 value = m_stack.back();
			m_stack.pop_back();

			unsigned inOff = (unsigned)m_stack.back();
			m_stack.pop_back();
			unsigned inSize = (unsigned)m_stack.back();
			m_stack.pop_back();
			unsigned outOff = (unsigned)m_stack.back();
			m_stack.pop_back();
			unsigned outSize = (unsigned)m_stack.back();
			m_stack.pop_back();

			if (_ext.balance(_ext.myAddress) >= value)
			{
				if (_ext.depth == 1024)
					BOOST_THROW_EXCEPTION(OutOfGas());
				_ext.subBalance(value);
				m_stack.push_back(_ext.call(inst == Instruction::CALL ? receiveAddress : _ext.myAddress, value, bytesConstRef(m_temp.data() + inOff, inSize), &gas, bytesRef(m_temp.data() + outOff, outSize), _onOp, Address(), receiveAddress));
			}
			else
				m_stack.push_back(0);

			m_gas += gas;
			break;
		}
		case Instruction::RETURN:
		{
			unsigned b = (unsigned)m_stack.back();
			m_stack.pop_back();
			unsigned s = (unsigned)m_stack.back();
			m_stack.pop_back();

			return bytesConstRef(m_temp.data() + b, s);
		}
		case Instruction::SUICIDE:
		{
			Address dest = asAddress(m_stack.back());
			_ext.suicide(dest);
			// ...follow through to...
		}
		case Instruction::STOP:
			return bytesConstRef();
		}
	}
	if (_steps == (uint64_t)-1)
		BOOST_THROW_EXCEPTION(StepsDone());
	return bytesConstRef();
}
}
