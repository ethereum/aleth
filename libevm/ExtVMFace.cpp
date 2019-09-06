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

#include "ExtVMFace.h"

#include <evmc/helpers.h>

namespace dev
{
namespace eth
{
static_assert(sizeof(Address) == sizeof(evmc_address), "Address types size mismatch");
static_assert(alignof(Address) == alignof(evmc_address), "Address types alignment mismatch");
static_assert(sizeof(h256) == sizeof(evmc_uint256be), "Hash types size mismatch");
static_assert(alignof(h256) == alignof(evmc_uint256be), "Hash types alignment mismatch");

bool EvmCHost::account_exists(evmc::address const& _addr) noexcept
{
    return m_extVM.exists(fromEvmC(_addr));
}

evmc::bytes32 EvmCHost::get_storage(evmc::address const& _addr, evmc::bytes32 const& _key) noexcept
{
    (void)_addr;
    assert(fromEvmC(_addr) == m_extVM.myAddress);
    return toEvmC(m_extVM.store(fromEvmC(_key)));
}

evmc_storage_status EvmCHost::set_storage(
    evmc::address const& _addr, evmc::bytes32 const& _key, evmc::bytes32 const& _value) noexcept
{
    (void)_addr;
    assert(fromEvmC(_addr) == m_extVM.myAddress);
    u256 const index = fromEvmC(_key);
    u256 const newValue = fromEvmC(_value);
    u256 const currentValue = m_extVM.store(index);

    if (newValue == currentValue)
        return EVMC_STORAGE_UNCHANGED;

    EVMSchedule const& schedule = m_extVM.evmSchedule();
    auto status = EVMC_STORAGE_MODIFIED;
    u256 const originalValue = m_extVM.originalStorageValue(index);
    if (originalValue == currentValue || !schedule.eip1283Mode)
    {
        if (currentValue == 0)
            status = EVMC_STORAGE_ADDED;
        else if (newValue == 0)
        {
            status = EVMC_STORAGE_DELETED;
            m_extVM.sub.refunds += schedule.sstoreRefundGas;
        }
    }
    else
    {
        status = EVMC_STORAGE_MODIFIED_AGAIN;
        if (originalValue != 0)
        {
            if (currentValue == 0)
                m_extVM.sub.refunds -= schedule.sstoreRefundGas;  // Can go negative.
            if (newValue == 0)
                m_extVM.sub.refunds += schedule.sstoreRefundGas;
        }
        if (originalValue == newValue)
        {
            if (originalValue == 0)
                m_extVM.sub.refunds += schedule.sstoreRefundGas + schedule.sstoreRefundNonzeroGas;
            else
                m_extVM.sub.refunds += schedule.sstoreRefundNonzeroGas;
        }
    }

    m_extVM.setStore(index, newValue);  // Interface uses native endianness

    return status;
}

evmc::uint256be EvmCHost::get_balance(evmc::address const& _addr) noexcept
{
    return toEvmC(m_extVM.balance(fromEvmC(_addr)));
}

size_t EvmCHost::get_code_size(evmc::address const& _addr) noexcept
{
    return m_extVM.codeSizeAt(fromEvmC(_addr));
}

evmc::bytes32 EvmCHost::get_code_hash(evmc::address const& _addr) noexcept
{
    return toEvmC(m_extVM.codeHashAt(fromEvmC(_addr)));
}

size_t EvmCHost::copy_code(
    evmc::address const& _addr, size_t _codeOffset, byte* _bufferData, size_t _bufferSize) noexcept
{
    Address addr = fromEvmC(_addr);
    bytes const& c = m_extVM.codeAt(addr);

    // Handle "big offset" edge case.
    if (_codeOffset >= c.size())
        return 0;

    size_t maxToCopy = c.size() - _codeOffset;
    size_t numToCopy = std::min(maxToCopy, _bufferSize);
    std::copy_n(&c[_codeOffset], numToCopy, _bufferData);
    return numToCopy;
}

void EvmCHost::selfdestruct(evmc::address const& _addr, evmc::address const& _beneficiary) noexcept
{
    (void)_addr;
    assert(fromEvmC(_addr) == m_extVM.myAddress);
    m_extVM.selfdestruct(fromEvmC(_beneficiary));
}


void EvmCHost::emit_log(evmc::address const& _addr, uint8_t const* _data, size_t _dataSize,
    evmc::bytes32 const _topics[], size_t _numTopics) noexcept
{
    (void)_addr;
    assert(fromEvmC(_addr) == m_extVM.myAddress);
    h256 const* pTopics = reinterpret_cast<h256 const*>(_topics);
    m_extVM.log(h256s{pTopics, pTopics + _numTopics}, bytesConstRef{_data, _dataSize});
}

evmc_tx_context EvmCHost::get_tx_context() noexcept
{
    evmc_tx_context result = {};
    result.tx_gas_price = toEvmC(m_extVM.gasPrice);
    result.tx_origin = toEvmC(m_extVM.origin);

    auto const& envInfo = m_extVM.envInfo();
    result.block_coinbase = toEvmC(envInfo.author());
    result.block_number = envInfo.number();
    result.block_timestamp = envInfo.timestamp();
    result.block_gas_limit = static_cast<int64_t>(envInfo.gasLimit());
    result.block_difficulty = toEvmC(envInfo.difficulty());
    result.chain_id = toEvmC(envInfo.chainID());
    return result;
}

evmc::bytes32 EvmCHost::get_block_hash(int64_t _number) noexcept
{
    return toEvmC(m_extVM.blockHash(_number));
}

evmc::result EvmCHost::create(evmc_message const& _msg) noexcept
{
    u256 gas = _msg.gas;
    u256 value = fromEvmC(_msg.value);
    bytesConstRef init = {_msg.input_data, _msg.input_size};
    u256 salt = fromEvmC(_msg.create2_salt);
    Instruction opcode = _msg.kind == EVMC_CREATE ? Instruction::CREATE : Instruction::CREATE2;

    // ExtVM::create takes the sender address from .myAddress.
    assert(fromEvmC(_msg.sender) == m_extVM.myAddress);

    CreateResult result = m_extVM.create(value, gas, init, opcode, salt, {});
    evmc_result evmcResult = {};
    evmcResult.status_code = result.status;
    evmcResult.gas_left = static_cast<int64_t>(gas);

    if (result.status == EVMC_SUCCESS)
        evmcResult.create_address = toEvmC(result.address);
    else
    {
        // Pass the output to the EVM without a copy. The EVM will delete it
        // when finished with it.

        // First assign reference. References are not invalidated when vector
        // of bytes is moved. See `.takeBytes()` below.
        evmcResult.output_data = result.output.data();
        evmcResult.output_size = result.output.size();

        // Place a new vector of bytes containing output in result's reserved memory.
        auto* data = evmc_get_optional_storage(&evmcResult);
        static_assert(sizeof(bytes) <= sizeof(*data), "Vector is too big");
        new (data) bytes(result.output.takeBytes());
        // Set the destructor to delete the vector.
        evmcResult.release = [](evmc_result const* _result) {
            auto* data = evmc_get_const_optional_storage(_result);
            auto& output = reinterpret_cast<bytes const&>(*data);
            // Explicitly call vector's destructor to release its data.
            // This is normal pattern when placement new operator is used.
            output.~bytes();
        };
    }
    return evmc::result{evmcResult};
}

evmc::result EvmCHost::call(evmc_message const& _msg) noexcept
{
    assert(_msg.gas >= 0 && "Invalid gas value");
    assert(_msg.depth == static_cast<int>(m_extVM.depth) + 1);

    // Handle CREATE separately.
    if (_msg.kind == EVMC_CREATE || _msg.kind == EVMC_CREATE2)
        return create(_msg);

    CallParameters params;
    params.gas = _msg.gas;
    params.apparentValue = fromEvmC(_msg.value);
    params.valueTransfer = _msg.kind == EVMC_DELEGATECALL ? 0 : params.apparentValue;
    params.senderAddress = fromEvmC(_msg.sender);
    params.codeAddress = fromEvmC(_msg.destination);
    params.receiveAddress = _msg.kind == EVMC_CALL ? params.codeAddress : m_extVM.myAddress;
    params.data = {_msg.input_data, _msg.input_size};
    params.staticCall = (_msg.flags & EVMC_STATIC) != 0;
    params.onOp = {};

    CallResult result = m_extVM.call(params);
    evmc_result evmcResult = {};
    evmcResult.status_code = result.status;
    evmcResult.gas_left = static_cast<int64_t>(params.gas);

    // Pass the output to the EVM without a copy. The EVM will delete it
    // when finished with it.

    // First assign reference. References are not invalidated when vector
    // of bytes is moved. See `.takeBytes()` below.
    evmcResult.output_data = result.output.data();
    evmcResult.output_size = result.output.size();

    // Place a new vector of bytes containing output in result's reserved memory.
    auto* data = evmc_get_optional_storage(&evmcResult);
    static_assert(sizeof(bytes) <= sizeof(*data), "Vector is too big");
    new (data) bytes(result.output.takeBytes());
    // Set the destructor to delete the vector.
    evmcResult.release = [](evmc_result const* _result) {
        auto* data = evmc_get_const_optional_storage(_result);
        auto& output = reinterpret_cast<bytes const&>(*data);
        // Explicitly call vector's destructor to release its data.
        // This is normal pattern when placement new operator is used.
        output.~bytes();
    };
    return evmc::result{evmcResult};
}

ExtVMFace::ExtVMFace(EnvInfo const& _envInfo, Address _myAddress, Address _caller, Address _origin,
    u256 _value, u256 _gasPrice, bytesConstRef _data, bytes _code, h256 const& _codeHash,
    u256 const& _version, unsigned _depth, bool _isCreate, bool _staticCall)
  : m_envInfo(_envInfo),
    myAddress(_myAddress),
    caller(_caller),
    origin(_origin),
    value(_value),
    gasPrice(_gasPrice),
    data(_data),
    code(std::move(_code)),
    codeHash(_codeHash),
    version(_version),
    depth(_depth),
    isCreate(_isCreate),
    staticCall(_staticCall)
{}

}  // namespace eth
}  // namespace dev
