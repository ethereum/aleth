// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "Host.h"
#include "Session.h"
#include "Peer.h"
#include "RLPxHandshake.h"
using namespace std;
using namespace dev;
using namespace dev::p2p;
using namespace dev::crypto;

constexpr std::chrono::milliseconds RLPXHandshake::c_timeout;

namespace
{
constexpr unsigned c_rlpxVersion = 4;
constexpr size_t c_ackCipherSizeBytes = 210;
constexpr size_t c_authCipherSizeBytes = 307;
}

RLPXHandshake::RLPXHandshake(Host* _host, std::shared_ptr<RLPXSocket> const& _socket)
  : RLPXHandshake(_host, _socket, {})
{}

RLPXHandshake::RLPXHandshake(
    Host* _host, std::shared_ptr<RLPXSocket> const& _socket, NodeID _remote)
  : m_host(_host),
    m_remote(_remote),
    m_originated(_remote),
    m_socket(_socket),
    m_idleTimer(m_socket->ref().get_executor()),
    m_failureReason{HandshakeFailureReason::NoFailure}
{
    auto const prefixAttr =
        boost::log::attributes::constant<std::string>{connectionDirectionString()};
    m_logger.add_attribute("Prefix", prefixAttr);
    m_errorLogger.add_attribute("Prefix", prefixAttr);

    stringstream remoteInfoStream;
    remoteInfoStream << "(" << _remote;
    if (remoteSocketConnected())
        remoteInfoStream << "@" << m_socket->remoteEndpoint();
    remoteInfoStream << ")";
    auto const suffixAttr = boost::log::attributes::constant<std::string>{remoteInfoStream.str()};
    m_logger.add_attribute("Suffix", suffixAttr);
    m_errorLogger.add_attribute("Suffix", suffixAttr);

    crypto::Nonce::get().ref().copyTo(m_nonce.ref());
}


void RLPXHandshake::writeAuth()
{
    LOG(m_logger) << "auth to";
    m_auth.resize(Signature::size + h256::size + Public::size + h256::size + 1);
    bytesRef sig(&m_auth[0], Signature::size);
    bytesRef hepubk(&m_auth[Signature::size], h256::size);
    bytesRef pubk(&m_auth[Signature::size + h256::size], Public::size);
    bytesRef nonce(&m_auth[Signature::size + h256::size + Public::size], h256::size);
    
    // E(remote-pubk, S(ecdhe-random, ecdh-shared-secret^nonce) || H(ecdhe-random-pubk) || pubk || nonce || 0x0)
    Secret staticShared;
    crypto::ecdh::agree(m_host->m_alias.secret(), m_remote, staticShared);
    sign(m_ecdheLocal.secret(), staticShared.makeInsecure() ^ m_nonce).ref().copyTo(sig);
    sha3(m_ecdheLocal.pub().ref(), hepubk);
    m_host->m_alias.pub().ref().copyTo(pubk);
    m_nonce.ref().copyTo(nonce);
    m_auth[m_auth.size() - 1] = 0x0;
    encryptECIES(m_remote, &m_auth, m_authCipher);

    auto self(shared_from_this());
    ba::async_write(m_socket->ref(), ba::buffer(m_authCipher), [this, self](boost::system::error_code ec, std::size_t)
    {
        transition(ec);
    });
}

void RLPXHandshake::writeAck()
{
    LOG(m_logger) << "ack to";
    m_ack.resize(Public::size + h256::size + 1);
    bytesRef epubk(&m_ack[0], Public::size);
    bytesRef nonce(&m_ack[Public::size], h256::size);
    m_ecdheLocal.pub().ref().copyTo(epubk);
    m_nonce.ref().copyTo(nonce);
    m_ack[m_ack.size() - 1] = 0x0;
    encryptECIES(m_remote, &m_ack, m_ackCipher);

    auto self(shared_from_this());
    ba::async_write(m_socket->ref(), ba::buffer(m_ackCipher), [this, self](boost::system::error_code ec, std::size_t)
    {
        transition(ec);
    });
}

