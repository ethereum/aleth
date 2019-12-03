// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "SealEngine.h"
#include "TransactionBase.h"
#include <libethcore/CommonJS.h>

using namespace std;
namespace dev
{
namespace eth
{
SealEngineRegistrar* SealEngineRegistrar::s_this = nullptr;

void NoProof::init()
{
    ETH_REGISTER_SEAL_ENGINE(NoProof);
}

void NoReward::init()
{
    ETH_REGISTER_SEAL_ENGINE(NoReward);
}

void NoProof::populateFromParent(BlockHeader& _bi, BlockHeader const& _parent) const
{
    SealEngineFace::populateFromParent(_bi, _parent);
    _bi.setDifficulty(calculateEthashDifficulty(chainParams(), _bi, _parent));
    _bi.setGasLimit(calculateGasLimit(chainParams(), _bi));
}

void NoProof::generateSeal(BlockHeader const& _bi)
{
    BlockHeader header(_bi);
    header.setSeal(NonceField, h64{0});
    header.setSeal(MixHashField, h256{0});
    RLPStream ret;
    header.streamRLP(ret);
    if (m_onSealGenerated)
        m_onSealGenerated(ret.out());
}

void NoProof::verify(Strictness _s, BlockHeader const& _bi, BlockHeader const& _parent, bytesConstRef _block) const
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
}

StringHashMap NoProof::jsInfo(BlockHeader const& _bi) const
{
    return {{"difficulty", toJS(_bi.difficulty())}};
}

void SealEngineFace::verify(Strictness _s, BlockHeader const& _bi, BlockHeader const& _parent, bytesConstRef _block) const
{
    _bi.verify(_s, _parent, _block);

    if (_s != CheckNothingNew)
    {
        if (_bi.difficulty() < chainParams().minimumDifficulty)
            BOOST_THROW_EXCEPTION(
                        InvalidDifficulty() << RequirementError(
                            bigint(chainParams().minimumDifficulty), bigint(_bi.difficulty())));

        if (_bi.gasLimit() < chainParams().minGasLimit)
            BOOST_THROW_EXCEPTION(InvalidGasLimit() << RequirementError(
                                      bigint(chainParams().minGasLimit), bigint(_bi.gasLimit())));

        if (_bi.gasLimit() > chainParams().maxGasLimit)
            BOOST_THROW_EXCEPTION(InvalidGasLimit() << RequirementError(
                                      bigint(chainParams().maxGasLimit), bigint(_bi.gasLimit())));

        if (_bi.number() && _bi.extraData().size() > chainParams().maximumExtraDataSize)
        {
            BOOST_THROW_EXCEPTION(
                        ExtraDataTooBig()
                        << RequirementError(bigint(chainParams().maximumExtraDataSize),
                                            bigint(_bi.extraData().size()))
                        << errinfo_extraData(_bi.extraData()));
        }

        u256 const& daoHardfork = chainParams().daoHardforkBlock;
        if (daoHardfork != 0 && daoHardfork + 9 >= daoHardfork && _bi.number() >= daoHardfork &&
                _bi.number() <= daoHardfork + 9)
            if (_bi.extraData() != fromHex("0x64616f2d686172642d666f726b"))
                BOOST_THROW_EXCEPTION(
                            ExtraDataIncorrect()
                            << errinfo_comment("Received block from the wrong fork (invalid extradata)."));
    }

    if (_parent)
    {
        auto gasLimit = _bi.gasLimit();
        auto parentGasLimit = _parent.gasLimit();
        if (gasLimit < chainParams().minGasLimit || gasLimit > chainParams().maxGasLimit ||
                gasLimit <= parentGasLimit - parentGasLimit / chainParams().gasLimitBoundDivisor ||
                gasLimit >= parentGasLimit + parentGasLimit / chainParams().gasLimitBoundDivisor)
            BOOST_THROW_EXCEPTION(
                        InvalidGasLimit()
                        << errinfo_min(
                            (bigint)((bigint)parentGasLimit -
                                     (bigint)(parentGasLimit / chainParams().gasLimitBoundDivisor)))
                        << errinfo_got((bigint)gasLimit)
                        << errinfo_max((bigint)((bigint)parentGasLimit +
                                                parentGasLimit / chainParams().gasLimitBoundDivisor)));
    }
}

void SealEngineFace::populateFromParent(BlockHeader& _bi, BlockHeader const& _parent) const
{
    _bi.populateFromParent(_parent);
}

void SealEngineFace::verifyTransaction(ImportRequirements::value _ir, TransactionBase const& _t,
                                       BlockHeader const& _header, u256 const& _gasUsed) const
{
    if ((_ir & ImportRequirements::TransactionSignatures) && _header.number() < chainParams().EIP158ForkBlock && _t.isReplayProtected())
        BOOST_THROW_EXCEPTION(InvalidSignature());

    if ((_ir & ImportRequirements::TransactionSignatures) &&
        _header.number() < chainParams().experimentalForkBlock && _t.hasZeroSignature())
        BOOST_THROW_EXCEPTION(InvalidSignature());

    if ((_ir & ImportRequirements::TransactionBasic) &&
        _header.number() >= chainParams().experimentalForkBlock && _t.hasZeroSignature() &&
        (_t.value() != 0 || _t.gasPrice() != 0 || _t.nonce() != 0))
        BOOST_THROW_EXCEPTION(InvalidZeroSignatureTransaction() << errinfo_got((bigint)_t.gasPrice()) << errinfo_got((bigint)_t.value()) << errinfo_got((bigint)_t.nonce()));

    if (_header.number() >= chainParams().homesteadForkBlock && (_ir & ImportRequirements::TransactionSignatures) && _t.hasSignature())
        _t.checkLowS();

    eth::EVMSchedule const& schedule = evmSchedule(_header.number());

    // Pre calculate the gas needed for execution
    if ((_ir & ImportRequirements::TransactionBasic) && _t.baseGasRequired(schedule) > _t.gas())
        BOOST_THROW_EXCEPTION(OutOfGasIntrinsic() << RequirementError(
                                  (bigint)(_t.baseGasRequired(schedule)), (bigint)_t.gas()));

    // Avoid transactions that would take us beyond the block gas limit.
    if (_gasUsed + (bigint)_t.gas() > _header.gasLimit())
        BOOST_THROW_EXCEPTION(BlockGasLimitReached() << RequirementErrorComment(
                                  (bigint)(_header.gasLimit() - _gasUsed), (bigint)_t.gas(),
                                  string("_gasUsed + (bigint)_t.gas() > _header.gasLimit()")));
}

SealEngineFace* SealEngineRegistrar::create(ChainOperationParams const& _params)
{
    SealEngineFace* ret = create(_params.sealEngineName);
    assert(ret && "Seal engine not found.");
    if (ret)
        ret->setChainParams(_params);
    return ret;
}

EVMSchedule const& SealEngineBase::evmSchedule(u256 const& _blockNumber) const
{
    return chainParams().scheduleForBlockNumber(_blockNumber);
}

u256 SealEngineBase::blockReward(u256 const& _blockNumber) const
{
    EVMSchedule const& schedule{evmSchedule(_blockNumber)};
    return chainParams().blockReward(schedule);
}

u256 calculateEthashDifficulty(
    ChainOperationParams const& _chainParams, BlockHeader const& _bi, BlockHeader const& _parent)
{
    const unsigned c_expDiffPeriod = 100000;

    if (!_bi.number())
        throw GenesisBlockCannotBeCalculated();
    auto const& minimumDifficulty = _chainParams.minimumDifficulty;
    auto const& difficultyBoundDivisor = _chainParams.difficultyBoundDivisor;
    auto const& durationLimit = _chainParams.durationLimit;

    bigint target;  // stick to a bigint for the target. Don't want to risk going negative.
    if (_bi.number() < _chainParams.homesteadForkBlock)
        // Frontier-era difficulty adjustment
        target = _bi.timestamp() >= _parent.timestamp() + durationLimit ?
                     _parent.difficulty() - (_parent.difficulty() / difficultyBoundDivisor) :
                     (_parent.difficulty() + (_parent.difficulty() / difficultyBoundDivisor));
    else
    {
        bigint const timestampDiff = bigint(_bi.timestamp()) - _parent.timestamp();
        bigint const adjFactor =
            _bi.number() < _chainParams.byzantiumForkBlock ?
                max<bigint>(1 - timestampDiff / 10, -99) :  // Homestead-era difficulty adjustment
                max<bigint>((_parent.hasUncles() ? 2 : 1) - timestampDiff / 9,
                    -99);  // Byzantium-era difficulty adjustment

        target = _parent.difficulty() + _parent.difficulty() / 2048 * adjFactor;
    }

    bigint o = target;
    unsigned exponentialIceAgeBlockNumber = unsigned(_parent.number() + 1);

    // EIP-2384 Istanbul/Berlin Difficulty Bomb Delay
    if (_bi.number() >= _chainParams.muirGlacierForkBlock)
    {
        if (exponentialIceAgeBlockNumber >= 9000000)
            exponentialIceAgeBlockNumber -= 9000000;
        else
            exponentialIceAgeBlockNumber = 0;
    }
    // EIP-1234 Constantinople Ice Age delay
    else if (_bi.number() >= _chainParams.constantinopleForkBlock)
    {
        if (exponentialIceAgeBlockNumber >= 5000000)
            exponentialIceAgeBlockNumber -= 5000000;
        else
            exponentialIceAgeBlockNumber = 0;
    }
    // EIP-649 Byzantium Ice Age delay
    else if (_bi.number() >= _chainParams.byzantiumForkBlock)
    {
        if (exponentialIceAgeBlockNumber >= 3000000)
            exponentialIceAgeBlockNumber -= 3000000;
        else
            exponentialIceAgeBlockNumber = 0;
    }

    unsigned periodCount = exponentialIceAgeBlockNumber / c_expDiffPeriod;
    if (periodCount > 1)
        o += (bigint(1) << (periodCount - 2));  // latter will eventually become huge, so ensure
                                                // it's a bigint.

    o = max<bigint>(minimumDifficulty, o);
    return u256(min<bigint>(o, std::numeric_limits<u256>::max()));
}

u256 calculateGasLimit(
    ChainOperationParams const& _chainParams, BlockHeader const& _bi, u256 const& _gasFloorTarget)
{
    u256 gasFloorTarget = _gasFloorTarget == Invalid256 ? 3141562 : _gasFloorTarget;
    u256 gasLimit = _bi.gasLimit();
    u256 boundDivisor = _chainParams.gasLimitBoundDivisor;
    if (gasLimit < gasFloorTarget)
        return min<u256>(gasFloorTarget, gasLimit + gasLimit / boundDivisor - 1);
    else
        return max<u256>(gasFloorTarget,
            gasLimit - gasLimit / boundDivisor + 1 + (_bi.gasUsed() * 6 / 5) / boundDivisor);
}
}
}  // namespace dev eth
