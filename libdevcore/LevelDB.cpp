// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2017-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.
#include "LevelDB.h"
#include "Assertions.h"

namespace dev
{
namespace db
{
namespace
{
inline leveldb::Slice toLDBSlice(Slice _slice)
{
    return leveldb::Slice(_slice.data(), _slice.size());
}

DatabaseStatus toDatabaseStatus(leveldb::Status const& _status)
{
    if (_status.ok())
        return DatabaseStatus::Ok;
    else if (_status.IsIOError())
        return DatabaseStatus::IOError;
    else if (_status.IsCorruption())
        return DatabaseStatus::Corruption;
    else if (_status.IsNotFound())
        return DatabaseStatus::NotFound;
    else
        return DatabaseStatus::Unknown;
}

void checkStatus(leveldb::Status const& _status, boost::filesystem::path const& _path = {})
{
    if (_status.ok())
        return;

    DatabaseError ex;
    ex << errinfo_dbStatusCode(toDatabaseStatus(_status))
       << errinfo_dbStatusString(_status.ToString());
    if (!_path.empty())
        ex << errinfo_path(_path.string());

    BOOST_THROW_EXCEPTION(ex);
}

class LevelDBWriteBatch : public WriteBatchFace
{
public:
    void insert(Slice _key, Slice _value) override;
    void kill(Slice _key) override;

    leveldb::WriteBatch const& writeBatch() const { return m_writeBatch; }
    leveldb::WriteBatch& writeBatch() { return m_writeBatch; }

private:
    leveldb::WriteBatch m_writeBatch;
};

void LevelDBWriteBatch::insert(Slice _key, Slice _value)
{
    m_writeBatch.Put(toLDBSlice(_key), toLDBSlice(_value));
}

void LevelDBWriteBatch::kill(Slice _key)
{
    m_writeBatch.Delete(toLDBSlice(_key));
}

}  // namespace

leveldb::ReadOptions LevelDB::defaultReadOptions()
{
    return leveldb::ReadOptions();
}

leveldb::WriteOptions LevelDB::defaultWriteOptions()
{
    return leveldb::WriteOptions();
}

leveldb::Options LevelDB::defaultDBOptions()
{
    leveldb::Options options;
    options.create_if_missing = true;
    options.max_open_files = 256;
    return options;
}

LevelDB::LevelDB(boost::filesystem::path const& _path, leveldb::ReadOptions _readOptions,
    leveldb::WriteOptions _writeOptions, leveldb::Options _dbOptions)
  : m_db(nullptr), m_readOptions(std::move(_readOptions)), m_writeOptions(std::move(_writeOptions))
{
    auto db = static_cast<leveldb::DB*>(nullptr);
    auto const status = leveldb::DB::Open(_dbOptions, _path.string(), &db);
    checkStatus(status, _path);

    assert(db);
    m_db.reset(db);
}

std::string LevelDB::lookup(Slice _key) const
{
    leveldb::Slice const key(_key.data(), _key.size());
    std::string value;
    auto const status = m_db->Get(m_readOptions, key, &value);
    if (status.IsNotFound())
        return std::string();

    checkStatus(status);
    return value;
}

bool LevelDB::exists(Slice _key) const
{
    std::string value;
    leveldb::Slice const key(_key.data(), _key.size());
    auto const status = m_db->Get(m_readOptions, key, &value);
    if (status.IsNotFound())
        return false;

    checkStatus(status);
    return true;
}

void LevelDB::insert(Slice _key, Slice _value)
{
    leveldb::Slice const key(_key.data(), _key.size());
    leveldb::Slice const value(_value.data(), _value.size());
    auto const status = m_db->Put(m_writeOptions, key, value);
    checkStatus(status);
}

void LevelDB::kill(Slice _key)
{
    leveldb::Slice const key(_key.data(), _key.size());
    auto const status = m_db->Delete(m_writeOptions, key);
    checkStatus(status);
}

std::unique_ptr<WriteBatchFace> LevelDB::createWriteBatch() const
{
    return std::unique_ptr<WriteBatchFace>(new LevelDBWriteBatch());
}

void LevelDB::commit(std::unique_ptr<WriteBatchFace> _batch)
{
    if (!_batch)
    {
        BOOST_THROW_EXCEPTION(DatabaseError() << errinfo_comment("Cannot commit null batch"));
    }
    auto* batchPtr = dynamic_cast<LevelDBWriteBatch*>(_batch.get());
    if (!batchPtr)
    {
        BOOST_THROW_EXCEPTION(
            DatabaseError() << errinfo_comment("Invalid batch type passed to LevelDB::commit"));
    }
    auto const status = m_db->Write(m_writeOptions, &batchPtr->writeBatch());
    checkStatus(status);
}

void LevelDB::forEach(std::function<bool(Slice, Slice)> _f) const
{
    std::unique_ptr<leveldb::Iterator> itr(m_db->NewIterator(m_readOptions));
    if (itr == nullptr)
    {
        BOOST_THROW_EXCEPTION(DatabaseError() << errinfo_comment("null iterator"));
    }
    auto keepIterating = true;
    for (itr->SeekToFirst(); keepIterating && itr->Valid(); itr->Next())
    {
        auto const dbKey = itr->key();
        auto const dbValue = itr->value();
        Slice const key(dbKey.data(), dbKey.size());
        Slice const value(dbValue.data(), dbValue.size());
        keepIterating = _f(key, value);
    }
}

}  // namespace db
}  // namespace dev
