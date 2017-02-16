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
#include <libdevcore/Exceptions.h>
#include <libethcore/Common.h>
#include <libevmcore/Instruction.h>
#include <libdevcore/SHA3.h>
#include <libethcore/BlockHeader.h>
#include "VMFace.h"

namespace dev
{
namespace eth
{

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


struct InstructionMetric
{
	Tier gasPriceTier;
	int args;
	int ret;
};


/**
 */
class VM: public VMFace
{
public:
	virtual owning_bytes_ref exec(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp) override final;

	bytes const& memory() const { return m_mem; }
	u256s stack() const { assert(m_stack <= m_sp + 1); return u256s(m_stack, m_sp + 1); };

private:

	u256* io_gas = 0;
	uint64_t m_io_gas = 0;
	ExtVMFace* m_ext = 0;
	OnOpFunc m_onOp;

	static std::array<InstructionMetric, 256> c_metrics;
	static void initMetrics();
	static u256 exp256(u256 _base, u256 _exponent);
	void copyCode(int);
	const void* const* c_jumpTable = 0;
	bool m_caseInit = false;
	
	typedef void (VM::*MemFnPtr)();
	MemFnPtr m_bounce = 0;
	MemFnPtr m_onFail = 0;
	uint64_t m_nSteps = 0;
	EVMSchedule const* m_schedule = nullptr;

	// return bytes
	owning_bytes_ref m_output;

	// space for memory
	bytes m_mem;

	// space for code and pointer to data
	bytes m_codeSpace;
	byte* m_code = nullptr;

	// space for stack and pointer to data
	u256 m_stackSpace[1025];
	u256* m_stack = m_stackSpace + 1;
	
#if EVM_JUMPS_AND_SUBS
	// space for return stack and pointer to data
	uint64_t m_returnSpace[1025];
	uint64_t* m_return = m_returnSpace + 1;
#endif

	// constant pool
	u256 m_pool[256];

	// interpreter state
	Instruction m_op;                   // current operator
	uint64_t    m_pc = 0;               // program counter
	u256*       m_sp = m_stack - 1;     // stack pointer
#if EVM_JUMPS_AND_SUBS
	uint64_t*   m_rp = m_return - 1;    // return pointer
#endif

	// metering and memory state
	uint64_t m_runGas = 0;
	uint64_t m_newMemSize = 0;
	uint64_t m_copyMemSize = 0;

	// initialize interpreter
	void initEntry();
	void optimize();

	// interpreter loop & switch
	void interpretCases();

	// interpreter cases that call out
	void caseCreate();
	bool caseCallSetup(CallParameters*, bytesRef& o_output);
	void caseCall();

	void copyDataToMemory(bytesConstRef _data, u256*& m_sp);
	uint64_t memNeed(u256 _offset, u256 _size);

	void throwOutOfGas();
	void throwBadInstruction();
	void throwBadJumpDestination();
	void throwBadStack(unsigned _size, unsigned _n, unsigned _d);

	void reportStackUse();

	std::vector<uint64_t> m_beginSubs;
	std::vector<uint64_t> m_jumpDests;
	int64_t verifyJumpDest(u256 const& _dest, bool _throw = true);

	int poolConstant(const u256&);

	void onOperation();
	void checkStack(unsigned _n, unsigned _d);
	uint64_t gasForMem(u512 _size);
	void updateIOGas();
	void updateGas();
	void updateMem();
	void logGasMem();
	void fetchInstruction();
	
	uint64_t decodeJumpDest(const byte* const _code, uint64_t& _pc);
	uint64_t decodeJumpvDest(const byte* const _code, uint64_t& _pc, u256*& _sp);

	template<class T> uint64_t toUint64(T v)
	{
		// check for overflow
		if (v > 0x7FFFFFFFFFFFFFFF)
			throwOutOfGas();
		uint64_t w = uint64_t(v);
		return w;
	}
	};

}
}
