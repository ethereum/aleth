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


struct InstructionMetric
{
	int gasPriceTier;
	int args;
	int ret;
};


/**
 */
class VM: public VMFace
{
public:
	virtual bytesConstRef execImpl(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp) override final;

	bytes const& memory() const { return m_mem; }
	u256s stack() const { assert(m_stack <= m_SP + 1); return u256s(m_stack, m_SP + 1); };

	VM(): m_stack_vector(1025), m_stack(m_stack_vector.data() + 1) {};

private:

	u256* m_io_gas = 0;
	ExtVMFace* m_ext = 0;
	OnOpFunc const* m_onOp = 0;

	static std::array<InstructionMetric, 256> c_metrics;
	static void initMetrics();

	void makeJumpDestTable(ExtVMFace& _ext);
	uint64_t verifyJumpDest(u256 const& _dest);
	void copyDataToMemory(bytesConstRef _data, u256*& m_SP);
	uint64_t memNeed(u256 _offset, u256 _size);
	void throwOutOfGas();
	void throwBadInstruction();
	void throwBadJumpDestination();
	void throwBadStack(unsigned _size, unsigned _n, unsigned _d);
	void reportStackUse();

	std::unordered_set<uint64_t> m_jumpDests;

	typedef void (VM::*MemFnPtr)();
	MemFnPtr m_bounce = 0;
	MemFnPtr m_onFail = 0;
	uint64_t m_nSteps = 0;
	EVMSchedule const* m_schedule = nullptr;

	// return bytes
	bytesConstRef m_bytes = bytesConstRef();

	// space for memory
	bytes m_mem;

	// space for stack
	u256s m_stack_vector;
	u256* m_stack;

	// interpreter state
	uint64_t m_PC = 0;
	u256* m_SP = m_stack - 1;
	Instruction m_inst;

	// metering and memory state
	uint64_t m_runGas = 0;
	uint64_t m_newMemSize = 0;
	uint64_t m_copyMemSize = 0;

	void onOperation();
	void checkStack(unsigned _n, unsigned _d);
	uint64_t gasForMem(u512 _size);
	void updateIOGas();
	void updateGas();
	void updateMem();
	void logGasMem(Instruction m_inst);

	// interpreter loop & switch
	void interpretCases();

	// interpreter cases that call out
	void caseCreate();
	bool caseCallSetup(CallParameters*);
	void caseCall();

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
