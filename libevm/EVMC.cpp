// Copyright 2018 cpp-ethereum Authors.
// Licensed under the GNU General Public License v3. See the LICENSE file.

#include "EVMC.h"

#include <libdevcore/Log.h>
#include <libevm/VM.h>
#include <libevm/VMFactory.h>

namespace dev
{
namespace eth
{
EVM::EVM(evm_instance* _instance) noexcept : m_instance(_instance)
{
    assert(m_instance != nullptr);
    assert(m_instance->abi_version == EVM_ABI_VERSION);

    // Set the options.
    for (auto& pair : evmcOptions())
        m_instance->set_option(m_instance, pair.first.c_str(), pair.second.c_str());
}

owning_bytes_ref EVMC::exec(u256& io_gas, ExtVMFace& _ext, OnOpFunc const&)
{
    constexpr int64_t int64max = std::numeric_limits<int64_t>::max();

    // TODO: The following checks should be removed by changing the types
    //       used for gas, block number and timestamp.
    (void)int64max;
    assert(io_gas <= int64max);
    assert(_ext.envInfo().number() <= int64max);
    assert(_ext.envInfo().timestamp() <= int64max);
    assert(_ext.envInfo().gasLimit() <= int64max);
    assert(_ext.depth <= std::numeric_limits<int32_t>::max());

    auto gas = static_cast<int64_t>(io_gas);
    EVM::Result r = execute(_ext, gas);

    // TODO: Add EVM-C result codes mapping with exception types.
    if (r.status() == EVM_FAILURE)
        BOOST_THROW_EXCEPTION(OutOfGas());

    io_gas = r.gasLeft();
    // FIXME: Copy the output for now, but copyless version possible.
    owning_bytes_ref output{r.output().toVector(), 0, r.output().size()};

    if (r.status() == EVM_REVERT)
        throw RevertInstruction(std::move(output));

    return output;
}

evm_revision toRevision(EVMSchedule const& _schedule)
{
	if (_schedule.haveCreate2)
		return EVM_CONSTANTINOPLE;
	if (_schedule.haveRevert)
		return EVM_BYZANTIUM;
	if (_schedule.eip158Mode)
		return EVM_SPURIOUS_DRAGON;
	if (_schedule.eip150Mode)
		return EVM_TANGERINE_WHISTLE;
	if (_schedule.haveDelegateCall)
		return EVM_HOMESTEAD;
	return EVM_FRONTIER;
}

}
}
