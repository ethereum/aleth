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
/** @file StructuredLogger.h
 * @author Lefteris Karapetsas <lefteris@ethdev.com>
 * @date 2015
 *
 * A simple helper class for the structured logging
 */

#include "StructuredLogger.h"
#include <boost/asio/ip/tcp.hpp>
#include <json/json.h>

#include <libdevcore/CommonIO.h>
#include "Guards.h"

namespace ba = boost::asio;
using namespace std;

namespace dev
{

void StructuredLogger::outputJson(Json::Value const& _value, std::string const& _name) const
{
	Json::Value event;
	static Mutex s_lock;
	Json::FastWriter fastWriter;
	Guard l(s_lock);
	event[_name] = _value;
	cout << fastWriter.write(event) << endl;
}

void StructuredLogger::starting(string const& _clientImpl, const char* _ethVersion)
{
	if (get().m_enabled)
	{
		Json::Value event;
		event["client_impl"] = _clientImpl;
		event["eth_version"] = std::string(_ethVersion);
		event["ts"] = dev::toString(chrono::system_clock::now(), get().m_timeFormat.c_str());

		get().outputJson(event, "starting");
	}
}

void StructuredLogger::stopping(string const& _clientImpl, const char* _ethVersion)
{
	if (get().m_enabled)
	{
		Json::Value event;
		event["client_impl"] = _clientImpl;
		event["eth_version"] = std::string(_ethVersion);
		event["ts"] = dev::toString(chrono::system_clock::now(), get().m_timeFormat.c_str());

		get().outputJson(event, "stopping");
	}
}

void StructuredLogger::p2pConnected(
	string const& _id,
	bi::tcp::endpoint const& _addr,
	chrono::system_clock::time_point const& _ts,
	string const& _remoteVersion,
	unsigned int _numConnections)
{
	if (get().m_enabled)
	{
		std::stringstream addrStream;
		addrStream << _addr;
		Json::Value event;
		event["remote_version_string"] = _remoteVersion;
		event["remote_addr"] = addrStream.str();
		event["remote_id"] = _id;
		event["num_connections"] = Json::Value(_numConnections);
		event["ts"] = dev::toString(_ts, get().m_timeFormat.c_str());

		get().outputJson(event, "p2p.connected");
	}
}

void StructuredLogger::p2pDisconnected(string const& _id, bi::tcp::endpoint const& _addr, unsigned int _numConnections)
{
	if (get().m_enabled)
	{
		std::stringstream addrStream;
		addrStream << _addr;
		Json::Value event;
		event["remote_addr"] = addrStream.str();
		event["remote_id"] = _id;
		event["num_connections"] = Json::Value(_numConnections);
		event["ts"] = dev::toString(chrono::system_clock::now(), get().m_timeFormat.c_str());

		get().outputJson(event, "p2p.disconnected");
	}
}

void StructuredLogger::minedNewBlock(
	string const& _hash,
	string const& _blockNumber,
	string const& _chainHeadHash,
	string const& _prevHash)
{
	if (get().m_enabled)
	{
		Json::Value event;
		event["block_hash"] = _hash;
		event["block_number"] = _blockNumber;
		event["chain_head_hash"] = _chainHeadHash;
		event["ts"] = dev::toString(chrono::system_clock::now(), get().m_timeFormat.c_str());
		event["block_prev_hash"] = _prevHash;

		get().outputJson(event, "eth.miner.new_block");
	}
}

void StructuredLogger::chainReceivedNewBlock(
	string const& _hash,
	string const& _blockNumber,
	string const& _chainHeadHash,
	string const& _remoteID,
	string const& _prevHash)
{
	if (get().m_enabled)
	{
		Json::Value event;
		event["block_hash"] = _hash;
		event["block_number"] = _blockNumber;
		event["chain_head_hash"] = _chainHeadHash;
		event["remote_id"] = _remoteID;
		event["ts"] = dev::toString(chrono::system_clock::now(), get().m_timeFormat.c_str());
		event["block_prev_hash"] = _prevHash;

		get().outputJson(event, "eth.chain.received.new_block");
	}
}

void StructuredLogger::chainNewHead(
	string const& _hash,
	string const& _blockNumber,
	string const& _chainHeadHash,
	string const& _prevHash)
{
	if (get().m_enabled)
	{
		Json::Value event;
		event["block_hash"] = _hash;
		event["block_number"] = _blockNumber;
		event["chain_head_hash"] = _chainHeadHash;
		event["ts"] = dev::toString(chrono::system_clock::now(), get().m_timeFormat.c_str());
		event["block_prev_hash"] = _prevHash;

		get().outputJson(event, "eth.miner.new_block");
	}
}

void StructuredLogger::transactionReceived(string const& _hash, string const& _remoteId)
{
	if (get().m_enabled)
	{
		Json::Value event;
		event["tx_hash"] = _hash;
		event["remote_id"] = _remoteId;
		event["ts"] = dev::toString(chrono::system_clock::now(), get().m_timeFormat.c_str());

		get().outputJson(event, "eth.tx.received");
	}
}


}
