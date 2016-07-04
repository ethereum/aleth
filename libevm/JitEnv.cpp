#include <libevm/ExtVMFace.h>

#include "JitUtils.h"

extern "C"
{
	#ifdef _MSC_VER
		#define EXPORT __declspec(dllexport)
		#pragma warning(disable: 4190)  // MSVC 2013 complains about using i256 type on extern "C" functions.
	#else
		#define EXPORT
	#endif

	using namespace dev;
	using namespace dev::eth;
	using evmjit::i256;

	EXPORT void env_create(ExtVMFace* _env, int64_t* io_gas, i256* _endowment, byte* _initBeg, uint64_t _initSize, h256* o_address)
	{
		auto endowment = jit2eth(*_endowment);
		if (_env->balance(_env->myAddress) >= endowment && _env->depth < 1024)
		{
			u256 gas = *io_gas;
			h256 address(_env->create(endowment, gas, {_initBeg, (size_t)_initSize}, {}), h256::AlignRight);
			*io_gas = static_cast<int64_t>(gas);
			*o_address = address;
		}
		else
			*o_address = {};
	}

	EXPORT bool env_call(ExtVMFace* _env, int64_t* io_gas, int64_t _callGas, h256* _senderAddress, h256* _receiveAddress, h256* _codeAddress, i256* _valueTransfer, i256* _apparentValue, byte* _inBeg, uint64_t _inSize, byte* _outBeg, uint64_t _outSize)
	{
		EVMSchedule const& schedule = _env->evmSchedule();

		CallParameters params;

		params.apparentValue = jit2eth(*_apparentValue);
		params.valueTransfer = jit2eth(*_valueTransfer);

		params.senderAddress = right160(*_senderAddress);
		params.receiveAddress = right160(*_receiveAddress);
		params.codeAddress = right160(*_codeAddress);
		params.data = {_inBeg, (size_t)_inSize};
		params.out = {_outBeg, (size_t)_outSize};
		params.onOp = {};
		// We can have params.receiveAddress == params.codeAddress although it is not a call,
		// but in this case, the results are the same (see usage of isCall below).
		const auto isCall = params.receiveAddress == params.codeAddress; // OPT: The same address pointer can be used if not CODECALL

		*io_gas -= _callGas;
		if (*io_gas < 0)
			return false;

		if (isCall && !_env->exists(params.receiveAddress))
			*io_gas -= static_cast<int64_t>(schedule.callNewAccountGas); // no underflow, *io_gas non-negative before

		if (params.valueTransfer > 0)
		{
			/*static*/ assert(schedule.callValueTransferGas > schedule.callStipend && "Overflow possible");
			*io_gas -= static_cast<int64_t>(schedule.callValueTransferGas); // no underflow
			_callGas += static_cast<int64_t>(schedule.callStipend); // overflow possibility, but in the same time *io_gas < 0
		}

		if (*io_gas < 0)
			return false;

		auto ret = false;
		params.gas = u256{_callGas};
		if (_env->balance(_env->myAddress) >= params.valueTransfer && _env->depth < 1024)
			ret = _env->call(params);

		*io_gas += static_cast<int64_t>(params.gas); // it is never more than initial _callGas
		return ret;
	}

	EXPORT void env_log(ExtVMFace* _env, byte* _beg, uint64_t _size, h256* _topic1, h256* _topic2, h256* _topic3, h256* _topic4)
	{
		dev::h256s topics;

		if (_topic1)
			topics.push_back(*_topic1);

		if (_topic2)
			topics.push_back(*_topic2);

		if (_topic3)
			topics.push_back(*_topic3);

		if (_topic4)
			topics.push_back(*_topic4);

		_env->log(std::move(topics), {_beg, (size_t)_size});
	}
}
