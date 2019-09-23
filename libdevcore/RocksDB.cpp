// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2018-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "RocksDB.h"
#include "Assertions.h"

namespace dev
{
namespace db
{
namespace
{

using errinfo_rocksdbStatusCode = boost::error_info<struct tag_rocksdbStatusCode, rocksdb::Status::Code>;
using errinfo_rocksdbStatusSubCode = boost::error_info<struct tag_rocksdbStatusSubCode, rocksdb::Status::SubCode>;

DatabaseStatus toDatabaseStatus(rocksdb::Status const& _status)
{
    switch (_status.code())
    {
        case rocksdb::Status::kOk:
            return DatabaseStatus::Ok;
        case rocksdb::Status::kNotFound:
            return DatabaseStatus::NotFound;
        case rocksdb::Status::kCorruption:
            return DatabaseStatus::Corruption;
        case rocksdb::Status::kNotSupported:
            return DatabaseStatus::NotSupported;
        case rocksdb::Status::kInvalidArgument:
            return DatabaseStatus::InvalidArgument;
        case rocksdb::Status::kIOError:
            return DatabaseStatus::IOError;
        default:
            return DatabaseStatus::Unknown;
    }
}

void checkStatus(rocksdb::Status const& _status, boost::filesystem::path const& _path = {})
{
    if (_status.ok())
        return;

    DatabaseError ex;
    ex << errinfo_dbStatusCode(toDatabaseStatus(_status))
       << errinfo_rocksdbStatusCode(_status.code())
       << errinfo_rocksdbStatusSubCode(_status.subcode())
       << errinfo_dbStatusString(_status.ToString());
    if (!_path.empty())
        ex << errinfo_path(_path.string());

    BOOST_THROW_EXCEPTION(ex);
}

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
    checkStatus(status);
}

void RocksDBWriteBatch::kill(Slice _key)
{
    auto const status = m_writeBatch.Delete(rocksdb::Slice(_key.data(), _key.size()));
    checkStatus(status);
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
    checkStatus(status, _path);

    assert(db);
    m_db.reset(db);
}

std::string RocksDB::lookup(Slice _key) const
{
    rocksdb::Slice const key(_key.data(), _key.size());
    std::string value;
    auto const status = m_db->Get(m_readOptions, key, &value);
    if (status.IsNotFound())
        return std::string();

    checkStatus(status);
    return value;
}

bool RocksDB::exists(Slice _key) const
{
    std::string value;
    rocksdb::Slice const key(_key.data(), _key.size());
    if (!m_db->KeyMayExist(m_readOptions, key, &value, nullptr))
        return false;

    auto const status = m_db->Get(m_readOptions, key, &value);
    if (status.IsNotFound())
        return false;

    checkStatus(status);
    return true;
}

void RocksDB::insert(Slice _key, Slice _value)
{
    rocksdb::Slice const key(_key.data(), _key.size());
    rocksdb::Slice const value(_value.data(), _value.size());
    auto const status = m_db->Put(m_writeOptions, key, value);
    checkStatus(status);
}

void RocksDB::kill(Slice _key)
{
    rocksdb::Slice const key(_key.data(), _key.size());
    auto const status = m_db->Delete(m_writeOptions, key);
    checkStatus(status);
}

std::unique_ptr<WriteBatchFace> RocksDB::createWriteBatch() const
{
    return std::unique_ptr<WriteBatchFace>(new RocksDBWriteBatch());
}

void RocksDB::commit(std::unique_ptr<WriteBatchFace> _batch)
{
    if (!_batch)
        BOOST_THROW_EXCEPTION(DatabaseError() << errinfo_comment("Cannot commit null batch"));

    auto* batchPtr = dynamic_cast<RocksDBWriteBatch*>(_batch.get());
    if (!batchPtr)
        BOOST_THROW_EXCEPTION(DatabaseError() << errinfo_comment("Invalid batch type passed to rocksdb::commit"));

    auto const status = m_db->Write(m_writeOptions, &batchPtr->writeBatch());
    checkStatus(status);
}

void RocksDB::forEach(std::function<bool(Slice, Slice)> f) const
{
    std::unique_ptr<rocksdb::Iterator> itr(m_db->NewIterator(m_readOptions));
    if (itr == nullptr)
        BOOST_THROW_EXCEPTION(DatabaseError() << errinfo_comment("null iterator"));

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
