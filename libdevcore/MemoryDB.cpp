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

#include "MemoryDB.h"

namespace dev
{
namespace db
{

void MemoryDBWriteBatch::insert(Slice _key, Slice _value)
{
    m_batch[_key.toString()] = _value.toString();
}

void MemoryDBWriteBatch::kill(Slice _key)
{
    m_batch.erase(_key.toString());
}

std::string MemoryDB::lookup(Slice _key) const
{
    Guard lock(m_mutex);
    auto const& it = m_db.find(_key.toString());
    if (it != m_db.end())
        return it->second;
    return {};
}

bool MemoryDB::exists(Slice _key) const
{
    Guard lock(m_mutex);
    return m_db.count(_key.toString()) != 0;
}

void MemoryDB::insert(Slice _key, Slice _value)
{
    Guard lock(m_mutex);
    m_db[_key.toString()] = _value.toString();
}

void MemoryDB::kill(Slice _key)
{
    Guard lock(m_mutex);
    m_db.erase(_key.toString());
}

std::unique_ptr<WriteBatchFace> MemoryDB::createWriteBatch() const
{
    return std::unique_ptr<WriteBatchFace>(new MemoryDBWriteBatch);
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
        BOOST_THROW_EXCEPTION(
            DatabaseError() << errinfo_comment("Invalid batch type passed to MemoryDB::commit"));
    }
    auto const& batch = batchPtr->writeBatch();
    Guard lock(m_mutex);
    for (auto& e : batch)
    {
        m_db[e.first] = e.second;
    }
}

// A database must implement the `forEach` method that allows the caller
// to pass in a function `f`, which will be called with the key and value
// of each record in the database. If `f` returns false, the `forEach`
// method must return immediately.
void MemoryDB::forEach(std::function<bool(Slice, Slice)> _f) const
{
    Guard lock(m_mutex);
    for (auto const& e : m_db)
    {
        if (!_f(Slice(e.first), Slice(e.second)))
        {
            return;
        }
    }
}

}  // namespace db
}  // namespace dev