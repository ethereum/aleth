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

#include "Ethash.h"
#include "EthashCPUMiner.h"

#include <libethcore/ChainOperationParams.h>
#include <libethcore/CommonJS.h>
#include <libethereum/Interface.h>

#include <ethash/ethash.hpp>

using namespace std;
using namespace dev;
using namespace eth;

void Ethash::init()
{
    ETH_REGISTER_SEAL_ENGINE(Ethash);
}

Ethash::Ethash()
{
    map<string, GenericFarm<EthashProofOfWork>::SealerDescriptor> sealers;
    sealers["cpu"] = GenericFarm<EthashProofOfWork>::SealerDescriptor{&EthashCPUMiner::instances, [](GenericMiner<EthashProofOfWork>::ConstructionInfo ci){ return new EthashCPUMiner(ci); }};
    m_farm.setSealers(sealers);
    m_farm.onSolutionFound([=](EthashProofOfWork::Solution const& sol)
    {
        std::unique_lock<Mutex> l(m_submitLock);
//        cdebug << m_farm.work().seedHash << m_farm.work().headerHash << sol.nonce << EthashAux::eval(m_farm.work().seedHash, m_farm.work().headerHash, sol.nonce).value;
        setMixHash(m_sealing, sol.mixHash);
        setNonce(m_sealing, sol.nonce);
        if (!quickVerifySeal(m_sealing))
            return false;

        if (m_onSealGenerated)
        {
            RLPStream ret;
            m_sealing.streamRLP(ret);
            l.unlock();
            m_onSealGenerated(ret.out());
        }
        return true;
    });
}

Ethash::~Ethash()
{
    // onSolutionFound closure sometimes has references to destroyed members.
    m_farm.onSolutionFound({});
}

strings Ethash::sealers() const
{
    return {"cpu"};
}

h256 Ethash::seedHash(BlockHeader const& _bi)
{
    auto const seed =
        ethash::calculate_epoch_seed(ethash::get_epoch_number(static_cast<int>(_bi.number())));
    return h256{seed.bytes, h256::ConstructFromPointer};
}

StringHashMap Ethash::jsInfo(BlockHeader const& _bi) const
{
    return { { "nonce", toJS(nonce(_bi)) }, { "seedHash", toJS(seedHash(_bi)) }, { "mixHash", toJS(mixHash(_bi)) }, { "boundary", toJS(boundary(_bi)) }, { "difficulty", toJS(_bi.difficulty()) } };
}

void Ethash::verify(Strictness _s, BlockHeader const& _bi, BlockHeader const& _parent, bytesConstRef _block) const
{
    SealEngineFace::verify(_s, _bi, _parent, _block);

    if (_parent)
    {
        // Check difficulty is correct given the two timestamps.
        auto expected = calculateEthashDifficulty(chainParams(), _bi, _parent);
        auto difficulty = _bi.difficulty();
        if (difficulty != expected)
            BOOST_THROW_EXCEPTION(InvalidDifficulty() << RequirementError((bigint)expected, (bigint)difficulty));
    }

    // check it hashes according to proof of work or that it's the genesis block.
    if (_s == CheckEverything && _bi.parentHash() && !verifySeal(_bi))
    {
        ethash::result result =
            ethash::hash(ethash::get_global_epoch_context(ethash::get_epoch_number(_bi.number())),
                toEthash(_bi.hash(WithoutSeal)), toEthash(nonce(_bi)));

        h256 mix{result.mix_hash.bytes, h256::ConstructFromPointer};
        h256 final{result.final_hash.bytes, h256::ConstructFromPointer};

        InvalidBlockNonce ex;
        ex << errinfo_nonce(nonce(_bi));
        ex << errinfo_mixHash(mixHash(_bi));
        ex << errinfo_seedHash(seedHash(_bi));
        ex << errinfo_ethashResult(make_tuple(final, mix));
        ex << errinfo_hash256(_bi.hash(WithoutSeal));
        ex << errinfo_difficulty(_bi.difficulty());
        ex << errinfo_target(boundary(_bi));
        BOOST_THROW_EXCEPTION(ex);
    }
    else if (_s == QuickNonce && _bi.parentHash() && !quickVerifySeal(_bi))
    {
        InvalidBlockNonce ex;
        ex << errinfo_hash256(_bi.hash(WithoutSeal));
        ex << errinfo_difficulty(_bi.difficulty());
        ex << errinfo_nonce(nonce(_bi));
        BOOST_THROW_EXCEPTION(ex);
    }
}

void Ethash::verifyTransaction(ImportRequirements::value _ir, TransactionBase const& _t, BlockHeader const& _header, u256 const& _startGasUsed) const
{
    SealEngineFace::verifyTransaction(_ir, _t, _header, _startGasUsed);

    if (_ir & ImportRequirements::TransactionSignatures)
    {
        if (_header.number() >= chainParams().EIP158ForkBlock)
        {
            int chainID = chainParams().chainID;
            _t.checkChainId(chainID);
        }
        else
            _t.checkChainId(-4);
    }
}

void Ethash::manuallySubmitWork(const h256& _mixHash, Nonce _nonce)
{
    m_farm.submitProof(EthashProofOfWork::Solution{_nonce, _mixHash}, nullptr);
}

void Ethash::populateFromParent(BlockHeader& _bi, BlockHeader const& _parent) const
{
    SealEngineFace::populateFromParent(_bi, _parent);
    _bi.setDifficulty(calculateEthashDifficulty(chainParams(), _bi, _parent));
    _bi.setGasLimit(calculateGasLimit(chainParams(), _bi));
}

bool Ethash::quickVerifySeal(BlockHeader const& _blockHeader) const
{
    h256 const h = _blockHeader.hash(WithoutSeal);
    h256 const b = boundary(_blockHeader);
    Nonce const n = nonce(_blockHeader);
    h256 const m = mixHash(_blockHeader);

    return ethash::verify_final_hash(toEthash(h), toEthash(m), toEthash(n), toEthash(b));
}

bool Ethash::verifySeal(BlockHeader const& _blockHeader) const
{
    h256 const h = _blockHeader.hash(WithoutSeal);
    h256 const b = boundary(_blockHeader);
    Nonce const n = nonce(_blockHeader);
    h256 const m = mixHash(_blockHeader);

    auto& context =
        ethash::get_global_epoch_context(ethash::get_epoch_number(_blockHeader.number()));
    return ethash::verify(context, toEthash(h), toEthash(m), toEthash(n), toEthash(b));
}

void Ethash::generateSeal(BlockHeader const& _bi)
{
    Guard l(m_submitLock);
    m_sealing = _bi;
    m_farm.setWork(m_sealing);
    m_farm.start(m_sealer);
    m_farm.setWork(m_sealing);
}

bool Ethash::shouldSeal(Interface*)
{
    return true;
}
