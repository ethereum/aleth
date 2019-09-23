// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#pragma once

#include <functional>
#include <unordered_map>
#include <libdevcore/Guards.h>
#include <libdevcore/RLP.h>
#include "BlockHeader.h"
#include "Common.h"

namespace dev
{
namespace eth
{

class BlockHeader;
struct ChainOperationParams;
class Interface;
class PrecompiledFace;
class TransactionBase;
class EnvInfo;

class SealEngineFace
{
public:
    virtual ~SealEngineFace() {}

    /// @returns Tuple of hash of the current block to be mined minus nonce, seed hash, target boundary.
    virtual std::tuple<h256, h256, h256> getWork(BlockHeader const&)
    {
        return std::tuple<h256, h256, h256>{};
    }
    virtual bool isMining() const { return false; }
    virtual unsigned revision() const { return 0; }
    virtual unsigned sealFields() const { return 0; }
    virtual bytes sealRLP() const { return bytes(); }
    virtual StringHashMap jsInfo(BlockHeader const&) const { return StringHashMap(); }

    /// Don't forget to call Super::verify when subclassing & overriding.
    virtual void verify(Strictness _s, BlockHeader const& _bi, BlockHeader const& _parent = BlockHeader(), bytesConstRef _block = bytesConstRef()) const;
    /// Additional verification for transactions in blocks.
    virtual void verifyTransaction(ImportRequirements::value _ir, TransactionBase const& _t, BlockHeader const& _header, u256 const& _startGasUsed) const;
    /// Don't forget to call Super::populateFromParent when subclassing & overriding.
    virtual void populateFromParent(BlockHeader& _bi, BlockHeader const& _parent) const;

    bytes option(std::string const& _name) const { Guard l(x_options); return m_options.count(_name) ? m_options.at(_name) : bytes(); }
    bool setOption(std::string const& _name, bytes const& _value) { Guard l(x_options); try { if (onOptionChanging(_name, _value)) { m_options[_name] = _value; return true; } } catch (...) {} return false; }

    virtual strings sealers() const { return { "default" }; }
    virtual std::string sealer() const { return "default"; }
    virtual void setSealer(std::string const&) {}

    virtual bool shouldSeal(Interface*) { return true; }
    virtual void generateSeal(BlockHeader const& _bi) = 0;
    virtual void onSealGenerated(std::function<void(bytes const& s)> const& _f) = 0;
    virtual void cancelGeneration() {}

    ChainOperationParams const& chainParams() const { return m_params; }
    void setChainParams(ChainOperationParams const& _params) { m_params = _params; }
    SealEngineFace* withChainParams(ChainOperationParams const& _params) { setChainParams(_params); return this; }
    virtual EVMSchedule const& evmSchedule(u256 const& _blockNumber) const = 0;
    virtual u256 blockReward(u256 const& _blockNumber) const = 0;

    virtual bool isPrecompiled(Address const& _a, u256 const& _blockNumber) const
    {
        return m_params.precompiled.count(_a) != 0 && _blockNumber >= m_params.precompiled.at(_a).startingBlock();
    }
    virtual bigint costOfPrecompiled(
        Address const& _a, bytesConstRef _in, u256 const& _blockNumber) const
    {
        return m_params.precompiled.at(_a).cost(_in, m_params, _blockNumber);
    }
    virtual std::pair<bool, bytes> executePrecompiled(Address const& _a, bytesConstRef _in, u256 const&) const { return m_params.precompiled.at(_a).execute(_in); }

protected:
    virtual bool onOptionChanging(std::string const&, bytes const&) { return true; }

private:
    mutable Mutex x_options;
    std::unordered_map<std::string, bytes> m_options;

    ChainOperationParams m_params;
};

class SealEngineBase: public SealEngineFace
{
public:
    enum
    {
        MixHashField = 0,
        NonceField = 1
    };
    void onSealGenerated(std::function<void(bytes const&)> const& _f) override { m_onSealGenerated = _f; }
    EVMSchedule const& evmSchedule(u256 const& _blockNumber) const override;
    u256 blockReward(u256 const& _blockNumber) const override;

protected:
    std::function<void(bytes const& s)> m_onSealGenerated;
};

using SealEngineFactory = std::function<SealEngineFace*()>;

class SealEngineRegistrar
{
public:
    /// Creates the seal engine and uses it to "polish" the params (i.e. fill in implicit values) as necessary. Use this rather than the other two
    /// unless you *know* that the params contain all information regarding the seal on the Genesis block.
    static SealEngineFace* create(ChainOperationParams const& _params);
    static SealEngineFace* create(std::string const& _name) { if (!get()->m_sealEngines.count(_name)) return nullptr; return get()->m_sealEngines[_name](); }

    template <class SealEngine> static SealEngineFactory registerSealEngine(std::string const& _name) { return (get()->m_sealEngines[_name] = [](){return new SealEngine;}); }
    static void unregisterSealEngine(std::string const& _name) { get()->m_sealEngines.erase(_name); }

private:
    static SealEngineRegistrar* get() { if (!s_this) s_this = new SealEngineRegistrar; return s_this; }

    std::unordered_map<std::string, SealEngineFactory> m_sealEngines;
    static SealEngineRegistrar* s_this;
};

#define ETH_REGISTER_SEAL_ENGINE(Name) static SealEngineFactory __eth_registerSealEngineFactory ## Name = SealEngineRegistrar::registerSealEngine<Name>(#Name)

class NoProof: public eth::SealEngineBase
{
public:
    static std::string name() { return "NoProof"; }
    static void init();
    void generateSeal(BlockHeader const& _bi) override;
    void populateFromParent(BlockHeader& _bi, BlockHeader const& _parent) const override;
    void verify(Strictness _s, BlockHeader const& _bi, BlockHeader const& _parent, bytesConstRef _block) const override;
    StringHashMap jsInfo(BlockHeader const& _bi) const override;
};

u256 calculateEthashDifficulty(
    ChainOperationParams const& _chainParams, BlockHeader const& _bi, BlockHeader const& _parent);

u256 calculateGasLimit(ChainOperationParams const& _chainParams, BlockHeader const& _bi,
    u256 const& _gasFloorTarget = Invalid256);


class NoReward : public NoProof
{
public:
    static std::string name() { return "NoReward"; }
    static void init();
    u256 blockReward(u256 const&) const override { return 0; }
};
}
}
