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
/** @file Common.h
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 *
 */

#pragma once

#include <libdevcore/Log.h>
#include <libdevcore/RLP.h>

namespace dev
{

struct RPCWarn: public LogChannel { static const char* name() { return "!X!"; } static const int verbosity = 0; };
struct RPCNote: public LogChannel { static const char* name() { return "*X*"; } static const int verbosity = 1; };
struct RPCMessageSummary: public LogChannel { static const char* name() { return "-X-"; } static const int verbosity = 2; };
struct RPCConnect: public LogChannel { static const char* name() { return "+X+"; } static const int verbosity = 4; };

typedef uint8_t NetMsgServiceType;
typedef uint8_t NetMsgType;
typedef uint16_t NetMsgSequence;

enum NetServices : NetMsgServiceType
{
	WebThreeService = 0x01,
	EthereumService,
	SwarmService,
	WhisperService,
	P2PService
};
	
class NetMsg;
typedef std::function<void(NetMsg const& _msg)> messageHandler;
typedef std::map<NetMsgServiceType,messageHandler> messageHandlers;
	
class MessageTooLarge: public Exception {};
class MessageTooSmall: public Exception {};
class MessageServiceInvalid: public Exception {};

class RPCException: public Exception {};
class RPCRequestTimeout: public Exception {};
	
}