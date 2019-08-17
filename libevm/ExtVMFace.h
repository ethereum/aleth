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

#pragma once

#include "Instruction.h"

#include <libdevcore/Common.h>
#include <libdevcore/CommonData.h>
#include <libdevcore/SHA3.h>
#include <libethcore/BlockHeader.h>
#include <libethcore/ChainOperationParams.h>
#include <libethcore/Common.h>
#include <libethcore/LogEntry.h>

#include <evmc/evmc.hpp>

#include <boost/optional.hpp>
#include <functional>
#include <set>

namespace dev
{
namespace eth
{
/// Reference to a slice of buffer that also owns the buffer.
///
/// This is extension to the concept C++ STL library names as array_view
/// (also known as gsl::span, array_ref, here vector_ref) -- reference to
/// continuous non-modifiable memory. The extension makes the object also owning
/// the referenced buffer.
///
/// This type is used by VMs to return output coming from RETURN instruction.
/// To avoid memory copy, a VM returns its whole memory + the information what
/// part of this memory is actually the output. This simplifies the VM design,
/// because there are multiple options how the output will be used (can be
/// ignored, part of it copied, or all of it copied). The decision what to do
/// with it was moved out of VM interface making VMs "stateless".
///
/// The type is movable, but not copyable. Default constructor available.
class owning_bytes_ref : public vector_ref<byte const>
{
public:
    owning_bytes_ref() = default;

    /// @param _bytes  The buffer.
    /// @param _begin  The index of the first referenced byte.
    /// @param _size   The number of referenced bytes.
    owning_bytes_ref(bytes&& _bytes, size_t _begin, size_t _size) : m_bytes(std::move(_bytes))
    {
        // Set the reference *after* the buffer is moved to avoid
        // pointer invalidation.
        retarget(&m_bytes[_begin], _size);
    }

    owning_bytes_ref(owning_bytes_ref const&) = delete;
    owning_bytes_ref(owning_bytes_ref&&) = default;
    owning_bytes_ref& operator=(owning_bytes_ref const&) = delete;
    owning_bytes_ref& operator=(owning_bytes_ref&&) = default;

    /// Moves the bytes vector out of here. The object cannot be used any more.
    bytes&& takeBytes()
    {
        reset();  // Reset reference just in case.
        return std::move(m_bytes);
    }

private:
    bytes m_bytes;
};

struct SubState
{
    std::set<Address> suicides;  ///< Any accounts that have suicided.
    LogEntries logs;             ///< Any logs.
    int64_t refunds = 0;         ///< Refund counter for storage changes.

    SubState& operator+=(SubState const& _s)
    {
        suicides += _s.suicides;
        refunds += _s.refunds;
        logs += _s.logs;
        return *this;
    }

    void clear()
    {
        suicides.clear();
        logs.clear();
        refunds = 0;
    }
};

class ExtVMFace;
class LastBlockHashesFace;
class VMFace;

using OnOpFunc = std::function<void(uint64_t /*steps*/, uint64_t /* PC */, Instruction /*instr*/,
    bigint /*newMemSize*/, bigint /*gasCost*/, bigint /*gas*/, VMFace const*, ExtVMFace const*)>;

struct CallParameters
{
    CallParameters() = default;
    CallParameters(Address _senderAddress, Address _codeAddress, Address _receiveAddress,
        u256 _valueTransfer, u256 _apparentValue, u256 _gas, bytesConstRef _data,
        OnOpFunc _onOpFunc)
      : senderAddress(_senderAddress),
        codeAddress(_codeAddress),
        receiveAddress(_receiveAddress),
        valueTransfer(_valueTransfer),
        apparentValue(_apparentValue),
        gas(_gas),
        data(_data),
        onOp(_onOpFunc)
    {}
    Address senderAddress;
    Address codeAddress;
    Address receiveAddress;
    u256 valueTransfer;
    u256 apparentValue;
    u256 gas;
    bytesConstRef data;
    bool staticCall = false;
    OnOpFunc onOp;
};

class EnvInfo
{
public:
    EnvInfo(BlockHeader const& _current, LastBlockHashesFace const& _lh, u256 const& _gasUsed,
        u256 const& _chainID)
      : m_headerInfo(_current), m_lastHashes(_lh), m_gasUsed(_gasUsed), m_chainID(_chainID)
    {}
    // Constructor with custom gasLimit - used in some synthetic scenarios like eth_estimateGas RPC
    // method
    EnvInfo(BlockHeader const& _current, LastBlockHashesFace const& _lh, u256 const& _gasUsed,
        u256 const& _gasLimit, u256 const& _chainID)
      : EnvInfo(_current, _lh, _gasUsed, _chainID)
    {
        m_headerInfo.setGasLimit(_gasLimit);
    }

    BlockHeader const& header() const { return m_headerInfo; }

    int64_t number() const { return m_headerInfo.number(); }
    Address const& author() const { return m_headerInfo.author(); }
    int64_t timestamp() const { return m_headerInfo.timestamp(); }
    u256 const& difficulty() const { return m_headerInfo.difficulty(); }
    u256 const& gasLimit() const { return m_headerInfo.gasLimit(); }
    LastBlockHashesFace const& lastHashes() const { return m_lastHashes; }
    u256 const& gasUsed() const { return m_gasUsed; }
    u256 const& chainID() const { return m_chainID; }

private:
    BlockHeader m_headerInfo;
    LastBlockHashesFace const& m_lastHashes;
    u256 m_gasUsed;
    u256 m_chainID;
};

/// Represents a call result.
///
/// @todo: Replace with evmc_result in future.
struct CallResult
{
    evmc_status_code status;
    owning_bytes_ref output;

