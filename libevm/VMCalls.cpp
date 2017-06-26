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



void VM::copyDataToMemory(bytesConstRef _data, u256*_sp)
{
	auto offset = static_cast<size_t>(_sp[0]);
	s512 bigIndex = _sp[1];
	auto index = static_cast<size_t>(bigIndex);
	auto size = static_cast<size_t>(_sp[2]);

	size_t sizeToBeCopied = bigIndex + size > _data.size() ? _data.size() < bigIndex ? 0 : _data.size() - index : size;

	if (sizeToBeCopied > 0)
		std::memcpy(m_mem.data() + offset, _data.data() + index, sizeToBeCopied);
	if (size > sizeToBeCopied)
		std::memset(m_mem.data() + offset + sizeToBeCopied, 0, size - sizeToBeCopied);
}


// consolidate exception throws to avoid spraying boost code all over interpreter

void VM::throwOutOfGas()
{
	if (m_onFail)
		(this->*m_onFail)();
	BOOST_THROW_EXCEPTION(OutOfGas());
}

void VM::throwBadInstruction()
{
	if (m_onFail)
		(this->*m_onFail)();
	BOOST_THROW_EXCEPTION(BadInstruction());
}

void VM::throwBadJumpDestination()
{
	if (m_onFail)
		(this->*m_onFail)();
	BOOST_THROW_EXCEPTION(BadJumpDestination());
}

void VM::throwDisallowedStateChange()
{
	if (m_onFail)
		(this->*m_onFail)();
	BOOST_THROW_EXCEPTION(DisallowedStateChange());
}

void VM::throwBadStack(unsigned _removed, unsigned _added)
{
	bigint size = m_stackEnd - m_SPP;
	if (size < _removed)
	{
		if (m_onFail)
			(this->*m_onFail)();
		BOOST_THROW_EXCEPTION(StackUnderflow() << RequirementError((bigint)_removed, size));
	}
	else
	{
		if (m_onFail)
			(this->*m_onFail)();
		BOOST_THROW_EXCEPTION(OutOfStack() << RequirementError((bigint)(_added - _removed), size));
	}
}

void VM::throwRevertInstruction(owning_bytes_ref&& _output)
{
	// We can't use BOOST_THROW_EXCEPTION here because it makes a copy of exception inside and RevertInstruction has no copy constructor 
	throw RevertInstruction(move(_output));
}

void VM::throwBufferOverrun(bigint const& _endOfAccess)
{
	if (m_onFail)
		(this->*m_onFail)();
	BOOST_THROW_EXCEPTION(BufferOverrun() << RequirementError(_endOfAccess, bigint(m_returnData.size())));
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
	m_runGas = toInt63(m_schedule->createGas);
	updateMem(memNeed(m_SP[1], m_SP[2]));
	ON_OP();
	updateIOGas();

	auto const& endowment = m_SP[0];
	uint64_t initOff;
	uint64_t initSize;
	u256 salt;
	if (m_OP == Instruction::CREATE)
	{
		initOff = (uint64_t)m_SP[1];
		initSize = (uint64_t)m_SP[2];
	}
	else
	{
		salt = m_SP[1];
		initOff = (uint64_t)m_SP[2];
		initSize = (uint64_t)m_SP[3];
	}

	// Clear the return data buffer. This will not free the memory.
	m_returnData.clear();

	if (m_ext->balance(m_ext->myAddress) >= endowment && m_ext->depth < 1024)
	{
		*m_io_gas_p = m_io_gas;
		u256 createGas = *m_io_gas_p;
		if (!m_schedule->staticCallDepthLimit())
			createGas -= createGas / 64;
		u256 gas = createGas;
		h160 addr;
		owning_bytes_ref output;
		std::tie(addr, output) = m_ext->create(endowment, gas, bytesConstRef(m_mem.data() + initOff, initSize), m_OP, salt, m_onOp);
		m_SPP[0] = (u160)addr;
		m_returnData = output.toBytes();

		*m_io_gas_p -= (createGas - gas);
		m_io_gas = uint64_t(*m_io_gas_p);
	}
	else
		m_SPP[0] = 0;
	++m_PC;
}

