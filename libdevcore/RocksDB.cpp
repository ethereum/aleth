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

#include "RocksDB.h"
#include "Assertions.h"

namespace dev
{
namespace db
{
namespace
{

class RocksDBWriteBatch : public WriteBatchFace
{
public:
    void insert(Slice _key, Slice _value) override;
    void kill(Slice _key) override;

    rocksdb::WriteBatch const& writeBatch() const { return m_writeBatch; }
    rocksdb::WriteBatch& writeBatch() { return m_writeBatch; }

private:
    rocksdb::WriteBatch m_writeBatch;
};

void RocksDBWriteBatch::insert(Slice _key, Slice _value)
{
    auto const status = m_writeBatch.Put(
        rocksdb::Slice(_key.data(), _key.size()),
        rocksdb::Slice(_value.data(), _value.size())
    );
    if (!status.ok())
        BOOST_THROW_EXCEPTION(FailedInsertInDB(status.ToString()));
}

void RocksDBWriteBatch::kill(Slice _key)
{
    auto const status = m_writeBatch.Delete(rocksdb::Slice(_key.data(), _key.size()));
    if (!status.ok())
        BOOST_THROW_EXCEPTION(FailedDeleteInDB(status.ToString()));
}

}  // namespace

rocksdb::ReadOptions RocksDB::defaultReadOptions()
{
    return rocksdb::ReadOptions();
}

rocksdb::WriteOptions RocksDB::defaultWriteOptions()
{
    return rocksdb::WriteOptions();
}

rocksdb::Options RocksDB::defaultDBOptions()
{
    rocksdb::Options options;
    options.create_if_missing = true;
    options.max_open_files = 256;
    return options;
}

RocksDB::RocksDB(boost::filesystem::path const& _path, rocksdb::ReadOptions _readOptions,
    rocksdb::WriteOptions _writeOptions, rocksdb::Options _dbOptions)
  : m_db(nullptr), m_readOptions(std::move(_readOptions)), m_writeOptions(std::move(_writeOptions))
{
    auto db = static_cast<rocksdb::DB*>(nullptr);
    auto const status = rocksdb::DB::Open(_dbOptions, _path.string(), &db);
    if (!status.ok())
        BOOST_THROW_EXCEPTION(FailedToOpenDB(status.ToString()));
    if (!db)
        BOOST_THROW_EXCEPTION(FailedToOpenDB("null database pointer"));

    m_db.reset(db);
}

std::string RocksDB::lookup(Slice _key) const
{
    rocksdb::Slice const key(_key.data(), _key.size());
    std::string value;
    auto const status = m_db->Get(m_readOptions, key, &value);
    if (!status.ok())
        BOOST_THROW_EXCEPTION(FailedLookupInDB(status.ToString()));

    return value;
}

bool RocksDB::exists(Slice _key) const
{
    std::string value;
    rocksdb::Slice const key(_key.data(), _key.size());
    if (!m_db->KeyMayExist(m_readOptions, key, &value, nullptr))
        return false;

    auto const status = m_db->Get(m_readOptions, key, &value);
    return status.ok();
}

void RocksDB::insert(Slice _key, Slice _value)
{
    rocksdb::Slice const key(_key.data(), _key.size());
    rocksdb::Slice const value(_value.data(), _value.size());
    auto const status = m_db->Put(m_writeOptions, key, value);
    if (!status.ok())
        BOOST_THROW_EXCEPTION(FailedInsertInDB(status.ToString()));
}

void RocksDB::kill(Slice _key)
{
    rocksdb::Slice const key(_key.data(), _key.size());
    auto const status = m_db->Delete(m_writeOptions, key);
    if (!status.ok())
        BOOST_THROW_EXCEPTION(FailedDeleteInDB(status.ToString()));
}

std::unique_ptr<WriteBatchFace> RocksDB::createWriteBatch() const
{
    return std::unique_ptr<WriteBatchFace>(new RocksDBWriteBatch());
}

void RocksDB::commit(std::unique_ptr<WriteBatchFace> _batch)
{
    if (!_batch)
        BOOST_THROW_EXCEPTION(FailedCommitInDB("Cannot commit null batch"));

    auto* batchPtr = dynamic_cast<RocksDBWriteBatch*>(_batch.get());
    if (!batchPtr)
        BOOST_THROW_EXCEPTION(FailedCommitInDB("Invalid batch type passed to rocksdb::commit"));

    auto const status = m_db->Write(m_writeOptions, &batchPtr->writeBatch());
    if (!status.ok())
        BOOST_THROW_EXCEPTION(FailedCommitInDB(status.ToString()));
}

void RocksDB::forEach(std::function<bool(Slice, Slice)> f) const
{
    std::unique_ptr<rocksdb::Iterator> itr(m_db->NewIterator(m_readOptions));
    if (itr == nullptr)
        BOOST_THROW_EXCEPTION(FailedIterateDB("null iterator"));

    auto keepIterating = true;
    for (itr->SeekToFirst(); keepIterating && itr->Valid(); itr->Next())
    {
        auto const dbKey = itr->key();
        auto const dbValue = itr->value();
        Slice const key(dbKey.data(), dbKey.size());
        Slice const value(dbValue.data(), dbValue.size());
        keepIterating = f(key, value);
    }
}

}  // namespace db
}  // namespace dev
