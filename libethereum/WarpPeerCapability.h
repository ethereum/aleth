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

#pragma once

#include "CommonNet.h"

#include <libdevcore/Common.h>
#include <libp2p/Capability.h>

namespace dev
{
namespace eth
{

class SnapshotStorageFace;

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

class WarpPeerCapability: public p2p::Capability
{
public:
	WarpPeerCapability(std::shared_ptr<p2p::SessionFace> _s, p2p::HostCapabilityFace* _h, unsigned _i, p2p::CapDesc const& _cap);

	static std::string name() { return "par"; }

	static u256 version() { return c_WarpProtocolVersion; }

	static unsigned messageCount() { return WarpSubprotocolPacketCount; }

	void init(unsigned _hostProtocolVersion, u256 _hostNetworkId, u256 _chainTotalDifficulty, h256 _chainCurrentHash, h256 _chainGenesisHash, std::shared_ptr<SnapshotStorageFace const> _snapshot);

private:
	using p2p::Capability::sealAndSend;

	bool interpret(unsigned _id, RLP const& _r) override;

	void requestStatus(unsigned _hostProtocolVersion, u256 const& _hostNetworkId, u256 const& _chainTotalDifficulty, h256 const& _chainCurrentHash, h256 const& _chainGenesisHash,
		h256 const& _snapshotBlockHash, u256 const& _snapshotBlockNumber);

	std::shared_ptr<SnapshotStorageFace const> m_snapshot;
};

}
}
