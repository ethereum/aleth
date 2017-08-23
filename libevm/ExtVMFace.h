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
/** @file ExtVMFace.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <set>
#include <functional>
#include <boost/optional.hpp>
#include <evm.h>
#include <libdevcore/Common.h>
#include <libdevcore/CommonData.h>
#include <libdevcore/RLP.h>
#include <libdevcore/SHA3.h>
#include <libethcore/Common.h>
#include <libethcore/BlockHeader.h>
#include <libethcore/ChainOperationParams.h>
#include "Instruction.h"

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
class owning_bytes_ref: public vector_ref<byte const>
{
public:
	owning_bytes_ref() = default;

	/// @param _bytes  The buffer.
	/// @param _begin  The index of the first referenced byte.
	/// @param _size   The number of referenced bytes.
	owning_bytes_ref(bytes&& _bytes, size_t _begin, size_t _size):
			m_bytes(std::move(_bytes))
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

enum class BlockPolarity
{
	Unknown,
	Dead,
	Live
};

struct LogEntry
{
	LogEntry() {}
	LogEntry(RLP const& _r) { address = (Address)_r[0]; topics = _r[1].toVector<h256>(); data = _r[2].toBytes(); }
	LogEntry(Address const& _address, h256s const& _ts, bytes&& _d): address(_address), topics(_ts), data(std::move(_d)) {}

	void streamRLP(RLPStream& _s) const { _s.appendList(3) << address << topics << data; }

	LogBloom bloom() const
	{
		LogBloom ret;
		ret.shiftBloom<3>(sha3(address.ref()));
		for (auto t: topics)
			ret.shiftBloom<3>(sha3(t.ref()));
		return ret;
	}

	Address address;
	h256s topics;
	bytes data;
};

using LogEntries = std::vector<LogEntry>;

struct LocalisedLogEntry: public LogEntry
{
	LocalisedLogEntry() {}
	explicit LocalisedLogEntry(LogEntry const& _le): LogEntry(_le) {}

	explicit LocalisedLogEntry(
		LogEntry const& _le,
		h256 _special
	):
		LogEntry(_le),
		isSpecial(true),
		special(_special)
	{}

	explicit LocalisedLogEntry(
		LogEntry const& _le,
		h256 const& _blockHash,
		BlockNumber _blockNumber,
		h256 const& _transactionHash,
		unsigned _transactionIndex,
		unsigned _logIndex,
		BlockPolarity _polarity = BlockPolarity::Unknown
	):
		LogEntry(_le),
		blockHash(_blockHash),
		blockNumber(_blockNumber),
		transactionHash(_transactionHash),
		transactionIndex(_transactionIndex),
		logIndex(_logIndex),
		polarity(_polarity),
		mined(true)
	{}

	h256 blockHash;
	BlockNumber blockNumber = 0;
	h256 transactionHash;
	unsigned transactionIndex = 0;
	unsigned logIndex = 0;
	BlockPolarity polarity = BlockPolarity::Unknown;
	bool mined = false;
	bool isSpecial = false;
	h256 special;
};

using LocalisedLogEntries = std::vector<LocalisedLogEntry>;

inline LogBloom bloom(LogEntries const& _logs)
{
	LogBloom ret;
	for (auto const& l: _logs)
		ret |= l.bloom();
	return ret;
}

struct SubState
{
	std::set<Address> suicides;	///< Any accounts that have suicided.
	LogEntries logs;			///< Any logs.
	u256 refunds;				///< Refund counter of SSTORE nonzero->zero.

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
class VM;

using OnOpFunc = std::function<void(uint64_t /*steps*/, uint64_t /* PC */, Instruction /*instr*/, bigint /*newMemSize*/, bigint /*gasCost*/, bigint /*gas*/, VM*, ExtVMFace const*)>;

