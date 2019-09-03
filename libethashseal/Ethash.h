/*
    This file is part of aleth.

    aleth is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    aleth is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with aleth.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file
 * A proof of work algorithm.
 */

#pragma once

#include "EthashProofOfWork.h"
#include <libethcore/SealEngine.h>
#include <libethereum/Client.h>
#include <libethereum/GenericFarm.h>

#include <ethash/ethash.hpp>

namespace dev
{

namespace eth
{
inline ethash::hash256 toEthash(h256 const& hash) noexcept
{
    return ethash::hash256_from_bytes(hash.data());
}

inline uint64_t toEthash(Nonce const& nonce) noexcept
{
    return static_cast<uint64_t>(static_cast<u64>(nonce));
}

class Ethash: public SealEngineBase
{
public:
    Ethash();
    ~Ethash();

    static std::string name() { return "Ethash"; }
    unsigned revision() const override { return 1; }
    unsigned sealFields() const override { return 2; }
    bytes sealRLP() const override { return rlp(h256()) + rlp(Nonce()); }

    StringHashMap jsInfo(BlockHeader const& _bi) const override;
    void verify(Strictness _s, BlockHeader const& _bi, BlockHeader const& _parent, bytesConstRef _block) const override;
    void verifyTransaction(ImportRequirements::value _ir, TransactionBase const& _t, BlockHeader const& _header, u256 const& _startGasUsed) const override;
    void populateFromParent(BlockHeader& _bi, BlockHeader const& _parent) const override;

    strings sealers() const override;
    std::string sealer() const override { return m_sealer; }
    void setSealer(std::string const& _sealer) override { m_sealer = _sealer; }
    void cancelGeneration() override { m_farm.stop(); }
    void generateSeal(BlockHeader const& _bi) override;
    bool shouldSeal(Interface* _i) override;

    /// Update to the latest transactions and get hash of the current block to be mined minus the
    /// nonce (the 'work hash') and the difficulty to be met.
    /// @returns Tuple of hash without seal, seed hash, target boundary.
    std::tuple<h256, h256, h256> getWork(BlockHeader const& _bi) override;
    bool isMining() const override;

    static h256 seedHash(BlockHeader const& _bi);
    static Nonce nonce(BlockHeader const& _bi) { return _bi.seal<Nonce>(NonceField); }
    static h256 mixHash(BlockHeader const& _bi) { return _bi.seal<h256>(MixHashField); }

    static h256 boundary(BlockHeader const& _bi)
    {
        auto const& d = _bi.difficulty();
        return d > 1 ? h256{u256((u512(1) << 256) / d)} : ~h256{};
    }

    static BlockHeader& setNonce(BlockHeader& _bi, Nonce _v) { _bi.setSeal(NonceField, _v); return _bi; }
    static BlockHeader& setMixHash(BlockHeader& _bi, h256 const& _v) { _bi.setSeal(MixHashField, _v); return _bi; }

    static void init();

    /// The hashrate...
    u256 hashrate() const;

    /// Check the progress of the mining.
    WorkingProgress miningProgress() const;

    /// @brief Submit the proof for the proof-of-work.
    /// @param _s A valid solution.
    /// @return true if the solution was indeed valid and accepted.
    bool submitEthashWork(h256 const& _mixHash, h64 const& _nonce);

    void submitExternalHashrate(u256 const& _rate, h256 const& _id);

private:
    bool verifySeal(BlockHeader const& _blockHeader) const;
    bool quickVerifySeal(BlockHeader const& _blockHeader) const;
    u256 externalHashrate() const;
    void manuallySetWork(BlockHeader const& _work) { m_sealing = _work; }
    void manuallySubmitWork(h256 const& _mixHash, Nonce _nonce);

    eth::GenericFarm<EthashProofOfWork> m_farm;
    std::string m_sealer = "cpu";
    BlockHeader m_sealing;

    /// A mutex covering m_sealing
    Mutex m_submitLock;

    // external hashrate
    mutable std::unordered_map<h256, std::pair<u256, std::chrono::steady_clock::time_point>>
        m_externalRates;
    mutable SharedMutex x_externalRates;
};

DEV_SIMPLE_EXCEPTION(InvalidSealEngine);
Ethash& asEthash(SealEngineFace& _f);
}
}
