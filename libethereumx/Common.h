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
 */

#pragma once

#include <libdevcore/Exceptions.h>

#define clogS(X) LogOutputStream<X, true>(true)

namespace dev
{
namespace eth
{

/// read/write exceptions or errors occurred
class RPCSocketError: public Exception {};

/// invalid message created by server
class RPCInvalidMessage: public Exception {};
	
/// invalid response is received by client or server
/// @todo replace w/InvalidMessage
class RPCInvalidResponse: public Exception {};

/// server went away
class RPCServerWentAway: public Exception {};
	
/// TODO: Use w/asio deadline timer
/// timeout occured when reading socket
// class RPCResponseTimeout: public Exception {};

/// Thrown by client when server responds w/exception. Used internally by server to
/// @todo (?) refactor/remove in favor of bool
class RPCRemoteException: public Exception {};
	
enum RPCRequestPacketType
{
	RequestVersion = 0x00, // fixme: naming convention for setting options
	RequestSubmitTransaction,
	RequestCreateContract,
	RequestRLPInject,
	RequestFlushTransactions,
	RequestCallTransaction,
	RequestBalanceAt,
	RequestCountAt,
	RequestStateAt,
	RequestCodeAt,
	RequestStorageAt,
	RequestMessages,			// fixme: serialize
	RequestPeers,
	RequestPeerCount,
	ConnectToPeer = 0x10
};

enum RPCResponsePacketType
{
	ResponseSuccess = 0x21,
	ResponseException
};

}
}

