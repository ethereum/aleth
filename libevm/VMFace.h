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

#include <memory>
#include <libdevcore/Exceptions.h>
#include "ExtVMFace.h"

namespace dev
{
namespace eth
{

struct VMException: Exception {};
#define ETH_SIMPLE_EXCEPTION_VM(X) struct X: VMException { const char* what() const noexcept override { return #X; } }
ETH_SIMPLE_EXCEPTION_VM(BadInstruction);
ETH_SIMPLE_EXCEPTION_VM(BadJumpDestination);
ETH_SIMPLE_EXCEPTION_VM(OutOfGas);
ETH_SIMPLE_EXCEPTION_VM(OutOfStack);
ETH_SIMPLE_EXCEPTION_VM(StackUnderflow);
ETH_SIMPLE_EXCEPTION_VM(DisallowedStateChange);

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
};

}
}
