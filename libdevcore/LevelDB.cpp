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
    if (!status.ok())
    {
        BOOST_THROW_EXCEPTION(FailedToOpenDB(status.ToString()));
    }
    if (!db)
    {
        BOOST_THROW_EXCEPTION(FailedToOpenDB("null database pointer"));
    }
    m_db.reset(db);
}

std::string LevelDB::lookup(Slice _key) const
{
    leveldb::Slice const key(_key.data(), _key.size());
    std::string value;
    auto const status = m_db->Get(m_readOptions, key, &value);
    if (!status.ok())
    {
        BOOST_THROW_EXCEPTION(FailedLookupInDB(status.ToString()));
    }
    return value;
}

bool LevelDB::exists(Slice _key) const
{
    std::string value;
    leveldb::Slice const key(_key.data(), _key.size());
    auto const status = m_db->Get(m_readOptions, key, &value);
    return status.ok();
}

void LevelDB::insert(Slice _key, Slice _value)
{
    leveldb::Slice const key(_key.data(), _key.size());
    leveldb::Slice const value(_value.data(), _value.size());
    auto const status = m_db->Put(m_writeOptions, key, value);
    if (!status.ok())
    {
        BOOST_THROW_EXCEPTION(FailedInsertInDB(status.ToString()));
    }
}

void LevelDB::kill(Slice _key)
{
    leveldb::Slice const key(_key.data(), _key.size());
    auto const status = m_db->Delete(m_writeOptions, key);
    if (!status.ok())
    {
        BOOST_THROW_EXCEPTION(FailedDeleteInDB(status.ToString()));
    }
}

std::unique_ptr<WriteBatchFace> LevelDB::createWriteBatch() const
{
    return std::unique_ptr<WriteBatchFace>(new LevelDBWriteBatch());
}

void LevelDB::commit(std::unique_ptr<WriteBatchFace> _batch)
{
    if (!_batch)
    {
        BOOST_THROW_EXCEPTION(FailedCommitInDB("Cannot commit null batch"));
    }
    auto* batchPtr = dynamic_cast<LevelDBWriteBatch*>(_batch.get());
    if (!batchPtr)
    {
        BOOST_THROW_EXCEPTION(FailedCommitInDB("Invalid batch type passed to LevelDB::commit"));
    }
    auto const status = m_db->Write(m_writeOptions, &batchPtr->writeBatch());
    if (!status.ok())
    {
        BOOST_THROW_EXCEPTION(FailedCommitInDB(status.ToString()));
    }
}

void LevelDB::forEach(std::function<bool(Slice, Slice)> f) const
{
    std::unique_ptr<leveldb::Iterator> itr(m_db->NewIterator(m_readOptions));
    if (itr == nullptr)
    {
        BOOST_THROW_EXCEPTION(FailedIterateDB("null iterator"));
    }
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
