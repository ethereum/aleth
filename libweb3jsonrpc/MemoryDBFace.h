#pragma once
#include "AbstractDb.h"

class MemoryDBFace: public AbstractDb
{
public:
	virtual bool db_put(std::string const& _name, std::string const& _key, std::string const& _value) override;
	virtual std::string db_get(std::string const& _name, std::string const& _key) override;
	
private:
	std::map<std::string, std::string> m_db;
};