// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include "Common.h"

namespace dev
{
namespace p2p
{

class ENR
{
public:
    // return bytes, Signature is EC-specific
    using SignFunction = std::function<bytes(bytesConstRef)>;

    // parse from RLP
    explicit ENR(RLP _rlp);
    // create with given sign function
    ENR(uint64_t _seq, std::map<std::string, bytes> const& _keyValues, SignFunction _signFunction);

    void streamRLP(RLPStream& _s) const;

private:
    uint64_t m_seq = 0;
    std::map<std::string, bytes> m_map;
    bytes m_signature;

    std::vector<bytes> content() const;
    bytes contentRlpList() const;
};

ENR createV4ENR(Secret const& _secret, boost::asio::ip::address const& _ip, uint16_t _tcpPort,  uint16_t _udpPort);

std::ostream& operator<<(std::ostream& _out, ENR const& _enr);

}
}
