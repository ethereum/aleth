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

namespace dev
{
namespace eth
{
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

void VM::throwDisallowedStateChange()
{
    BOOST_THROW_EXCEPTION(DisallowedStateChange());
}

// throwBadStack is called from fetchInstruction() -> adjustStack()
// its the only exception that can happen before ON_OP() log is done for an opcode case in VM.cpp
// so the call to m_onFail is needed here
void VM::throwBadStack(unsigned _removed, unsigned _added)
{
    bigint size = m_stackEnd - m_SPP;
    if (size < _removed)
        BOOST_THROW_EXCEPTION(StackUnderflow() << RequirementError((bigint)_removed, size));
    else
        BOOST_THROW_EXCEPTION(OutOfStack() << RequirementError((bigint)(_added - _removed), size));
}

void VM::throwRevertInstruction(owning_bytes_ref&& _output)
{
    // We can't use BOOST_THROW_EXCEPTION here because it makes a copy of exception inside and
    // RevertInstruction has no copy constructor
    throw RevertInstruction(std::move(_output));
}

void VM::throwBufferOverrun(bigint const& _endOfAccess)
{
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
    m_runGas = VMSchedule::createGas;

    // Collect arguments.
    u256 endowment = m_SP[0];
    u256 salt;
    u256 initOff;
    u256 initSize;

    if (m_OP == Instruction::CREATE)
    {
        initOff = m_SP[1];
        initSize = m_SP[2];
    }
    else
    {
        salt = m_SP[1];
        initOff = m_SP[2];
        initSize = m_SP[3];
    }

    updateMem(memNeed(initOff, initSize));
    updateIOGas();

    // Clear the return data buffer. This will not free the memory.
    m_returnData.clear();

    evmc_uint256be rawBalance;
    m_context->fn_table->get_balance(&rawBalance, m_context, &m_message->destination);
    u256 balance = fromEvmC(rawBalance);
    if (balance >= endowment && m_message->depth < 1024)
    {
        evmc_message msg = {};
        msg.gas = m_io_gas;
        if (m_rev >= EVMC_TANGERINE_WHISTLE)
            msg.gas -= msg.gas / 64;

        // Get init code. Casts are safe because the memory cost has been paid.
        auto off = static_cast<size_t>(initOff);
        auto size = static_cast<size_t>(initSize);

        msg.input_data = &m_mem[off];
        msg.input_size = size;
        msg.sender = m_message->destination;
        msg.depth = m_message->depth + 1;
        msg.kind = EVMC_CREATE;  // FIXME: In EVM-C move the kind to the top.
        msg.value = toEvmC(endowment);

        evmc_result result;
        m_context->fn_table->call(&result, m_context, &msg);

        if (result.status_code == EVMC_SUCCESS)
            m_SPP[0] = fromAddress(fromEvmC(result.create_address));
        else
            m_SPP[0] = 0;
        m_returnData.assign(result.output_data, result.output_data + result.output_size);

        m_io_gas -= (msg.gas - result.gas_left);

        if (result.release)
            result.release(&result);
    }
    else
        m_SPP[0] = 0;
    ++m_PC;
}

void VM::caseCall()
{
    m_bounce = &VM::interpretCases;

    evmc_message msg = {};

    // Clear the return data buffer. This will not free the memory.
    m_returnData.clear();

    bytesRef output;
    if (caseCallSetup(msg, output))
    {
        evmc_result result;
        m_context->fn_table->call(&result, m_context, &msg);

        m_returnData.assign(result.output_data, result.output_data + result.output_size);
        bytesConstRef{&m_returnData}.copyTo(output);

        m_SPP[0] = result.status_code == EVMC_SUCCESS ? 1 : 0;
        m_io_gas += result.gas_left;

        if (result.release)
            result.release(&result);
    }
    else
    {
        m_SPP[0] = 0;
        m_io_gas += msg.gas;
    }
    ++m_PC;
}

bool VM::caseCallSetup(evmc_message& o_msg, bytesRef& o_output)
{
    m_runGas = m_rev >= EVMC_TANGERINE_WHISTLE ? 700 : 40;

    switch (m_OP)
    {
    case Instruction::CALL:
    case Instruction::STATICCALL:
    default:
        o_msg.kind = EVMC_CALL;
        break;
    case Instruction::CALLCODE:
        o_msg.kind = EVMC_CALLCODE;
        break;
    case Instruction::DELEGATECALL:
        o_msg.kind = EVMC_DELEGATECALL;
        break;
    }

    if (m_OP == Instruction::STATICCALL || m_message->flags & EVMC_STATIC)
        o_msg.flags = EVMC_STATIC;

    bool const haveValueArg = m_OP == Instruction::CALL || m_OP == Instruction::CALLCODE;

    evmc_address destination = toEvmC(asAddress(m_SP[1]));
    int destinationExists = m_context->fn_table->account_exists(m_context, &destination);

    if (m_OP == Instruction::CALL && !destinationExists)
    {
        if (m_SP[2] > 0 || m_rev < EVMC_SPURIOUS_DRAGON)
            m_runGas += VMSchedule::callNewAccount;
    }

    if (haveValueArg && m_SP[2] > 0)
        m_runGas += VMSchedule::valueTransferGas;

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
    u256 callGas = m_SP[0];
    if (m_rev >= EVMC_TANGERINE_WHISTLE)
    {
        // Apply "all but one 64th" rule.
        u256 maxAllowedCallGas = m_io_gas - m_io_gas / 64;
        callGas = std::min(callGas, maxAllowedCallGas);
    }

    o_msg.gas = toInt63(callGas);
    m_runGas = o_msg.gas;
    updateIOGas();

    o_msg.destination = destination;
    o_msg.sender = m_message->destination;
    o_msg.input_data = m_mem.data() + size_t(inputOffset);
    o_msg.input_size = size_t(inputSize);

    bool balanceOk = true;
    if (haveValueArg)
    {
        u256 value = m_SP[2];
        if (value > 0)
        {
            o_msg.value = toEvmC(m_SP[2]);
            o_msg.gas += VMSchedule::callStipend;
            {
                evmc_uint256be rawBalance;
                m_context->fn_table->get_balance(&rawBalance, m_context, &m_message->destination);
                u256 balance = fromEvmC(rawBalance);
                balanceOk = balance >= value;
            }
        }
    }
    else if (m_OP == Instruction::DELEGATECALL)
    {
        o_msg.sender = m_message->sender;
        o_msg.value = m_message->value;
    }

    if (balanceOk && m_message->depth < 1024)
    {
        size_t outOff = size_t(outputOffset);
        size_t outSize = size_t(outputSize);
        o_output = bytesRef(m_mem.data() + outOff, outSize);
        return true;
    }
    return false;
}
}
}
