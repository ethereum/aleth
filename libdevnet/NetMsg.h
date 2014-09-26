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

#include "NetCommon.h"

namespace dev
{

class RLP;
	
/**
 * @brief Used by Net classes for sending and receiving network messages.
 * @todo Encapsulate and maintain header independent of payload in order to separate memory storage (to prevent copy and facilitate other storage options for large streams).
 */
class NetMsg: public std::enable_shared_from_this<NetMsg>
{
	/// NetConnection calls private constructors.
	friend class NetConnection;
	
public:
	/// Constructor for creating egress (outbound) messages
	NetMsg(NetMsgServiceType _service, NetMsgSequence _seq, NetMsgType _packetType, RLP const& _req);
	~NetMsg() {}
	
	NetMsgServiceType service() const { return m_service; }
	NetMsgSequence sequence() const { return m_sequence; }
	NetMsgType type() const { return m_messageType; }
	bytes rlp() const { return m_rlpBytes; }
	bytes payload() const { return m_messageBytes; }
	
	bool isControlMessage() const { return !m_service; }
	bool isServiceMessage() const { return m_service && !m_messageType; }
	bool isDataMessage() const { return m_service && m_messageType; }
	
private:
	/// Constructors for parsing ingress (inbound) messages
	NetMsg(bytes const& _packetData);
	NetMsg(bytesConstRef _packetData);
	
	/// Used by constructor to parse m_messageBytes
	bytes packetify() const;
	
	NetMsgServiceType m_service;			///< Service channel. Zero is reserved for control frames.
	NetMsgSequence m_sequence;			///< Message sequence. Initial value is random and chosen by client. When message type is present the sequence is for data, else it is for service messages.
	NetMsgType m_messageType;				///< Message type. Zero is omitted and indicates service message.
	bytes m_rlpBytes;					///< RLP Payload.
	bytes m_messageBytes;					///< Bytes of network message.
};
	
}

