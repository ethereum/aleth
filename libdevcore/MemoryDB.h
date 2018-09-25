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

#include <mutex>
#include "Common.h"
#include "db.h"

namespace dev
{
namespace db
{
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
    void forEach(std::function<bool(Slice, Slice)> f) const override;

private:
        std::unordered_map<std::string, std::string> m_db;
        mutable std::mutex m_mutex;
};
}
}