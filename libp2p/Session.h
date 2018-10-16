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
/** @file Session.h
 * @author Gav Wood <i@gavwood.com>
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 */

#pragma once

#include "Common.h"
#include "HostCapability.h"
#include "RLPXFrameCoder.h"
#include "RLPXSocket.h"
#include <libdevcore/Common.h>
#include <libdevcore/Guards.h>
#include <libdevcore/RLP.h>
#include <array>
#include <deque>
#include <memory>
#include <mutex>
#include <set>
#include <utility>

namespace dev
{

namespace p2p
{

class Peer;
class ReputationManager;

class SessionFace
{
public:
    virtual ~SessionFace() {}

    virtual void start() = 0;
    virtual void disconnect(DisconnectReason _reason) = 0;

    virtual void ping() = 0;

    virtual bool isConnected() const = 0;

    virtual NodeID id() const = 0;

    virtual void sealAndSend(RLPStream& _s) = 0;

    virtual int rating() const = 0;
    virtual void addRating(int _r) = 0;

    virtual void addNote(std::string const& _k, std::string const& _v) = 0;

    virtual PeerSessionInfo info() const = 0;
    virtual std::chrono::steady_clock::time_point connectionTime() = 0;

    virtual void registerCapability(
        CapDesc const& _desc, unsigned _offset, std::shared_ptr<HostCapabilityFace> _p) = 0;

    virtual std::vector<CapDesc> capabilities() const = 0;

    virtual std::shared_ptr<Peer> peer() const = 0;

    virtual std::chrono::steady_clock::time_point lastReceived() const = 0;

    virtual ReputationManager& repMan() = 0;

    virtual void disableCapability(
        std::string const& _capabilityName, std::string const& _problem) = 0;

    virtual boost::optional<unsigned> capabilityOffset(
        std::string const& _capabilityName) const = 0;
};

/**
 * @brief The Session class
 * @todo Document fully.
 */
class Session: public SessionFace, public std::enable_shared_from_this<SessionFace>
{
public:
    Session(Host* _server, std::unique_ptr<RLPXFrameCoder>&& _io, std::shared_ptr<RLPXSocket> const& _s, std::shared_ptr<Peer> const& _n, PeerSessionInfo _info);
    virtual ~Session();

    void start() override;
    void disconnect(DisconnectReason _reason) override;

    void ping() override;

    bool isConnected() const override { return m_socket->ref().is_open(); }

    NodeID id() const override;

    void sealAndSend(RLPStream& _s) override;

    int rating() const override;
    void addRating(int _r) override;

    void addNote(std::string const& _k, std::string const& _v) override { Guard l(x_info); m_info.notes[_k] = _v; }

    PeerSessionInfo info() const override { Guard l(x_info); return m_info; }
    std::chrono::steady_clock::time_point connectionTime() override { return m_connect; }

    void registerCapability(CapDesc const& _desc, unsigned _offset, std::shared_ptr<HostCapabilityFace> _p) override;

    std::vector<CapDesc> capabilities() const override { return keysOf(m_capabilities); }

    std::shared_ptr<Peer> peer() const override { return m_peer; }

    std::chrono::steady_clock::time_point lastReceived() const override { return m_lastReceived; }

    ReputationManager& repMan() override;

    void disableCapability(
        std::string const& _capabilityName, std::string const& _problem) override;

    boost::optional<unsigned> capabilityOffset(std::string const& _capabilityName) const override;

private:
    static RLPStream& prep(RLPStream& _s, PacketType _t, unsigned _args = 0);

    void send(bytes&& _msg);

    /// Drop the connection for the reason @a _r.
    void drop(DisconnectReason _r);

    /// Perform a read on the socket.
    void doRead();
    
    /// Check error code after reading and drop peer if error code.
    bool checkRead(std::size_t _expected, boost::system::error_code _ec, std::size_t _length);

    /// Perform a single round of the write operation. This could end up calling itself asynchronously.
    void write();

    /// Deliver RLPX packet to Session or PeerCapability for interpretation.
    bool readPacket(uint16_t _capId, PacketType _t, RLP const& _r);

    /// Interpret an incoming Session packet.
    bool interpret(PacketType _t, RLP const& _r);
    
    /// @returns true iff the _msg forms a valid message for sending or receiving on the network.
    static bool checkPacket(bytesConstRef _msg);

    bool capabilityEnabled(std::string const& _capability) const
    {
        return m_disabledCapabilities.find(_capability) == m_disabledCapabilities.end();
    }

    bool canHandle(
        std::string const& _capability, unsigned _messageCount, unsigned _packetType) const;

    Host* m_server;							///< The host that owns us. Never null.

    std::unique_ptr<RLPXFrameCoder> m_io;	///< Transport over which packets are sent.
    std::shared_ptr<RLPXSocket> m_socket;		///< Socket of peer's connection.
    Mutex x_framing;						///< Mutex for the write queue.
    std::deque<bytes> m_writeQueue;			///< The write queue.
    std::vector<byte> m_data;			    ///< Buffer for ingress packet data.
    bytes m_incoming;						///< Read buffer for ingress bytes.

    std::shared_ptr<Peer> m_peer;			///< The Peer object.
    bool m_dropped = false;					///< If true, we've already divested ourselves of this peer. We're just waiting for the reads & writes to fail before the shared_ptr goes OOS and the destructor kicks in.

    mutable Mutex x_info;
    PeerSessionInfo m_info;						///< Dynamic information about this peer.

    std::chrono::steady_clock::time_point m_connect;		///< Time point of connection.
    std::chrono::steady_clock::time_point m_ping;			///< Time point of last ping.
    std::chrono::steady_clock::time_point m_lastReceived;	///< Time point of last message.

    /// The peer's capability set.
    std::map<CapDesc, std::shared_ptr<HostCapabilityFace>> m_capabilities;

    /// Map of capability to packet id offset in the session
    std::unordered_map<std::string, unsigned> m_capabilityOffsets;

    std::set<std::string> m_disabledCapabilities;

    std::string const m_logContext;
};

}
}
