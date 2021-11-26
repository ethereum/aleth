// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#include <libdevcore/Common.h>
#include <libdevcore/Log.h>
#include <libdevcore/RLP.h>
#include <libdevcore/TrieDB.h>
#include <libdevcore/StateCacheDB.h>
#include <libdevcore/TrieHash.h>
#include <libethcore/Common.h>
#include "Exceptions.h"
#include "BlockHeader.h"
using namespace std;
using namespace dev;
using namespace dev::eth;

BlockHeader::BlockHeader()
{
}

BlockHeader::BlockHeader(bytesConstRef _block, BlockDataType _bdt, h256 const& _hashWith)
{
    RLP header = _bdt == BlockData ? extractHeader(_block) : RLP(_block);
    m_hash = _hashWith ? _hashWith : sha3(header.data());
    populate(header);
}

BlockHeader::BlockHeader(BlockHeader const& _other) :
    m_parentHash(_other.parentHash()),
    m_sha3Uncles(_other.sha3Uncles()),
    m_stateRoot(_other.stateRoot()),
    m_transactionsRoot(_other.transactionsRoot()),
    m_receiptsRoot(_other.receiptsRoot()),
    m_logBloom(_other.logBloom()),
    m_number(_other.number()),
    m_gasLimit(_other.gasLimit()),
    m_gasUsed(_other.gasUsed()),
    m_extraData(_other.extraData()),
    m_timestamp(_other.timestamp()),
    m_author(_other.author()),
    m_difficulty(_other.difficulty()),
    m_seal(_other.seal()),
    m_hash(_other.hashRawRead()),
    m_hashWithout(_other.hashWithoutRawRead())
{
    assert(*this == _other);
}

BlockHeader& BlockHeader::operator=(BlockHeader const& _other)
{
    if (this == &_other)
        return *this;
    m_parentHash = _other.parentHash();
    m_sha3Uncles = _other.sha3Uncles();
    m_stateRoot = _other.stateRoot();
    m_transactionsRoot = _other.transactionsRoot();
    m_receiptsRoot = _other.receiptsRoot();
    m_logBloom = _other.logBloom();
    m_number = _other.number();
    m_gasLimit = _other.gasLimit();
    m_gasUsed = _other.gasUsed();
    m_extraData = _other.extraData();
    m_timestamp = _other.timestamp();
    m_author = _other.author();
    m_difficulty = _other.difficulty();
    std::vector<bytes> seal = _other.seal();
    {
        Guard l(m_sealLock);
        m_seal = std::move(seal);
    }
    h256 hash = _other.hashRawRead();
    h256 hashWithout = _other.hashWithoutRawRead();
    {
        Guard l(m_hashLock);
        m_hash = std::move(hash);
        m_hashWithout = std::move(hashWithout);
    }
    assert(*this == _other);
    return *this;
}

void BlockHeader::clear()
{
    m_parentHash = h256();
    m_sha3Uncles = EmptyListSHA3;
    m_author = Address();
    m_stateRoot = EmptyTrie;
    m_transactionsRoot = EmptyTrie;
    m_receiptsRoot = EmptyTrie;
    m_logBloom = LogBloom();
    m_difficulty = 0;
    m_number = 0;
    m_gasLimit = 0;
    m_gasUsed = 0;
    m_timestamp = -1;
    m_extraData.clear();
    m_seal.clear();
    noteDirty();
}

h256 BlockHeader::hash(IncludeSeal _i) const
{
    h256 dummy;
    Guard l(m_hashLock);
    h256& memo = _i == WithSeal ? m_hash : _i == WithoutSeal ? m_hashWithout : dummy;
    if (!memo)
    {
        RLPStream s;
        streamRLP(s, _i);
        memo = sha3(s.out());
    }
    return memo;
}

void BlockHeader::streamRLPFields(RLPStream& _s) const
{
    bigint nnn = m_number;     // Fix rlp stream issue with uint65_t
    bigint ttt = m_timestamp;  // Fix rlp stream issue with uint65_t
    _s	<< m_parentHash << m_sha3Uncles << m_author << m_stateRoot << m_transactionsRoot << m_receiptsRoot << m_logBloom
        << m_difficulty << nnn << m_gasLimit << m_gasUsed << ttt << m_extraData;
}

void BlockHeader::streamRLP(RLPStream& _s, IncludeSeal _i) const
{
    if (_i != OnlySeal)
    {
        _s.appendList(BlockHeader::BasicFields + (_i == WithoutSeal ? 0 : m_seal.size()));
        BlockHeader::streamRLPFields(_s);
    }
    if (_i != WithoutSeal)
        for (unsigned i = 0; i < m_seal.size(); ++i)
            _s.appendRaw(m_seal[i]);
}

h256 BlockHeader::headerHashFromBlock(bytesConstRef _block)
{
    return sha3(RLP(_block)[0].data());
}

