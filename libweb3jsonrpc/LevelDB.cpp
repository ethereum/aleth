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
 * @authors:
 *   Gav Wood <i@gavwood.com>
 *   Marek Kotewicz <marek@ethdev.com>
 * @date 2015
 */

#include "LevelDB.h"
#include <leveldb/db.h>
#include <boost/filesystem.hpp>
#include <libdevcore/FileSystem.h>
#include <libdevcore/SHA3.h>

using namespace std;
using namespace dev;
using namespace dev::rpc;
namespace fs = boost::filesystem;
namespace ldb = leveldb;

LevelDB::LevelDB()
{
	auto path = getDataDir() + "/.web3";
	fs::create_directories(path);
	DEV_IGNORE_EXCEPTIONS(fs::permissions(path, fs::owner_all));
	ldb::Options o;
	o.create_if_missing = true;
	ldb::DB::Open(o, path, &m_db);
}

bool LevelDB::db_put(std::string const& _name, std::string const& _key, std::string const& _value)
{
	bytes k = sha3(_name).asBytes() + sha3(_key).asBytes();
	auto status = m_db->Put(ldb::WriteOptions(),
							ldb::Slice((char const*)k.data(), k.size()),
							ldb::Slice((char const*)_value.data(), _value.size()));
	return status.ok();
}

std::string LevelDB::db_get(std::string const& _name, std::string const& _key)
{
	bytes k = sha3(_name).asBytes() + sha3(_key).asBytes();
	string ret;
	m_db->Get(ldb::ReadOptions(), ldb::Slice((char const*)k.data(), k.size()), &ret);
	return ret;
}