    CallResult(evmc_status_code status, owning_bytes_ref&& output)
      : status{status}, output{std::move(output)}
    {}
};

/// Represents a CREATE result.
///
/// @todo: Replace with evmc_result in future.
struct CreateResult
{
    evmc_status_code status;
    owning_bytes_ref output;
    h160 address;

    CreateResult(evmc_status_code status, owning_bytes_ref&& output, h160 const& address)
      : status{status}, output{std::move(output)}, address{address}
    {}
};

/**
 * @brief Interface and null implementation of the class for specifying VM externalities.
 */
class ExtVMFace
{
public:
    /// Full constructor.
    ExtVMFace(EnvInfo const& _envInfo, Address _myAddress, Address _caller, Address _origin,
        u256 _value, u256 _gasPrice, bytesConstRef _data, bytes _code, h256 const& _codeHash,
        u256 const& _version, unsigned _depth, bool _isCreate, bool _staticCall);

    ExtVMFace(ExtVMFace const&) = delete;
    ExtVMFace& operator=(ExtVMFace const&) = delete;

    virtual ~ExtVMFace() = default;

    /// Read storage location.
    virtual u256 store(u256) { return 0; }

    /// Write a value in storage.
    virtual void setStore(u256, u256) {}

    /// Read original storage value (before modifications in the current transaction).
    virtual u256 originalStorageValue(u256 const&) { return 0; }

    /// Read address's balance.
    virtual u256 balance(Address) { return 0; }

    /// Read address's code.
    virtual bytes const& codeAt(Address) { return NullBytes; }

    /// @returns the size of the code in bytes at the given address.
    virtual size_t codeSizeAt(Address) { return 0; }

    /// @returns the hash of the code at the given address.
    virtual h256 codeHashAt(Address) { return h256{}; }

    /// Does the account exist?
    virtual bool exists(Address) { return false; }

    /// Suicide the associated contract and give proceeds to the given address.
    virtual void suicide(Address) { sub.suicides.insert(myAddress); }

    /// Create a new (contract) account.
    virtual CreateResult create(u256, u256&, bytesConstRef, Instruction, u256, OnOpFunc const&) = 0;

    /// Make a new message call.
    virtual CallResult call(CallParameters&) = 0;

    /// Revert any changes made (by any of the other calls).
    virtual void log(h256s&& _topics, bytesConstRef _data)
    {
        sub.logs.push_back(LogEntry(myAddress, std::move(_topics), _data.toBytes()));
    }

    /// Hash of a block if within the last 256 blocks, or h256() otherwise.
    virtual h256 blockHash(u256 _number) = 0;

    /// Get the execution environment information.
    EnvInfo const& envInfo() const { return m_envInfo; }

    /// Return the EVM gas-price schedule for this execution context.
    virtual EVMSchedule const& evmSchedule() const { return DefaultSchedule; }

private:
    EnvInfo const& m_envInfo;

public:
    // TODO: make private
    Address myAddress;  ///< Address associated with executing code (a contract, or contract-to-be).
    Address caller;     ///< Address which sent the message (either equal to origin or a contract).
    Address origin;     ///< Original transactor.
    u256 value;         ///< Value (in Wei) that was passed to this address.
    u256 gasPrice;      ///< Price of gas (that we already paid).
    bytesConstRef data;       ///< Current input data.
    bytes code;               ///< Current code that is executing.
    h256 codeHash;            ///< SHA3 hash of the executing code
    u256 version;             ///< Version of the VM to execute code
    u256 salt;                ///< Values used in new address construction by CREATE2
    SubState sub;             ///< Sub-band VM state (suicides, refund counter, logs).
    unsigned depth = 0;       ///< Depth of the present call.
    bool isCreate = false;    ///< Is this a CREATE call?
    bool staticCall = false;  ///< Throw on state changing.
};

class EvmCHost : public evmc::Host
{
public:
    explicit EvmCHost(ExtVMFace& _extVM) : m_extVM{_extVM} {}

    bool account_exists(const evmc::address& addr) noexcept override;

    evmc::bytes32 get_storage(
        const evmc::address& addr, const evmc::bytes32& key) noexcept override;

    evmc_storage_status set_storage(const evmc::address& _addr, const evmc::bytes32& _key,
        const evmc::bytes32& _value) noexcept override;

    evmc::uint256be get_balance(const evmc::address& _addr) noexcept override;

    size_t get_code_size(const evmc::address& _addr) noexcept override;

    evmc::bytes32 get_code_hash(const evmc::address& _addr) noexcept override;

    size_t copy_code(const evmc::address& _addr, size_t _codeOffset, uint8_t* _bufferData,
        size_t _bufferSize) noexcept override;

    void selfdestruct(
        const evmc::address& _addr, const evmc::address& _beneficiary) noexcept override;

    evmc::result call(const evmc_message& _msg) noexcept override;

    evmc_tx_context get_tx_context() noexcept override;

    evmc::bytes32 get_block_hash(int64_t _blockNumber) noexcept override;

    void emit_log(const evmc::address& _addr, const uint8_t* _data, size_t _dataSize,
        const evmc::bytes32 _topics[], size_t _numTopics) noexcept override;

private:
    evmc::result create(evmc_message const& _msg) noexcept;

private:
    ExtVMFace& m_extVM;
};

inline evmc::address toEvmC(Address const& _addr)
{
    return reinterpret_cast<evmc_address const&>(_addr);
}

inline evmc_uint256be toEvmC(h256 const& _h)
{
    return reinterpret_cast<evmc_uint256be const&>(_h);
}

inline u256 fromEvmC(evmc_uint256be const& _n)
{
    return fromBigEndian<u256>(_n.bytes);
}

inline Address fromEvmC(evmc::address const& _addr)
{
    return reinterpret_cast<Address const&>(_addr);
}
}  // namespace eth
}  // namespace dev
