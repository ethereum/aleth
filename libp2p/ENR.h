// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include "Common.h"

namespace dev
{
namespace p2p
{
DEV_SIMPLE_EXCEPTION(ENRIsTooBig);
DEV_SIMPLE_EXCEPTION(ENRSignatureIsInvalid);
DEV_SIMPLE_EXCEPTION(ENRKeysAreNotUniqueSorted);
DEV_SIMPLE_EXCEPTION(ENRUnknownIdentityScheme);
DEV_SIMPLE_EXCEPTION(ENRSecp256k1NotFound);
DEV_SIMPLE_EXCEPTION(ENRInvalidAddress);

/// Class representing Ethereum Node Record - see EIP-778
class ENR
{
public:
    // ENR class implementation is independent of Identity Scheme.
    // Identity Scheme specifics are passed to ENR as functions.

    /// Sign function gets serialized ENR contents and signs it according to some Identity Scheme
    using SignFunction = std::function<bytes(bytesConstRef)>;
    /// Verify function gets ENR key-value pairs, signature, and serialized content and validates the
    /// signature according to some Identity Scheme
    using VerifyFunction =
        std::function<bool(std::map<std::string, bytes> const&, bytesConstRef, bytesConstRef)>;

    /// Parse from RLP with given signature verification function
    ENR(RLP const& _rlp, VerifyFunction const& _verifyFunction);
    /// Create with given sign function
    ENR(uint64_t _seq, std::map<std::string, bytes> const& _keyValuePairs,
        SignFunction const& _signFunction);

    uint64_t sequenceNumber() const { return m_seq; }
    bytes const& signature() const { return m_signature; }

    std::map<std::string, bytes> const& keyValuePairs() const { return m_keyValuePairs; }

    /// Pre-defined keys
    std::string id() const;
    boost::asio::ip::address_v4 ip() const;
    boost::asio::ip::address_v6 ip6() const;
    uint16_t tcpPort() const;
    uint16_t udpPort() const;

    /// Serialize to given RLP stream
    void streamRLP(RLPStream& _s) const;

    /// Create new ENR succeeding current one with updated @a _keyValuePairs
    ENR update(
        std::map<std::string, bytes> const& _keyValuePair, SignFunction const& _signFunction) const;

    std::string textEncoding() const;

private:
    uint64_t m_seq = 0;
    std::map<std::string, bytes> m_keyValuePairs;
    bytes m_signature;

    bytes content() const;
    size_t contentRlpListItemCount() const { return m_keyValuePairs.size() * 2 + 1; }
    void streamContent(RLPStream& _s) const;
};

class IdentitySchemeV4
{
public:
    static ENR createENR(Secret const& _secret, boost::asio::ip::address const& _ip,
        uint16_t _tcpPort, uint16_t _udpPort);

    static ENR updateENR(ENR const& _enr, Secret const& _secret,
        boost::asio::ip::address const& _ip, uint16_t _tcpPort, uint16_t _udpPort);

    static ENR parseENR(RLP const& _rlp);

    static PublicCompressed publicKey(ENR const& _enr);

private:
    static bytes sign(bytesConstRef _data, Secret const& _secret);
    static std::map<std::string, bytes> createKeyValuePairs(Secret const& _secret,
        boost::asio::ip::address const& _ip, uint16_t _tcpPort, uint16_t _udpPort);
};

std::ostream& operator<<(std::ostream& _out, ENR const& _enr);

}
}
