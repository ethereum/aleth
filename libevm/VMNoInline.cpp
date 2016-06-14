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


// Executive swallows exceptions in some circumstances
//#undef BOOST_THROW_EXCEPTION
//#define BOOST_THROW_EXCEPTION(X) ((cerr << "VM EXCEPTION " << boost::diagnostic_information(X) << endl), abort())

// consolidate exception throws to avoid spraying boost code all over interpreter
void dev::eth::throwVMException(VMException x) {
	BOOST_THROW_EXCEPTION(x);
}


array<InstructionMetric, 256> VM::metrics()
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


void VM::makeJumpDestTable(ExtVMFace& _ext)
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


void VM::copyDataToMemory(bytesConstRef _data, u256*& SP)
{
	auto offset = static_cast<size_t>(*SP--);
	soword bigIndex = *SP--;
	auto index = static_cast<size_t>(bigIndex);
	auto size = static_cast<size_t>(*SP--);

	size_t sizeToBeCopied = bigIndex + size > _data.size() ? _data.size() < bigIndex ? 0 : _data.size() - index : size;

	if (sizeToBeCopied > 0)
		std::memcpy(m_mem.data() + offset, _data.data() + index, sizeToBeCopied);
	if (size > sizeToBeCopied)
		std::memset(m_mem.data() + offset + sizeToBeCopied, 0, size - sizeToBeCopied);
}