void VM::caseCall()
{
	m_bounce = &VM::interpretCases;

	// TODO: Please check if that does not actually increases the stack size.
	//       That was the case before.
	unique_ptr<CallParameters> callParams(new CallParameters());

	// Clear the return data buffer. This will not free the memory.
	m_returnData.clear();

	bytesRef output;
	if (caseCallSetup(callParams.get(), output))
	{
		bool success = false;
		owning_bytes_ref outputRef;
		std::tie(success, outputRef) = m_ext->call(*callParams);
		outputRef.copyTo(output);

		// Here we have 2 options:
		// 1. Keep the whole returned memory buffer (owning_bytes_ref):
		//    higher memory footprint, no memory copy.
		// 2. Copy only the return data from the returned memory buffer:
		//    minimal memory footprint, additional memory copy.
		// Option 2 used:
		m_returnData = outputRef.toBytes();

		m_SPP[0] = success ? 1 : 0;
	}
	else
		m_SPP[0] = 0;
	m_io_gas += uint64_t(callParams->gas);
	++m_PC;
}

bool VM::caseCallSetup(CallParameters *callParams, bytesRef& o_output)
{
	// Make sure the params were properly initialized.
	assert(callParams->valueTransfer == 0);
	assert(callParams->apparentValue == 0);

	m_runGas = toInt63(m_schedule->callGas);

	callParams->staticCall = (m_OP == Instruction::STATICCALL || m_ext->staticCall);

	bool const haveValueArg = m_OP == Instruction::CALL || m_OP == Instruction::CALLCODE;

	Address destinationAddr = asAddress(m_SP[1]);
	if (m_OP == Instruction::CALL && !m_ext->exists(destinationAddr))
		if (m_SP[2] > 0 || m_schedule->zeroValueTransferChargesNewAccountGas())
			m_runGas += toInt63(m_schedule->callNewAccountGas);

	if (haveValueArg && m_SP[2] > 0)
		m_runGas += toInt63(m_schedule->callValueTransferGas);

	size_t const sizesOffset = haveValueArg ? 3 : 2;
	u256 inputOffset  = m_SP[sizesOffset];
	u256 inputSize    = m_SP[sizesOffset + 1];
	u256 outputOffset = m_SP[sizesOffset + 2];
	u256 outputSize   = m_SP[sizesOffset + 3];
	uint64_t inputMemNeed = memNeed(inputOffset, inputSize);
	uint64_t outputMemNeed = memNeed(outputOffset, outputSize);

	m_newMemSize = std::max(inputMemNeed, outputMemNeed);
	updateMem(m_newMemSize);
	updateIOGas();

	// "Static" costs already applied. Calculate call gas.
	if (m_schedule->staticCallDepthLimit())
	{
		// With static call depth limit we just charge the provided gas amount.
		callParams->gas = m_SP[0];
	}
	else
	{
		// Apply "all but one 64th" rule.
		u256 maxAllowedCallGas = m_io_gas - m_io_gas / 64;
		callParams->gas = std::min(m_SP[0], maxAllowedCallGas);
	}

	m_runGas = toInt63(callParams->gas);
	ON_OP();
	updateIOGas();

	if (haveValueArg && m_SP[2] > 0)
		callParams->gas += m_schedule->callStipend;

	callParams->codeAddress = destinationAddr;

	if (haveValueArg)
	{
		callParams->valueTransfer = m_SP[2];
		callParams->apparentValue = m_SP[2];
	}
	else if (m_OP == Instruction::DELEGATECALL)
		// Forward VALUE.
		callParams->apparentValue = m_ext->value;

	uint64_t inOff = (uint64_t)inputOffset;
	uint64_t inSize = (uint64_t)inputSize;
	uint64_t outOff = (uint64_t)outputOffset;
	uint64_t outSize = (uint64_t)outputSize;

	if (m_ext->balance(m_ext->myAddress) >= callParams->valueTransfer && m_ext->depth < 1024)
	{
		callParams->onOp = m_onOp;
		callParams->senderAddress = m_OP == Instruction::DELEGATECALL ? m_ext->caller : m_ext->myAddress;
		callParams->receiveAddress = (m_OP == Instruction::CALL || m_OP == Instruction::STATICCALL) ? callParams->codeAddress : m_ext->myAddress;
		callParams->data = bytesConstRef(m_mem.data() + inOff, inSize);
		o_output = bytesRef(m_mem.data() + outOff, outSize);
		return true;
	}
	return false;
}

