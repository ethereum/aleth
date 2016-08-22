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

static_assert(sizeof(Address) == sizeof(evm_hash160),
              "Address types size mismatch");
static_assert(alignof(Address) == alignof(evm_hash160),
              "Address types alignment mismatch");

inline evm_hash160 toEvmC(Address _addr)
{
	return *reinterpret_cast<evm_hash160*>(&_addr);
}

inline Address fromEvmC(evm_hash160 _addr)
{
	return *reinterpret_cast<Address*>(&_addr);
}

static_assert(sizeof(h256) == sizeof(evm_hash256),
              "Hash types size mismatch");
static_assert(alignof(h256) != alignof(evm_hash256),
              "Hash types alignment match -- update implementation");

inline evm_hash256 toEvmC(h256 _h)
{
	// Because the h256 only pretends to be aligned to 8 bytes we have to do
	// non-aligned memory copy.
	// TODO: h256 should be aligned to 8 bytes.
	evm_hash256 g;
	std::memcpy(&g, &_h, sizeof(g));
	return g;
}

inline evm_uint256 toEvmC(u256 _n)
{
	evm_uint256 m;
	m.words[0] = static_cast<uint64_t>(_n);
	_n >>= 64;
	m.words[1] = static_cast<uint64_t>(_n);
	_n >>= 64;
	m.words[2] = static_cast<uint64_t>(_n);
	_n >>= 64;
	m.words[3] = static_cast<uint64_t>(_n);
	return m;
}

inline u256 fromEvmC(evm_uint256 _n)
{
	u256 u = _n.words[3];
	u <<= 64;
	u |= _n.words[2];
	u <<= 64;
	u |= _n.words[1];
	u <<= 64;
	u |= _n.words[0];
	return u;
}

evm_variant evm_query(
	evm_env* _opaqueEnv,
	evm_query_key _key,
	evm_variant _arg
) noexcept
{
	auto &env = *reinterpret_cast<ExtVMFace*>(_opaqueEnv);
	evm_variant v;
	switch (_key)
	{
	case EVM_ADDRESS:
		v.address = toEvmC(env.myAddress);
		break;
	case EVM_CALLER:
		v.address = toEvmC(env.caller);
		break;
	case EVM_ORIGIN:
		v.address = toEvmC(env.origin);
		break;
	case EVM_GAS_PRICE:
		v.uint256 = toEvmC(env.gasPrice);
		break;
	case EVM_COINBASE:
		v.address = toEvmC(env.envInfo().author());
		break;
	case EVM_DIFFICULTY:
		v.uint256 = toEvmC(env.envInfo().difficulty());
		break;
	case EVM_GAS_LIMIT:
		v.uint256 = toEvmC(env.envInfo().gasLimit());
		break;
	case EVM_NUMBER:
		// TODO: Handle overflow / exception
		v.int64 = static_cast<int64_t>(env.envInfo().number());
		break;
	case EVM_TIMESTAMP:
		// TODO: Handle overflow / exception
		v.int64 = static_cast<int64_t>(env.envInfo().timestamp());
		break;
	case EVM_CODE_BY_ADDRESS:
	{
		auto addr = fromEvmC(_arg.address);
		auto &code = env.codeAt(addr);
		v.data = code.data();
		v.data_size = code.size();
		break;
	}
	case EVM_BALANCE:
	{
		auto addr = fromEvmC(_arg.address);
		v.uint256 = toEvmC(env.balance(addr));
		break;
	}
	case EVM_BLOCKHASH:
		v.hash256 = toEvmC(env.blockHash(_arg.int64));
		break;
	case EVM_SLOAD:
	{
		auto key = fromEvmC(_arg.uint256);
		v.uint256 = toEvmC(env.store(key));
		break;
	}
	}
	return v;
}

void evm_update(
	evm_env* _opaqueEnv,
	evm_update_key _key,
	evm_variant _arg1,
	evm_variant _arg2
) noexcept
{
	auto &env = *reinterpret_cast<ExtVMFace*>(_opaqueEnv);
	switch (_key)
	{
	case EVM_SSTORE:
	{
		auto index = fromEvmC(_arg1.uint256);
		auto value = fromEvmC(_arg2.uint256);
		if (value == 0 && env.store(index) != 0)                   // If delete
			env.sub.refunds += env.evmSchedule().sstoreRefundGas;  // Increase refund counter

		env.setStore(index, value);    // Interface uses native endianness
		break;
	}
	case EVM_LOG:
	{
		size_t numTopics = _arg2.data_size / sizeof(h256);
		h256 const* pTopics = reinterpret_cast<h256 const*>(_arg2.data);
		env.log({pTopics, pTopics + numTopics}, {_arg1.data, _arg1.data_size});
		break;
	}
	case EVM_SELFDESTRUCT:
		// Register selfdestruction beneficiary.
		env.suicide(fromEvmC(_arg1.address));
		break;
	}
}

