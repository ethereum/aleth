// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#pragma once

#include <libdevcore/Common.h>
#include <libethcore/EVMSchedule.h>
#include <libethcore/Precompiled.h>

#include "Common.h"

namespace dev
{
namespace eth
{
struct EVMSchedule;

class PrecompiledContract
{
public:
    PrecompiledContract(std::string const& _name, u256 const& _startingBlock = 0);

    bigint cost(
        bytesConstRef _in, ChainOperationParams const& _chainParams, u256 const& _blockNumber) const
    {
        return m_cost(_in, _chainParams, _blockNumber);
    }
    std::pair<bool, bytes> execute(bytesConstRef _in) const { return m_execute(_in); }

    u256 const& startingBlock() const { return m_startingBlock; }

private:
    PrecompiledPricer m_cost;
    PrecompiledExecutor m_execute;
    u256 m_startingBlock = 0;
};

constexpr int64_t c_infiniteBlockNumber = std::numeric_limits<int64_t>::max();

struct AdditionalEIPs
{
    bool eip1380 = false;
    bool eip2046 = false;
};

struct ChainOperationParams
{
    ChainOperationParams();

    explicit operator bool() const { return accountStartNonce != Invalid256; }

    /// The chain sealer name: e.g. Ethash, NoProof, BasicAuthority
    std::string sealEngineName = "NoProof";

    // Example of how to check EIP activation from outside of EVM:
    // bool isEIP2046Enabled(u256 const& _blockNumber) const
    // {
    //     return _blockNumber >= lastForkBlock && lastForkAdditionalEIPs.eip2046;
    // }
    // After hard fork finalization this is changed to:
    // bool isEIP2046Enabled(u256 const& _blockNumber) const
    // {
    //     return _blockNumber >= berlinForkBlock;
    // }

    /// General chain params.
private:
    u256 m_blockReward;

public:
    // returns schedule for the fork active at the given block
    // may include additional individually activated EIPs on top of the last fork block
    EVMSchedule const& scheduleForBlockNumber(u256 const& _blockNumber) const;
    // returns schedule according to the the fork rules active at the given block
    // doesn't include additional individually activated EIPs
    EVMSchedule const& forkScheduleForBlockNumber(u256 const& _blockNumber) const;
    u256 blockReward(EVMSchedule const& _schedule) const;
    void setBlockReward(u256 const& _newBlockReward);
    u256 maximumExtraDataSize = 32;
    u256 accountStartNonce = 0;
    bool tieBreakingGas = true;
    u256 minGasLimit;
    u256 maxGasLimit;
    u256 gasLimitBoundDivisor;
    u256 homesteadForkBlock = c_infiniteBlockNumber;
    u256 EIP150ForkBlock = c_infiniteBlockNumber;
    u256 EIP158ForkBlock = c_infiniteBlockNumber;
    u256 byzantiumForkBlock = c_infiniteBlockNumber;
    u256 eWASMForkBlock = c_infiniteBlockNumber;
    u256 constantinopleForkBlock = c_infiniteBlockNumber;
    u256 constantinopleFixForkBlock = c_infiniteBlockNumber;
    u256 daoHardforkBlock = c_infiniteBlockNumber;
    u256 experimentalForkBlock = c_infiniteBlockNumber;
    u256 istanbulForkBlock = c_infiniteBlockNumber;
    u256 muirGlacierForkBlock = c_infiniteBlockNumber;
    u256 berlinForkBlock = c_infiniteBlockNumber;
    u256 lastForkBlock = c_infiniteBlockNumber;
    AdditionalEIPs lastForkAdditionalEIPs;
    int chainID = 0;    // Distinguishes different chains (mainnet, Ropsten, etc).
    int networkID = 0;  // Distinguishes different sub protocols.

    u256 minimumDifficulty;
    u256 difficultyBoundDivisor;
    u256 durationLimit;
    bool allowFutureBlocks = false;

    /// Precompiled contracts as specified in the chain params.
    std::unordered_map<Address, PrecompiledContract> precompiled;

    EVMSchedule lastForkWithAdditionalEIPsSchedule;
};

}
}
