// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#include "ChainOperationParams.h"
#include <libdevcore/CommonData.h>
#include <libdevcore/Log.h>

using namespace std;
using namespace dev;
using namespace eth;

PrecompiledContract::PrecompiledContract(
    unsigned _base, unsigned _word, PrecompiledExecutor const& _exec, u256 const& _startingBlock)
  : PrecompiledContract(
        [=](bytesConstRef _in, ChainOperationParams const&, u256 const&) -> bigint {
            bigint s = _in.size();
            bigint b = _base;
            bigint w = _word;
            return b + (s + 31) / 32 * w;
        },
        _exec, _startingBlock)
{}

ChainOperationParams::ChainOperationParams():
    m_blockReward("0x4563918244F40000"),
    minGasLimit(0x1388),
    maxGasLimit("0x7fffffffffffffff"),
    gasLimitBoundDivisor(0x0400),
    networkID(0x0),
    minimumDifficulty(0x020000),
    difficultyBoundDivisor(0x0800),
    durationLimit(0x0d)
{
}

EVMSchedule const& ChainOperationParams::scheduleForBlockNumber(u256 const& _blockNumber) const
{
    if (_blockNumber >= lastForkBlock)
        return lastForkWithAdditionalEIPsSchedule;
    else
        return fixedScheduleForBlockNumber(_blockNumber);
}

EVMSchedule const& ChainOperationParams::fixedScheduleForBlockNumber(u256 const& _blockNumber) const
{
    if (_blockNumber >= experimentalForkBlock)
        return ExperimentalSchedule;
    else if (_blockNumber >= berlinForkBlock)
        return BerlinSchedule;
    else if (_blockNumber >= istanbulForkBlock)
        return IstanbulSchedule;
    else if (_blockNumber >= constantinopleFixForkBlock)
        return ConstantinopleFixSchedule;
    else if (_blockNumber >= constantinopleForkBlock)
        return ConstantinopleSchedule;
    else if (_blockNumber >= eWASMForkBlock)
        return EWASMSchedule;
    else if (_blockNumber >= byzantiumForkBlock)
        return ByzantiumSchedule;
    else if (_blockNumber >= EIP158ForkBlock)
        return EIP158Schedule;
    else if (_blockNumber >= EIP150ForkBlock)
        return EIP150Schedule;
    else if (_blockNumber >= homesteadForkBlock)
        return HomesteadSchedule;
    else
        return FrontierSchedule;
}

u256 ChainOperationParams::blockReward(EVMSchedule const& _schedule) const
{
    if (_schedule.blockRewardOverwrite)
        return *_schedule.blockRewardOverwrite;
    else
        return m_blockReward;
}

void ChainOperationParams::setBlockReward(u256 const& _newBlockReward)
{
    m_blockReward = _newBlockReward;
}
