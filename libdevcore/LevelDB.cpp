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
/** @file LevelDB.cpp
 * @author Isaac Hier <isaachier@gmail.com>
 * @date 2017
 */

#ifdef ETH_LEVELDB

#include "LevelDB.h"
#include "Assertions.h"

namespace dev
{
namespace db
{
namespace
{

class LevelDBTransaction: public Transaction
{
public:
	LevelDBTransaction(leveldb::DB* _db, leveldb::WriteOptions _writeOptions): m_db(_db), m_writeOptions(std::move(_writeOptions))
	{
	}

	~LevelDBTransaction();

	void insert(Slice const& _key, Slice const& _value) override;
	void kill(Slice const& _key) override;
	void commit() override;
	void rollback() override;

private:
	leveldb::DB* m_db;
	leveldb::WriteOptions m_writeOptions;
	leveldb::WriteBatch m_writeBatch;
};

LevelDBTransaction::~LevelDBTransaction()
{
	if (m_db) {
		DEV_IGNORE_EXCEPTIONS(rollback());
	}
}

void LevelDBTransaction::insert(Slice const& _key, Slice const& _value)
{
	m_writeBatch.Put(
		leveldb::Slice(_key.data(), _key.size()),
		leveldb::Slice(_value.data(), _value.size())
	);
}

void LevelDBTransaction::kill(Slice const& _key)
{
	m_writeBatch.Delete(leveldb::Slice(_key.data(), _key.size()));
}

void LevelDBTransaction::commit()
{
	assertThrow(
		m_db != nullptr,
		FailedCommitInDB,
		"cannot commit, transaction already committed or rolled back"
	);
	const auto status = m_db->Write(m_writeOptions, &m_writeBatch);
	if (!status.ok()) {
		BOOST_THROW_EXCEPTION(FailedCommitInDB(status.ToString()));
	}
	m_db = nullptr;
}

void LevelDBTransaction::rollback()
{
	assertThrow(
		m_db != nullptr,
		FailedRollbackInDB,
		"cannot rollback, transaction already committed or rolled back"
	);
	m_writeBatch.Clear();
	m_db = nullptr;
}

}

leveldb::ReadOptions LevelDB::defaultReadOptions()
{
	leveldb::ReadOptions readOptions;
	readOptions.verify_checksums = true;
	return readOptions;
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

LevelDB::LevelDB(
	std::string const& _path,
	leveldb::ReadOptions _readOptions,
	leveldb::WriteOptions _writeOptions,
	leveldb::Options _dbOptions
): m_db(nullptr), m_readOptions(std::move(_readOptions)), m_writeOptions(std::move(_writeOptions))
{
	auto db = static_cast<leveldb::DB*>(nullptr);
	const auto status = leveldb::DB::Open(_dbOptions, _path, &db);
	if (!status.ok()) {
		BOOST_THROW_EXCEPTION(FailedToOpenDB(status.ToString()));
	}
	if (!db) {
		BOOST_THROW_EXCEPTION(FailedToOpenDB("null database pointer"));
	}
	m_db.reset(db);
}

std::string LevelDB::lookup(Slice const& _key) const
{
	const leveldb::Slice key(_key.data(), _key.size());
	std::string value;
	const auto status = m_db->Get(m_readOptions, key, &value);
	if (!status.ok()) {
		BOOST_THROW_EXCEPTION(FailedLookupInDB(status.ToString()));
	}
	return value;
}

bool LevelDB::exists(Slice const& _key) const
{
	std::string value;
	const leveldb::Slice key(_key.data(), _key.size());
	const auto status = m_db->Get(m_readOptions, key, &value);
	return status.ok();
}

void LevelDB::insert(Slice const& _key, Slice const& _value)
{
	const leveldb::Slice key(_key.data(), _key.size());
	const leveldb::Slice value(_value.data(), _value.size());
	const auto status = m_db->Put(m_writeOptions, key, value);
	if (!status.ok()) {
		BOOST_THROW_EXCEPTION(FailedInsertInDB(status.ToString()));
	}
}

void LevelDB::kill(Slice const& _key)
{
	const leveldb::Slice key(_key.data(), _key.size());
	const auto status = m_db->Delete(m_writeOptions, key);
	if (!status.ok()) {
		BOOST_THROW_EXCEPTION(FailedDeleteInDB(status.ToString()));
	}
}

std::unique_ptr<Transaction> LevelDB::begin()
{
	return std::unique_ptr<Transaction>(new LevelDBTransaction(m_db.get(), m_writeOptions));
}

void LevelDB::forEach(std::function<bool(Slice const&, Slice const&)> f) const
{
	std::unique_ptr<leveldb::Iterator> itr(m_db->NewIterator(m_readOptions));
	if (itr == nullptr) {
		BOOST_THROW_EXCEPTION(FailedIterateDB("null iterator"));
	}
	auto keepIterating = true;
	for (itr->SeekToFirst(); keepIterating && itr->Valid(); itr->Next()) {
		const auto dbKey = itr->key();
		const auto dbValue = itr->value();
		const Slice key(dbKey.data(), dbKey.size());
		const Slice value(dbValue.data(), dbValue.size());
		keepIterating = f(key, value);
	}
}

}
}

#endif
