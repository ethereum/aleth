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
    return id && endpoint &&
           fallbackSeconds() != numeric_limits<unsigned>::max() &&
           chrono::system_clock::now() > m_lastAttempted + chrono::seconds(fallbackSeconds());
}

bool Peer::uselessPeer() const
{
    switch (m_lastHandshakeFailure)
    {
    case FrameDecryptionFailure:
    case ProtocolError:
        return true;
    default:
        break;
    }

    switch (m_lastDisconnect)
    {
    case BadProtocol:
    case UselessPeer:
    case IncompatibleProtocol:
    case UnexpectedIdentity:
    case UserReason:
        return true;
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

    switch (m_lastHandshakeFailure)
    {
        case FrameDecryptionFailure:
        case ProtocolError:
            return oneYearInSeconds;
        default:
            break;
    }

    switch (m_lastDisconnect)
    {
    case BadProtocol:
    case UselessPeer:
    case IncompatibleProtocol:
    case UnexpectedIdentity:
    case UserReason:
        return oneYearInSeconds;
    case TooManyPeers:
        return 25 * (m_failedAttempts + 1);
    case ClientQuit:
        return 15 * (m_failedAttempts + 1);
    case NoDisconnect:
    default:
        if (m_failedAttempts < 5)
            return m_failedAttempts ? m_failedAttempts * 5 : 5;
        else if (m_failedAttempts < 15)
            return 25 + (m_failedAttempts - 5) * 10;
        else
            return 25 + 100 + (m_failedAttempts - 15) * 20;
    }
}

}
}
