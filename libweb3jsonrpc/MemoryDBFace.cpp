#include "MemoryDBFace.h"

bool MemoryDBFace::db_put(std::string const& _name, std::string const& _key, std::string const& _value)
{
	std::string k(_name + "/" + _key);
	m_db[k] = _value;
	return true;
}

std::string MemoryDBFace::db_get(std::string const& _name, std::string const& _key)
{
	std::string k(_name + "/" + _key);
	return m_db[k];
}