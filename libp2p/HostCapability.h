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

#include "Common.h"

namespace dev
{

namespace p2p
{
/**
 * @brief The Capability interface.
 * This interface has to be implemented to support any devp2p subprotocol.
 */

class HostCapabilityFace
{
public:
    virtual ~HostCapabilityFace() = default;

    /// Subprotocol name, used in negotiating common capabilities with the peers.
    virtual std::string name() const = 0;
    /// Subprotocol version, used in negotiating common capabilities with the peers.
    virtual u256 version() const = 0;
    /// Number of messages supported by the subprotocol version.
    virtual unsigned messageCount() const = 0;

    /// Called by the Host when capability is activated
    /// (usually when network communication is being enabled)
    virtual void onStarting() = 0;
    /// Called by the Host when capability is deactivated
    /// (usually when network communication is being disabled)
    virtual void onStopping() = 0;

    /// Called by the Host when new peer is connected.
    /// Guaranteed to be called first before any interpretCapabilityPacket for this peer.
    virtual void onConnect(NodeID const& _nodeID, u256 const& _peerCapabilityVersion) = 0;
    /// Called by the Host when the messaege is received from the peer
    /// @returns true if the message was interpreted, false if the message had not supported type.
    virtual bool interpretCapabilityPacket(NodeID const& _nodeID, unsigned _id, RLP const&) = 0;
    /// Called by the Host when the peer is disconnected.
    /// Guaranteed to be called last after any interpretCapabilityPacket for this peer.
    virtual void onDisconnect(NodeID const& _nodeID) = 0;
};

}

}
