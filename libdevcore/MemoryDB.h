// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2017-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

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