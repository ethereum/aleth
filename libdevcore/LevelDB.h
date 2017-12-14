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
/** @file LevelDB.h
 * @author Isaac Hier <isaachier@gmail.com>
 * @date 2017
 */

#pragma once

#ifdef ETH_LEVELDB

#include "db.h"

#include <leveldb/db.h>
#include <leveldb/write_batch.h>

namespace dev
{
namespace db
{

class LevelDB: public DB
{
public:
	static leveldb::ReadOptions defaultReadOptions();
	static leveldb::WriteOptions defaultWriteOptions();
	static leveldb::Options defaultDBOptions();

	explicit LevelDB(
		std::string const& _path,
		leveldb::ReadOptions _readOptions = defaultReadOptions(),
		leveldb::WriteOptions _writeOptions = defaultWriteOptions(),
		leveldb::Options _dbOptions = defaultDBOptions()
	);

	std::string lookup(Slice const& _key) const override;
	bool exists(Slice const& _key) const override;
	void insert(Slice const& _key, Slice const& _value) override;
	void kill(Slice const& _key) override;
	std::unique_ptr<Transaction> begin() override;
	void forEach(std::function<bool(Slice const&, Slice const&)> f) const override;

private:
	std::unique_ptr<leveldb::DB> m_db;
	leveldb::ReadOptions m_readOptions;
	leveldb::WriteOptions m_writeOptions;
};

}
}

#endif
