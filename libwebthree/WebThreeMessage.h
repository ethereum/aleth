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
/** @file WebThreeMessage.h
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
 * @brief Used by WebThreeClient to send requests; message is immediately sent.
 * @todo implement send
 * @todo (notes) received on wire; sequence is parsed from RLP
 * @todo check RLP
 * @todo rename sequence to serviceSequence, implement dataSequence
 * @todo class WebThreeNotification: public WebThreeMessage; (requires serviceSequence tracking)
 */
class WebThreeMessage
{
	friend class WebThreeServer;
	friend class WebThreeConnection;
	friend class WebThreeRequest;
	friend class WebThreeResponse;
	
	// Ingress constructor
	WebThreeMessage(bytes& _packetData);
	// Egress constructor
	WebThreeMessage(uint16_t _seq, RLP const& _req);
	~WebThreeMessage();
	
	/// @returns if payload size matches length from 4-byte header, size of RLP is valid, and sequenceId is valid
	bool check(bytesConstRef _netMsg);
	/// @returns RLP message.
	bytes payload(bytesConstRef _netMsg);
	
	WebThreeServiceType service;
	uint16_t sequence;			///< Message sequence. Initial value is random and chosen by client.
	int requestType;				///< Message type; omitted from header if 0.
	bytes const& messageBytes;	///< RLP Bytes of the message.
};
	
}

