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
/** @file WhisperDB.h
* @author Vladislav Gluhovsky <vlad@ethdev.com>
* @date July 2015
 */

#pragma once

#include <libdevcore/db.h>
#include <libdevcore/FixedHash.h>
#include "Common.h"
#include "Message.h"

namespace dev
{
namespace shh
{

using WrongTypeLevelDB = dev::db::WrongTypeDB;
using FailedToOpenLevelDB = dev::db::FailedToOpenDB;
using FailedInsertInLevelDB = dev::db::FailedInsertInDB;
using FailedLookupInLevelDB = dev::db::FailedLookupInDB;
using FailedDeleteInLevelDB = dev::db::FailedDeleteInDB;

class WhisperHost;

class WhisperDB
{
public:
	explicit WhisperDB(std::string const& _type);
	virtual ~WhisperDB() {}
	std::string lookup(dev::h256 const& _key) const;
	void insert(dev::h256 const& _key, std::string const& _value);
	void insert(dev::h256 const& _key, bytes const& _value);
	void kill(dev::h256 const& _key);

protected:
#if ETH_LMDB
	void insert(dev::h256 const& _key, void* _valueData, size_t _valueSize);

	std::unique_ptr<MDB_env, &mdb_env_close> m_env;
	MDB_dbi m_db;
#else
	ldb::ReadOptions m_readOptions;
	ldb::WriteOptions m_writeOptions;
	std::unique_ptr<ldb::DB> m_db;
#endif
};

class WhisperMessagesDB: public WhisperDB
{
public:
	WhisperMessagesDB(): WhisperDB("messages") {}
	virtual ~WhisperMessagesDB() {}
	void loadAllMessages(std::map<h256, Envelope>& o_dst);
	void saveSingleMessage(dev::h256 const& _key, Envelope const& _e);
};

class WhisperFiltersDB: public WhisperDB
{
public:
	WhisperFiltersDB(): WhisperDB("filters") {}
	virtual ~WhisperFiltersDB() {}
};

}
}
