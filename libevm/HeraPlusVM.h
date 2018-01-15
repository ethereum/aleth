// Copyright 2018 cpp-ethereum Authors.
// Licensed under the GNU General Public License v3. See the LICENSE file.

#pragma once

#include "EVMC.h"

namespace dev
{
namespace eth
{

/// The eWASM VM with a fallback to EVM 1.0.
class HeraPlusVM: public EVM, public VMFace
{
public:
    HeraPlusVM();

    owning_bytes_ref exec(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp) final;
};

}
}
