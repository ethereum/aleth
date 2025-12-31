// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2018-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "CapabilityHost.h"
#include "Host.h"
#include "Session.h"

namespace dev
{
namespace p2p
{
namespace
{
class CapabilityHost : public CapabilityHostFace
{
public:
    explicit CapabilityHost(Host& _host) : m_host{_host} {}

    boost::optional<PeerSessionInfo> peerSessionInfo(NodeID const& _nodeID) const override
    {
        auto session = m_host.peerSession(_nodeID);
        return session ? session->info() : boost::optional<PeerSessionInfo>{};
    }

    void disconnect(NodeID const& _nodeID, DisconnectReason _reason) override
    {
        auto session = m_host.peerSession(_nodeID);
        if (session)
            session->disconnect(_reason);
    }

    void disableCapability(NodeID const& _nodeID, std::string const& _capabilityName,
        std::string const& _problem) override
    {
        auto session = m_host.peerSession(_nodeID);
        if (session)
            session->disableCapability(_capabilityName, _problem);
    }

    void updateRating(NodeID const& _nodeID, int _r) override
    {
        auto session = m_host.peerSession(_nodeID);
        if (session)
            session->addRating(_r);
    }

    RLPStream& prep(NodeID const& _nodeID, std::string const& _capabilityName, RLPStream& _s,
        unsigned _id, unsigned _args = 0) override
    {
        auto session = m_host.peerSession(_nodeID);
        if (!session)
            return _s;

        auto const offset = session->capabilityOffset(_capabilityName);
        if (!offset)
            return _s;

        return _s.appendRaw(bytes(1, _id + *offset)).appendList(_args);
    }

    void sealAndSend(NodeID const& _nodeID, RLPStream& _s) override
    {
        auto session = m_host.peerSession(_nodeID);
        if (session)
            session->sealAndSend(_s);
    }

    void addNote(NodeID const& _nodeID, std::string const& _k, std::string const& _v) override
    {
        auto session = m_host.peerSession(_nodeID);
        if (session)
            session->addNote(_k, _v);
    }

    bool isRude(NodeID const& _nodeID, std::string const& _capability) const override
    {
        auto s = m_host.peerSession(_nodeID);
        if (s)
            return s->repMan().isRude(*s, _capability);
        return false;
    }

    void setRude(NodeID const& _nodeID, std::string const& _capability) override
    {
        auto s = m_host.peerSession(_nodeID);
        if (!s)
            return;

        s->repMan().noteRude(*s, _capability);
    }

    void foreachPeer(
        std::string const& _capabilityName, std::function<bool(NodeID const&)> _f) const override
    {
        m_host.forEachPeer(_capabilityName, _f);
    }

    void postWork(std::function<void()> _f) override { m_host.postWork(move(_f)); }

private:
    Host& m_host;
};
}  // namespace

std::shared_ptr<CapabilityHostFace> createCapabilityHost(Host& _host)
{
    return std::make_shared<CapabilityHost>(_host);
}

}  // namespace p2p
}  // namespace dev
