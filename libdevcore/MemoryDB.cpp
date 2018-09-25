#include "MemoryDB.h"

namespace dev
{
namespace db
{

using MemoryDBBatch = std::unordered_map<std::string, std::string>;

class MemoryDBWriteBatch : public WriteBatchFace
{
public:
    void insert(Slice _key, Slice _value) override;
    void kill(Slice _key) override;

    MemoryDBBatch const& writeBatch() const { return m_batch; }
    MemoryDBBatch& writeBatch() { return m_batch; }

private:
    MemoryDBBatch m_batch;
};

void MemoryDBWriteBatch::insert(Slice _key, Slice _value)
{
    m_batch[_key.data()] = _value.data();
}

void MemoryDBWriteBatch::kill(Slice _key)
{
    m_batch.erase(_key.data());
}

std::string MemoryDB::lookup(Slice _key) const
{
    std::string value;
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_db.count(_key.data()))
    {
        value = m_db.find(_key.data())->first;
    }
    return value;
}

bool MemoryDB::exists(Slice _key) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_db.count(_key.data()) != 0;
}

void MemoryDB::insert(Slice _key, Slice _value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_db[_key.data()] = _value.data();
}

void MemoryDB::kill(Slice _key)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_db.erase(_key.data());
}

std::unique_ptr<WriteBatchFace> MemoryDB::createWriteBatch() const
{
    return std::unique_ptr<WriteBatchFace>(new MemoryDBWriteBatch());
}

void MemoryDB::commit(std::unique_ptr<WriteBatchFace> _batch)
{
    if (!_batch)
    {
        BOOST_THROW_EXCEPTION(DatabaseError() << errinfo_comment("Cannot commit null batch"));
    }

    auto* batchPtr = dynamic_cast<MemoryDBWriteBatch*>(_batch.get());
    if (!batchPtr)
    {
        BOOST_THROW_EXCEPTION(DatabaseError() << errinfo_comment("Invalid batch type passed to MemoryDB::commit"));
    }
    MemoryDBBatch batch = batchPtr->writeBatch();
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& e : batch)
    {
        m_db[e.first] = e.second;
    }
}

// A database must implement the `forEach` method that allows the caller
// to pass in a function `f`, which will be called with the key and value
// of each record in the database. If `f` returns false, the `forEach`
// method must return immediately.
void MemoryDB::forEach(std::function<bool(Slice, Slice)> f) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& e : m_db)
    {
        if (!f(Slice(e.first.c_str()), Slice(e.second.c_str())))
        {
            return;
        }
    }
}
    
}
}