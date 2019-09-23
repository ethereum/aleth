// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

/// @file
/// Miscellanea required for the PeerServer/Session classes.
#pragma once

#include <libdevcore/Common.h>
#include <libdevcore/Log.h>
#include <libp2p/Common.h>
#include <chrono>
#include <string>

namespace dev
{

class OverlayDB;

namespace eth
{

static const unsigned c_maxHeaders = 2048;      ///< Maximum number of hashes BlockHashes will ever send.
static const unsigned c_maxHeadersAsk = 2048;   ///< Maximum number of hashes GetBlockHashes will ever ask for.
static const unsigned c_maxBlocks = 128;        ///< Maximum number of blocks Blocks will ever send.
static const unsigned c_maxBlocksAsk = 128;     ///< Maximum number of blocks we ask to receive in Blocks (when using GetChain).
static const unsigned c_maxPayload = 262144;    ///< Maximum size of packet for us to send.
static const unsigned c_maxNodes = c_maxBlocks; ///< Maximum number of nodes will ever send.
static const unsigned c_maxReceipts = c_maxBlocks; ///< Maximum number of receipts will ever send.

class BlockChain;
class TransactionQueue;
class EthereumCapability;
class EthereumPeer;

enum EthSubprotocolPacketType : byte
{
    StatusPacket = 0x00,
    NewBlockHashesPacket = 0x01,
    TransactionsPacket = 0x02,
    GetBlockHeadersPacket = 0x03,
    BlockHeadersPacket = 0x04,
    GetBlockBodiesPacket = 0x05,
    BlockBodiesPacket = 0x06,
    NewBlockPacket = 0x07,

    GetNodeDataPacket = 0x0d,
    NodeDataPacket = 0x0e,
    GetReceiptsPacket = 0x0f,
    ReceiptsPacket = 0x10,

    PacketCount
};

char const* ethPacketTypeToString(EthSubprotocolPacketType _packetType);

enum class Asking
{
    State,
    BlockHeaders,
    BlockBodies,
    NodeData,
    Receipts,
    WarpManifest,
    WarpData,
    Nothing
};

enum class SyncState
{
    NotSynced,          ///< Initial chain sync has not started yet
    Idle,               ///< Initial chain sync complete. Waiting for new packets
    Waiting,            ///< Block downloading paused. Waiting for block queue to process blocks and free space
    Blocks,             ///< Downloading blocks
    State,              ///< Downloading state

    Size        /// Must be kept last
};

struct SyncStatus
{
    SyncState state = SyncState::Idle;
    unsigned protocolVersion = 0;
    unsigned startBlockNumber;
    unsigned currentBlockNumber;
    unsigned highestBlockNumber;
    bool majorSyncing = false;
};

using NodeID = p2p::NodeID;
}
}
