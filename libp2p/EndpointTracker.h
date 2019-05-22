// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include "Common.h"

namespace dev
{
namespace p2p
{
/// Class for keeping track of our external endpoint as seen by our peers.
/// Keeps track of what external endpoint is seen by every peer
/// and finds which endpoint is reported most often
class EndpointTracker
{
public:
    /// Register the statement about endpoint from one of the peers.
    /// @returns number of currently kept statements in favor of @a _externalEndpoint
    size_t addEndpointStatement(
        bi::udp::endpoint const& _sourceEndpoint, bi::udp::endpoint const& _externalEndpoint);

    /// Find endpoint with max number of statements
    bi::udp::endpoint bestEndpoint() const;

    /// Remove statements older than _timeToLive
    void garbageCollectStatements(std::chrono::seconds const& _timeToLive);

private:
    using EndpointAndTimePoint =
        std::pair<bi::udp::endpoint, std::chrono::steady_clock::time_point>;
    using SourceToStatementMap = std::map<bi::udp::endpoint, EndpointAndTimePoint>;

    size_t addStatement(
        bi::udp::endpoint const& _sourceEndpoint, bi::udp::endpoint const& _externalEndpoint);

    SourceToStatementMap::iterator removeStatement(SourceToStatementMap::iterator _it);

    /// Statements about our external endpoint, maps statement source peer => endpoint, timestamp
    SourceToStatementMap m_statementsMap;
    /// map external endpoint => how many sources reported it
    std::map<bi::udp::endpoint, size_t> m_endpointStatementCountMap;
};

}  // namespace p2p
}  // namespace dev