void RLPXHandshake::writeAckEIP8()
{
    LOG(m_logger) << "EIP-8 ack to";
    RLPStream rlp;
    rlp.appendList(3)
        << m_ecdheLocal.pub()
        << m_nonce
        << c_rlpxVersion;
    m_ack = rlp.out();
    int padAmount(rand()%100 + 100);
    m_ack.resize(m_ack.size() + padAmount, 0);

    bytes prefix(2);
    toBigEndian<uint16_t>(m_ack.size() + c_eciesOverhead, prefix);
    encryptECIES(m_remote, bytesConstRef(&prefix), &m_ack, m_ackCipher);
    m_ackCipher.insert(m_ackCipher.begin(), prefix.begin(), prefix.end());
    
    auto self(shared_from_this());
    ba::async_write(m_socket->ref(), ba::buffer(m_ackCipher),
        [this, self](boost::system::error_code ec, std::size_t) {
            transition(ec);
        });
}

void RLPXHandshake::setAuthValues(Signature const& _sig, Public const& _remotePubk, h256 const& _remoteNonce, uint64_t _remoteVersion)
{
    _remotePubk.ref().copyTo(m_remote.ref());
    _remoteNonce.ref().copyTo(m_remoteNonce.ref());
    m_remoteVersion = _remoteVersion;
    Secret sharedSecret;
    crypto::ecdh::agree(m_host->m_alias.secret(), _remotePubk, sharedSecret);
    m_ecdheRemote = recover(_sig, sharedSecret.makeInsecure() ^ _remoteNonce);
}

void RLPXHandshake::readAuth()
{
    m_authCipher.resize(c_authCipherSizeBytes);
    auto self(shared_from_this());
    ba::async_read(m_socket->ref(), ba::buffer(m_authCipher, c_authCipherSizeBytes),
        [this, self](boost::system::error_code ec, std::size_t) {
            if (ec)
                transition(ec);
            else if (decryptECIES(m_host->m_alias.secret(), bytesConstRef(&m_authCipher), m_auth))
            {
                LOG(m_logger) << "auth from";
                bytesConstRef data(&m_auth);
                Signature sig(data.cropped(0, Signature::size));
                Public pubk(data.cropped(Signature::size + h256::size, Public::size));
                h256 nonce(data.cropped(Signature::size + h256::size + Public::size, h256::size));
                setAuthValues(sig, pubk, nonce, 4);
                transition();
            }
            else
                readAuthEIP8();
        });
}

void RLPXHandshake::readAuthEIP8()
{
    assert(m_authCipher.size() == c_authCipherSizeBytes);
    uint16_t const size(m_authCipher[0] << 8 | m_authCipher[1]);
    LOG(m_logger) << size << " bytes EIP-8 auth from";
    m_authCipher.resize((size_t)size + 2);
    auto rest = ba::buffer(ba::buffer(m_authCipher) + c_authCipherSizeBytes);
    auto self(shared_from_this());
    ba::async_read(m_socket->ref(), rest, [this, self](boost::system::error_code ec, std::size_t)
    {
        bytesConstRef ct(&m_authCipher);
        if (ec)
            transition(ec);
        else if (decryptECIES(m_host->m_alias.secret(), ct.cropped(0, 2), ct.cropped(2), m_auth))
        {
            RLP rlp(m_auth, RLP::ThrowOnFail | RLP::FailIfTooSmall);
            setAuthValues(
                rlp[0].toHash<Signature>(),
                rlp[1].toHash<Public>(),
                rlp[2].toHash<h256>(),
                rlp[3].toInt<uint64_t>()
            );
            m_nextState = AckAuthEIP8;
            transition();
        }
        else
        {
            LOG(m_logger) << "EIP-8 auth decrypt failed";
            m_nextState = Error;
            m_failureReason = HandshakeFailureReason::FrameDecryptionFailure;
            transition();
        }
    });
}

