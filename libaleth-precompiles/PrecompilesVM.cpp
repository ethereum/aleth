// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2018-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "PrecompilesVM.h"
#include <aleth/version.h>
#include <evmc/evmc.hpp>
#include <libdevcore/Common.h>
#include <libdevcore/SHA3.h>
#include <libdevcrypto/Blake2.h>
#include <libdevcrypto/Common.h>
#include <libdevcrypto/Hash.h>
#include <libdevcrypto/LibSnark.h>
#include <libethcore/Common.h>

namespace dev
{
namespace
{
using Pricer = bigint (*)(bytesConstRef, evmc_revision);
using Executor = std::pair<bool, bytes> (*)(bytesConstRef);

bigint linearPrice(unsigned _base, unsigned _word, bytesConstRef _in)
{
    bigint const s = _in.size();
    bigint const b = _base;
    bigint const w = _word;
    return b + (s + 31) / 32 * w;
}

bigint ecrecoverPrice(bytesConstRef, evmc_revision)
{
    return 3000;
}

std::pair<bool, bytes> ecrecover(bytesConstRef _in)
{
    struct
    {
        h256 hash;
        h256 v;
        h256 r;
        h256 s;
    } in;

    memcpy(&in, _in.data(), std::min(_in.size(), sizeof(in)));

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
            catch (...)
            {
            }
        }
    }
    return {true, {}};
}

bigint sha256Price(bytesConstRef _in, evmc_revision)
{
    return linearPrice(60, 12, _in);
}

std::pair<bool, bytes> sha256(bytesConstRef _in)
{
    return {true, dev::sha256(_in).asBytes()};
}

bigint ripemd160Price(bytesConstRef _in, evmc_revision)
{
    return linearPrice(600, 120, _in);
}

std::pair<bool, bytes> ripemd160(bytesConstRef _in)
{
    return {true, h256(dev::ripemd160(_in), h256::AlignRight).asBytes()};
}

bigint identityPrice(bytesConstRef _in, evmc_revision)
{
    return linearPrice(15, 3, _in);
}

std::pair<bool, bytes> identity(bytesConstRef _in)
{
    return {true, _in.toBytes()};
}

// Parse _count bytes of _in starting with _begin offset as big endian int.
// If there's not enough bytes in _in, consider it infinitely right-padded with zeroes.
bigint parseBigEndianRightPadded(bytesConstRef _in, bigint const& _begin, bigint const& _count)
{
    if (_begin > _in.count())
        return 0;
    assert(_count <= std::numeric_limits<size_t>::max() / 8);  // Otherwise, the return value would
                                                               // not fit in the memory.

    size_t const begin{_begin};
    size_t const count{_count};

    // crop _in, not going beyond its size
    bytesConstRef cropped = _in.cropped(begin, std::min(count, _in.count() - begin));

    bigint ret = fromBigEndian<bigint>(cropped);
    // shift as if we had right-padding zeroes
    assert(count - cropped.count() <= std::numeric_limits<size_t>::max() / 8);
    ret <<= 8 * (count - cropped.count());

    return ret;
}

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

bigint modexpPrice(bytesConstRef _in, evmc_revision)
{
    bigint const baseLength(parseBigEndianRightPadded(_in, 0, 32));
    bigint const expLength(parseBigEndianRightPadded(_in, 32, 32));
    bigint const modLength(parseBigEndianRightPadded(_in, 64, 32));

    bigint const maxLength(max(modLength, baseLength));
    bigint const adjustedExpLength(expLengthAdjust(baseLength + 96, expLength, _in));

    return multComplexity(maxLength) * std::max<bigint>(adjustedExpLength, 1) / 20;
}

std::pair<bool, bytes> modexp(bytesConstRef _in)
{
    bigint const baseLength(parseBigEndianRightPadded(_in, 0, 32));
    bigint const expLength(parseBigEndianRightPadded(_in, 32, 32));
    bigint const modLength(parseBigEndianRightPadded(_in, 64, 32));
    assert(modLength <= std::numeric_limits<size_t>::max() / 8);   // Otherwise gas should be too
                                                                   // expensive.
    assert(baseLength <= std::numeric_limits<size_t>::max() / 8);  // Otherwise, gas should be too
                                                                   // expensive.
    if (modLength == 0 && baseLength == 0)
        return {true, bytes{}};  // This is a special case where expLength can be very big.
    assert(expLength <= std::numeric_limits<size_t>::max() / 8);

    bigint const base(parseBigEndianRightPadded(_in, 96, baseLength));
    bigint const exp(parseBigEndianRightPadded(_in, 96 + baseLength, expLength));
    bigint const mod(parseBigEndianRightPadded(_in, 96 + baseLength + expLength, modLength));

    bigint const result = mod != 0 ? boost::multiprecision::powm(base, exp, mod) : bigint{0};

    size_t const retLength(modLength);
    bytes ret(retLength);
    toBigEndian(result, ret);

    return {true, ret};
}

