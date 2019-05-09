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
/** @file CommonNet.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "CommonNet.h"
using namespace std;
using namespace dev;
using namespace dev::eth;

namespace dev
{
namespace eth
{
#pragma GCC diagnostic ignored "-Wunused-variable"
namespace
{
char dummy;
}


char const* ethPacketTypeToString(EthSubprotocolPacketType _packetType)
{
    switch (_packetType)
    {
    case StatusPacket:
        return "Status";
    case NewBlockHashesPacket:
        return "NewBlockHashes";
    case TransactionsPacket:
        return "Transactions";
    case GetBlockHeadersPacket:
        return "GetBlockHeaders";
    case BlockHeadersPacket:
        return "BlockHeaders";
    case GetBlockBodiesPacket:
        return "GetBlockBodies";
    case BlockBodiesPacket:
        return "BlockBodies";
    case NewBlockPacket:
        return "NewBlock";
    case GetNodeDataPacket:
        return "GetNodeData";
    case NodeDataPacket:
        return "NodeData";
    case GetReceiptsPacket:
        return "GetReceipts";
    case ReceiptsPacket:
        return "Receipts";
    default:
        return "UnknownEthPacket";
    }
}
}  // namespace eth
}  // namespace dev