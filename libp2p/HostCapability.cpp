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
/** @file HostCapability.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "HostCapability.h"

#include "Host.h"
#include "Session.h"
using namespace std;
using namespace dev;
using namespace dev::p2p;

std::vector<std::pair<std::shared_ptr<SessionFace>, std::shared_ptr<Peer>>>
HostCapabilityFace::peerSessions() const
{
    return peerSessions(version());
}

std::vector<std::pair<std::shared_ptr<SessionFace>, std::shared_ptr<Peer>>>
HostCapabilityFace::peerSessions(u256 const& _version) const
{
    RecursiveGuard l(m_host->x_sessions);
    std::vector<std::pair<std::shared_ptr<SessionFace>, std::shared_ptr<Peer>>> ret;
    for (auto const& i : m_host->m_sessions)
        if (std::shared_ptr<SessionFace> s = i.second.lock())
            if (s->capabilities().count(std::make_pair(name(), _version)))
                ret.push_back(make_pair(s, s->peer()));
    return ret;
}
