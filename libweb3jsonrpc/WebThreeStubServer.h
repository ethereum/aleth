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
/** @file WebThreeStubServer.h
 * @authors:
 *   Gav Wood <i@gavwood.com>
 *   Marek Kotewicz <marek@ethdev.com>
 * @date 2014
 */

#pragma once

#pragma warning(push)
#pragma warning(disable: 4100 4267)
#include <leveldb/db.h>
#pragma warning(pop)

#include <iostream>
#include <jsonrpc/rpc.h>
#include <libdevcrypto/Common.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "abstractwebthreestubserver.h"
#pragma GCC diagnostic pop

namespace ldb = leveldb;

namespace dev
{
class WebThreeDirect;
class KeyPair;
namespace eth
{
class Interface;
}
namespace shh
{
class Interface;
}
}

/**
 * @brief JSON-RPC api implementation
 * @todo filters should work on unsigned instead of int
 * unsigned are not supported in json-rpc-cpp and there are bugs with double in json-rpc-cpp version 0.2.1
 * @todo split these up according to subprotocol (eth, shh, db, p2p, web3) and make it /very/ clear about how to add other subprotocols.
 * @todo modularise everything so additional subprotocols don't need to change this file.
 */
class WebThreeStubServer: public AbstractWebThreeStubServer
{
public:
	WebThreeStubServer(jsonrpc::AbstractServerConnector* _conn, dev::WebThreeDirect& _web3, std::vector<dev::KeyPair> const& _accounts);
	
	virtual Json::Value eth_accounts();
	virtual std::string eth_balanceAt(std::string const& _address);
	virtual Json::Value eth_blockByHash(std::string const& _hash);
	virtual Json::Value eth_blockByNumber(int const& _number);
	virtual std::string eth_call(Json::Value const& _json);
	virtual bool eth_changed(int const& _id);
	virtual std::string eth_codeAt(std::string const& _address);
	virtual std::string eth_coinbase();
	virtual Json::Value eth_compilers();
	virtual double eth_countAt(std::string const& _address);
	virtual int eth_defaultBlock();
	virtual std::string eth_gasPrice();
	virtual Json::Value eth_filterLogs(int const& _id);
	virtual Json::Value eth_logs(Json::Value const& _json);
	virtual bool eth_listening();
	virtual bool eth_mining();
	virtual int eth_newFilter(Json::Value const& _json);
	virtual int eth_newFilterString(std::string const& _filter);
	virtual int eth_number();
	virtual int eth_peerCount();
	virtual bool eth_setCoinbase(std::string const& _address);
	virtual bool eth_setDefaultBlock(int const& _block);
	virtual bool eth_setListening(bool const& _listening);
	virtual std::string eth_lll(std::string const& _s);
	virtual std::string eth_serpent(std::string const& _s);
	virtual bool eth_setMining(bool const& _mining);
	virtual std::string eth_solidity(std::string const& _code);
	virtual std::string eth_stateAt(std::string const& _address, std::string const& _storage);
	virtual Json::Value eth_storageAt(std::string const& _address);
	virtual std::string eth_transact(Json::Value const& _json);
	virtual Json::Value eth_transactionByHash(std::string const& _hash, int const& _i);
	virtual Json::Value eth_transactionByNumber(int const& _number, int const& _i);
	virtual Json::Value eth_uncleByHash(std::string const& _hash, int const& _i);
	virtual Json::Value eth_uncleByNumber(int const& _number, int const& _i);
	virtual bool eth_uninstallFilter(int const& _id);

	virtual std::string db_get(std::string const& _name, std::string const& _key);
	virtual std::string db_getString(std::string const& _name, std::string const& _key);
	virtual bool db_put(std::string const& _name, std::string const& _key, std::string const& _value);
	virtual bool db_putString(std::string const& _name, std::string const& _key, std::string const& _value);

	virtual std::string shh_addToGroup(std::string const& _group, std::string const& _who);
	virtual Json::Value shh_changed(int const& _id);
	virtual bool shh_haveIdentity(std::string const& _id);
	virtual int shh_newFilter(Json::Value const& _json);
	virtual std::string shh_newGroup(std::string const& _id, std::string const& _who);
	virtual std::string shh_newIdentity();
	virtual bool shh_post(Json::Value const& _json);
	virtual bool shh_uninstallFilter(int const& _id);

	void setAccounts(std::vector<dev::KeyPair> const& _accounts);
	void setIdentities(std::vector<dev::KeyPair> const& _ids);
	std::map<dev::Public, dev::Secret> const& ids() const { return m_ids; }

private:
	dev::eth::Interface* client() const;
	dev::shh::Interface* face() const;
	dev::WebThreeDirect& m_web3;
	std::map<dev::Address, dev::KeyPair> m_accounts;
	
	ldb::ReadOptions m_readOptions;
	ldb::WriteOptions m_writeOptions;
	ldb::DB* m_db;
	
	std::map<dev::Public, dev::Secret> m_ids;
	std::map<unsigned, dev::Public> m_shhWatches;
};
