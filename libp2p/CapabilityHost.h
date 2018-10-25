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
* @brief The Capability Host interface.
* The set of functions available to Capability implemenations.
*/
class CapabilityHostFace
{
public:
    virtual ~CapabilityHostFace() = default;

    /// Get info about connected peer.
    /// @returns empty optional if peer is not connected, optional with PeerSessionInfo otherwise
    virtual boost::optional<PeerSessionInfo> peerSessionInfo(NodeID const& _nodeID) const = 0;

    /// Send devp2p Disconnect message and disconnect from peer afterwards.
    /// Has no effect if the peer is not connected.
    virtual void disconnect(NodeID const& _nodeID, DisconnectReason _reason) = 0;

    /// Disable one capability without disconnecting.
    /// Can be useful when talking to several capabilities in one session.
    /// Has no effect if the peer is not connected.
    virtual void disableCapability(
        NodeID const& _nodeID, std::string const& _capabilityName, std::string const& _problem) = 0;

    /// Prepare the message to be sent to the peer.
    /// Constructs the message header with the message ID and the number of message data fields.
    /// @param _nodeID Connected peer ID.
    /// @param _capabilityName Capability to which we are sending a message.
    /// @param _s RLPStream object that will accumulate the resulting message RLP.
    /// @param _id Message ID. Must be less than Capability::messageCount().
    /// @param _args The length of the message data RLP list.
    /// @returns The stream @a _s with appended message header.
    /// After this call the Capability implementation should append @a _args items to @a _s to finish the message constuction.
    /// Has no effect if the peer is not connected.
    virtual RLPStream& prep(NodeID const& _nodeID, std::string const& _capabilityName,
        RLPStream& _s, unsigned _id, unsigned _args = 0) = 0;

    /// Constructs an authenticated RLPx packet with the prepared message and sends it to the peer.
    /// Has no effect if the peer is not connected.
    virtual void sealAndSend(NodeID const& _nodeID, RLPStream& _s) = 0;

    /// Associate arbritrary key/value metadata with the peer.
    /// This saved data will be returned from peerSessionInfo().
    /// Has no effect if the peer is not connected.
    virtual void addNote(NodeID const& _nodeID, std::string const& _k, std::string const& _v) = 0;

    /// Adjust the rating of the connected peer.
    /// This affects the order of peer iteration in forEachPeer.
    /// Has no effect if the peer is not connected.
    virtual void addRating(NodeID const& _nodeID, int _r) = 0;

    /// Mark the connected peer as having poor behaviour.
    /// This affects only isRude() result.
    /// Has no effect if the peer is not connected.
    virtual void setRude(NodeID const& _nodeID, std::string const& _capability) = 0;

    /// @returns true is the peer was marked as rude by setRude().
    virtual bool isRude(NodeID const& _nodeID, std::string const& _capability) const = 0;

    /// Apply callback function to each of the connected peers with given capability.
    virtual void foreachPeer(std::string const& _capabilityName,
        std::function<bool(NodeID const& _nodeID)> _f) const = 0;
};

class Host;

std::shared_ptr<CapabilityHostFace> createCapabilityHost(Host& _host);

}  // namespace p2p
}  // namespace dev
