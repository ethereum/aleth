// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2018-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "PrecompilesVM.h"
#include <aleth/version.h>
#include <evmc/evmc.hpp>
#include <libdevcore/Common.h>

namespace dev
{
namespace
{
using Pricer = bigint (*)(bytesConstRef, evmc_revision);
using Executor = std::pair<bool, bytes> (*)(bytesConstRef);

bigint linearPrice(unsigned _base, unsigned _word, bytesConstRef _in)
{
    bigint const s = _in.size();
    bigint const b = _base;
    bigint const w = _word;
    return b + (s + 31) / 32 * w;
}

bigint identityPrice(bytesConstRef _in, evmc_revision)
{
    return linearPrice(15, 3, _in);
}

std::pair<bool, bytes> identity(bytesConstRef _in)
{
    return {true, _in.toBytes()};
}

evmc_result execute(evmc_vm*, const evmc_host_interface*, evmc_host_context*,
    enum evmc_revision _rev, const evmc_message* _msg, const uint8_t*, size_t) noexcept
{
    static std::array<std::pair<Pricer, Executor>, 9> const precompiles = {
        {{identityPrice, identity}, {identityPrice, identity}, {identityPrice, identity},
            {identityPrice, identity}, {identityPrice, identity}, {identityPrice, identity},
            {identityPrice, identity}, {identityPrice, identity}, {identityPrice, identity}}};

    // TODO check range

    auto const precompile = precompiles[_msg->destination.bytes[sizeof(_msg->destination) - 1] + 1];

    bytesConstRef input{_msg->input_data, _msg->input_size};

    // Check the gas cost.
    auto const gasCost = precompile.first(input, _rev);
    auto const gasLeft = _msg->gas - gasCost;
    if (gasLeft < 0)
        return evmc::make_result(EVMC_SUCCESS, 0, nullptr, 0);

    // Execute.
    auto const res = precompile.second(input);
    if (!res.first)
        return evmc::make_result(EVMC_PRECOMPILE_FAILURE, 0, nullptr, 0);

    // Return the result.
    return evmc::make_result(
        EVMC_SUCCESS, static_cast<int64_t>(gasLeft), res.second.data(), res.second.size());
}
}  // namespace
}  // namespace dev

evmc_vm* evmc_create_aleth_precompiles_vm() noexcept
{
    static evmc_vm vm = {
        EVMC_ABI_VERSION,
        "precompiles_vm",
        aleth_version,
        [](evmc_vm*) {},
        dev::execute,
        [](evmc_vm*) { return evmc_capabilities_flagset{EVMC_CAPABILITY_PRECOMPILES}; },
        nullptr,
    };
    return &vm;
}