struct CallParameters
{
	CallParameters() = default;
	CallParameters(
		Address _senderAddress,
		Address _codeAddress,
		Address _receiveAddress,
		u256 _valueTransfer,
		u256 _apparentValue,
		u256 _gas,
		bytesConstRef _data,
		OnOpFunc _onOpFunc
	):	senderAddress(_senderAddress), codeAddress(_codeAddress), receiveAddress(_receiveAddress),
		valueTransfer(_valueTransfer), apparentValue(_apparentValue), gas(_gas), data(_data), onOp(_onOpFunc)  {}
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
	EnvInfo(BlockHeader const& _current, LastBlockHashesFace const& _lh, u256 const& _gasUsed):
		m_headerInfo(_current),
		m_lastHashes(_lh),
		m_gasUsed(_gasUsed)
	{}
	// Constructor with custom gasLimit - used in some synthetic scenarios like eth_estimateGas	RPC method
	EnvInfo(BlockHeader const& _current, LastBlockHashesFace const& _lh, u256 const& _gasUsed, u256 const& _gasLimit):
		EnvInfo(_current, _lh, _gasUsed)
	{
		m_headerInfo.setGasLimit(_gasLimit);
	}

	BlockHeader const& header() const { return m_headerInfo;  }

	u256 const& number() const { return m_headerInfo.number(); }
	Address const& author() const { return m_headerInfo.author(); }
	u256 const& timestamp() const { return m_headerInfo.timestamp(); }
	u256 const& difficulty() const { return m_headerInfo.difficulty(); }
	u256 const& gasLimit() const { return m_headerInfo.gasLimit(); }
	LastBlockHashesFace const& lastHashes() const { return m_lastHashes; }
	u256 const& gasUsed() const { return m_gasUsed; }

private:
	BlockHeader m_headerInfo;
	LastBlockHashesFace const& m_lastHashes;
	u256 m_gasUsed;
};

/**
 * @brief Interface and null implementation of the class for specifying VM externalities.
 */
class ExtVMFace: public evm_context
{
public:
	/// Null constructor.
	ExtVMFace() = default;

	/// Full constructor.
	ExtVMFace(EnvInfo const& _envInfo, Address _myAddress, Address _caller, Address _origin, u256 _value, u256 _gasPrice, bytesConstRef _data, bytes _code, h256 const& _codeHash, unsigned _depth, bool _staticCall);

	virtual ~ExtVMFace() = default;

	ExtVMFace(ExtVMFace const&) = delete;
	ExtVMFace& operator=(ExtVMFace const&) = delete;

	/// Read storage location.
	virtual u256 store(u256) { return 0; }

	/// Write a value in storage.
	virtual void setStore(u256, u256) {}

	/// Read address's balance.
	virtual u256 balance(Address) { return 0; }

	/// Read address's code.
	virtual bytes const& codeAt(Address) { return NullBytes; }

	/// @returns the size of the code in bytes at the given address.
	virtual size_t codeSizeAt(Address) { return 0; }

	/// Does the account exist?
	virtual bool exists(Address) { return false; }

	/// Suicide the associated contract and give proceeds to the given address.
	virtual void suicide(Address) { sub.suicides.insert(myAddress); }

	/// Create a new (contract) account.
	virtual std::pair<h160, owning_bytes_ref> create(u256, u256&, bytesConstRef, Instruction, u256, OnOpFunc const&) = 0;

	/// Make a new message call.
	/// @returns success flag and output data, if any.
	virtual std::pair<bool, owning_bytes_ref> call(CallParameters&) = 0;

	/// Revert any changes made (by any of the other calls).
	virtual void log(h256s&& _topics, bytesConstRef _data) { sub.logs.push_back(LogEntry(myAddress, std::move(_topics), _data.toBytes())); }

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
	Address myAddress;			///< Address associated with executing code (a contract, or contract-to-be).
	Address caller;				///< Address which sent the message (either equal to origin or a contract).
	Address origin;				///< Original transactor.
	u256 value;					///< Value (in Wei) that was passed to this address.
	u256 gasPrice;				///< Price of gas (that we already paid).
	bytesConstRef data;			///< Current input data.
	bytes code;					///< Current code that is executing.
	h256 codeHash;				///< SHA3 hash of the executing code
	SubState sub;				///< Sub-band VM state (suicides, refund counter, logs).
	unsigned depth = 0;			///< Depth of the present call.
	bool staticCall = false;	///< Throw on state changing.
};

}
}
