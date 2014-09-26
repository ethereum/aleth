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
/** @file NetMsg.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "NetMsg.h"

using namespace dev;

NetMsg::NetMsg(NetMsgServiceType _service, NetMsgSequence _seq, NetMsgType _packetType, RLP const& _req): m_service(_service), m_sequence(_seq), m_messageType(_packetType), m_rlpBytes(std::move(_req.data().toBytes())), m_messageBytes(std::move(packetify()))
{
}

NetMsg::NetMsg(bytes const& _packetData): NetMsg(bytesConstRef(&_packetData))
{
}

NetMsg::NetMsg(bytesConstRef _packetData): m_messageBytes(std::move(_packetData.toBytes()))
{
	RLP r(bytesConstRef(&m_messageBytes).cropped(4));
	m_service = (unsigned)r[0].toInt();
	m_sequence = (unsigned)r[1].toInt();
	m_messageType = 0;
	if (r.itemCountStrict() == 3)
		m_rlpBytes = bytes(r[2].data().toBytes());
	else
	{
		m_messageType = r[2].toInt();
		m_rlpBytes = bytes(r[3].data().toBytes());
	}
	// todo: else throw exception
}

bytes NetMsg::packetify() const
{
	int listSize = (!m_service || !m_messageType) ? 3 : 4;
	
	RLPStream s;
	s.appendRaw(bytes(4, 0)); // pad for packet-length prefix
	s.appendList(listSize);
	s << m_service;
	s << m_sequence;
	if (listSize == 4)
		s << m_messageType;
	s.appendRaw(m_rlpBytes);
	
	bytes bytesout;
	s.swapOut(bytesout);
	uint32_t outlen = (uint32_t)bytesout.size() - 4;
	bytesout[0] = (outlen >> 24) & 0xff;
	bytesout[1] = (outlen >> 16) & 0xff;
	bytesout[2] = (outlen >> 8) & 0xff;
	bytesout[3] = outlen & 0xff;
	return bytesout;
}

