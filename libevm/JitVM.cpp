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

/// RAII wrapper for an evm instance.
class EVM
{
public:
	EVM():
		m_instance(evmjit_create())
	{
		assert(m_instance->abi_version == EVM_ABI_VERSION);
	}

	~EVM()
	{
		m_instance->destroy(m_instance);
	}

	EVM(EVM const&) = delete;
	EVM& operator=(EVM) = delete;

	class Result
	{
	public:
		explicit Result(evm_result const& _result):
			m_result(_result)
		{}

		~Result()
		{
			if (m_result.release)
				m_result.release(&m_result);
		}

		Result(Result&& _other) noexcept:
			m_result(_other.m_result)
		{
			// Disable releaser of the rvalue object.
			_other.m_result.release = nullptr;
		}

		Result(Result const&) = delete;
		Result& operator=(Result const&) = delete;

		evm_status_code status() const
		{
			return m_result.status_code;
		}

		int64_t gasLeft() const
		{
			return m_result.gas_left;
		}

		bytesConstRef output() const
		{
			return {m_result.output_data, m_result.output_size};
		}

	private:
		evm_result m_result;
	};

	/// Handy wrapper for evm_execute().
	Result execute(ExtVMFace& _ext, int64_t gas)
	{
		auto mode = JitVM::toRevision(_ext.evmSchedule());
		uint32_t flags = _ext.staticCall ? EVM_STATIC : 0;
		evm_message msg = {toEvmC(_ext.myAddress), toEvmC(_ext.caller),
						   toEvmC(_ext.value), _ext.data.data(),
						   _ext.data.size(), toEvmC(_ext.codeHash), gas,
						   static_cast<int32_t>(_ext.depth), EVM_CALL, flags};
		return Result{m_instance->execute(
			m_instance, &_ext, mode, &msg, _ext.code.data(), _ext.code.size()
		)};
	}

	bool isCodeReady(evm_revision _mode, uint32_t _flags, h256 _codeHash)
	{
		return m_instance->get_code_status(m_instance, _mode, _flags, toEvmC(_codeHash)) == EVM_READY;
	}

	void compile(evm_revision _mode, uint32_t _flags, bytesConstRef _code, h256 _codeHash)
	{
		m_instance->prepare_code(
			m_instance, _mode, _flags, toEvmC(_codeHash), _code.data(), _code.size()
		);
	}

private:
	/// The VM instance created with m_interface.create().
	evm_instance* m_instance = nullptr;
};

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

evm_revision JitVM::toRevision(EVMSchedule const& _schedule)
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
