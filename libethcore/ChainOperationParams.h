// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#pragma once

#include <libdevcore/Common.h>
#include <libethcore/Precompiled.h>

#include "Common.h"
#include "EVMSchedule.h"

namespace dev
{
namespace eth
{

class PrecompiledContract
{
public:
    PrecompiledContract() = default;
    PrecompiledContract(
        PrecompiledPricer const& _cost,
        PrecompiledExecutor const& _exec,
        u256 const& _startingBlock = 0
    ):
        m_cost(_cost),
        m_execute(_exec),
        m_startingBlock(_startingBlock)
    {}
    PrecompiledContract(
        unsigned _base,
        unsigned _word,
        PrecompiledExecutor const& _exec,
        u256 const& _startingBlock = 0
    );

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

struct ChainOperationParams
{
    ChainOperationParams();

    explicit operator bool() const { return accountStartNonce != Invalid256; }

    /// The chain sealer name: e.g. Ethash, NoProof, BasicAuthority
    std::string sealEngineName = "NoProof";

    /// General chain params.
private:
    u256 m_blockReward;
public:
    EVMSchedule const& scheduleForBlockNumber(u256 const& _blockNumber) const;
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
    u256 berlinForkBlock = c_infiniteBlockNumber;
    int chainID = 0;    // Distinguishes different chains (mainnet, Ropsten, etc).
    int networkID = 0;  // Distinguishes different sub protocols.

    u256 minimumDifficulty;
    u256 difficultyBoundDivisor;
    u256 durationLimit;
    bool allowFutureBlocks = false;

    /// Precompiled contracts as specified in the chain params.
    std::unordered_map<Address, PrecompiledContract> precompiled;
};

}
}
