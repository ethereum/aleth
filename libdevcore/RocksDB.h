// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2017-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
#pragma once

#include "db.h"

#include <boost/filesystem.hpp>
#include <rocksdb/db.h>
#include <rocksdb/write_batch.h>

namespace dev
{
namespace db
{
class RocksDB : public DatabaseFace
{
public:
    static rocksdb::ReadOptions defaultReadOptions();
    static rocksdb::WriteOptions defaultWriteOptions();
    static rocksdb::Options defaultDBOptions();

    explicit RocksDB(boost::filesystem::path const& _path,
        rocksdb::ReadOptions _readOptions = defaultReadOptions(),
        rocksdb::WriteOptions _writeOptions = defaultWriteOptions(),
        rocksdb::Options _dbOptions = defaultDBOptions());

    std::string lookup(Slice _key) const override;
    bool exists(Slice _key) const override;
    void insert(Slice _key, Slice _value) override;
    void kill(Slice _key) override;

    std::unique_ptr<WriteBatchFace> createWriteBatch() const override;
    void commit(std::unique_ptr<WriteBatchFace> _batch) override;

    void forEach(std::function<bool(Slice, Slice)> f) const override;

private:
    std::unique_ptr<rocksdb::DB> m_db;
    rocksdb::ReadOptions const m_readOptions;
    rocksdb::WriteOptions const m_writeOptions;
};

}  // namespace db
}  // namespace dev
