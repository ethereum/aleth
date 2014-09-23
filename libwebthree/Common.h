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

#include "NetCommon.h"

namespace dev
{

enum WebThreeServiceType : NetMsgServiceType
{
	WebThreeService = 0x01,
	EthereumService,
	SwarmService,
	WhisperService
};

class WebThreeRequestTimeout: public Exception {};

enum EthereumRPCRequest : NetMsgType
{
	EthereumRPC = 0x00,
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
	RequestMessages,
	RequestPeers,
	RequestPeerCount,
	ConnectToPeer = 0x10
};

	
}