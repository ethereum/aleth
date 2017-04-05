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

static_assert(sizeof(Address) == sizeof(evm_uint160be),
              "Address types size mismatch");
static_assert(alignof(Address) == alignof(evm_uint160be),
              "Address types alignment mismatch");

inline evm_uint160be toEvmC(Address _addr)
{
	return *reinterpret_cast<evm_uint160be*>(&_addr);
}

inline Address fromEvmC(evm_uint160be const* _addr)
{
	return *reinterpret_cast<Address const*>(_addr);
}

static_assert(sizeof(h256) == sizeof(evm_uint256be), "Hash types size mismatch");
static_assert(alignof(h256) == alignof(evm_uint256be), "Hash types alignment mismatch");

inline evm_uint256be toEvmC(h256 _h)
{
	return *reinterpret_cast<evm_uint256be*>(&_h);
}

inline u256 asUint(evm_uint256be const* _n)
{
	return fromBigEndian<u256>(_n->bytes);
}

void evm_query(
	evm_variant* o_result,
	evm_env* _opaqueEnv,
	evm_query_key _key,
	evm_uint160be const* _addr,
	evm_uint256be const* _storageKey
) noexcept
{
	auto &env = *reinterpret_cast<ExtVMFace*>(_opaqueEnv);
	auto addr = fromEvmC(_addr);
	switch (_key)
	{
	case EVM_CODE_BY_ADDRESS:
	{
		auto &code = env.codeAt(addr);
		o_result->data = code.data();
		o_result->data_size = code.size();
		break;
	}
	case EVM_CODE_SIZE:
		o_result->int64 = env.codeSizeAt(addr);
		break;
	case EVM_BALANCE:
		o_result->uint256be = toEvmC(env.balance(addr));
		break;
	case EVM_SLOAD:
	{
		auto storageKey = asUint(_storageKey);
		o_result->uint256be = toEvmC(env.store(storageKey));
		break;
	}
	case EVM_ACCOUNT_EXISTS:
		o_result->int64 = env.exists(addr);
		break;
	}
}

void updateState(
	evm_env* _opaqueEnv,
	evm_update_key _key,
	evm_uint160be const* _addr,
	evm_variant const* _arg1,
	evm_variant const* _arg2
) noexcept
{
	(void) _addr;
	auto &env = *reinterpret_cast<ExtVMFace*>(_opaqueEnv);
	assert(fromEvmC(_addr) == env.myAddress);
	switch (_key)
	{
	case EVM_SSTORE:
	{
		auto index = asUint(&_arg1->uint256be);
		auto value = asUint(&_arg2->uint256be);
		if (value == 0 && env.store(index) != 0)                   // If delete
			env.sub.refunds += env.evmSchedule().sstoreRefundGas;  // Increase refund counter

		env.setStore(index, value);    // Interface uses native endianness
		break;
	}
	case EVM_LOG:
	{
		size_t numTopics = _arg2->data_size / sizeof(h256);
		h256 const* pTopics = reinterpret_cast<h256 const*>(_arg2->data);
		env.log({pTopics, pTopics + numTopics}, {_arg1->data, _arg1->data_size});
		break;
	}
	case EVM_SELFDESTRUCT:
		// Register selfdestruction beneficiary.
		env.suicide(fromEvmC(&_arg1->address));
		break;
	}
}

void evm_get_tx_context(evm_tx_context* result, evm_env* _opaqueEnv) noexcept
{
	auto &env = *reinterpret_cast<ExtVMFace*>(_opaqueEnv);
	result->tx_gas_price = toEvmC(env.gasPrice);
	result->tx_origin = toEvmC(env.origin);
	result->block_coinbase = toEvmC(env.envInfo().author());
	result->block_number = static_cast<int64_t>(env.envInfo().number());
	result->block_timestamp = static_cast<int64_t>(env.envInfo().timestamp());
	result->block_gas_limit = env.envInfo().gasLimit();
	result->block_difficulty = toEvmC(env.envInfo().difficulty());
}

void getBlockHash(evm_uint256be* o_hash, evm_env* _envPtr, int64_t _number)
{
	auto &env = *reinterpret_cast<ExtVMFace*>(_envPtr);
	*o_hash = toEvmC(env.blockHash(_number));
}