bigint alt_bn128_G1_addPrice(bytesConstRef, evmc_revision _rev)
{
    return _rev < EVMC_ISTANBUL ? 500 : 150;
}

std::pair<bool, bytes> alt_bn128_G1_add(bytesConstRef _in)
{
    return dev::crypto::alt_bn128_G1_add(_in);
}

bigint alt_bn128_G1_mulPrice(bytesConstRef, evmc_revision _rev)
{
    return _rev < EVMC_ISTANBUL ? 40000 : 6000;
}

std::pair<bool, bytes> alt_bn128_G1_mul(bytesConstRef _in)
{
    return dev::crypto::alt_bn128_G1_mul(_in);
}

bigint alt_bn128_pairing_productPrice(bytesConstRef _in, evmc_revision _rev)
{
    auto const k = _in.size() / 192;
    return _rev < EVMC_ISTANBUL ? 100000 + k * 80000 : 45000 + k * 34000;
}

std::pair<bool, bytes> alt_bn128_pairing_product(bytesConstRef _in)
{
    return dev::crypto::alt_bn128_pairing_product(_in);
}

bigint blake2CompressionPrice(bytesConstRef _in, evmc_revision)
{
    auto const rounds = fromBigEndian<uint32_t>(_in.cropped(0, 4));
    return rounds;
}

std::pair<bool, bytes> blake2Compression(bytesConstRef _in)
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

evmc_result execute(evmc_vm*, const evmc_host_interface*, evmc_host_context*,
    enum evmc_revision _rev, const evmc_message* _msg, const uint8_t*, size_t) noexcept
{
    static constexpr std::pair<Pricer, Executor> c_precompiles[] = {{ecrecoverPrice, ecrecover},
        {sha256Price, sha256}, {ripemd160Price, ripemd160}, {identityPrice, identity},
        {modexpPrice, modexp}, {alt_bn128_G1_addPrice, alt_bn128_G1_add},
        {alt_bn128_G1_mulPrice, alt_bn128_G1_mul},
        {alt_bn128_pairing_productPrice, alt_bn128_pairing_product},
        {blake2CompressionPrice, blake2Compression}};
    static constexpr size_t c_precompilesSize = sizeof(c_precompiles) / sizeof(c_precompiles[0]);
    static_assert(c_precompilesSize < 0xFF, "Assuming no more than 256 precompiles");
    static constexpr evmc::address c_maxPrecompileAddress =
        evmc_address{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, c_precompilesSize}};

    // check that address is within the range of defined precompiles
    auto const destination = evmc::address{_msg->destination};
    if (destination == evmc::address{} || c_maxPrecompileAddress < destination)
        return evmc::make_result(EVMC_REJECTED, 0, nullptr, 0);

    // convert address to array index
    auto const precompileAddressLSB = _msg->destination.bytes[sizeof(_msg->destination) - 1];
    auto const precompile = c_precompiles[precompileAddressLSB - 1];

    bytesConstRef input{_msg->input_data, _msg->input_size};

    // Check the gas cost.
    auto const gasCost = precompile.first(input, _rev);
    auto const gasLeft = _msg->gas - gasCost;
    if (gasLeft < 0)
        return evmc::make_result(EVMC_OUT_OF_GAS, 0, nullptr, 0);

    // Execute.
    auto const res = precompile.second(input);
    if (!res.first)
        return evmc::make_result(EVMC_PRECOMPILE_FAILURE, 0, nullptr, 0);

    // Return the result.
    return evmc::make_result(
        EVMC_SUCCESS, static_cast<int64_t>(gasLeft), res.second.data(), res.second.size());
}
}  // namespace
}  // namespace dev

evmc_vm* evmc_create_aleth_precompiles_vm() noexcept
{
    static evmc_vm vm = {
        EVMC_ABI_VERSION,
        "precompiles_vm",
        aleth_version,
        [](evmc_vm*) {},
        dev::execute,
        [](evmc_vm*) { return evmc_capabilities_flagset{EVMC_CAPABILITY_PRECOMPILES}; },
        nullptr,
    };
    return &vm;
}