int64_t evm_call(
	evm_env* _opaqueEnv,
	evm_call_kind _kind,
	int64_t _gas,
	evm_hash160 _address,
	evm_uint256 _value,
	uint8_t const* _inputData,
	size_t _inputSize,
	uint8_t* _outputData,
	size_t _outputSize
) noexcept
{
	assert(_gas >= 0 && "Invalid gas value");
	auto &env = *reinterpret_cast<ExtVMFace*>(_opaqueEnv);
	auto value = fromEvmC(_value);
	bytesConstRef input{_inputData, _inputSize};

	if (_kind == EVM_CREATE)
	{
		assert(_outputSize == 20);
		if (env.depth >= 1024 || env.balance(env.myAddress) < value)
			return EVM_EXCEPTION;
		u256 gas = _gas;
		auto addr = env.create(value, gas, input, {});
		auto gasLeft = static_cast<decltype(_gas)>(gas);
		auto finalCost = _gas - gasLeft;
		if (addr)
			std::memcpy(_outputData, addr.data(), 20);
		else
			finalCost |= EVM_EXCEPTION;
		return finalCost;
	}

	CallParameters params;
	auto gas = _gas;
	auto cost = gas;

	params.apparentValue = _kind == EVM_DELEGATECALL ? env.value : value;
	params.valueTransfer = _kind == EVM_DELEGATECALL ? 0 : params.apparentValue;
	params.senderAddress = _kind == EVM_DELEGATECALL ? env.caller : env.myAddress;
	params.codeAddress = fromEvmC(_address);
	params.receiveAddress = _kind == EVM_CALL ? params.codeAddress : env.myAddress;
	params.data = input;
	params.out = {_outputData, _outputSize};
	params.onOp = {};

	if (params.valueTransfer)
	{
		gas += 2300;
		cost += 9000;
	}
	if (_kind == EVM_CALL && !env.exists(params.receiveAddress))
		cost += 25000;

	auto ret = false;
	if (env.depth < 1024 && env.balance(env.myAddress) >= params.valueTransfer)
	{
		params.gas = gas;
		ret = env.call(params);
		gas = static_cast<decltype(_gas)>(params.gas);  // Should not throw.
	}

	cost -= gas;

	// Saturate cost.
	// TODO: Move up and handle each +=.
	if (cost < 0)
		cost = std::numeric_limits<decltype(cost)>::max();

	// Add failure indicator.
	if (!ret)
		cost |= EVM_EXCEPTION;

	return cost;
}


/// RAII wrapper for an evm instance.
class EVM
{
public:
	EVM(evm_query_fn _queryFn, evm_update_fn _updateFn, evm_call_fn _callFn):
		m_instance(evm_create(_queryFn, _updateFn, _callFn))
	{}

	~EVM()
	{
		evm_destroy(m_instance);
	}

	EVM(EVM const&) = delete;
	EVM& operator=(EVM) = delete;

	class Result
	{
	public:
		Result(evm_result const& _result):
			m_result(_result)
		{}

		~Result()
		{
			evm_release_result_resources(&m_result);
		}

		Result(Result&& _other):
			m_result(_other.m_result)
		{
			// FIXME: It is not perfect as we must know what will be released
			//        by evm_release_result_resources().
			_other.m_result.internal_memory = nullptr;
		}

		Result(Result const&) = delete;
		Result& operator=(Result const&) = delete;

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
		auto mode = _ext.evmSchedule().haveDelegateCall ? EVM_HOMESTEAD
		                                                : EVM_FRONTIER;
		return evm_execute(
			m_instance, env, mode, toEvmC(_ext.codeHash), _ext.code.data(),
			_ext.code.size(), gas, _ext.data.data(), _ext.data.size(),
			toEvmC(_ext.value)
		);
	}

	bool isCodeReady(evm_mode _mode, h256 _codeHash)
	{
		return evmjit_is_code_ready(m_instance, _mode, toEvmC(_codeHash));
	}

	void compile(evm_mode _mode, bytesConstRef _code, h256 _codeHash)
	{
		evmjit_compile(
			m_instance, _mode, _code.data(), _code.size(), toEvmC(_codeHash)
		);
	}

private:
	evm_instance* m_instance = nullptr;
};

EVM& getJit()
{
	// Create EVM JIT instance by using EVM-C interface.
	static EVM jit(evm_query, evm_update, evm_call);
	return jit;
}

}

bytesConstRef JitVM::execImpl(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp)
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
		m_fallbackVM = VMFactory::create(VMKind::Interpreter);
		return m_fallbackVM->execImpl(io_gas, _ext, _onOp);
	}

	auto gas = static_cast<int64_t>(io_gas);
	auto r = getJit().execute(_ext, gas);
	if (r.gasLeft() < 0)
		BOOST_THROW_EXCEPTION(OutOfGas());

	io_gas = r.gasLeft();
	// FIXME: Copy the output for now, but copyless version possible.
	auto output = r.output();
	m_output.assign(output.begin(), output.end());
	return {m_output.data(), m_output.size()};
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
