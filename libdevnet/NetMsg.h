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
 * @todo Encapsulate and maintain header independent of payload
 * @todo Exception for malformed messages
 */
class NetMsg: public std::enable_shared_from_this<NetMsg>
{
	/// NetConnection calls private constructors.
	friend class NetConnection;
	
public:
	/// Constructor for new message.
	NetMsg(NetMsgServiceType _service, NetMsgSequence _seq, NetMsgType _packetType, RLP const& _req);
	~NetMsg() {}
	
	/// Service ID of message.
	NetMsgServiceType service() const { return m_service; }
	
	/// Sequence number of message.
	NetMsgSequence sequence() const { return m_sequence; }
	
	/// Type of message.
	NetMsgType type() const { return m_messageType; }
	
	/// RLP content of message.
	bytes const& rlp() const { return m_rlpBytes; }
	
	/// NetMsg in raw bytes.
	bytes const& payload() const { return m_messageBytes; }
	
	/// Whether message is control (only seen by network)
	bool isControlMessage() const { return !m_service; }
	
	/// Whether message is service message (only seen by service handler)
	bool isServiceMessage() const { return m_service && !m_messageType; }
	
	/// Whether message is data message (only seen by data handler)
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

