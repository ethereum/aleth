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
/** @file Session.cpp
 * @author Gav Wood <i@gavwood.com>
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 */

#include "Session.h"

#include "Host.h"
#include <libdevcore/Common.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Exceptions.h>
#include <chrono>

using namespace std;
using namespace dev;
using namespace dev::p2p;

Session::Session(Host* _h, unique_ptr<RLPXFrameCoder>&& _io, std::shared_ptr<RLPXSocket> const& _s,
    std::shared_ptr<Peer> const& _n, PeerSessionInfo _info)
  : m_server(_h),
    m_io(move(_io)),
    m_socket(_s),
    m_peer(_n),
    m_info(_info),
    m_ping(chrono::steady_clock::time_point::max()),
    m_logContext(_info.id.abridged() + "|" + _info.clientVersion)
{
    m_peer->m_lastDisconnect = NoDisconnect;
    m_lastReceived = m_connect = chrono::steady_clock::now();
    DEV_GUARDED(x_info)
        m_info.socketId = m_socket->ref().native_handle();
}

Session::~Session()
{
    LOG_SCOPED_CONTEXT(m_logContext);

    cnetlog << "Closing peer session :-(";
    m_peer->m_lastConnected = m_peer->m_lastAttempted - chrono::seconds(1);

    // Read-chain finished for one reason or another.
    for (auto& i : m_capabilities)
    {
        i.second->onDisconnect(id());
        i.second.reset();
    }

    try
    {
        bi::tcp::socket& socket = m_socket->ref();
        if (socket.is_open())
        {
            boost::system::error_code ec;
            socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            socket.close();
        }
    }
    catch (...){}
}

ReputationManager& Session::repMan()
{
    return m_server->repMan();
}

NodeID Session::id() const
{
    return m_peer ? m_peer->id : NodeID();
}

void Session::addRating(int _r)
{
    if (m_peer)
    {
        m_peer->m_rating += _r;
        m_peer->m_score += _r;
        if (_r >= 0)
            m_peer->noteSessionGood();
    }
}

int Session::rating() const
{
    return m_peer->m_rating;
}

template <class T> vector<T> randomSelection(vector<T> const& _t, unsigned _n)
{
    if (_t.size() <= _n)
        return _t;
    vector<T> ret = _t;
    while (ret.size() > _n)
    {
        auto i = ret.begin();
        advance(i, rand() % ret.size());
        ret.erase(i);
    }
    return ret;
}

bool Session::readPacket(uint16_t _capId, PacketType _packetType, RLP const& _r)
{
    m_lastReceived = chrono::steady_clock::now();
    clog(VerbosityTrace, "net") << "-> " << _packetType << " " << _r;
    try // Generic try-catch block designed to capture RLP format errors - TODO: give decent diagnostics, make a bit more specific over what is caught.
    {
        // v4 frame headers are useless, offset packet type used
        // v5 protocol type is in header, packet type not offset
        if (_capId == 0 && _packetType < UserPacket)
            return interpret(_packetType, _r);

        for (auto const& cap : m_capabilities)
        {
            auto const& name = cap.first.first;
            auto const& capability = cap.second;

            if (canHandle(name, capability->messageCount(), _packetType))
                return capabilityEnabled(name) ?
                           capability->interpretCapabilityPacket(
                               id(), _packetType - capabilityOffset(name), _r) :
                           true;
        }

        return false;
    }
    catch (std::exception const& _e)
    {
        cnetlog << "Exception caught in p2p::Session::interpret(): " << _e.what()
                << ". PacketType: " << _packetType << ". RLP: " << _r;
        disconnect(BadProtocol);
        return true;
    }
    return true;
}