RLP BlockHeader::extractHeader(bytesConstRef _block)
{
    RLP root(_block);
    if (!root.isList())
        BOOST_THROW_EXCEPTION(InvalidBlockFormat() << errinfo_comment("Block must be a list") << BadFieldError(0, _block.toString()));
    RLP header = root[0];
    if (!header.isList())
        BOOST_THROW_EXCEPTION(InvalidBlockFormat() << errinfo_comment("Block header must be a list") << BadFieldError(0, header.data().toString()));
    if (!root[1].isList())
        BOOST_THROW_EXCEPTION(InvalidBlockFormat() << errinfo_comment("Block transactions must be a list") << BadFieldError(1, root[1].data().toString()));
    if (!root[2].isList())
        BOOST_THROW_EXCEPTION(InvalidBlockFormat() << errinfo_comment("Block uncles must be a list") << BadFieldError(2, root[2].data().toString()));
    return header;
}

void BlockHeader::populate(RLP const& _header)
{
    int field = 0;
    try
    {
        m_parentHash = _header[field = 0].toHash<h256>(RLP::VeryStrict);
        m_sha3Uncles = _header[field = 1].toHash<h256>(RLP::VeryStrict);
        m_author = _header[field = 2].toHash<Address>(RLP::VeryStrict);
        m_stateRoot = _header[field = 3].toHash<h256>(RLP::VeryStrict);
        m_transactionsRoot = _header[field = 4].toHash<h256>(RLP::VeryStrict);
        m_receiptsRoot = _header[field = 5].toHash<h256>(RLP::VeryStrict);
        m_logBloom = _header[field = 6].toHash<LogBloom>(RLP::VeryStrict);
        m_difficulty = _header[field = 7].toInt<u256>();
        m_number = _header[field = 8].toPositiveInt64();
        m_gasLimit = _header[field = 9].toInt<u256>();
        m_gasUsed = _header[field = 10].toInt<u256>();
        m_timestamp = _header[field = 11].toPositiveInt64();
        m_extraData = _header[field = 12].toBytes();
        m_seal.clear();
        for (unsigned i = 13; i < _header.itemCount(); ++i)
            m_seal.push_back(_header[i].data().toBytes());
    }
    catch (Exception const& _e)
    {
        _e << errinfo_name("invalid block header format") << BadFieldError(field, toHex(_header[field].data().toBytes()));
        throw;
    }
}

void BlockHeader::populateFromParent(BlockHeader const& _parent)
{
    m_stateRoot = _parent.stateRoot();
    m_number = _parent.m_number + 1;
    m_parentHash = _parent.m_hash;
    m_gasLimit = _parent.m_gasLimit;
    m_difficulty = _parent.m_difficulty;
    m_gasUsed = 0;
}

void BlockHeader::verify(Strictness _s, BlockHeader const& _parent, bytesConstRef _block) const
{
    if (m_number > ~(unsigned)0)
        BOOST_THROW_EXCEPTION(InvalidNumber());

    if (_s != CheckNothingNew && m_gasUsed > m_gasLimit)
        BOOST_THROW_EXCEPTION(TooMuchGasUsed() << RequirementError(bigint(m_gasLimit), bigint(m_gasUsed)));

    if (_parent)
    {
        if (m_parentHash && _parent.hash() != m_parentHash)
            BOOST_THROW_EXCEPTION(InvalidParentHash());

        if (m_timestamp <= _parent.m_timestamp)
            BOOST_THROW_EXCEPTION(InvalidTimestamp());

        if (m_number != _parent.m_number + 1)
            BOOST_THROW_EXCEPTION(InvalidNumber());
    }

    if (_block)
    {
        RLP root(_block);

        auto txList = root[1];
        auto expectedRoot = trieRootOver(txList.itemCount(), [&](unsigned i){ return rlp(i); }, [&](unsigned i){ return txList[i].data().toBytes(); });

        LOG(m_logger) << "Expected trie root: " << toString(expectedRoot);
        if (m_transactionsRoot != expectedRoot)
        {
            StateCacheDB tm;
            GenericTrieDB<StateCacheDB> transactionsTrie(&tm);
            transactionsTrie.init();

            vector<bytesConstRef> txs;

            for (unsigned i = 0; i < txList.itemCount(); ++i)
            {
                RLPStream k;
                k << i;

                transactionsTrie.insert(&k.out(), txList[i].data());

                txs.push_back(txList[i].data());
                cdebug << toHex(k.out()) << toHex(txList[i].data());
            }
            cdebug << "trieRootOver" << expectedRoot;
            cdebug << "orderedTrieRoot" << orderedTrieRoot(txs);
            cdebug << "TrieDB" << transactionsTrie.root();
            cdebug << "Contents:";
            for (auto const& t: txs)
                cdebug << toHex(t);

            BOOST_THROW_EXCEPTION(InvalidTransactionsRoot() << Hash256RequirementError(expectedRoot, m_transactionsRoot));
        }
        LOG(m_logger) << "Expected uncle hash: " << toString(sha3(root[2].data()));
        if (m_sha3Uncles != sha3(root[2].data()))
            BOOST_THROW_EXCEPTION(InvalidUnclesHash() << Hash256RequirementError(sha3(root[2].data()), m_sha3Uncles));
    }
}
