#pragma once
#include "AbstractDb.h"

namespace leveldb
{
	class DB;
}

class LevelDBFace: public AbstractDb
{
public:
	LevelDBFace();
	virtual bool db_put(std::string const& _name, std::string const& _key, std::string const& _value) override;
	virtual std::string db_get(std::string const& _name, std::string const& _key) override;
	
private:
	leveldb::DB* m_db;
};