void RLPXHandshake::readAck()
{
    m_ackCipher.resize(c_ackCipherSizeBytes);
    auto self(shared_from_this());
    ba::async_read(m_socket->ref(), ba::buffer(m_ackCipher, c_ackCipherSizeBytes),
        [this, self](boost::system::error_code ec, std::size_t) {
            if (ec)
                transition(ec);
            else if (decryptECIES(m_host->m_alias.secret(), bytesConstRef(&m_ackCipher), m_ack))
            {
                LOG(m_logger) << "ack from";
                bytesConstRef(&m_ack).cropped(0, Public::size).copyTo(m_ecdheRemote.ref());
                bytesConstRef(&m_ack).cropped(Public::size, h256::size).copyTo(m_remoteNonce.ref());
                m_remoteVersion = c_rlpxVersion;
                transition();
            }
            else
                readAckEIP8();
        });
}

void RLPXHandshake::readAckEIP8()
{
    assert(m_ackCipher.size() == c_ackCipherSizeBytes);
    uint16_t const size(m_ackCipher[0] << 8 | m_ackCipher[1]);
    LOG(m_logger) << size << " bytes EIP-8 ack from";
    m_ackCipher.resize((size_t)size + 2);
    auto rest = ba::buffer(ba::buffer(m_ackCipher) + c_ackCipherSizeBytes);
    auto self(shared_from_this());
    ba::async_read(m_socket->ref(), rest, [this, self](boost::system::error_code ec, std::size_t)
    {
        bytesConstRef ct(&m_ackCipher);
        if (ec)
            transition(ec);
        else if (decryptECIES(m_host->m_alias.secret(), ct.cropped(0, 2), ct.cropped(2), m_ack))
        {
            RLP rlp(m_ack, RLP::ThrowOnFail | RLP::FailIfTooSmall);
            m_ecdheRemote = rlp[0].toHash<Public>();
            m_remoteNonce = rlp[1].toHash<h256>();
            m_remoteVersion = rlp[2].toInt<uint64_t>();
            transition();
        }
        else
        {
            LOG(m_logger) << "EIP-8 ack decrypt failed";
            m_failureReason = HandshakeFailureReason::FrameDecryptionFailure;
            m_nextState = Error;
            transition();
        }
    });
}

void RLPXHandshake::cancel()
{
    m_cancel = true;
    m_idleTimer.cancel();
    m_socket->close();
    m_io.reset();
}

void RLPXHandshake::error(boost::system::error_code _ech)
{
    m_host->onHandshakeFailed(m_remote, m_failureReason);

    stringstream errorStream;
    errorStream << "Handshake failed";
    if (_ech)
        errorStream << " (I/O error: " << _ech.message() << ")";
    if (remoteSocketConnected())
        errorStream << ". Disconnecting from";
    else
        errorStream << " (Connection reset by peer)";

    LOG(m_logger) << errorStream.str();

    cancel();
}

