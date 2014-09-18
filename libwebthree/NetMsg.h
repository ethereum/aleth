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
/** @file NetMsg.h
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include "Common.h"

namespace dev
{

class RLP;
	
/**
 * @brief Used by WebThreeClient to send requests; message is immediately sent. NOT thread safe.
 * @todo implement send
 * @todo (notes) received on wire; sequence is parsed from RLP
 * @todo check RLP
 * @todo rename sequence to serviceSequence, implement dataSequence
 * @todo class WebThreeNotification: public NetMsg; (requires serviceSequence tracking)
 */
class NetMsg: public std::enable_shared_from_this<NetMsg>
{
	friend class NetConnection;
	
public:
	// Egress constructor
	NetMsg(NetMsgServiceType _service, NetMsgSequence _seq, NetMsgType _packetType, RLP const& _req);
	~NetMsg() {}
	
	NetMsgServiceType service() { return m_service; }
	NetMsgSequence sequence() { return m_sequence; }
	NetMsgType type() { return m_messageType; }
	bytes const& rlp() { return m_rlpBytes; }
	bytes const& payload() const { return m_messageBytes; }
	
private:
	// Ingress constructor
	NetMsg(bytes const& _packetData);
	NetMsg(bytesConstRef _packetData);
	
//	/// @returns if payload size matches length from 4-byte header, size of RLP is valid, and sequenceId is valid
//	bool check(bytesConstRef _netMsg) const {};
//	/// @returns RLP message
//	bytes payload(bytesConstRef _netMsg) const {};
	bytes packetify() const;
	
	NetMsgServiceType m_service;
	NetMsgSequence m_sequence;			///< Message sequence. Initial value is random and chosen by client.
	NetMsgType m_messageType;				///< Message type; omitted from header if 0.
	bytes m_rlpBytes;				///< RLP Payload.
	bytes m_messageBytes;			///< Bytes of network message.
};
	
}

