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
/** @file WebThreeRequest.h
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include "NetMsg.h"

namespace dev
{

class WebThreeClient;
class NetConnection;

/**
 * @brief Private class used by WebThreeClient to send requests and by WebThree. (messages are sent immediately)
 * @todo move implementation into cpp and remove cast from webthreeresponse
 * @todo (notes) received on wire; sequence is parsed from RLP
 * @todo check RLP
 */
class WebThreeRequest: public NetMsg
{
	friend class WebThreeClient;
	
public:
	WebThreeRequest(WebThreeClient *_c, NetMsgType _packetType, RLP const& _request);
	void respond(NetConnection *_s, RLP const& _response);
};

}

