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
/** @file RocksDB.cpp
 * @author Isaac Hier <isaachier@gmail.com>
 * @date 2017
 */

#ifdef ETH_ROCKSDB

#include "RocksDB.h"
#include "Assertions.h"

namespace dev
{
namespace db
{
namespace
{

class RocksDBTransaction: public Transaction
{
public:
	RocksDBTransaction(rocksdb::DB* _db, rocksdb::WriteOptions _writeOptions): m_db(_db), m_writeOptions(std::move(_writeOptions))
	{
		m_writeBatch.SetSavePoint();
	}

	~RocksDBTransaction();

	void insert(Slice const& _key, Slice const& _value) override;
	void kill(Slice const& _key) override;
	void commit() override;
	void rollback() override;

private:
	rocksdb::DB* m_db;
	rocksdb::WriteOptions m_writeOptions;
	rocksdb::WriteBatch m_writeBatch;
};

RocksDBTransaction::~RocksDBTransaction()
{
	if (m_db) {
		DEV_IGNORE_EXCEPTIONS(rollback());
	}
}

void RocksDBTransaction::insert(Slice const& _key, Slice const& _value)
{
	const auto status = m_writeBatch.Put(
		rocksdb::Slice(_key.data(), _key.size()),
		rocksdb::Slice(_value.data(), _value.size())
	);
	if (!status.ok()) {
		BOOST_THROW_EXCEPTION(FailedInsertInDB(status.ToString()));
	}
}

void RocksDBTransaction::kill(Slice const& _key)
{
	const auto status = m_writeBatch.Delete(rocksdb::Slice(_key.data(), _key.size()));
	if (!status.ok()) {
		BOOST_THROW_EXCEPTION(FailedDeleteInDB(status.ToString()));
	}
}

void RocksDBTransaction::commit()
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

void RocksDBTransaction::rollback()
{
	assertThrow(
		m_db != nullptr,
		FailedRollbackInDB,
		"cannot rollback, transaction already committed or rolled back"
	);
	const auto status = m_writeBatch.RollbackToSavePoint();
	if (!status.ok()) {
		BOOST_THROW_EXCEPTION(FailedRollbackInDB(status.ToString()));
	}
	m_db = nullptr;
}

}

rocksdb::ReadOptions RocksDB::defaultReadOptions()
{
	rocksdb::ReadOptions readOptions;
	readOptions.verify_checksums = true;
	return readOptions;
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

RocksDB::RocksDB(
	std::string const& _path,
	rocksdb::ReadOptions _readOptions,
	rocksdb::WriteOptions _writeOptions,
	rocksdb::Options _dbOptions
): m_db(nullptr), m_readOptions(std::move(_readOptions)), m_writeOptions(std::move(_writeOptions))
{
	auto db = static_cast<rocksdb::DB*>(nullptr);
	const auto status = rocksdb::DB::Open(_dbOptions, _path, &db);
	if (!status.ok()) {
		BOOST_THROW_EXCEPTION(FailedToOpenDB(status.ToString()));
	}
	m_db.reset(db);
}

std::string RocksDB::lookup(Slice const& _key) const
{
	const rocksdb::Slice key(_key.data(), _key.size());
	std::string value;
	const auto status = m_db->Get(m_readOptions, key, &value);
	if (!status.ok()) {
		BOOST_THROW_EXCEPTION(FailedLookupInDB(status.ToString()));
	}
	return value;
}

bool RocksDB::exists(Slice const& _key) const
{
	std::string value;
	const rocksdb::Slice key(_key.data(), _key.size());
	if (!m_db->KeyMayExist(m_readOptions, key, &value, nullptr)) {
		return false;
	}
	const auto status = m_db->Get(m_readOptions, key, &value);
	return status.ok();
}

void RocksDB::insert(Slice const& _key, Slice const& _value)
{
	const rocksdb::Slice key(_key.data(), _key.size());
	const rocksdb::Slice value(_value.data(), _value.size());
	const auto status = m_db->Put(m_writeOptions, key, value);
	if (!status.ok()) {
		BOOST_THROW_EXCEPTION(FailedInsertInDB(status.ToString()));
	}
}

void RocksDB::kill(Slice const& _key)
{
	const rocksdb::Slice key(_key.data(), _key.size());
	const auto status = m_db->Delete(m_writeOptions, key);
	if (!status.ok()) {
		BOOST_THROW_EXCEPTION(FailedDeleteInDB(status.ToString()));
	}
}

std::unique_ptr<Transaction> RocksDB::begin()
{
	return std::unique_ptr<Transaction>(new RocksDBTransaction(m_db.get(), m_writeOptions));
}

void RocksDB::forEach(std::function<bool(Slice const&, Slice const&)> f) const
{
	std::unique_ptr<rocksdb::Iterator> itr(m_db->NewIterator(m_readOptions));
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
