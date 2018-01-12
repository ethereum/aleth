#include "JitVM.h"

#include <libdevcore/Log.h>
#include <libevm/VM.h>
#include <libevm/VMFactory.h>

namespace dev
{
namespace eth
{

namespace
{



EVM& getJit()
{
	// Create EVM JIT instance by using EVM-C interface.
	static EVM jit;
	return jit;
}

}  // End of anonymous namespace.

owning_bytes_ref JitVM::exec(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp)
{
	bool rejected = false;
	// TODO: Rejecting transactions with gas limit > 2^63 can be used by attacker to take JIT out of scope
	rejected |= io_gas > std::numeric_limits<int64_t>::max(); // Do not accept requests with gas > 2^63 (int64 max)
	rejected |= _ext.envInfo().number() > std::numeric_limits<int64_t>::max();
	rejected |= _ext.envInfo().timestamp() > std::numeric_limits<int64_t>::max();
	rejected |= _ext.envInfo().gasLimit() > std::numeric_limits<int64_t>::max();
	if (rejected)
	{
		cwarn << "Execution rejected by EVM JIT (gas limit: " << io_gas << "), executing with interpreter";
		return VMFactory::create(VMKind::Interpreter)->exec(io_gas, _ext, _onOp);
	}

	auto gas = static_cast<int64_t>(io_gas);
	auto r = getJit().execute(_ext, gas);

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

bool JitVM::isCodeReady(evm_revision _mode, uint32_t _flags, h256 _codeHash)
{
	return getJit().isCodeReady(_mode, _flags, _codeHash);
}

void JitVM::compile(evm_revision _mode, uint32_t _flags, bytesConstRef _code, h256 _codeHash)
{
	getJit().compile(_mode, _flags, _code, _codeHash);
}

}
}
