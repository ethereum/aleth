// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#include "Precompiled.h"
#include "ChainOperationParams.h"
#include <libdevcore/Log.h>
#include <libdevcore/SHA3.h>
#include <libdevcrypto/Blake2.h>
#include <libdevcrypto/Common.h>
#include <libdevcrypto/Hash.h>
#include <libdevcrypto/LibSnark.h>
#include <libethcore/Common.h>
using namespace std;
using namespace dev;
using namespace dev::eth;

PrecompiledRegistrar* PrecompiledRegistrar::s_this = nullptr;

PrecompiledExecutor const& PrecompiledRegistrar::executor(std::string const& _name)
{
    if (!get()->m_execs.count(_name))
        BOOST_THROW_EXCEPTION(ExecutorNotFound());
    return get()->m_execs[_name];
}

PrecompiledPricer const& PrecompiledRegistrar::pricer(std::string const& _name)
{
    if (!get()->m_pricers.count(_name))
        BOOST_THROW_EXCEPTION(PricerNotFound());
    return get()->m_pricers[_name];
}

namespace
{
bigint linearPricer(unsigned _base, unsigned _word, bytesConstRef _in)
{
    bigint const s = _in.size();
    bigint const b = _base;
    bigint const w = _word;
    return b + (s + 31) / 32 * w;
}

ETH_REGISTER_PRECOMPILED_PRICER(ecrecover)
(bytesConstRef /*_in*/, ChainOperationParams const& /*_chainParams*/, u256 const& /*_blockNumber*/)
{
    return 3000;
}

ETH_REGISTER_PRECOMPILED(ecrecover)(bytesConstRef _in)
{
    struct
    {
        h256 hash;
        h256 v;
        h256 r;
        h256 s;
    } in;

    memcpy(&in, _in.data(), min(_in.size(), sizeof(in)));

    h256 ret;
    u256 v = (u256)in.v;
    if (v >= 27 && v <= 28)
    {
        SignatureStruct sig(in.r, in.s, (byte)((int)v - 27));
        if (sig.isValid())
        {
            try
            {
                if (Public rec = recover(sig, in.hash))
                {
                    ret = dev::sha3(rec);
                    memset(ret.data(), 0, 12);
                    return {true, ret.asBytes()};
                }
            }
            catch (...) {}
        }
    }
    return {true, {}};
}

ETH_REGISTER_PRECOMPILED_PRICER(sha256)
(bytesConstRef _in, ChainOperationParams const& /*_chainParams*/, u256 const& /*_blockNumber*/)
{
    return linearPricer(60, 12, _in);
}

ETH_REGISTER_PRECOMPILED(sha256)(bytesConstRef _in)
{
    return {true, dev::sha256(_in).asBytes()};
}

ETH_REGISTER_PRECOMPILED_PRICER(ripemd160)
(bytesConstRef _in, ChainOperationParams const& /*_chainParams*/, u256 const& /*_blockNumber*/)
{
    return linearPricer(600, 120, _in);
}

ETH_REGISTER_PRECOMPILED(ripemd160)(bytesConstRef _in)
{
    return {true, h256(dev::ripemd160(_in), h256::AlignRight).asBytes()};
}

ETH_REGISTER_PRECOMPILED_PRICER(identity)
(bytesConstRef _in, ChainOperationParams const& /*_chainParams*/, u256 const& /*_blockNumber*/)
{
    return linearPricer(15, 3, _in);
}

ETH_REGISTER_PRECOMPILED(identity)(bytesConstRef _in)
{
    return {true, _in.toBytes()};
}

// Parse _count bytes of _in starting with _begin offset as big endian int.
// If there's not enough bytes in _in, consider it infinitely right-padded with zeroes.
bigint parseBigEndianRightPadded(bytesConstRef _in, bigint const& _begin, bigint const& _count)
{
    if (_begin > _in.count())
        return 0;
    assert(_count <= numeric_limits<size_t>::max() / 8); // Otherwise, the return value would not fit in the memory.

    size_t const begin{_begin};
    size_t const count{_count};

    // crop _in, not going beyond its size
    bytesConstRef cropped = _in.cropped(begin, min(count, _in.count() - begin));

    bigint ret = fromBigEndian<bigint>(cropped);
    // shift as if we had right-padding zeroes
    assert(count - cropped.count() <= numeric_limits<size_t>::max() / 8);
    ret <<= 8 * (count - cropped.count());

    return ret;
}

ETH_REGISTER_PRECOMPILED(modexp)(bytesConstRef _in)
{
    bigint const baseLength(parseBigEndianRightPadded(_in, 0, 32));
    bigint const expLength(parseBigEndianRightPadded(_in, 32, 32));
    bigint const modLength(parseBigEndianRightPadded(_in, 64, 32));
    assert(modLength <= numeric_limits<size_t>::max() / 8); // Otherwise gas should be too expensive.
    assert(baseLength <= numeric_limits<size_t>::max() / 8); // Otherwise, gas should be too expensive.
    if (modLength == 0 && baseLength == 0)
        return {true, bytes{}}; // This is a special case where expLength can be very big.
    assert(expLength <= numeric_limits<size_t>::max() / 8);

    bigint const base(parseBigEndianRightPadded(_in, 96, baseLength));
    bigint const exp(parseBigEndianRightPadded(_in, 96 + baseLength, expLength));
    bigint const mod(parseBigEndianRightPadded(_in, 96 + baseLength + expLength, modLength));

    bigint const result = mod != 0 ? boost::multiprecision::powm(base, exp, mod) : bigint{0};

    size_t const retLength(modLength);
    bytes ret(retLength);
    toBigEndian(result, ret);

    return {true, ret};
}

namespace
{
    bigint expLengthAdjust(bigint const& _expOffset, bigint const& _expLength, bytesConstRef _in)
    {
        if (_expLength <= 32)
        {
            bigint const exp(parseBigEndianRightPadded(_in, _expOffset, _expLength));
            return exp ? msb(exp) : 0;
        }
        else
        {
            bigint const expFirstWord(parseBigEndianRightPadded(_in, _expOffset, 32));
            size_t const highestBit(expFirstWord ? msb(expFirstWord) : 0);
            return 8 * (_expLength - 32) + highestBit;
        }
    }

