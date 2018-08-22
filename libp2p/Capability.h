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
/** @file Capability.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include "Common.h"
#include "HostCapability.h"

namespace dev
{
namespace p2p
{

class ReputationManager;

class Capability: public std::enable_shared_from_this<Capability>
{
public:
    Capability(std::weak_ptr<SessionFace> _s, std::string const& _name, unsigned _messageCount,
        unsigned _idOffset);
    virtual ~Capability() = default;

    // Implement these in the derived class.
    // static std::string name() { return ""; }
    // static u256 version() { return 0; }
    // static unsigned messageCount() { return 0; }

    bool enabled() const { return m_enabled; }
    bool canHandle(unsigned _packetType) const
    {
        return _packetType >= m_idOffset && _packetType < m_messageCount + m_idOffset;
    }

    bool interpret(unsigned _packetType, RLP const& _rlp)
    {
        return interpretCapabilityPacket(_packetType - m_idOffset, _rlp);
    }

    virtual void onDisconnect() {}

protected:
    std::shared_ptr<SessionFace> session() const { return m_session.lock(); }
    virtual bool interpretCapabilityPacket(unsigned _id, RLP const&) = 0;

    void disable(std::string const& _problem);

    RLPStream& prep(RLPStream& _s, unsigned _id, unsigned _args = 0);
    void sealAndSend(RLPStream& _s);
    void addRating(int _r);

private:
    std::weak_ptr<SessionFace> m_session;
    bool m_enabled = true;
    std::string const m_name;
    unsigned const m_messageCount;
    unsigned const m_idOffset;
};

}
}
