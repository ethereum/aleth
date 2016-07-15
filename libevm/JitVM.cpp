
#pragma GCC diagnostic ignored "-Wconversion"

#include "JitVM.h"

#include <libdevcore/Log.h>
#include <libevm/VM.h>
#include <libevm/VMFactory.h>

#include "JitUtils.h"

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

evm_hash160 toEvmC(Address _addr)
{
	return *reinterpret_cast<evm_hash160*>(&_addr);
}

Address fromEvmC(evm_hash160 _addr)
{
	return *reinterpret_cast<Address*>(&_addr);
}

static_assert(sizeof(h256) == sizeof(evm_hash256),
              "Hash types size mismatch");
static_assert(alignof(h256) != alignof(evm_hash256),
              "Hash types alignment match -- update implementation");

evm_hash256 toEvmC(h256 _h)
{
	// Because the h256 only pretends to be aligned to 8 bytes we have to do
	// non-aligned memory copy.
	// TODO: h256 should be aligned to 8 bytes.
	evm_hash256 g;
	std::memcpy(&g, &_h, sizeof(g));
	return g;
}

evm_uint256 toEvmC(u256 _n)
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

u256 fromEvmC(evm_uint256 _n)
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

evm_variant evm_query(evm_env* _opaqueEnv, evm_query_key _key,
                      evm_variant _arg) noexcept
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
		v.bytes.bytes = reinterpret_cast<char const*>(code.data());
		v.bytes.size = code.size();
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
	case EVM_STORAGE:
	{
		auto key = fromEvmC(_arg.uint256);
		v.uint256 = toEvmC(env.store(key));
		break;
	}
	}
	return v;
}

void evm_update(evm_env* _opaqueEnv, evm_update_key _key,
				evm_variant _arg1, evm_variant _arg2) noexcept
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
		bytesConstRef data{reinterpret_cast<byte const*>(_arg1.bytes.bytes), _arg1.bytes.size};
		size_t numTopics = _arg2.bytes.size / sizeof(h256);
		h256 const* pTopics = reinterpret_cast<h256 const*>(_arg2.bytes.bytes);
		env.log({pTopics, pTopics + numTopics}, data);
		break;
	}
	case EVM_SELFDESTRUCT:
		// Register selfdestruction beneficiary.
		env.suicide(fromEvmC(_arg1.address));
		break;
	}
}

int64_t evm_call(evm_env* _opaqueEnv,
                 evm_call_kind _kind,
                 int64_t _gas,
                 evm_hash160 _address,
                 evm_uint256 _value,
				 char const* _inputData,
				 size_t _inputSize,
                 char* _outputData,
				 size_t _outputSize) noexcept
{
	assert(_gas >= 0 && "Invalid gas value");
	auto &env = *reinterpret_cast<ExtVMFace*>(_opaqueEnv);

	CallParameters params;
	auto gas = _gas;
	auto cost = gas;

	params.apparentValue = _kind == EVM_DELEGATECALL ? env.value : fromEvmC(_value);
	params.valueTransfer = _kind == EVM_DELEGATECALL ? 0 : params.apparentValue;
	params.senderAddress = _kind == EVM_DELEGATECALL ? env.caller : env.myAddress;
	params.codeAddress = fromEvmC(_address);
	params.receiveAddress = _kind == EVM_CALL ? params.codeAddress : env.myAddress;
	params.data = {reinterpret_cast<byte const*>(_inputData), _inputSize};
	params.out = {reinterpret_cast<byte*>(_outputData), _outputSize};
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
		gas = static_cast<int64_t>(params.gas);  // Should not throw.
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

}

extern "C" void env_create(); // fake declaration for linker symbol stripping workaround, see a call below

bytesConstRef JitVM::execImpl(u256& io_gas, ExtVMFace& _ext, OnOpFunc const& _onOp)
{
	evmjit::JIT::init(evm_query, evm_update, evm_call);

	auto rejected = false;
	// TODO: Rejecting transactions with gas limit > 2^63 can be used by attacker to take JIT out of scope
	rejected |= io_gas > std::numeric_limits<decltype(m_data.gas)>::max(); // Do not accept requests with gas > 2^63 (int64 max)
	rejected |= _ext.envInfo().number() > std::numeric_limits<int64_t>::max();
	rejected |= _ext.envInfo().timestamp() > std::numeric_limits<int64_t>::max();
	rejected |= _ext.envInfo().gasLimit() > std::numeric_limits<int64_t>::max();
	if (!toJITSchedule(_ext.evmSchedule(), m_schedule))
	{
		cwarn << "Schedule changed, not suitable for JIT!";
		rejected = true;
	}
	if (rejected)
	{
		cwarn << "Execution rejected by EVM JIT (gas limit: " << io_gas << "), executing with interpreter";
		m_fallbackVM = VMFactory::create(VMKind::Interpreter);
		return m_fallbackVM->execImpl(io_gas, _ext, _onOp);
	}

	m_data.gas 			= static_cast<decltype(m_data.gas)>(io_gas);
	m_data.callData 	= _ext.data.data();
	m_data.callDataSize = _ext.data.size();
	m_data.transferredValue = eth2jit(_ext.value);
	m_data.apparentValue = eth2jit(_ext.value);
	m_data.code     	= _ext.code.data();
	m_data.codeSize 	= _ext.code.size();
	m_data.codeHash		= eth2jit(_ext.codeHash);

	// Pass pointer to ExtVMFace casted to evmjit::Env* opaque type.
	// JIT will do nothing with the pointer, just pass it to Env callback functions implemented in Env.cpp.
	m_context.init(m_data, reinterpret_cast<evmjit::Env*>(&_ext));
	auto exitCode = evmjit::JIT::exec(m_context, m_schedule);
	switch (exitCode)
	{
	case evmjit::ReturnCode::OutOfGas:
		BOOST_THROW_EXCEPTION(OutOfGas());
	case evmjit::ReturnCode::LinkerWorkaround:	// never happens
		env_create();					// but forces linker to include env_* JIT callback functions
		break;
	default:
		break;
	}

	io_gas = m_data.gas;
	return {std::get<0>(m_context.returnData), std::get<1>(m_context.returnData)};
}

}
}
