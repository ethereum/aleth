// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 cpp-ethereum Authors.
// Licensed under the GNU General Public License v3.

#pragma once

#include <libevm/VMFace.h>

namespace dev
{
namespace eth
{
/// The wrapper implementing the VMFace interface with a EVMC VM as a backend.
class EVMC : public evmc::vm, public VMFace
{
public:
    explicit EVMC(evmc_instance* _instance) noexcept;

    owning_bytes_ref exec(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp) final;
};
}  // namespace eth
}  // namespace dev
