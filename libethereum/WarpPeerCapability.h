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
/** @file WarpPeerCapability.h
 */

#pragma once

#include "CommonNet.h"

#include <libdevcore/Common.h>
#include <libp2p/Capability.h>

namespace dev
{
namespace eth
{

class WarpPeerCapability;

const unsigned c_WarpProtocolVersion = 1;

enum WarpSubprotocolPacketType: byte
{
	WarpStatusPacket = 0x00,
	GetSnapshotManifest = 0x11,
	SnapshotManifest = 0x12,
	GetSnapshotData = 0x13,
	SnapshotData = 0x14,

	WarpSubprotocolPacketCount
};

class WarpPeerObserverFace
{
public:
	virtual ~WarpPeerObserverFace() {}

	virtual void onPeerStatus(std::shared_ptr<WarpPeerCapability> _peer) = 0;

	virtual void onPeerManifest(std::shared_ptr<WarpPeerCapability> _peer, RLP const& _r) = 0;

	virtual void onPeerData(std::shared_ptr<WarpPeerCapability> _peer, RLP const& _r) = 0;

	virtual void onPeerRequestTimeout(std::shared_ptr<WarpPeerCapability> _peer, Asking _asking) = 0;
};


class WarpPeerCapability: public p2p::Capability
{
public:
	WarpPeerCapability(std::shared_ptr<p2p::SessionFace> _s, p2p::HostCapabilityFace* _h, unsigned _i, p2p::CapDesc const& _cap, uint16_t _capID);

	static std::string name() { return "par"; }

	static u256 version() { return c_WarpProtocolVersion; }

	static unsigned messageCount() { return WarpSubprotocolPacketCount; }

	void init(unsigned _hostProtocolVersion, u256 _hostNetworkId, u256 _chainTotalDifficulty, h256 _chainCurrentHash, h256 _chainGenesisHash, std::shared_ptr<WarpPeerObserverFace> _observer);

	/// Validates whether peer is able to communicate with the host, disables peer if not
	bool validateStatus(h256 const& _genesisHash, std::vector<unsigned> const& _protocolVersions, u256 const& _networkId);

	void requestStatus(u256 _hostNetworkId, u256 _chainTotalDifficulty, h256 _chainCurrentHash, h256 _chainGenesisHash);
	void requestManifest();
	void requestData(h256 const& _chunkHash);

	u256 snapshotNumber() const { return m_snapshotNumber; }


private:
	using p2p::Capability::sealAndSend;

	/// Interpret an incoming message.
	bool interpret(unsigned _id, RLP const& _r) override;

	void setAsking(Asking _a);
		
	void setIdle() { setAsking(Asking::Nothing); }

	unsigned m_hostProtocolVersion = 0;

	/// Peer's protocol version.
	unsigned m_protocolVersion = 0;

	/// Peer's network id.
	u256 m_networkId;

	/// What, if anything, we last asked the other peer for.
	Asking m_asking = Asking::Nothing;
	/// When we asked for it. Allows a time out.
	std::atomic<time_t> m_lastAsk;

	/// These are determined through either a Status message.
	h256 m_latestHash;						///< Peer's latest block's hash.
	u256 m_totalDifficulty;					///< Peer's latest block's total difficulty.
	h256 m_genesisHash;						///< Peer's genesis hash
	h256 m_snapshotHash;
	u256 m_snapshotNumber;

	std::shared_ptr<WarpPeerObserverFace> m_observer;
};

}
}
