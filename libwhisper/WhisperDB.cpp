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
/** @file WhisperDB.cpp
* @author Vladislav Gluhovsky <vlad@ethdev.com>
* @date July 2015
 */

#include "WhisperDB.h"
#include <boost/filesystem.hpp>
#include <libdevcore/FileSystem.h>
#include "WhisperHost.h"

using namespace std;
using namespace dev;
using namespace dev::shh;
namespace fs = boost::filesystem;

#if ETH_LMDB

namespace
{

template <typename ExceptionType>
void mdbThrowOnError(int returnCode)
{
	if (returnCode != 0)
		BOOST_THROW_EXCEPTION(ExceptionType(mdb_strerror(returnCode)));
}

auto transactionCloser = [](MDB_txn* transaction) {
	if (transaction)
		mdb_txn_abort(transaction);
};

using Transaction = std::unique_ptr<MDB_txn, decltype(transactionCloser)>;

}  // anonymous namespace

WhisperDB::WhisperDB(string const& _type)
{
	fs::path path = dev::getDataDir("shh");
	fs::create_directories(path);
	DEV_IGNORE_EXCEPTIONS(fs::permissions(path, fs::owner_all));
	path += "/" + _type;
	MDB_env* env = nullptr;
	mdbThrowOnError<FailedToOpenLevelDB>(mdb_env_create(&env));
	mdbThrowOnError<FailedToOpenLevelDB>(mdb_env_open(env, path.string().c_str(), 0));
	m_env.reset(env);
	Transaction transaction;
	mdbThrowOnError<FailedToOpenLevelDB>(mdb_txn_begin(m_env.get(), nullptr, 0, &transaction.get()));
	MDB_dbi db = 0;
	mdbThrowOnError<FailedToOpenLevelDB>(mdb_dbi_open(m_env.get(), nullptr, MDB_CREATE, &db));
	mdbThrowOnError<FailedToOpenLevelDB>(mdb_txn_commit(transaction.get()));
	m_db = db;
}

string WhisperDB::lookup(dev::h256 const& _key) const
{
	Transaction transaction;
	mdbThrowOnError<FailedLookupInLevelDB>(mdb_txn_begin(m_env.get(), nullptr, MDB_RDONLY, &transaction.get()));
	MDB_val key{_key.size, _key.data()};
	MDB_val value{0, nullptr};
	mdbThrowOnError<FailedLookupInLevelDB>(mdb_get(transaction.get(), m_db, &key, &value));
	return string(static_cast<const char*>(value.mv_data), value.mv_size);
}

void WhisperDB::insert(dev::h256 const& _key, string const& _value)
{
	string valueCopy(value);
	insert(_key, &valueCopy[0], valueCopy.size());
}

void WhisperDB::insert(dev::h256 const& _key, bytes const& _value)
{
	bytes valueCopy(value);
	insert(_key, &valueCopy[0], valueCopy.size());
}

void WhisperDB::insert(dev::h256 const& _key, void* _valueData, size_t _valueSize)
{
	dev::h256 keyCopy(_key);
	MDB_val key{keyCopy.size, static_cast<void*>(keyCopy.data())};
	MDB_val value{_valueSize, _valueData}
	Transaction transaction;
	mdbThrowOnError<FailedInsertInLevelDB>(mdb_txn_begin(m_db.env(), nullptr, 0, &transaction.get()));
	mdbThrowOnError<FailedInsertInLevelDB>(mdb_put(transaction.get(), m_db, &key, &value, 0));
	mdbThrowOnError<FailedInsertInLevelDB>(mdb_txn_commit(transaction.get()));
}

void WhisperDB::insert(dev::h256 const& _key, bytes const& _value)
{
	dev::h256 keyCopy(key);
	MDB_val key{keyCopy.size, static_cast<void*>(keyCopy.data())};
	bytes valueCopy(_value);
	MDB_val value{valueCopy.size(), static_cast<void*>(&valueCopy[0])};
	Transaction transaction;
	mdbThrowOnError<FailedInsertInLevelDB>(mdb_txn_begin(m_db.env(), nullptr, 0, &transaction.get()));
	mdbThrowOnError<FailedInsertInLevelDB>(mdb_put(transaction.get(), m_db, &key, &value, 0));
	mdbThrowOnError<FailedInsertInLevelDB>(mdb_txn_commit(transaction.get()));
}

void WhisperDB::kill(dev::h256 const& _key)
{
	ldb::Slice slice((char const*)_key.data(), _key.size);
	ldb::Status status = m_db->Delete(m_writeOptions, slice);
	if (!status.ok())
		BOOST_THROW_EXCEPTION(FailedDeleteInLevelDB(status.ToString()));
}

void WhisperMessagesDB::loadAllMessages(std::map<h256, Envelope>& o_dst)
{
	ldb::ReadOptions op;
	op.fill_cache = false;
	op.verify_checksums = true;
	vector<string> wasted;
	unique_ptr<ldb::Iterator> it(m_db->NewIterator(op));
	unsigned const now = utcTime();

	for (it->SeekToFirst(); it->Valid(); it->Next())
	{
		ldb::Slice const k = it->key();
		ldb::Slice const v = it->value();
		bool useless = true;

		try
		{
			RLP rlp((byte const*)v.data(), v.size());
			Envelope e(rlp);
			h256 h2 = e.sha3();
			h256 h1;

			if (k.size() == h256::size)
				h1 = h256((byte const*)k.data(), h256::ConstructFromPointer);

			if (h1 != h2)
				cwarn << "Corrupted data in Level DB:" << h1.hex() << "versus" << h2.hex();
			else if (e.expiry() > now)
			{
				o_dst[h1] = e;
				useless = false;
			}
		}
		catch(RLPException const& ex)
		{
			cwarn << "RLPException in WhisperDB::loadAll():" << ex.what();
		}
		catch(Exception const& ex)
		{
			cwarn << "Exception in WhisperDB::loadAll():" << ex.what();
		}

		if (useless)
			wasted.push_back(k.ToString());
	}

	cdebug << "WhisperDB::loadAll(): loaded " << o_dst.size() << ", deleted " << wasted.size() << "messages";

	for (auto const& k: wasted)
	{
		ldb::Status status = m_db->Delete(m_writeOptions, k);
		if (!status.ok())
			cwarn << "Failed to delete an entry from Level DB:" << k;
	}
}

