// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
#pragma once

#include "ExtVMFace.h"
#include <libdevcore/Exceptions.h>
#include <memory>

namespace dev
{
namespace eth
{

struct VMException: Exception {};
#define ETH_SIMPLE_EXCEPTION_VM(X) struct X: VMException { const char* what() const noexcept override { return #X; } }
ETH_SIMPLE_EXCEPTION_VM(InvalidInstruction);
ETH_SIMPLE_EXCEPTION_VM(BadInstruction);
ETH_SIMPLE_EXCEPTION_VM(BadJumpDestination);
ETH_SIMPLE_EXCEPTION_VM(OutOfGas);
ETH_SIMPLE_EXCEPTION_VM(OutOfStack);
ETH_SIMPLE_EXCEPTION_VM(StackUnderflow);
ETH_SIMPLE_EXCEPTION_VM(DisallowedStateChange);
ETH_SIMPLE_EXCEPTION_VM(BufferOverrun);
ETH_SIMPLE_EXCEPTION_VM(PrecompileFailure);

/// Reports VM internal error. This is not based on VMException because it must be handled
/// differently than defined consensus exceptions.
struct InternalVMError : Exception {};

/// Error info for EVMC status code.
using errinfo_evmcStatusCode = boost::error_info<struct tag_evmcStatusCode, evmc_status_code>;

struct RevertInstruction: VMException
{
	explicit RevertInstruction(owning_bytes_ref&& _output) : m_output(std::move(_output)) {}
	RevertInstruction(RevertInstruction const&) = delete;
	RevertInstruction(RevertInstruction&&) = default;
	RevertInstruction& operator=(RevertInstruction const&) = delete;
	RevertInstruction& operator=(RevertInstruction&&) = default;

	char const* what() const noexcept override { return "Revert instruction"; }

	owning_bytes_ref&& output() { return std::move(m_output); }

private:
	owning_bytes_ref m_output;
};


/// EVM Virtual Machine interface
class VMFace
{
public:
	VMFace() = default;
	virtual ~VMFace() = default;
	VMFace(VMFace const&) = delete;
	VMFace& operator=(VMFace const&) = delete;

	/// VM implementation
	virtual owning_bytes_ref exec(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp) = 0;

    /// Executes code without providing it access to the state.
    ///
    /// This method is useful for special cases like calling precompiles from EVMC module, where
    /// execution is guaranteed not to require any Host access.
    virtual owning_bytes_ref exec(u256& io_gas, Address const& _myAddress, Address const& _caller,
        u256 const& _value, bytesConstRef _data, unsigned _depth, bool _isCreate,
        bool _isStaticCall, EVMSchedule const& _schedule) = 0;
};

/// Helpers:

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

// Checks whether address is in the address range for precompiles according to EIP-1352
inline bool isPrecompiledContract(Address const& _addr) noexcept
{
    static Address const c_maxPrecompiledAddress{0xffff};
    return _addr <= c_maxPrecompiledAddress;
}
}
}
