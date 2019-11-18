// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#pragma once

#include <unordered_map>
#include <libdevcore/Log.h>
#include <libdevcore/RLP.h>
#include "TransactionReceipt.h"

namespace dev
{
namespace eth
{

// TODO: OPTIMISE: constructors take bytes, RLP used only in necessary classes.

constexpr unsigned c_bloomIndexSize = 16;
constexpr unsigned c_bloomIndexLevels = 2;

constexpr unsigned c_invalidNumber = (unsigned)-1;

struct BlockDetails
{
    BlockDetails(): number(c_invalidNumber), totalDifficulty(Invalid256) {}
    BlockDetails(unsigned _number, u256 const& _totalDifficulty, h256 const& _parentHash, h256s const& _childHashes,
        size_t _blockSizeBytes)
      : number{_number},
        totalDifficulty{_totalDifficulty},
        parentHash{_parentHash},
        childHashes{_childHashes},
        blockSizeBytes{_blockSizeBytes}
    {}
    BlockDetails(RLP const& _r);
    bytes rlp() const;

    bool isNull() const { return number == c_invalidNumber; }
    explicit operator bool() const { return !isNull(); }

    unsigned number = c_invalidNumber;
    u256 totalDifficulty = Invalid256;
    h256 parentHash;
    h256s childHashes;

    // Size of the BlockDetails RLP (in bytes). Used for computing blockchain memory usage
    // statistics. Field name must be 'size' as BlockChain::getHashSize depends on this
    mutable unsigned size;

    // Size of the block RLP data in bytes
    size_t blockSizeBytes;
};

struct BlockLogBlooms
{
    BlockLogBlooms() {}
    BlockLogBlooms(RLP const& _r) { blooms = _r.toVector<LogBloom>(); size = _r.data().size(); }
    bytes rlp() const { bytes r = dev::rlp(blooms); size = r.size(); return r; }

    LogBlooms blooms;
    mutable unsigned size;
};

struct BlocksBlooms
{
    BlocksBlooms() {}
    BlocksBlooms(RLP const& _r) { blooms = _r.toArray<LogBloom, c_bloomIndexSize>(); size = _r.data().size(); }
    bytes rlp() const { bytes r = dev::rlp(blooms); size = r.size(); return r; }

    std::array<LogBloom, c_bloomIndexSize> blooms;
    mutable unsigned size;
};

struct BlockReceipts
{
    BlockReceipts() {}
    BlockReceipts(RLP const& _r) { for (auto const& i: _r) receipts.emplace_back(i.data()); size = _r.data().size(); }
    bytes rlp() const { RLPStream s(receipts.size()); for (TransactionReceipt const& i: receipts) i.streamRLP(s); size = s.out().size(); return s.out(); }

    TransactionReceipts receipts;
    mutable unsigned size = 0;
};

struct BlockHash
{
    BlockHash() {}
    BlockHash(h256 const& _h): value(_h) {}
    BlockHash(RLP const& _r) { value = _r.toHash<h256>(); }
    bytes rlp() const { return dev::rlp(value); }

    h256 value;
    static const unsigned size = 65;
};

struct TransactionAddress
{
    TransactionAddress() {}
    TransactionAddress(RLP const& _rlp) { blockHash = _rlp[0].toHash<h256>(); index = _rlp[1].toInt<unsigned>(); }
    bytes rlp() const { RLPStream s(2); s << blockHash << index; return s.out(); }

    explicit operator bool() const { return !!blockHash; }

    h256 blockHash;
    unsigned index = 0;

    static const unsigned size = 67;
};

using BlockDetailsHash = std::unordered_map<h256, BlockDetails>;
using BlockLogBloomsHash = std::unordered_map<h256, BlockLogBlooms>;
using BlockReceiptsHash = std::unordered_map<h256, BlockReceipts>;
using TransactionAddressHash = std::unordered_map<h256, TransactionAddress>;
using BlockHashHash = std::unordered_map<uint64_t, BlockHash>;
using BlocksBloomsHash = std::unordered_map<h256, BlocksBlooms>;

static const BlockDetails NullBlockDetails;
static const BlockLogBlooms NullBlockLogBlooms;
static const BlockReceipts NullBlockReceipts;
static const TransactionAddress NullTransactionAddress;
static const BlockHash NullBlockHash;
static const BlocksBlooms NullBlocksBlooms;

}
}
