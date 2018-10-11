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

class HostCapabilityFace
{
public:
    virtual ~HostCapabilityFace() = default;

    virtual std::string name() const = 0;
    virtual u256 version() const = 0;
    virtual unsigned messageCount() const = 0;

    virtual void onStarting() = 0;
    virtual void onStopping() = 0;

    virtual void onConnect(NodeID const& _nodeID, u256 const& _peerCapabilityVersion) = 0;
    virtual bool interpretCapabilityPacket(NodeID const& _nodeID, unsigned _id, RLP const&) = 0;
    virtual void onDisconnect(NodeID const& _nodeID) = 0;
};

}

}
