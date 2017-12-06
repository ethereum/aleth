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
/** @file DB.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include "Exceptions.h"
#include "dbfwd.h"

#include <memory>
#include <string>

namespace dev
{
namespace db
{

// Transaction implements database transaction for a specific concrete database
// implementation. The transaction implementation must call rollback in its
// destructor if commit has not yet been called (making sure to catch any
// exceptions).
class Transaction
{
public:
	virtual ~Transaction() = default;

	virtual void insert(Slice const& _key, Slice const& _value) = 0;
	virtual void kill(Slice const& _key) = 0;

	virtual void commit() = 0;
	virtual void rollback() = 0;

protected:
	Transaction() = default;
	// Noncopyable
	Transaction(const Transaction&) = delete;
	Transaction& operator=(const Transaction&) = delete;
	// Movable
	Transaction(Transaction&&) = default;
	Transaction& operator=(Transaction&&) = default;
};

class DB
{
public:
	virtual ~DB() = default;
	virtual std::string lookup(Slice const& _key) const = 0;
	virtual bool exists(Slice const& _key) const = 0;
	virtual void insert(Slice const& _key, Slice const& _value) = 0;
	virtual void kill(Slice const& _key) = 0;
	virtual std::unique_ptr<Transaction> begin() = 0;
};

struct WrongTypeDB: virtual Exception { using Exception::Exception; };
struct FailedToOpenDB: virtual Exception { using Exception::Exception; };
struct FailedInsertInDB: virtual Exception { using Exception::Exception; };
struct FailedLookupInDB: virtual Exception { using Exception::Exception; };
struct FailedDeleteInDB: virtual Exception { using Exception::Exception; };
struct FailedCommitInDB: virtual Exception { using Exception::Exception; };
struct FailedRollbackInDB: virtual Exception { using Exception::Exception; };

}
}
