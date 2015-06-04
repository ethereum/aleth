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

struct VMException: virtual Exception {};
struct StepsDone: virtual VMException {};
struct BreakPointHit: virtual VMException {};
struct BadInstruction: virtual VMException {};
struct BadJumpDestination: virtual VMException {};
struct OutOfGas: virtual VMException {};
struct OutOfStack: virtual VMException {};
struct StackUnderflow: virtual VMException {};

/// EVM Virtual Machine interface
class VMFace
{
public:
	VMFace() = default;
	virtual ~VMFace() = default;
	VMFace(VMFace const&) = delete;
	VMFace& operator=(VMFace const&) = delete;

	virtual bytesConstRef go(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp = {}, uint64_t _steps = (uint64_t)-1) = 0;
};

}
}
