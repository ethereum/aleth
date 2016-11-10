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


#include <libethereum/ExtVM.h>
#include "VMConfig.h"
#include "VM.h"
using namespace std;
using namespace dev;
using namespace dev::eth;



void VM::copyDataToMemory(bytesConstRef _data, u256*& _sp)
{
	auto offset = static_cast<size_t>(*_sp--);
	s512 bigIndex = *_sp--;
	auto index = static_cast<size_t>(bigIndex);
	auto size = static_cast<size_t>(*_sp--);

	size_t sizeToBeCopied = bigIndex + size > _data.size() ? _data.size() < bigIndex ? 0 : _data.size() - index : size;

	if (sizeToBeCopied > 0)
		std::memcpy(m_mem.data() + offset, _data.data() + index, sizeToBeCopied);
	if (size > sizeToBeCopied)
		std::memset(m_mem.data() + offset + sizeToBeCopied, 0, size - sizeToBeCopied);
}


// consolidate exception throws to avoid spraying boost code all over interpreter

void VM::throwOutOfGas()
{
	BOOST_THROW_EXCEPTION(OutOfGas());
}

void VM::throwBadInstruction()
{
	BOOST_THROW_EXCEPTION(BadInstruction());
}

void VM::throwBadJumpDestination()
{
	BOOST_THROW_EXCEPTION(BadJumpDestination());
}

void VM::throwBadStack(unsigned _size, unsigned _removed, unsigned _added)
{
	if (_size < _removed)
	{
		if (m_onFail)
			(this->*m_onFail)();
		BOOST_THROW_EXCEPTION(StackUnderflow() << RequirementError((bigint)_removed, (bigint)_size));
	}
	if (_size - _removed + _added > 1024)
	{
		if (m_onFail)
			(this->*m_onFail)();
		BOOST_THROW_EXCEPTION(OutOfStack() << RequirementError((bigint)(_added - _removed), (bigint)_size));
	}
}

int64_t VM::verifyJumpDest(u256 const& _dest, bool _throw)
{
	
	// check for overflow
	if (_dest <= 0x7FFFFFFFFFFFFFFF) {

		// check for within bounds and to a jump destination
		// use binary search of array because hashtable collisions are exploitable
		uint64_t pc = uint64_t(_dest);
		if (std::binary_search(m_jumpDests.begin(), m_jumpDests.end(), pc))
			return pc;
	}
	if (_throw)
		throwBadJumpDestination();
	return -1;
}


//
// interpreter cases that call out
//

void VM::caseCreate()
{
	m_bounce = &VM::interpretCases;
	m_newMemSize = memNeed(*(m_sp - 1), *(m_sp - 2));
	m_runGas = toUint64(m_schedule->createGas);
	updateMem();
	ON_OP();
	updateIOGas();

	auto const& endowment = *m_sp--;
	uint64_t initOff = (uint64_t)*m_sp--;
	uint64_t initSize = (uint64_t)*m_sp--;

	if (m_ext->balance(m_ext->myAddress) >= endowment && m_ext->depth < 1024)
	{
		*io_gas = m_io_gas;
		u256 createGas = *io_gas;
		if (!m_schedule->staticCallDepthLimit())
			createGas -= createGas / 64;
		u256 gas = createGas;
		*++m_sp = (u160)m_ext->create(endowment, gas, bytesConstRef(m_mem.data() + initOff, initSize), m_onOp);
		*io_gas -= (createGas - gas);
		m_io_gas = uint64_t(*io_gas);
	}
	else
		*++m_sp = 0;
	++m_pc;
}

void VM::caseCall()
{
	m_bounce = &VM::interpretCases;
	unique_ptr<CallParameters> callParams(new CallParameters());
	if (caseCallSetup(&*callParams))
		*++m_sp = m_ext->call(*callParams);
	else
		*++m_sp = 0;
	m_io_gas += uint64_t(callParams->gas);
	++m_pc;
}

bool VM::caseCallSetup(CallParameters *callParams)
{
	m_runGas = toUint64(m_schedule->callGas);

	if (m_op == Instruction::CALL && !m_ext->exists(asAddress(*(m_sp - 1))))
		m_runGas += toUint64(m_schedule->callNewAccountGas);

	if (m_op != Instruction::DELEGATECALL && *(m_sp - 2) > 0)
		m_runGas += toUint64(m_schedule->callValueTransferGas);

	unsigned sizesOffset = m_op == Instruction::DELEGATECALL ? 3 : 4;
	m_newMemSize = std::max(
		memNeed(m_stack[(1 + m_sp - m_stack) - sizesOffset - 2], m_stack[(1 + m_sp - m_stack) - sizesOffset - 3]),
		memNeed(m_stack[(1 + m_sp - m_stack) - sizesOffset], m_stack[(1 + m_sp - m_stack) - sizesOffset - 1])
	);
	updateMem();
	updateIOGas();

	// "Static" costs already applied. Calculate call gas.
	if (m_schedule->staticCallDepthLimit())
		// With static call depth limit we just charge the provided gas amount.
		callParams->gas = *m_sp;
	else
	{
		// Apply "all but one 64th" rule.
		u256 maxAllowedCallGas = m_io_gas - m_io_gas / 64;
		callParams->gas = std::min(*m_sp, maxAllowedCallGas);
	}

	m_runGas = toUint64(callParams->gas);
	ON_OP();
	updateIOGas();

	if (m_op != Instruction::DELEGATECALL && *(m_sp - 2) > 0)
		callParams->gas += m_schedule->callStipend;
	--m_sp;

	callParams->codeAddress = asAddress(*m_sp);
	--m_sp;

	if (m_op == Instruction::DELEGATECALL)
	{
		callParams->apparentValue = m_ext->value;
		callParams->valueTransfer = 0;
	}
	else
	{
		callParams->apparentValue = callParams->valueTransfer = *m_sp;
		--m_sp;
	}

	uint64_t inOff = (uint64_t)*m_sp--;
	uint64_t inSize = (uint64_t)*m_sp--;
	uint64_t outOff = (uint64_t)*m_sp--;
	uint64_t outSize = (uint64_t)*m_sp--;

	if (m_ext->balance(m_ext->myAddress) >= callParams->valueTransfer && m_ext->depth < 1024)
	{
		callParams->onOp = m_onOp;
		callParams->senderAddress = m_op == Instruction::DELEGATECALL ? m_ext->caller : m_ext->myAddress;
		callParams->receiveAddress = m_op == Instruction::CALL ? callParams->codeAddress : m_ext->myAddress;
		callParams->data = bytesConstRef(m_mem.data() + inOff, inSize);
		callParams->out = bytesRef(m_mem.data() + outOff, outSize);
		return true;
	}
	else
		return false;
}

