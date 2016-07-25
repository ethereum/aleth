#pragma once

#include <evmjit/JIT.h>
#include <libevmcore/EVMSchedule.h>

namespace dev
{
namespace eth
{

/// Converts eth type dev::h256 to EVM JIT representation of 256-bit hash value.
inline evmjit::h256 eth2jit(h256 _u)
{
	/// Just directly copies memory
	return *(evmjit::h256*)&_u;
}

/// Converts an EVMSchedule to a JITSchedule and returns false if this is not possible.
inline bool toJITSchedule(EVMSchedule const& _schedule, evmjit::JITSchedule& o_schedule)
{
	using namespace evmjit;
	if (_schedule.tierStepGas.size() != 8) return false;
	if (_schedule.tierStepGas[0] != JITSchedule::stepGas0::value) return false;
	if (_schedule.tierStepGas[1] != JITSchedule::stepGas1::value) return false;
	if (_schedule.tierStepGas[2] != JITSchedule::stepGas2::value) return false;
	if (_schedule.tierStepGas[3] != JITSchedule::stepGas3::value) return false;
	if (_schedule.tierStepGas[4] != JITSchedule::stepGas4::value) return false;
	if (_schedule.tierStepGas[5] != JITSchedule::stepGas5::value) return false;
	if (_schedule.tierStepGas[6] != JITSchedule::stepGas6::value) return false;
	if (_schedule.tierStepGas[7] != JITSchedule::stepGas7::value) return false;
	if (_schedule.stackLimit != JITSchedule::stackLimit::value) return false;
	if (_schedule.expByteGas != JITSchedule::expByteGas::value) return false;
	if (_schedule.sha3Gas != JITSchedule::sha3Gas::value) return false;
	if (_schedule.sha3WordGas != JITSchedule::sha3WordGas::value) return false;
	if (_schedule.sloadGas != JITSchedule::sloadGas::value) return false;
	if (_schedule.sstoreSetGas != JITSchedule::sstoreSetGas::value) return false;
	if (_schedule.sstoreResetGas != JITSchedule::sstoreResetGas::value) return false;
	if (bigint(_schedule.sstoreSetGas) - _schedule.sstoreRefundGas != JITSchedule::sstoreClearGas::value) return false;
	if (_schedule.jumpdestGas != JITSchedule::jumpdestGas::value) return false;
	if (_schedule.logGas != JITSchedule::logGas::value) return false;
	if (_schedule.logDataGas != JITSchedule::logDataGas::value) return false;
	if (_schedule.logTopicGas != JITSchedule::logTopicGas::value) return false;
	if (_schedule.createGas != JITSchedule::createGas::value) return false;
	if (_schedule.callGas != JITSchedule::callGas::value) return false;
	if (_schedule.copyGas != JITSchedule::copyGas::value) return false;
	o_schedule.haveDelegateCall = _schedule.haveDelegateCall;
	return true;
}

}
}
