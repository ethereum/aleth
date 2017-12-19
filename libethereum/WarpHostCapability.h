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

#include "WarpPeerCapability.h"

#include <libdevcore/Worker.h>

namespace dev
{

namespace eth
{

class WarpHostCapability: public p2p::HostCapability<WarpPeerCapability>
{
public:
	WarpHostCapability(BlockChain const& _blockChain, u256 const& _networkId, boost::filesystem::path const& _dataDirPath);

protected:
	std::shared_ptr<p2p::Capability> newPeerCapability(std::shared_ptr<p2p::SessionFace> const& _s, unsigned _idOffset, p2p::CapDesc const& _cap) override;

private:
	BlockChain const& m_blockChain;
	u256 const m_networkId;

	std::shared_ptr<SnapshotStorageFace> m_snapshot;
};

}
}