int64_t evm_call(
	evm_env* _opaqueEnv,
	evm_message const* _msg,
	uint8_t* _outputData,
	size_t _outputSize
) noexcept
{
	assert(_msg->gas >= 0 && "Invalid gas value");
	auto &env = *reinterpret_cast<ExtVMFace*>(_opaqueEnv);
	auto value = asUint(&_msg->value);
	bytesConstRef input{_msg->input, _msg->input_size};

	if (_msg->kind == EVM_CREATE)
	{
		assert(_outputSize == 20);
		u256 gas = _msg->gas;
		// TODO: How it knows who the sender is?
		auto addr = env.create(value, gas, input, {});
		auto gasLeft = static_cast<int64_t>(gas);
		if (addr)
			std::memcpy(_outputData, addr.data(), 20);
		else
			gasLeft |= EVM_CALL_FAILURE;
		return gasLeft;
	}

	CallParameters params;
	params.gas = _msg->gas;
	params.apparentValue = value;
	params.valueTransfer = _msg->kind == EVM_DELEGATECALL ? 0 : params.apparentValue;
	params.senderAddress = fromEvmC(&_msg->sender);
	params.codeAddress = fromEvmC(&_msg->address);
	params.receiveAddress = _msg->kind == EVM_CALL ? params.codeAddress : env.myAddress;
	params.data = input;
	params.onOp = {};

	auto output = env.call(params);
	auto gasLeft = static_cast<int64_t>(params.gas);

	output.second.copyTo({_outputData, _outputSize});
	if (!output.first)
		// Add failure indicator.
		gasLeft |= EVM_CALL_FAILURE;

	return gasLeft;
}


/// RAII wrapper for an evm instance.
class EVM
{
public:
	EVM(evm_query_state_fn _queryFn, evm_update_state_fn _updateFn, evm_call_fn _callFn,
		evm_get_tx_context_fn _getTxContextFn,
		evm_get_block_hash_fn _getBlockHashFn
	)
	{
		auto factory = evmjit_get_factory();
		m_instance = factory.create(
				_queryFn, _updateFn, _callFn, _getTxContextFn, _getBlockHashFn
		);
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

		Result(Result&& _other):
			m_result(_other.m_result)
		{
			// Disable releaser of the rvalue object.
			_other.m_result.release = nullptr;
		}

		Result(Result const&) = delete;
		Result& operator=(Result const&) = delete;

		evm_result_code code() const
		{
			return m_result.code;
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
		auto env = reinterpret_cast<evm_env*>(&_ext);
		auto mode = JitVM::scheduleToMode(_ext.evmSchedule());
		evm_message msg = {toEvmC(_ext.myAddress), toEvmC(_ext.caller),
						   toEvmC(_ext.value), _ext.data.data(),
						   _ext.data.size(), toEvmC(_ext.codeHash), gas,
						   static_cast<int32_t>(_ext.depth), EVM_CALL};
		return Result{m_instance->execute(
			m_instance, env, mode, &msg, _ext.code.data(), _ext.code.size()
		)};
	}

	bool isCodeReady(evm_mode _mode, h256 _codeHash)
	{
		return m_instance->get_code_status(m_instance, _mode, toEvmC(_codeHash)) == EVM_READY;
	}

	void compile(evm_mode _mode, bytesConstRef _code, h256 _codeHash)
	{
		m_instance->prepare_code(
			m_instance, _mode, toEvmC(_codeHash), _code.data(), _code.size()
		);
	}

private:
	/// The VM instance created with m_interface.create().
	evm_instance* m_instance = nullptr;
};

EVM& getJit()
{
	// Create EVM JIT instance by using EVM-C interface.
	static EVM jit(evm_query, updateState, evm_call, evm_get_tx_context, getBlockHash);
	return jit;
}

}

owning_bytes_ref JitVM::exec(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp)
{
	auto rejected = false;
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
	if (r.code() == EVM_FAILURE)
		BOOST_THROW_EXCEPTION(OutOfGas());

	io_gas = r.gasLeft();
	// FIXME: Copy the output for now, but copyless version possible.
	owning_bytes_ref output{r.output().toVector(), 0, r.output().size()};

	if (r.code() == EVM_REVERT)
		throw RevertInstruction(std::move(output));

	return output;
}

evm_mode JitVM::scheduleToMode(EVMSchedule const& _schedule)
{
	if (_schedule.haveRevert)
		return EVM_METROPOLIS;
	if (_schedule.eip158Mode)
		return EVM_CLEARING;
	if (_schedule.eip150Mode)
		return EVM_ANTI_DOS;
	return _schedule.haveDelegateCall ? EVM_HOMESTEAD : EVM_FRONTIER;
}

bool JitVM::isCodeReady(evm_mode _mode, h256 _codeHash)
{
	return getJit().isCodeReady(_mode, _codeHash);
}

void JitVM::compile(evm_mode _mode, bytesConstRef _code, h256 _codeHash)
{
	getJit().compile(_mode, _code, _codeHash);
}

}
}
