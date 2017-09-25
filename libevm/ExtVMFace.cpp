/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ExtVMFace.h"

namespace dev
{
namespace eth
{
namespace
{

static_assert(sizeof(Address) == sizeof(evm_uint160be), "Address types size mismatch");
static_assert(alignof(Address) == alignof(evm_uint160be), "Address types alignment mismatch");

inline Address fromEvmC(evm_uint160be const& _addr)
{
	return reinterpret_cast<Address const&>(_addr);
}

static_assert(sizeof(h256) == sizeof(evm_uint256be), "Hash types size mismatch");
static_assert(alignof(h256) == alignof(evm_uint256be), "Hash types alignment mismatch");

inline u256 fromEvmC(evm_uint256be const& _n)
{
	return fromBigEndian<u256>(_n.bytes);
}

int accountExists(evm_context* _context, evm_uint160be const* _addr) noexcept
{
	auto& env = static_cast<ExtVMFace&>(*_context);
	Address addr = fromEvmC(*_addr);
	return env.exists(addr) ? 1 : 0;
}

void getStorage(
	evm_uint256be* o_result,
	evm_context* _context,
	evm_uint160be const* _addr,
	evm_uint256be const* _key
) noexcept
{
	(void) _addr;
	auto& env = static_cast<ExtVMFace&>(*_context);
	assert(fromEvmC(*_addr) == env.myAddress);
	u256 key = fromEvmC(*_key);
	*o_result = toEvmC(env.store(key));
}

void setStorage(
	evm_context* _context,
	evm_uint160be const* _addr,
	evm_uint256be const* _key,
	evm_uint256be const* _value
) noexcept
{
	(void) _addr;
	auto& env = static_cast<ExtVMFace&>(*_context);
	assert(fromEvmC(*_addr) == env.myAddress);
	u256 index = fromEvmC(*_key);
	u256 value = fromEvmC(*_value);
	if (value == 0 && env.store(index) != 0)                   // If delete
		env.sub.refunds += env.evmSchedule().sstoreRefundGas;  // Increase refund counter

	env.setStore(index, value);    // Interface uses native endianness
}

void getBalance(
	evm_uint256be* o_result,
	evm_context* _context,
	evm_uint160be const* _addr
) noexcept
{
	auto& env = static_cast<ExtVMFace&>(*_context);
	*o_result = toEvmC(env.balance(fromEvmC(*_addr)));
}

size_t getCode(byte const** o_code, evm_context* _context, evm_uint160be const* _addr)
{
	auto& env = static_cast<ExtVMFace&>(*_context);
	Address addr = fromEvmC(*_addr);
	if (o_code != nullptr)
	{
		auto& code = env.codeAt(addr);
		*o_code = code.data();
		return code.size();
	}
	return env.codeSizeAt(addr);
}

void selfdestruct(
	evm_context* _context,
	evm_uint160be const* _addr,
	evm_uint160be const* _beneficiary
) noexcept
{
	(void) _addr;
	auto& env = static_cast<ExtVMFace&>(*_context);
	assert(fromEvmC(*_addr) == env.myAddress);
	env.suicide(fromEvmC(*_beneficiary));
}


void log(
	evm_context* _context,
	evm_uint160be const* _addr,
	uint8_t const* _data,
	size_t _dataSize,
	evm_uint256be const _topics[],
	size_t _numTopics
) noexcept
{
	(void) _addr;
	auto& env = static_cast<ExtVMFace&>(*_context);
	assert(fromEvmC(*_addr) == env.myAddress);
	h256 const* pTopics = reinterpret_cast<h256 const*>(_topics);
	env.log(h256s{pTopics, pTopics + _numTopics},
			bytesConstRef{_data, _dataSize});
}

void getTxContext(evm_tx_context* result, evm_context* _context) noexcept
{
	auto& env = static_cast<ExtVMFace&>(*_context);
	result->tx_gas_price = toEvmC(env.gasPrice);
	result->tx_origin = toEvmC(env.origin);
	result->block_coinbase = toEvmC(env.envInfo().author());
	result->block_number = static_cast<int64_t>(env.envInfo().number());
	result->block_timestamp = static_cast<int64_t>(env.envInfo().timestamp());
	result->block_gas_limit = static_cast<int64_t>(env.envInfo().gasLimit());
	result->block_difficulty = toEvmC(env.envInfo().difficulty());
}

void getBlockHash(evm_uint256be* o_hash, evm_context* _envPtr, int64_t _number)
{
	auto& env = static_cast<ExtVMFace&>(*_envPtr);
	*o_hash = toEvmC(env.blockHash(_number));
}

void create(evm_result* o_result, ExtVMFace& _env, evm_message const* _msg) noexcept
{
	u256 gas = _msg->gas;
	u256 value = fromEvmC(_msg->value);
	bytesConstRef init = {_msg->input, _msg->input_size};
	// ExtVM::create takes the sender address from .myAddress.
	assert(fromEvmC(_msg->sender) == _env.myAddress);

	h160 addr;
	owning_bytes_ref output;
	std::tie(addr, output) = _env.create(value, gas, init, Instruction::CREATE,
										 u256(0), {});
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
	} else
	{
		o_result->code = EVM_REVERT;

		// Pass the output to the EVM without a copy. The EVM will delete it
		// when finished with it.

		// First assign reference. References are not invalidated when vector
		// of bytes is moved. See `.takeBytes()` below.
		o_result->output_data = output.data();
		o_result->output_size = output.size();

		// Place a new vector of bytes containing output in result's reserved memory.
		static_assert(sizeof(bytes) <= sizeof(o_result->reserved),
					  "Vector is too big");
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
}

void call(evm_result* o_result, evm_context* _context, evm_message const* _msg) noexcept
{
	assert(_msg->gas >= 0 && "Invalid gas value");
	auto& env = static_cast<ExtVMFace&>(*_context);

	// Handle CREATE separately.
	if (_msg->kind == EVM_CREATE)
		return create(o_result, env, _msg);

	CallParameters params;
	params.gas = _msg->gas;
	params.apparentValue = fromEvmC(_msg->value);
	params.valueTransfer =
		_msg->kind == EVM_DELEGATECALL ? 0 : params.apparentValue;
	params.senderAddress = fromEvmC(_msg->sender);
	params.codeAddress = fromEvmC(_msg->address);
	params.receiveAddress =
		_msg->kind == EVM_CALL ? params.codeAddress : env.myAddress;
	params.data = {_msg->input, _msg->input_size};
	params.staticCall = (_msg->flags & EVM_STATIC) != 0;
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
	static_assert(sizeof(bytes) <= sizeof(o_result->reserved),
				  "Vector is too big");
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

evm_host const fnTable = {
	accountExists,
	getStorage,
	setStorage,
	getBalance,
	getCode,
	selfdestruct,
	eth::call,
	getTxContext,
	getBlockHash,
	eth::log
};

}

ExtVMFace::ExtVMFace(
	EnvInfo const& _envInfo,
	Address _myAddress,
	Address _caller,
	Address _origin,
	u256 _value,
	u256 _gasPrice,
	bytesConstRef _data,
	bytes _code,
	h256 const& _codeHash,
	unsigned _depth,
	bool _staticCall
):
	evm_context{&fnTable},
	m_envInfo(_envInfo),
	myAddress(_myAddress),
	caller(_caller),
	origin(_origin),
	value(_value),
	gasPrice(_gasPrice),
	data(_data),
	code(std::move(_code)),
	codeHash(_codeHash),
	depth(_depth),
	staticCall(_staticCall)
{}

}
}
