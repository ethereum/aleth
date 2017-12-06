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
/** @file JsonHelper.h
 * @authors:
 *   Gav Wood <i@gavwood.com>
 * @date 2015
 */
#pragma once

#include <json/json.h>
#include <libp2p/Common.h>
#include <libethcore/Common.h>
#include <libethcore/BlockHeader.h>
#include <libethereum/LogFilter.h>

namespace dev
{

Json::Value toJson(std::map<h256, std::pair<u256, u256>> const& _storage);
Json::Value toJson(std::unordered_map<u256, u256> const& _storage);
Json::Value toJson(Address const& _address);

namespace p2p
{

Json::Value toJson(PeerSessionInfo const& _p);

}

namespace eth
{

class Transaction;
class LocalisedTransaction;
class SealEngineFace;
struct BlockDetails;
class Interface;
using Transactions = std::vector<Transaction>;
using UncleHashes = h256s;
using TransactionHashes = h256s;

Json::Value toJson(BlockHeader const& _bi, SealEngineFace* _face = nullptr);
//TODO: wrap these params into one structure eg. "LocalisedTransaction"
Json::Value toJson(Transaction const& _t, std::pair<h256, unsigned> _location, BlockNumber _blockNumber);
Json::Value toJson(BlockHeader const& _bi, BlockDetails const& _bd, UncleHashes const& _us, Transactions const& _ts, SealEngineFace* _face = nullptr);
Json::Value toJson(BlockHeader const& _bi, BlockDetails const& _bd, UncleHashes const& _us, TransactionHashes const& _ts, SealEngineFace* _face = nullptr);
Json::Value toJson(TransactionSkeleton const& _t);
Json::Value toJson(Transaction const& _t);
Json::Value toJson(LocalisedTransaction const& _t);
Json::Value toJson(TransactionReceipt const& _t);
Json::Value toJson(LocalisedTransactionReceipt const& _t);
Json::Value toJson(LocalisedLogEntry const& _e);
Json::Value toJson(LogEntry const& _e);
Json::Value toJson(std::unordered_map<h256, LocalisedLogEntries> const& _entriesByBlock);
Json::Value toJsonByBlock(LocalisedLogEntries const& _entries);
TransactionSkeleton toTransactionSkeleton(Json::Value const& _json);
LogFilter toLogFilter(Json::Value const& _json);
LogFilter toLogFilter(Json::Value const& _json, Interface const& _client);	// commented to avoid warning. Uncomment once in use @ PoC-7.

class AddressResolver
{
public:
	static Address fromJS(std::string const& _address);
};

}

namespace rpc
{
h256 h256fromHex(std::string const& _s);
}

template <class T>
Json::Value toJson(std::vector<T> const& _es)
{
	Json::Value res(Json::arrayValue);
	for (auto const& e: _es)
		res.append(toJson(e));
	return res;
}

template <class T>
Json::Value toJson(std::unordered_set<T> const& _es)
{
	Json::Value res(Json::arrayValue);
	for (auto const& e: _es)
		res.append(toJson(e));
	return res;
}

template <class T>
Json::Value toJson(std::set<T> const& _es)
{
	Json::Value res(Json::arrayValue);
	for (auto const& e: _es)
		res.append(toJson(e));
	return res;
}

}