bool Session::interpret(PacketType _t, RLP const& _r)
{
    switch (_t)
    {
    case DisconnectPacket:
    {
        string reason = "Unspecified";
        auto r = (DisconnectReason)_r[0].toInt<int>();
        if (!_r[0].isInt())
            drop(BadProtocol);
        else
        {
            reason = reasonOf(r);
            cnetlog << "Disconnect (reason: " << reason << ")";
            drop(DisconnectRequested);
        }
        break;
    }
    case PingPacket:
    {
        cnetdetails << "Ping " << m_info.id;
        RLPStream s;
        sealAndSend(prep(s, PongPacket));
        break;
    }
    case PongPacket:
        DEV_GUARDED(x_info)
        {
            m_info.lastPing = std::chrono::steady_clock::now() - m_ping;
            cnetdetails << "Latency: "
                        << chrono::duration_cast<chrono::milliseconds>(m_info.lastPing).count()
                        << " ms";
        }
        break;
    case GetPeersPacket:
    case PeersPacket:
        break;
    default:
        return false;
    }
    return true;
}

void Session::ping()
{
    RLPStream s;
    sealAndSend(prep(s, PingPacket));
    m_ping = std::chrono::steady_clock::now();
}

RLPStream& Session::prep(RLPStream& _s, PacketType _id, unsigned _args)
{
    return _s.append((unsigned)_id).appendList(_args);
}

void Session::sealAndSend(RLPStream& _s)
{
    bytes b;
    _s.swapOut(b);
    send(move(b));
}

bool Session::checkPacket(bytesConstRef _msg)
{
    if (_msg[0] > 0x7f || _msg.size() < 2)
        return false;
    if (RLP(_msg.cropped(1)).actualSize() + 1 != _msg.size())
        return false;
    return true;
}

void Session::send(bytes&& _msg)
{
    bytesConstRef msg(&_msg);
    clog(VerbosityTrace, "net") << "<- " << RLP(msg.cropped(1));
    if (!checkPacket(msg))
        cnetlog << "INVALID PACKET CONSTRUCTED!";

    if (!m_socket->ref().is_open())
        return;

    bool doWrite = false;
    DEV_GUARDED(x_framing)
    {
        m_writeQueue.push_back(std::move(_msg));
        doWrite = (m_writeQueue.size() == 1);
    }

    if (doWrite)
        write();
}

void Session::write()
{
    bytes const* out = nullptr;
    DEV_GUARDED(x_framing)
    {
        m_io->writeSingleFramePacket(&m_writeQueue[0], m_writeQueue[0]);
        out = &m_writeQueue[0];
    }
    auto self(shared_from_this());
    ba::async_write(m_socket->ref(), ba::buffer(*out),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            LOG_SCOPED_CONTEXT(m_logContext);

            // must check queue, as write callback can occur following dropped()
            if (ec)
            {
                cnetlog << "Error sending: " << ec.message();
                drop(TCPError);
                return;
            }

            DEV_GUARDED(x_framing)
            {
                m_writeQueue.pop_front();
                if (m_writeQueue.empty())
                    return;
            }
            write();
        });
}

namespace
{
    void halveAtomicInt(atomic<int>& i)
    {
        // atomic<int> doesn't have /= operator, so we do it manually
        int oldInt = 0;
        int newInt = 0;
        do
        {
            oldInt = i;
            newInt = oldInt / 2;
            // Current value could already change when we get to exchange,
            // we'll need to retry in the loop in this case
        } while (!i.atomic::compare_exchange_weak(oldInt, newInt));
    }
}

void Session::drop(DisconnectReason _reason)
{
    if (m_dropped)
        return;
    bi::tcp::socket& socket = m_socket->ref();
    if (socket.is_open())
        try
        {
            boost::system::error_code ec;
            cnetdetails << "Closing " << socket.remote_endpoint(ec) << " (" << reasonOf(_reason)
                        << ")";
            socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            socket.close();
        }
        catch (...) {}

    m_peer->m_lastDisconnect = _reason;
    if (_reason == BadProtocol)
    {
        halveAtomicInt(m_peer->m_rating);
        halveAtomicInt(m_peer->m_score);
    }
    m_dropped = true;
}

void Session::disconnect(DisconnectReason _reason)
{
    cnetdetails << "Disconnecting (our reason: " << reasonOf(_reason) << ")";

    if (m_socket->ref().is_open())
    {
        RLPStream s;
        prep(s, DisconnectPacket, 1) << (int)_reason;
        sealAndSend(s);
    }
    drop(_reason);
}

