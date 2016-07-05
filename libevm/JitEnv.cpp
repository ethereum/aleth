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
}