void WhisperMessagesDB::saveSingleMessage(h256 const& _key, Envelope const& _e)
{
	try
	{
		RLPStream rlp;
		_e.streamRLP(rlp);
		bytes b;
		rlp.swapOut(b);
		insert(_key, b);
	}
	catch(RLPException const& ex)
	{
		cwarn << boost::diagnostic_information(ex);
	}
	catch(FailedInsertInLevelDB const& ex)
	{
		cwarn << boost::diagnostic_information(ex);
	}
}

#else

WhisperDB::WhisperDB(string const& _type)
{
	m_readOptions.verify_checksums = true;
	fs::path path = dev::getDataDir("shh");
	fs::create_directories(path);
	DEV_IGNORE_EXCEPTIONS(fs::permissions(path, fs::owner_all));
	path += "/" + _type;
	ldb::Options op;
	op.create_if_missing = true;
	op.max_open_files = 256;
	ldb::DB* p = nullptr;
	ldb::Status status = ldb::DB::Open(op, path.string(), &p);
	m_db.reset(p);
	if (!status.ok())
		BOOST_THROW_EXCEPTION(FailedToOpenLevelDB(status.ToString()));
}

string WhisperDB::lookup(dev::h256 const& _key) const
{
	string ret;
	ldb::Slice slice((char const*)_key.data(), _key.size);
	ldb::Status status = m_db->Get(m_readOptions, slice, &ret);
	if (!status.ok() && !status.IsNotFound())
		BOOST_THROW_EXCEPTION(FailedLookupInLevelDB(status.ToString()));

	return ret;
}

void WhisperDB::insert(dev::h256 const& _key, string const& _value)
{
	ldb::Slice slice((char const*)_key.data(), _key.size);
	ldb::Status status = m_db->Put(m_writeOptions, slice, _value);
	if (!status.ok())
		BOOST_THROW_EXCEPTION(FailedInsertInLevelDB(status.ToString()));
}

void WhisperDB::insert(dev::h256 const& _key, bytes const& _value)
{
	ldb::Slice k((char const*)_key.data(), _key.size);
	ldb::Slice v((char const*)_value.data(), _value.size());
	ldb::Status status = m_db->Put(m_writeOptions, k, v);
	if (!status.ok())
		BOOST_THROW_EXCEPTION(FailedInsertInLevelDB(status.ToString()));
}

void WhisperDB::kill(dev::h256 const& _key)
{
	ldb::Slice slice((char const*)_key.data(), _key.size);
	ldb::Status status = m_db->Delete(m_writeOptions, slice);
	if (!status.ok())
		BOOST_THROW_EXCEPTION(FailedDeleteInLevelDB(status.ToString()));
}

void WhisperMessagesDB::loadAllMessages(std::map<h256, Envelope>& o_dst)
{
	ldb::ReadOptions op;
	op.fill_cache = false;
	op.verify_checksums = true;
	vector<string> wasted;
	unique_ptr<ldb::Iterator> it(m_db->NewIterator(op));
	unsigned const now = utcTime();

	for (it->SeekToFirst(); it->Valid(); it->Next())
	{
		ldb::Slice const k = it->key();
		ldb::Slice const v = it->value();
		bool useless = true;

		try
		{
			RLP rlp((byte const*)v.data(), v.size());
			Envelope e(rlp);
			h256 h2 = e.sha3();
			h256 h1;

			if (k.size() == h256::size)
				h1 = h256((byte const*)k.data(), h256::ConstructFromPointer);

			if (h1 != h2)
				cwarn << "Corrupted data in Level DB:" << h1.hex() << "versus" << h2.hex();
			else if (e.expiry() > now)
			{
				o_dst[h1] = e;
				useless = false;
			}
		}
		catch(RLPException const& ex)
		{
			cwarn << "RLPException in WhisperDB::loadAll():" << ex.what();
		}
		catch(Exception const& ex)
		{
			cwarn << "Exception in WhisperDB::loadAll():" << ex.what();
		}

		if (useless)
			wasted.push_back(k.ToString());
	}

	cdebug << "WhisperDB::loadAll(): loaded " << o_dst.size() << ", deleted " << wasted.size() << "messages";

	for (auto const& k: wasted)
	{
		ldb::Status status = m_db->Delete(m_writeOptions, k);
		if (!status.ok())
			cwarn << "Failed to delete an entry from Level DB:" << k;
	}
}

void WhisperMessagesDB::saveSingleMessage(h256 const& _key, Envelope const& _e)
{
	try
	{
		RLPStream rlp;
		_e.streamRLP(rlp);
		bytes b;
		rlp.swapOut(b);
		insert(_key, b);
	}
	catch(RLPException const& ex)
	{
		cwarn << boost::diagnostic_information(ex);
	}
	catch(FailedInsertInLevelDB const& ex)
	{
		cwarn << boost::diagnostic_information(ex);
	}
}

#endif
