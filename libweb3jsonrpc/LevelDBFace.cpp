#include "LevelDBFace.h"
#include <leveldb/db.h>
#include <boost/filesystem.hpp>
#include <libdevcore/FileSystem.h>
#include <libdevcore/SHA3.h>

using namespace std;
using namespace dev;
namespace fs = boost::filesystem;
namespace ldb = leveldb;

LevelDBFace::LevelDBFace()
{
	auto path = getDataDir() + "/.web3";
	fs::create_directories(path);
	DEV_IGNORE_EXCEPTIONS(fs::permissions(path, fs::owner_all));
	ldb::Options o;
	o.create_if_missing = true;
	ldb::DB::Open(o, path, &m_db);
}

bool LevelDBFace::db_put(std::string const& _name, std::string const& _key, std::string const& _value)
{
	bytes k = sha3(_name).asBytes() + sha3(_key).asBytes();
	auto status = m_db->Put(ldb::WriteOptions(),
							ldb::Slice((char const*)k.data(), k.size()),
							ldb::Slice((char const*)_value.data(), _value.size()));
	return status.ok();
}

std::string LevelDBFace::db_get(std::string const& _name, std::string const& _key)
{
	bytes k = sha3(_name).asBytes() + sha3(_key).asBytes();
	string ret;
	m_db->Get(ldb::ReadOptions(), ldb::Slice((char const*)k.data(), k.size()), &ret);
	return ret;
}