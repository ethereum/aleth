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

inline evm_uint160be toEvmC(Address const& _addr)
{
	return reinterpret_cast<evm_uint160be const&>(_addr);
}

inline Address fromEvmC(evm_uint160be const& _addr)
{
	return reinterpret_cast<Address const&>(_addr);
}

static_assert(sizeof(h256) == sizeof(evm_uint256be), "Hash types size mismatch");
static_assert(alignof(h256) == alignof(evm_uint256be), "Hash types alignment mismatch");

inline evm_uint256be toEvmC(h256 const& _h)
{
	return reinterpret_cast<evm_uint256be const&>(_h);
}

inline u256 fromEvmC(evm_uint256be const& _n)
{
	return fromBigEndian<u256>(_n.bytes);
}

void queryState(
	evm_variant* o_result,
	evm_env* _opaqueEnv,
	evm_query_key _key,
	evm_uint160be const* _addr,
	evm_uint256be const* _storageKey
) noexcept
{
	auto &env = reinterpret_cast<ExtVMFace&>(*_opaqueEnv);
	Address addr = fromEvmC(*_addr);
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
		auto storageKey = fromEvmC(*_storageKey);
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
	auto &env = reinterpret_cast<ExtVMFace&>(*_opaqueEnv);
	assert(fromEvmC(*_addr) == env.myAddress);
	switch (_key)
	{
	case EVM_SSTORE:
	{
		u256 index = fromEvmC(_arg1->uint256be);
		u256 value = fromEvmC(_arg2->uint256be);
		if (value == 0 && env.store(index) != 0)                   // If delete
			env.sub.refunds += env.evmSchedule().sstoreRefundGas;  // Increase refund counter

		env.setStore(index, value);    // Interface uses native endianness
		break;
	}
	}
}

void selfdestruct(
	evm_env* _opaqueEnv,
	evm_uint160be const* _addr,
	evm_uint160be const* _beneficiary
) noexcept
{
	(void) _addr;
	auto &env = reinterpret_cast<ExtVMFace&>(*_opaqueEnv);
	assert(fromEvmC(*_addr) == env.myAddress);
	env.suicide(fromEvmC(*_beneficiary));
}


void log(
	evm_env* _opaqueEnv,
	evm_uint160be const* _addr,
	uint8_t const* _data,
	size_t _dataSize,
	evm_uint256be _topics[],
	size_t _numTopics
) noexcept
{
	(void) _addr;
	auto &env = reinterpret_cast<ExtVMFace&>(*_opaqueEnv);
	assert(fromEvmC(*_addr) == env.myAddress);
	h256 const* pTopics = reinterpret_cast<h256 const*>(_topics);
	env.log({pTopics, pTopics + _numTopics}, {_data, _dataSize});
}

void getTxContext(evm_tx_context* result, evm_env* _opaqueEnv) noexcept
{
	auto &env = reinterpret_cast<ExtVMFace&>(*_opaqueEnv);
	result->tx_gas_price = toEvmC(env.gasPrice);
	result->tx_origin = toEvmC(env.origin);
	result->block_coinbase = toEvmC(env.envInfo().author());
	result->block_number = static_cast<int64_t>(env.envInfo().number());
	result->block_timestamp = static_cast<int64_t>(env.envInfo().timestamp());
	result->block_gas_limit = static_cast<int64_t>(env.envInfo().gasLimit());
	result->block_difficulty = toEvmC(env.envInfo().difficulty());
}

void getBlockHash(evm_uint256be* o_hash, evm_env* _envPtr, int64_t _number)
{
	auto &env = reinterpret_cast<ExtVMFace&>(*_envPtr);
	*o_hash = toEvmC(env.blockHash(_number));
}

