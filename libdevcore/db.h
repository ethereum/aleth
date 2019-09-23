// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include "Exceptions.h"
#include "dbfwd.h"

#include <memory>
#include <string>

namespace dev
{
namespace db
{
// WriteBatchFace implements database write batch for a specific concrete
// database implementation.
class WriteBatchFace
{
public:
    virtual ~WriteBatchFace() = default;

    virtual void insert(Slice _key, Slice _value) = 0;
    virtual void kill(Slice _key) = 0;

protected:
    WriteBatchFace() = default;
    // Noncopyable
    WriteBatchFace(WriteBatchFace const&) = delete;
    WriteBatchFace& operator=(WriteBatchFace const&) = delete;
    // Nonmovable
    WriteBatchFace(WriteBatchFace&&) = delete;
    WriteBatchFace& operator=(WriteBatchFace&&) = delete;
};

class DatabaseFace
{
public:
    virtual ~DatabaseFace() = default;
    virtual std::string lookup(Slice _key) const = 0;
    virtual bool exists(Slice _key) const = 0;
    virtual void insert(Slice _key, Slice _value) = 0;
    virtual void kill(Slice _key) = 0;

    virtual std::unique_ptr<WriteBatchFace> createWriteBatch() const = 0;
    virtual void commit(std::unique_ptr<WriteBatchFace> _batch) = 0;

    // A database must implement the `forEach` method that allows the caller
    // to pass in a function `f`, which will be called with the key and value
    // of each record in the database. If `f` returns false, the `forEach`
    // method must return immediately.
    virtual void forEach(std::function<bool(Slice, Slice)> f) const = 0;
};

DEV_SIMPLE_EXCEPTION(DatabaseError);

enum class DatabaseStatus
{
    Ok,
    NotFound,
    Corruption,
    NotSupported,
    InvalidArgument,
    IOError,
    Unknown
};

using errinfo_dbStatusCode = boost::error_info<struct tag_dbStatusCode, DatabaseStatus>;
using errinfo_dbStatusString = boost::error_info<struct tag_dbStatusString, std::string>;

}  // namespace db
}  // namespace dev
