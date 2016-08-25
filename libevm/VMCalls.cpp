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


#include "VM.h"
#include <libethereum/ExtVM.h>
using namespace std;
using namespace dev;
using namespace dev::eth;



void VM::copyDataToMemory(bytesConstRef _data, u256*& SP)
{
	auto offset = static_cast<size_t>(*SP--);
	s512 bigIndex = *SP--;
	auto index = static_cast<size_t>(bigIndex);
	auto size = static_cast<size_t>(*SP--);

	size_t sizeToBeCopied = bigIndex + size > _data.size() ? _data.size() < bigIndex ? 0 : _data.size() - index : size;

	if (sizeToBeCopied > 0)
		std::memcpy(m_mem.data() + offset, _data.data() + index, sizeToBeCopied);
	if (size > sizeToBeCopied)
		std::memset(m_mem.data() + offset + sizeToBeCopied, 0, size - sizeToBeCopied);
}


// Executive swallows exceptions in some circumstances
//#undef BOOST_THROW_EXCEPTION
//#define BOOST_THROW_EXCEPTION(X) ((cerr << "VM EXCEPTION " << boost::diagnostic_information(X) << endl), abort())

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

void VM::throwBadStack(unsigned _size, unsigned _n, unsigned _d)
{
	if (_size < _n)
	{
		if (m_onFail)
			(this->*m_onFail)();
		BOOST_THROW_EXCEPTION(StackUnderflow() << RequirementError((bigint)_n, (bigint)_size));
	}
	if (_size - _n + _d > 1024)
	{
		if (m_onFail)
			(this->*m_onFail)();
		BOOST_THROW_EXCEPTION(OutOfStack() << RequirementError((bigint)(_d - _n), (bigint)_size));
	}
}

uint64_t VM::verifyJumpDest(u256 const& _dest)
{
	
	// check for overflow
	if (_dest > 0x7FFFFFFFFFFFFFFF)
		throwBadJumpDestination();

	// check for within bounds and to a jump destination
	// use binary search of array because hashtable collisions are exploitable
	uint64_t pc = uint64_t(_dest);
	if (!std::binary_search(m_jumpDests.begin(), m_jumpDests.end(), pc))
		throwBadJumpDestination();

	return pc;
}


//
// interpreter cases that call out
//

void VM::caseCreate()
{
	m_bounce = &VM::interpretCases;
	m_newMemSize = memNeed(*(SP - 1), *(SP - 2));
	m_runGas = toUint64(m_schedule->createGas);
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
	m_runGas = toUint64(u512(*SP) + m_schedule->callGas);

	if (INST == Instruction::CALL && !m_ext->exists(asAddress(*(SP - 1))))
		m_runGas += toUint64(m_schedule->callNewAccountGas);

	if (INST != Instruction::DELEGATECALL && *(SP - 2) > 0)
		m_runGas += toUint64(m_schedule->callValueTransferGas);

	unsigned sizesOffset = INST == Instruction::DELEGATECALL ? 3 : 4;
	m_newMemSize = std::max(
		memNeed(mStack[(1 + SP - mStack) - sizesOffset - 2], mStack[(1 + SP - mStack) - sizesOffset - 3]),
		memNeed(mStack[(1 + SP - mStack) - sizesOffset], mStack[(1 + SP - mStack) - sizesOffset - 1])
	);
	updateMem();
	onOperation();
	updateIOGas();

	callParams->gas = *SP;
	if (INST != Instruction::DELEGATECALL && *(SP - 2) > 0)
		callParams->gas += m_schedule->callStipend;
	--SP;

	callParams->codeAddress = asAddress(*SP);
	--SP;

	if (INST == Instruction::DELEGATECALL)
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
		callParams->senderAddress = INST == Instruction::DELEGATECALL ? m_ext->caller : m_ext->myAddress;
		callParams->receiveAddress = INST == Instruction::CALL ? callParams->codeAddress : m_ext->myAddress;
		callParams->data = bytesConstRef(m_mem.data() + inOff, inSize);
		callParams->out = bytesRef(m_mem.data() + outOff, outSize);
		return true;
	}
	else
		return false;
}