void create(evm_result* o_result, ExtVMFace& _env, evm_message const* _msg) noexcept
{
	u256 gas = _msg->gas;
	u256 value = fromEvmC(_msg->value);
	bytesConstRef init = {_msg->input, _msg->input_size};
	// ExtVM::create takes the sender address from .myAddress.
	assert(fromEvmC(_msg->sender) == _env.myAddress);

	// TODO: EVMJIT does not support RETURNDATA at the moment, so
	//       the output is ignored here.
	h160 addr;
	std::tie(addr, std::ignore) = _env.create(value, gas, init, {});
	o_result->gas_left = static_cast<int64_t>(gas);
	o_result->release = nullptr;
	if (addr)
	{
		o_result->code = EVM_SUCCESS;
		// Use reserved data to store the address.
		static_assert(sizeof(o_result->reserved.data) >= addr.size,
			"Not enough space to store an address");
		std::copy(addr.begin(), addr.end(), o_result->reserved.data);
		o_result->output_data = o_result->reserved.data;
		o_result->output_size = addr.size;
	}
	else
	{
		o_result->code = EVM_FAILURE;
		o_result->output_data = nullptr;
		o_result->output_size = 0;
	}
}

void call(evm_result* o_result, evm_env* _opaqueEnv, evm_message const* _msg) noexcept
{
	assert(_msg->gas >= 0 && "Invalid gas value");
	auto &env = reinterpret_cast<ExtVMFace&>(*_opaqueEnv);

	// Handle CREATE separately.
	if (_msg->kind == EVM_CREATE)
		return create(o_result, env, _msg);

	CallParameters params;
	params.gas = _msg->gas;
	params.apparentValue = fromEvmC(_msg->value);
	params.valueTransfer = _msg->kind == EVM_DELEGATECALL ? 0 : params.apparentValue;
	params.senderAddress = fromEvmC(_msg->sender);
	params.codeAddress = fromEvmC(_msg->address);
	params.receiveAddress = _msg->kind == EVM_CALL ? params.codeAddress : env.myAddress;
	params.data = {_msg->input, _msg->input_size};
	params.onOp = {};

	bool success = false;
	owning_bytes_ref output;
	std::tie(success, output) = env.call(params);
	// FIXME: We have a mess here. It is hard to distinguish reverts from failures.
	// In first case we want to keep the output, in the second one the output
	// is optional and should not be passed to the contract, but can be useful
	// for EVM in general.
	o_result->code = success ? EVM_SUCCESS : EVM_REVERT;
	o_result->gas_left = static_cast<int64_t>(params.gas);

	// Pass the output to the EVM without a copy. The EVM will delete it
	// when finished with it.

	// First assign reference. References are not invalidated when vector
	// of bytes is moved. See `.takeBytes()` below.
	o_result->output_data = output.data();
	o_result->output_size = output.size();

	// Place a new vector of bytes containing output in result's reserved memory.
	static_assert(sizeof(bytes) <= sizeof(o_result->reserved), "Vector is too big");
	new(&o_result->reserved) bytes(output.takeBytes());
	// Set the destructor to delete the vector.
	o_result->release = [](evm_result const* _result)
	{
		auto& output = reinterpret_cast<bytes const&>(_result->reserved);
		// Explicitly call vector's destructor to release its data.
		// This is normal pattern when placement new operator is used.
		output.~bytes();
	};
}


/// RAII wrapper for an evm instance.
class EVM
{
public:
	EVM(
		evm_query_state_fn _queryFn,
		evm_update_state_fn _updateFn,
		evm_selfdestruct_fn _selfdestructFn,
		evm_call_fn _callFn,
		evm_get_tx_context_fn _getTxContextFn,
		evm_get_block_hash_fn _getBlockHashFn,
		evm_log_fn _logFn
	)
	{
		auto factory = evmjit_get_factory();
		m_instance = factory.create(
				_queryFn, _updateFn,
				_selfdestructFn,
				_callFn, _getTxContextFn, _getBlockHashFn,
				_logFn
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
	static EVM jit(queryState, updateState, selfdestruct, call, getTxContext, getBlockHash, log);
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
