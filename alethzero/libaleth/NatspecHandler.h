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

/** @file NatspecHandler.h
 * @author Lefteris Karapetsas <lefteris@ethdev.com>
 * @date 2015
 */

#pragma once

#include <libdevcore/db.h>
#include <json/json.h>
#include <libdevcore/FixedHash.h>
#include "Common.h"

namespace dev
{
namespace aleth
{

class NatspecHandler: public NatSpecFace
{
  public:
	NatspecHandler();
	~NatspecHandler();

	/// Stores locally in a levelDB a key value pair of contract code hash to natspec documentation
	virtual void add(dev::h256 const& _contractHash, std::string const& _doc) override;
	/// Retrieves the natspec documentation as a string given a contract code hash
	std::string retrieve(dev::h256 const& _contractHash) const override;

	/// Given a json natspec string and the transaction data return the user notice
	virtual std::string userNotice(std::string const& json, const dev::bytes& _transactionData) override;
	/// Given a contract code hash and the transaction's data retrieve the natspec documention's
	/// user notice for that transaction.
	/// @returns The user notice or an empty string if no natspec for the contract exists
	///          or if the existing natspec does not document the @c _methodName
	virtual std::string userNotice(dev::h256 const& _contractHash, dev::bytes const& _transactionDacta) override;
	
  private:
	ldb::ReadOptions m_readOptions;
	ldb::WriteOptions m_writeOptions;
	ldb::DB* m_db = nullptr;
	Json::Reader m_reader;
};

}
}
