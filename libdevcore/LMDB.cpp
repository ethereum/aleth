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
/** @file LMDB.cpp
 * @author Isaac Hier <isaachier@gmail.com>
 * @date 2017
 */

#ifdef ETH_LMDB

#include "LMDB.h"

namespace dev
{
namespace db
{
namespace
{

template <typename Exception, typename Function>
typename Function::result_type protectedCall(Function fn)
{
	try
	{
		return fn();
	}
	catch (const std::exception& ex)
	{
		BOOST_THROW_EXCEPTION(Exception(ex.what()));
	}
	catch (...)
	{
		BOOST_THROW_EXCEPTION(Exception("An error occurred"));
	}
}

std::shared_ptr<lmdb::env> makeEnv(
	std::string const& _path, LMDB::Options const& _options
)
{
	std::function<std::shared_ptr<lmdb::env>()> func(
		[&_path, &_options]() {
		std::shared_ptr<lmdb::env> env(
			new lmdb::env(lmdb::env::create(_options.m_envOptions.m_createFlags))
		);
		env->open(
			_path.c_str(),
			_options.m_envOptions.m_openFlags,
			_options.m_envOptions.m_mode
		);

		if (_options.m_envOptions.m_maxDBs != LMDB::EnvOptions::kDefaultValue) {
			env->set_max_dbs(_options.m_envOptions.m_maxDBs);
		}
		if (_options.m_envOptions.m_mapSize != LMDB::EnvOptions::kDefaultValue) {
			env->set_mapsize(_options.m_envOptions.m_mapSize);
		}
		if (_options.m_envOptions.m_maxReaders != LMDB::EnvOptions::kDefaultValue) {
			env->set_max_readers(_options.m_envOptions.m_maxReaders);
		}
		return env;
	});
	return protectedCall<FailedToOpenDB>(func);
}

class LMDBTransaction: public Transaction
{
public:
	LMDBTransaction(lmdb::dbi* _db, lmdb::txn&& _txn): m_db(_db), m_txn(std::move(_txn))
	{
	}

	void insert(Slice const& _key, Slice const& _value) override
	{
		std::function<void()> func([this, &_key, &_value]() {
			const lmdb::val key(_key.data(), _key.size());
			const lmdb::val value(_value.data(), _value.size());
			m_db->put(m_txn.handle(), key, value);
		});
		protectedCall<FailedInsertInDB>(func);
	}

	void kill(Slice const& _key) override
	{
		std::function<void()> func([this, &_key]() {
			const lmdb::val key(_key.data(), _key.size());
			m_db->del(m_txn.handle(), key);
		});
		protectedCall<FailedInsertInDB>(func);
	}

	void commit() override
	{
		std::function<void()> func([this]() {
			m_txn.commit();
		});
		protectedCall<FailedCommitInDB>(func);
	}

	void rollback() override
	{
		std::function<void()> func([this]() {
			m_txn.abort();
		});
		protectedCall<FailedRollbackInDB>(func);
	}

private:
	lmdb::dbi* m_db;
	lmdb::txn m_txn;
};

}

LMDB::LMDB(std::string const& _path, Options const& _options)
	: LMDB(makeEnv(_path, _options), _options.m_dbOptions)
{
}


LMDB::LMDB(std::shared_ptr<lmdb::env> _env, DBOptions const& _dbOptions)
	: m_env(_env)
{
	std::function<void()> func([this, &_dbOptions]() {
		auto transaction = lmdb::txn::begin(m_env->handle());
		std::unique_ptr<lmdb::dbi> db(
			new lmdb::dbi(lmdb::dbi::open(
				transaction.handle(),
				_dbOptions.m_name.empty() ? nullptr : _dbOptions.m_name.c_str(),
				_dbOptions.m_flags)));
		transaction.commit();
		m_db = std::move(db);
	});
	protectedCall<FailedToOpenDB>(func);
}

std::string LMDB::lookup(Slice const& _key) const
{
	std::function<std::string()> func([this, &_key]() {
		auto transaction = lmdb::txn::begin(m_env->handle());
		const lmdb::val key(_key.begin(), _key.size());
		lmdb::val value;
		m_db->get(transaction.handle(), key, value);
		return std::string(value.data(), value.size());
	});
	return protectedCall<FailedLookupInDB>(func);
}

bool LMDB::exists(Slice const& _key) const
{
	try
	{
		lookup(_key);
		return true;
	}
	catch (const FailedLookupInDB& /* ex */)
	{
		return false;
	}
}

void LMDB::insert(Slice const& _key, Slice const& _value)
{
	std::function<void()> func([this, &_key, &_value]() {
		auto transaction = lmdb::txn::begin(m_env->handle());
		const lmdb::val key(_key.data(), _key.size());
		const lmdb::val value(_value.data(), _value.size());
		m_db->put(transaction.handle(), key, value);
	});
	protectedCall<FailedInsertInDB>(func);
}

void LMDB::kill(Slice const& _key)
{
	std::function<void()> func([this, &_key]() {
		auto transaction = lmdb::txn::begin(m_env->handle());
		const lmdb::val key(_key.data(), _key.size());
		m_db->del(transaction.handle(), key);
	});
	protectedCall<FailedDeleteInDB>(func);
}

std::unique_ptr<Transaction> LMDB::begin()
{
	return std::unique_ptr<Transaction>(new LMDBTransaction(m_db.get(), lmdb::txn::begin(m_env->handle())));
}

}
}

#endif
