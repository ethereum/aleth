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
#include "HostCapability.h"

namespace dev
{
namespace p2p
{
class PeerCapabilityFace
{
public:
    virtual ~PeerCapabilityFace() = default;

    virtual bool enabled() const = 0;

    virtual bool canHandle(unsigned _packetType) const = 0;
    virtual bool interpret(unsigned _packetType, RLP const& _rlp) = 0;

    virtual void onDisconnect() = 0;
};


class PeerCapability : public PeerCapabilityFace,
                       public std::enable_shared_from_this<PeerCapabilityFace>
{
public:
    PeerCapability(std::weak_ptr<SessionFace> _s, std::string const& _name,
        unsigned _messageCount, unsigned _idOffset);

    // Implement these in the derived class.
    // static std::string name() { return ""; }
    // static u256 version() { return 0; }
    // static unsigned messageCount() { return 0; }

    bool enabled() const override { return m_enabled; }
    bool canHandle(unsigned _packetType) const override
    {
        return _packetType >= m_idOffset && _packetType < m_messageCount + m_idOffset;
    }

    bool interpret(unsigned _packetType, RLP const& _rlp) override
    {
        return interpretCapabilityPacket(_packetType - m_idOffset, _rlp);
    }

    void onDisconnect() override {}

protected:
    virtual bool interpretCapabilityPacket(unsigned _id, RLP const&) = 0;

    std::shared_ptr<SessionFace> session() const { return m_session.lock(); }

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
