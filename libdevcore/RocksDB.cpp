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
/** @file RocksDB.h
 * @author Isaac Hier <isaachier@gmail.com>
 * @date 2017
 */

#include "RocksDB.h"

namespace dev
{
namespace db
{
namespace
{

class RocksDBTransaction: public Transaction
{
public:
	RocksDBTransaction(rocksdb::DB* _db, rocksdb::WriteOptions _writeOptions): m_db(db), m_writeOptions(std::move(_writeOptions)), m_writeBatch()
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
	rocksdb::ColumnFamilyHandle* m_columnFamily;
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
		m_columnFamily,
		rocksdb::Slice(_key.data(), _key.size()),
		rocksdb::Slice(_value.data(), _value.size())
	);
	if (!status.ok()) {
		BOOST_THROW_EXCEPTION(FailedInsertInDB(status.ToString()));
	}
}

void RocksDBTransaction::kill(Slice const& _key)
{
	const auto status = m_writeBatch.Delete(
		m_columnFamily,
		rocksdb::Slice(_key.data(), _key.size())
	)
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
	const auto status = m_db->Commit(m_writeOptions, m_writeBatch);
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
}

}

rocksdb::ReadOptions RocksDB::defaultReadOptions()
{
	rocksdb::ReadOptions readOptions;
	readOptions.verify_checksums = true;
	readOptions.create_if_missing = true;
	return readOptions;
}

rocksdb::WriteOptions RocksDB::defaultWriteOptions()
{
	return rocksdb::WriteOptions();
}

std::string RocksDB::lookup(Slice const& _key) const
{
	const rocksdb::Slice key(_key.data(), _key.size());
	std::string value;
	const auto status = m_db.Get(m_readOptions, key, &value);
	if (!status.ok()) {
		BOOST_THROW_EXCEPTION(FailedLookupInDB(status.ToString()));
	}
	return value;
}

bool RocksDB::exists(Slice const& _key) const
{
	if (!m_db.KeyMayExist(m_readOptions)) {
		return false;
	}
	std::string value;
	const auto status = m_db.Get(m_readOptions, key, &value);
	return status.ok();
}

void RocksDB::insert(Slice const& _key, Slice const& _value)
{
	const rocksdb::Slice key(_key.data(), _key.size());
	const rocksdb::Slice value(_value.data(), _value.size());
	const auto status = m_db.Put(m_writeOptions, key, value);
	if (!status.ok()) {
		BOOST_THROW_EXCEPTION(FailedInsertInDB(status.ToString()));
	}
}

void RocksDB::kill(Slice const& _key)
{
	const rocksdb::Slice key(_key.data(), _key.size());
	const auto status = m_db.Delete(m_writeOptions, key);
	if (!status.ok()) {
		BOOST_THROW_EXCEPTION(FailedDeleteInDB(status.ToString()));
	}
}

std::unique_ptr<Transaction> RocksDB::begin()
{
	return std::unique_ptr<Transaction>(new RocksDBTransaction(m_db, m_writeOptions));
}

}
}