    bigint multComplexity(bigint const& _x)
    {
        if (_x <= 64)
            return _x * _x;
        if (_x <= 1024)
            return (_x * _x) / 4 + 96 * _x - 3072;
        else
            return (_x * _x) / 16 + 480 * _x - 199680;
    }
}

ETH_REGISTER_PRECOMPILED_PRICER(modexp)(bytesConstRef _in, ChainOperationParams const&, u256 const&)
{
    bigint const baseLength(parseBigEndianRightPadded(_in, 0, 32));
    bigint const expLength(parseBigEndianRightPadded(_in, 32, 32));
    bigint const modLength(parseBigEndianRightPadded(_in, 64, 32));

    bigint const maxLength(max(modLength, baseLength));
    bigint const adjustedExpLength(expLengthAdjust(baseLength + 96, expLength, _in));

    return multComplexity(maxLength) * max<bigint>(adjustedExpLength, 1) / 20;
}

ETH_REGISTER_PRECOMPILED(alt_bn128_G1_add)(bytesConstRef _in)
{
    return dev::crypto::alt_bn128_G1_add(_in);
}

ETH_REGISTER_PRECOMPILED_PRICER(alt_bn128_G1_add)
(bytesConstRef /*_in*/, ChainOperationParams const& _chainParams, u256 const& _blockNumber)
{
    return _blockNumber < _chainParams.istanbulForkBlock ? 500 : 150;
}

ETH_REGISTER_PRECOMPILED(alt_bn128_G1_mul)(bytesConstRef _in)
{
    return dev::crypto::alt_bn128_G1_mul(_in);
}

ETH_REGISTER_PRECOMPILED_PRICER(alt_bn128_G1_mul)
(bytesConstRef /*_in*/, ChainOperationParams const& _chainParams, u256 const& _blockNumber)
{
    return _blockNumber < _chainParams.istanbulForkBlock ? 40000 : 6000;
}

ETH_REGISTER_PRECOMPILED(alt_bn128_pairing_product)(bytesConstRef _in)
{
    return dev::crypto::alt_bn128_pairing_product(_in);
}

ETH_REGISTER_PRECOMPILED_PRICER(alt_bn128_pairing_product)
(bytesConstRef _in, ChainOperationParams const& _chainParams, u256 const& _blockNumber)
{
    auto const k = _in.size() / 192;
    return _blockNumber < _chainParams.istanbulForkBlock ? 100000 + k * 80000 : 45000 + k * 34000;
}

ETH_REGISTER_PRECOMPILED(blake2_compression)(bytesConstRef _in)
{
    static constexpr size_t roundsSize = 4;
    static constexpr size_t stateVectorSize = 8 * 8;
    static constexpr size_t messageBlockSize = 16 * 8;
    static constexpr size_t offsetCounterSize = 8;
    static constexpr size_t finalBlockIndicatorSize = 1;
    static constexpr size_t totalInputSize = roundsSize + stateVectorSize + messageBlockSize +
                                             2 * offsetCounterSize + finalBlockIndicatorSize;

    if (_in.size() != totalInputSize)
        return {false, {}};

    auto const rounds = fromBigEndian<uint32_t>(_in.cropped(0, roundsSize));
    auto const stateVector = _in.cropped(roundsSize, stateVectorSize);
    auto const messageBlockVector = _in.cropped(roundsSize + stateVectorSize, messageBlockSize);
    auto const offsetCounter0 =
        _in.cropped(roundsSize + stateVectorSize + messageBlockSize, offsetCounterSize);
    auto const offsetCounter1 = _in.cropped(
        roundsSize + stateVectorSize + messageBlockSize + offsetCounterSize, offsetCounterSize);
    uint8_t const finalBlockIndicator =
        _in[roundsSize + stateVectorSize + messageBlockSize + 2 * offsetCounterSize];

    if (finalBlockIndicator != 0 && finalBlockIndicator != 1)
        return {false, {}};

    return {true, dev::crypto::blake2FCompression(rounds, stateVector, offsetCounter0,
                      offsetCounter1, finalBlockIndicator, messageBlockVector)};
}

ETH_REGISTER_PRECOMPILED_PRICER(blake2_compression)
(bytesConstRef _in, ChainOperationParams const&, u256 const&)
{
    auto const rounds = fromBigEndian<uint32_t>(_in.cropped(0, 4));
    return rounds;
}
}
