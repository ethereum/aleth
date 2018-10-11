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
class CapabilityHostFace
{
public:
    virtual ~CapabilityHostFace() = default;

    virtual boost::optional<PeerSessionInfo> peerSessionInfo(NodeID const& _nodeID) const = 0;

    virtual void disableCapability(
        NodeID const& _nodeID, std::string const& _capabilityName, std::string const& _problem) = 0;

    virtual void disconnect(NodeID const& _nodeID, DisconnectReason _reason) = 0;

    virtual void addRating(NodeID const& _nodeID, int _r) = 0;

    virtual RLPStream& prep(NodeID const& _nodeID, std::string const& _capabilityName,
        RLPStream& _s, unsigned _id, unsigned _args = 0) = 0;

    virtual void sealAndSend(NodeID const& _nodeID, RLPStream& _s) = 0;

    virtual void addNote(NodeID const& _nodeID, std::string const& _k, std::string const& _v) = 0;

    virtual bool isRude(NodeID const& _nodeID, std::string const& _capability) const = 0;

    virtual void foreachPeer(std::string const& _name, u256 const& _version,
        std::function<bool(NodeID const& _nodeID)> _f) const = 0;
};

}  // namespace p2p
}  // namespace dev
