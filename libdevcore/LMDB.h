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
/** @file LMDB.h
 * @author Isaac Hier <isaachier@gmail.com>
 * @date 2017
 */

#pragma once

#ifdef ETH_LMDB

#include "db.h"

#include <lmdbxx/lmdb++.h>

namespace dev
{
namespace db
{

class LMDB: public DB
{
public:
	struct EnvOptions {
		static constexpr auto kDefaultValue = 0;

		unsigned int m_createFlags = lmdb::env::default_flags;
		unsigned int m_openFlags = lmdb::env::default_flags;
		lmdb::mode m_mode;
		MDB_dbi m_maxDBs = kDefaultValue;
		std::size_t m_mapSize = kDefaultValue;
		unsigned int m_maxReaders = kDefaultValue;
	};

	struct DBOptions {
		unsigned int m_flags = lmdb::dbi::default_flags;
		std::string m_name;
	};

	struct Options {
		EnvOptions m_envOptions;
		DBOptions m_dbOptions;
	};

	LMDB(std::string const& _path, Options const& _options);
	LMDB(std::shared_ptr<lmdb::env> _env, DBOptions const& _dbOptions);
	std::string lookup(Slice const& _key) const override;
	bool exists(Slice const& _key) const override;
	void insert(Slice const& _key, Slice const& _value) override;
	void kill(Slice const& _key) override;
	std::unique_ptr<Transaction> begin() override;

private:
	std::shared_ptr<lmdb::env> m_env;
	std::unique_ptr<lmdb::dbi> m_db;
};

}
}

#endif
