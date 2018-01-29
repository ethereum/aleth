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
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

namespace dev
{
namespace db
{
class LevelDB : public DatabaseFace
{
public:
    static leveldb::ReadOptions defaultReadOptions();
    static leveldb::WriteOptions defaultWriteOptions();
    static leveldb::Options defaultDBOptions();

    explicit LevelDB(boost::filesystem::path const& _path,
        leveldb::ReadOptions _readOptions = defaultReadOptions(),
        leveldb::WriteOptions _writeOptions = defaultWriteOptions(),
        leveldb::Options _dbOptions = defaultDBOptions());

    std::string lookup(Slice _key) const override;
    bool exists(Slice _key) const override;
    void insert(Slice _key, Slice _value) override;
    void kill(Slice _key) override;

    std::unique_ptr<WriteBatchFace> createWriteBatch() const override;
    void commit(std::unique_ptr<WriteBatchFace> _batch) override;

    void forEach(std::function<bool(Slice, Slice)> f) const override;

private:
    std::unique_ptr<leveldb::DB> m_db;
    leveldb::ReadOptions const m_readOptions;
    leveldb::WriteOptions const m_writeOptions;
};

}  // namespace db
}  // namespace dev
