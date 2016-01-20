#pragma once

#include <evmjit/JIT.h>
#include <libevmcore/EVMSchedule.h>

namespace dev
{
namespace eth
{

/// Converts EVM JIT representation of 256-bit integer to eth type dev::u256.
inline u256 jit2eth(evmjit::i256 _i)
{
	u256 u = _i.words[3];
	u <<= 64;
	u |= _i.words[2];
	u <<= 64;
	u |= _i.words[1];
	u <<= 64;
	u |= _i.words[0];
	return u;
}

/// Converts eth type dev::u256 to EVM JIT representation of 256-bit integer.
inline evmjit::i256 eth2jit(u256 _u)
{
	evmjit::i256 i;
	i.words[0] = static_cast<uint64_t>(_u);
	_u >>= 64;
	i.words[1] = static_cast<uint64_t>(_u);
	_u >>= 64;
	i.words[2] = static_cast<uint64_t>(_u);
	_u >>= 64;
	i.words[3] = static_cast<uint64_t>(_u);
	return i;
}

/// Converts eth type dev::h256 to EVM JIT representation of 256-bit hash value.
inline evmjit::h256 eth2jit(h256 _u)
{
	/// Just directly copies memory
	return *(evmjit::h256*)&_u;
}

/// Converts an EVMSchedule to a JITSchedule and returns false if this is not possible.
inline bool toJITSchedule(EVMSchedule const& _schedule, evmjit::JITSchedule& o_schedule)
{
	if (_schedule.tierStepGas.size() != 8) return false;
	for (size_t i = 0; i < _schedule.tierStepGas.size(); ++i)
		if (_schedule.tierStepGas[i] > 20) return false;
		else o_schedule.stepGas[i] = int64_t(_schedule.tierStepGas[i]);

	if (_schedule.stackLimit > 1024) return false;
	o_schedule.stackLimit = int64_t(_schedule.stackLimit);
	if (_schedule.expByteGas > 10) return false;
	o_schedule.expByteGas = int64_t(_schedule.expByteGas);
	if (_schedule.sha3Gas > 30) return false;
	o_schedule.sha3Gas = int64_t(_schedule.sha3Gas);
	if (_schedule.sha3WordGas > 6) return false;
	o_schedule.sha3WordGas = int64_t(_schedule.sha3WordGas);
	if (_schedule.sloadGas > 50) return false;
	o_schedule.sloadGas = int64_t(_schedule.sloadGas);
	if (_schedule.sstoreSetGas > 20000) return false;
	o_schedule.sstoreSetGas = int64_t(_schedule.sstoreSetGas);
	if (_schedule.sstoreResetGas > 5000) return false;
	o_schedule.sstoreResetGas = int64_t(_schedule.sstoreResetGas);
	if (_schedule.sstoreSetGas < _schedule.sstoreRefundGas || _schedule.sstoreSetGas - _schedule.sstoreRefundGas > 5000) return false;
	o_schedule.sstoreClearGas = int64_t(_schedule.sstoreSetGas - _schedule.sstoreRefundGas);
	if (_schedule.jumpdestGas > 1) return false;
	o_schedule.jumpdestGas = int64_t(_schedule.jumpdestGas);
	if (_schedule.logGas > 375) return false;
	o_schedule.logGas = int64_t(_schedule.logGas);
	if (_schedule.logDataGas > 8) return false;
	o_schedule.logDataGas = int64_t(_schedule.logDataGas);
	if (_schedule.logTopicGas > 375) return false;
	o_schedule.logTopicGas = int64_t(_schedule.logTopicGas);
	if (_schedule.createGas > 32000) return false;
	o_schedule.createGas = int64_t(_schedule.createGas);
	if (_schedule.callGas > 40) return false;
	o_schedule.callGas = int64_t(_schedule.callGas);
	if (_schedule.copyGas > 3) return false;
	o_schedule.copyGas = int64_t(_schedule.copyGas);
	o_schedule.haveDelegateCall = _schedule.haveDelegateCall;
	return true;
}

}
}
