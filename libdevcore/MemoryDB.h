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

#pragma once

#include "Common.h"
#include "Guards.h"
#include "db.h"

namespace dev
{
namespace db
{
class MemoryDBWriteBatch : public WriteBatchFace
{
public:
    void insert(Slice _key, Slice _value) override;
    void kill(Slice _key) override;

    std::unordered_map<std::string, std::string>& writeBatch() { return m_batch; }
    size_t size() { return m_batch.size(); }

private:
    std::unordered_map<std::string, std::string> m_batch;
};

class MemoryDB : public DatabaseFace
{
public:
    std::string lookup(Slice _key) const override;
    bool exists(Slice _key) const override;
    void insert(Slice _key, Slice _value) override;
    void kill(Slice _key) override;

    std::unique_ptr<WriteBatchFace> createWriteBatch() const override;
    void commit(std::unique_ptr<WriteBatchFace> _batch) override;

    // A database must implement the `forEach` method that allows the caller
    // to pass in a function `f`, which will be called with the key and value
    // of each record in the database. If `f` returns false, the `forEach`
    // method must return immediately.
    void forEach(std::function<bool(Slice, Slice)> _f) const override;

    size_t size() const { return m_db.size(); }

private:
    std::unordered_map<std::string, std::string> m_db;
    mutable Mutex m_mutex;
};
}  // namespace db
}  // namespace dev