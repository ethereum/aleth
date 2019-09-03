/*
    This file is part of aleth.

    aleth is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    aleth is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with aleth.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file Peer.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "Peer.h"
using namespace std;
using namespace dev;
using namespace dev::p2p;

namespace dev
{

namespace p2p
{
namespace
{
unsigned defaultFallbackSeconds(unsigned _failedAttempts)
{
    if (_failedAttempts < 5)
        return _failedAttempts ? _failedAttempts * 5 : 5;
    else if (_failedAttempts < 15)
        return 25 + (_failedAttempts - 5) * 10;
    else
        return 25 + 100 + (_failedAttempts - 15) * 20;
}
}  // namespace

Peer::Peer(Peer const& _original)
  : Node(_original),
    m_lastConnected(_original.m_lastConnected),
    m_lastAttempted(_original.m_lastAttempted),
    m_lastDisconnect(_original.m_lastDisconnect),
    m_lastHandshakeFailure(_original.m_lastHandshakeFailure),
    m_session(_original.m_session)
{
    m_score = _original.m_score.load();
    m_rating = _original.m_rating.load();
    m_failedAttempts = _original.m_failedAttempts.load();
}

bool Peer::shouldReconnect() const
{
    return id && endpoint && !isUseless() &&
           chrono::system_clock::now() > m_lastAttempted + chrono::seconds(fallbackSeconds());
}

bool Peer::isUseless() const
{
    if (peerType == PeerType::Required)
        return false;

    switch (m_lastHandshakeFailure)
    {
    case HandshakeFailureReason::FrameDecryptionFailure:
    case HandshakeFailureReason::ProtocolError:
        return true;
    default:
        break;
    }

    switch (m_lastDisconnect)
    {
    // Critical cases
    case BadProtocol:
    case UselessPeer:
    case IncompatibleProtocol:
    case UnexpectedIdentity:
    case DuplicatePeer:
    case NullIdentity:
        return true;
    // Potentially transient cases which can resolve quickly
    case PingTimeout:
    case TCPError:
    case TooManyPeers:
        return m_failedAttempts >= 10;
    // Potentially transient cases which can take longer to resolve
    case ClientQuit:
    case UserReason:
        return m_failedAttempts >= 25;
    default:
        break;
    }
    return false;
}

unsigned Peer::fallbackSeconds() const
{
    constexpr unsigned oneYearInSeconds{60 * 60 * 24 * 360};

    if (peerType == PeerType::Required)
        return 5;

    if (isUseless())
        return oneYearInSeconds;

    switch (m_lastDisconnect)
    {
    case TCPError:
    case PingTimeout:
    case TooManyPeers:
        return 15 * (m_failedAttempts + 1);
    case ClientQuit:
    case UserReason:
        return 25 * (m_failedAttempts + 1);
    default:
        return defaultFallbackSeconds(m_failedAttempts);
    }
}

}
}
