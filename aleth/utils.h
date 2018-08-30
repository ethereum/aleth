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

#include <string>
#include <vector>
#include <libp2p/Common.h>

namespace utils
{
struct PeerOption
{
    bool required;
    std::string publicKey;
    std::string host;
    unsigned short port;
};

using PeerNodeListMap = std::map<dev::p2p::NodeID, std::pair<dev::p2p::NodeIPEndpoint, bool>>;

// type:[enode://]publickey@ipAddress[:port]
static std::string const peerPattern =
    "(required|default):(enode://"
    ")?([\\dabcdef]{128})@(\\d{1,3}.\\d{1,3}.\\d{1,3}.\\d{1,3})(:\\d{2,5})?";

PeerOption* parsePeersetOption(std::string const& _peer);
bool createPeerNodeList(std::string const& _peerset, PeerNodeListMap& _nodes);
}
