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

#include <iostream>
#include <regex>
#include <boost/algorithm/string.hpp>

#include <libp2p/Network.h>
#include "utils.h"

using namespace std;
using namespace dev;
using namespace dev::p2p;

namespace utils
{
PeerOption* parsePeersetOption(string const& _peer)
{
    regex rx(peerPattern);
    smatch match;

    try
    {
        if (regex_match(_peer, match, rx))
        {
            bool required = match.str(1) == "required";
            unsigned short port = c_defaultListenPort;

            if (!match.str(5).empty())
                port = (uint16_t)atoi(match.str(5).substr(1).c_str());

            auto p = new PeerOption();

            p->required = required;
            p->publicKey = match.str(3);
            p->host = match.str(4);
            p->port = port;

            return p;
        }
        else
        {
            return nullptr;
        }
    }
    catch (regex_error &error)
    {
        cerr << error.what() << "\n";
        return nullptr;
    }
}

bool createPeerNodeList(string const& _peerset, PeerNodeListMap& _nodes)
{
    vector<string> peers;
    boost::split(peers, _peerset, boost::is_any_of("\t "));

    for (auto const& p: peers)
    {
        auto po = parsePeersetOption(p);

        if (po == nullptr)
            return false;

        Public key(fromHex(po->publicKey));

        try
        {
            _nodes[key] =
                make_pair(NodeIPEndpoint(bi::address::from_string(po->host), po->port, po->port),
                    po->required);
            delete po;
        }
        catch (...)
        {
            delete po;
            return false;
        }
    }

    return true;
}

}
