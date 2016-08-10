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
/** @file EthereumPeer.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <mutex>
#include <array>
#include <memory>
#include <utility>

#include <libdevcore/RLP.h>
#include <libdevcore/Guards.h>
#include <libethcore/Common.h>
#include <libp2p/Capability.h>
#include "CommonNet.h"

namespace dev
{
namespace eth
{

/**
 * @brief The EthereumPeer class
 * @todo Document fully.
 * @todo make state transitions thread-safe.
 */
class EthereumPeer: public p2p::Capability
{
	friend class EthereumHost; //TODO: remove this
	friend class BlockChainSync; //TODO: remove this

public:
	/// Basic constructor.
	EthereumPeer(std::shared_ptr<p2p::Session> _s, p2p::HostCapabilityFace* _h, unsigned _i, p2p::CapDesc const& _cap, uint16_t _capID);

	/// Basic destructor.
	virtual ~EthereumPeer();

	/// What is our name?
	static std::string name() { return "eth"; }

	/// What is our version?
	static u256 version() { return c_protocolVersion; }

	/// How many message types do we have?
	static unsigned messageCount() { return PacketCount; }

	/// What is the ethereum subprotocol host object.
	EthereumHost* host() const;

	/// Abort sync and reset fetch
	void setIdle();

	/// Request hashes for given parent hash.
	void requestBlockHeaders(h256 const& _startHash, unsigned _count, unsigned _skip, bool _reverse);
	void requestBlockHeaders(unsigned _startNumber, unsigned _count, unsigned _skip, bool _reverse);

	/// Request specified blocks from peer.
	void requestBlockBodies(h256s const& _blocks);

	/// Check if this node is rude.
	bool isRude() const;

	/// Set that it's a rude node.
	void setRude();

	/// Abort the sync operation.
	void abortSync();

private:
	using p2p::Capability::sealAndSend;

	/// Figure out the amount of blocks we should be asking for.
	unsigned askOverride() const;

	/// Interpret an incoming message.
	virtual bool interpret(unsigned _id, RLP const& _r);

	/// Request status. Called from constructor
	void requestStatus();


	/// Clear all known transactions.
	void clearKnownTransactions() { std::lock_guard<std::mutex> l(x_knownTransactions); m_knownTransactions.clear(); }

	/// Update our asking state.
	void setAsking(Asking _g);

	/// Do we presently need syncing with this peer?
	bool needsSyncing() const { return !isRude() && !!m_latestHash; }

	/// Are we presently in the process of communicating with this peer?
	bool isConversing() const;

	/// Are we presently in a critical part of the syncing process with this peer?
	bool isCriticalSyncing() const;

	/// Runs period checks to check up on the peer.
	void tick();

	/// Peer's protocol version.
	unsigned m_protocolVersion;

	/// Peer's network id.
	u256 m_networkId;

	/// What, if anything, we last asked the other peer for.
	Asking m_asking = Asking::Nothing;
	/// When we asked for it. Allows a time out.
	std::atomic<time_t> m_lastAsk;

	/// These are determined through either a Status message or from NewBlock.
	h256 m_latestHash;						///< Peer's latest block's hash that we know about or default null value if no need to sync.
	u256 m_totalDifficulty;					///< Peer's latest block's total difficulty.
	h256 m_genesisHash;						///< Peer's genesis hash

	/// This is built as we ask for hashes. Once no more hashes are given, we present this to the
	/// host who initialises the DownloadMan and m_sub becomes active for us to begin asking for blocks.
	u256 m_syncHashNumber = 0;				///< Number of latest hash we sync to (PV61+)
	u256 m_height = 0;						///< Chain height
	h256 m_syncHash;						///< Latest hash we sync to (PV60)

	u256 m_peerCapabilityVersion;			///< Protocol version this peer supports received as capability
	/// Have we received a GetTransactions packet that we haven't yet answered?
	bool m_requireTransactions = false;

	Mutex x_knownBlocks;
	h256Hash m_knownBlocks;					///< Blocks that the peer already knows about (that don't need to be sent to them).
	Mutex x_knownTransactions;
	h256Hash m_knownTransactions;			///< Transactions that the peer already knows of.
	unsigned m_unknownNewBlocks = 0;		///< Number of unknown NewBlocks received from this peer
	unsigned m_lastAskedHeaders = 0;		///< Number of hashes asked
};

}
}