void RLPXHandshake::transition(boost::system::error_code _ech)
{
    // reset timeout
    m_idleTimer.cancel();
    
    if (_ech || m_nextState == Error || m_cancel)
    {
        if (_ech)
            m_failureReason = HandshakeFailureReason::TCPError;
        return error(_ech);
    }
    
    auto self(shared_from_this());
    assert(m_nextState != StartSession);
    m_idleTimer.expires_after(c_timeout);
    m_idleTimer.async_wait([this, self](boost::system::error_code const& _ec)
    {
        if (!_ec)
        {
            LOG(m_logger) << "Disconnecting (Handshake Timeout) from";
            m_failureReason = HandshakeFailureReason::Timeout;
            m_nextState = Error;
            transition();
        }
    });
    
    if (m_nextState == New)
    {
        m_nextState = AckAuth;
        if (m_originated)
            writeAuth();
        else
            readAuth();
    }
    else if (m_nextState == AckAuth)
    {
        m_nextState = WriteHello;
        if (m_originated)
            readAck();
        else
            writeAck();
    }
    else if (m_nextState == AckAuthEIP8)
    {
        m_nextState = WriteHello;
        if (m_originated)
            readAck();
        else
            writeAckEIP8();
    }
    else if (m_nextState == WriteHello)
    {
        // Send the p2p capability Hello frame
        LOG(m_logger) << p2pPacketTypeToString(HelloPacket) << " to";

        m_nextState = ReadHello;

        /// This pointer will be freed if there is an error otherwise
        /// it will be passed to Host which will take ownership.
        m_io.reset(new RLPXFrameCoder(*this));

        RLPStream s;
        s.append((unsigned)HelloPacket).appendList(5)
            << dev::p2p::c_protocolVersion
            << m_host->m_clientVersion
            << m_host->caps()
            << m_host->listenPort()
            << m_host->id();

        bytes packet;
        s.swapOut(packet);
        m_io->writeSingleFramePacket(&packet, m_handshakeOutBuffer);
        ba::async_write(m_socket->ref(), ba::buffer(m_handshakeOutBuffer),
            [this, self](boost::system::error_code ec, std::size_t) {
                transition(ec);
            });
    }
    else if (m_nextState == ReadHello)
    {
        // Authenticate and decrypt initial hello frame with initial RLPXFrameCoder
        // and request m_host to start session.
        m_nextState = StartSession;
        
        // read frame header
        constexpr size_t handshakeSizeBytes = 32;
        m_handshakeInBuffer.resize(handshakeSizeBytes);
        ba::async_read(m_socket->ref(),
            boost::asio::buffer(m_handshakeInBuffer, handshakeSizeBytes),
            [this, self](boost::system::error_code ec, std::size_t) {
                if (ec)
                    transition(ec);
                else
                {
                    if (!m_io)
                    {
                        LOG(m_errorLogger)
                            << "Internal error in handshake: RLPXFrameCoder disappeared ("
                            << m_remote << ")";
                        m_failureReason = HandshakeFailureReason::InternalError;
                        m_nextState = Error;
                        transition();
                        return;
                    }

                    LOG(m_logger) << "Frame header from";

                    /// authenticate and decrypt header
                    if (!m_io->authAndDecryptHeader(
                            bytesRef(m_handshakeInBuffer.data(), m_handshakeInBuffer.size())))
                    {
                        m_failureReason = HandshakeFailureReason::FrameDecryptionFailure;
                        m_nextState = Error;
                        transition();
                        return;
                    }

                    LOG(m_logger) << "Successfully decrypted frame header, validating contents...";

                    /// check frame size
                    bytes const& header = m_handshakeInBuffer;
                    uint32_t const frameSize = (uint32_t)(header[2]) | (uint32_t)(header[1]) << 8 |
                                               (uint32_t)(header[0]) << 16;
                    constexpr size_t expectedFrameSizeBytes = 1024;
                    if (frameSize > expectedFrameSizeBytes)
                    {
                        // all future frames: 16777216
                        LOG(m_logger)
                            << "Frame is too large! Expected size: " << expectedFrameSizeBytes
                            << " bytes, actual size: " << frameSize << " bytes";
                        m_failureReason = HandshakeFailureReason::ProtocolError;
                        m_nextState = Error;
                        transition();
                        return;
                    }

                    /// rlp of header has protocol-type, sequence-id[, total-packet-size]
                    bytes headerRLP(header.size() - 3 - h128::size);  // this is always 32 - 3 - 16
                                                                      // = 13. wtf?
                    bytesConstRef(&header).cropped(3).copyTo(&headerRLP);

                    /// read padded frame and mac
                    constexpr size_t byteBoundary = 16;
                    m_handshakeInBuffer.resize(
                        frameSize + ((byteBoundary - (frameSize % byteBoundary)) % byteBoundary) +
                        h128::size);

                    LOG(m_logger) << "Frame header contents validated";

                    ba::async_read(m_socket->ref(),
                        boost::asio::buffer(m_handshakeInBuffer, m_handshakeInBuffer.size()),
                        [this, self, headerRLP](boost::system::error_code ec, std::size_t) {
                            m_idleTimer.cancel();

                            if (ec)
                                transition(ec);
                            else
                            {
                                if (!m_io)
                                {
                                    LOG(m_errorLogger) << "Internal error in handshake: "
                                                          "RLPXFrameCoder disappeared";
                                    m_failureReason = HandshakeFailureReason::InternalError;
                                    m_nextState = Error;
                                    transition();
                                    return;
                                }
                                LOG(m_logger) << "Frame body from";
                                bytesRef frame(&m_handshakeInBuffer);
                                if (!m_io->authAndDecryptFrame(frame))
                                {
                                    LOG(m_logger) << "Frame body decrypt failed";
                                    m_failureReason =
                                        HandshakeFailureReason::FrameDecryptionFailure;
                                    m_nextState = Error;
                                    transition();
                                    return;
                                }

                                // 0x80 = special case for 0
                                P2pPacketType packetType =
                                    frame[0] == 0x80 ? HelloPacket : static_cast<P2pPacketType>(frame[0]);
                                if (packetType != HelloPacket)
                                {
                                    LOG(m_logger)
                                        << "Invalid packet type. Expected: "
                                        << p2pPacketTypeToString(HelloPacket)
                                        << ", received: " << p2pPacketTypeToString(packetType);
                                    m_failureReason = HandshakeFailureReason::ProtocolError;
                                    if (packetType == DisconnectPacket)
                                    {
                                        try
                                        {
                                            // Explicitly avoid RLP::FailIfTooLarge exception if RLP
                                            // data is smaller than bytes buffer since msg data is
                                            // significantly smaller than buffer size
                                            RLP rlp{frame.cropped(1),
                                                RLP::ThrowOnFail | RLP::FailIfTooSmall};
                                            if (rlp)
                                            {
                                                auto const reason = static_cast<DisconnectReason>(
                                                    rlp[0].toInt<int>());
                                                LOG(m_logger)
                                                    << "Disconnect reason: " << reasonOf(reason);

                                                // We set a failure reason of DisconnectRequested
                                                // since it's not a violation of the protocol (a
                                                // disconnect packet is allowed at any moment) so we
                                                // will try to reconnect later.
                                                m_failureReason =
                                                    HandshakeFailureReason::DisconnectRequested;
                                            }
                                        }
                                        catch (std::exception const& _e)
                                        {
                                            LOG(m_errorLogger)
                                                << "Exception occurred while decoding RLP msg data "
                                                   "in "
                                                << p2pPacketTypeToString(DisconnectPacket) << ": "
                                                << _e.what();
                                        }
                                    }
                                    m_nextState = Error;
                                    transition();
                                    return;
                                }

                                LOG(m_logger) << p2pPacketTypeToString(HelloPacket)
                                              << " verified. Starting session with";
                                try
                                {
                                    RLP rlp(
                                        frame.cropped(1), RLP::ThrowOnFail | RLP::FailIfTooSmall);
                                    m_host->startPeerSession(m_remote, rlp, move(m_io), m_socket);
                                }
                                catch (std::exception const& _e)
                                {
                                    LOG(m_errorLogger)
                                        << "Handshake causing an exception: " << _e.what();
                                    m_failureReason = HandshakeFailureReason::UnknownFailure;
                                    m_nextState = Error;
                                    transition();
                                }
                            }
                        });
                }
            });
    }
}

bool RLPXHandshake::remoteSocketConnected() const
{
    return m_socket && m_socket->isConnected() &&
           !m_socket->remoteEndpoint().address().is_unspecified();
}