void Session::start()
{
    ping();
    doRead();
}

void Session::doRead()
{
    // ignore packets received while waiting to disconnect.
    if (m_dropped)
        return;

    auto self(shared_from_this());
    m_data.resize(h256::size);
    ba::async_read(m_socket->ref(), boost::asio::buffer(m_data, h256::size),
        [this, self](boost::system::error_code ec, std::size_t length) {
            LOG_SCOPED_CONTEXT(m_logContext);

            if (!checkRead(h256::size, ec, length))
                return;
            else if (!m_io->authAndDecryptHeader(bytesRef(m_data.data(), length)))
            {
                cnetlog << "header decrypt failed";
                drop(BadProtocol);  // todo: better error
                return;
            }

            uint16_t hProtocolId;
            uint32_t hLength;
            uint8_t hPadding;
            try
            {
                RLPXFrameInfo header(bytesConstRef(m_data.data(), length));
                hProtocolId = header.protocolId;
                hLength = header.length;
                hPadding = header.padding;
            }
            catch (std::exception const& _e)
            {
                cnetlog << "Exception decoding frame header RLP: " << _e.what() << " "
                        << bytesConstRef(m_data.data(), h128::size).cropped(3);
                drop(BadProtocol);
                return;
            }

            /// read padded frame and mac
            auto tlen = hLength + hPadding + h128::size;
            m_data.resize(tlen);
            ba::async_read(m_socket->ref(), boost::asio::buffer(m_data, tlen),
                [this, self, hLength, hProtocolId, tlen](
                    boost::system::error_code ec, std::size_t length) {
                    LOG_SCOPED_CONTEXT(m_logContext);

                    if (!checkRead(tlen, ec, length))
                        return;
                    else if (!m_io->authAndDecryptFrame(bytesRef(m_data.data(), tlen)))
                    {
                        cnetlog << "frame decrypt failed";
                        drop(BadProtocol);  // todo: better error
                        return;
                    }

                    bytesConstRef frame(m_data.data(), hLength);
                    if (!checkPacket(frame))
                    {
                        cerr << "Received " << frame.size() << ": " << toHex(frame) << endl;
                        cnetlog << "INVALID MESSAGE RECEIVED";
                        disconnect(BadProtocol);
                        return;
                    }
                    else
                    {
                        auto packetType = (PacketType)RLP(frame.cropped(0, 1)).toInt<unsigned>();
                        RLP r(frame.cropped(1));
                        bool ok = readPacket(hProtocolId, packetType, r);
                        if (!ok)
                            cnetlog << "Couldn't interpret packet. " << RLP(r);
                    }
                    doRead();
                });
        });
}

bool Session::checkRead(std::size_t _expected, boost::system::error_code _ec, std::size_t _length)
{
    if (_ec && _ec.category() != boost::asio::error::get_misc_category() && _ec.value() != boost::asio::error::eof)
    {
        cnetdetails << "Error reading: " << _ec.message();
        drop(TCPError);
        return false;
    }
    else if (_ec && _length < _expected)
    {
        cnetlog << "Error reading - Abrupt peer disconnect: " << _ec.message();
        repMan().noteRude(*this);
        drop(TCPError);
        return false;
    }
    else if (_length != _expected)
    {
        // with static m_data-sized buffer this shouldn't happen unless there's a regression
        // sec recommends checking anyways (instead of assert)
        cnetlog << "Error reading - TCP read buffer length differs from expected frame size.";
        disconnect(UserReason);
        return false;
    }

    return true;
}

void Session::registerCapability(CapDesc const& _desc, unsigned _offset, std::shared_ptr<HostCapabilityFace> _p)
{
    DEV_GUARDED(x_framing)
    {
        m_capabilities[_desc] = move(_p);
        m_capabilityOffsets[_desc.first] = _offset;
    }
}

bool Session::canHandle(
    std::string const& _capability, unsigned _messageCount, unsigned _packetType) const
{
    auto const offset = capabilityOffset(_capability);

    return _packetType >= offset && _packetType < _messageCount + offset;
}
