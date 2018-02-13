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
