// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "EndpointTracker.h"

namespace dev
{
namespace p2p
{
/// Register the statement about endpoint from one othe peers.
/// @returns number of currently kept statements in favor of @a _externalEndpoint
size_t EndpointTracker::addEndpointStatement(
    bi::udp::endpoint const& _sourceEndpoint, bi::udp::endpoint const& _externalEndpoint)
{
    // remove previous statement by this peer
    auto it = m_statementsMap.find(_sourceEndpoint);
    if (it != m_statementsMap.end())
        removeStatement(it);

    return addStatement(_sourceEndpoint, _externalEndpoint);
}

/// Find endpoint with max number of statemens
bi::udp::endpoint EndpointTracker::bestEndpoint() const
{
    size_t maxCount = 0;
    bi::udp::endpoint bestEndpoint;
    for (auto const& endpointAndCount : m_endpointStatementCountMap)
        if (endpointAndCount.second > maxCount)
            std::tie(bestEndpoint, maxCount) = endpointAndCount;

    return bestEndpoint;
}

/// Remove old statements
void EndpointTracker::garbageCollectStatements(std::chrono::seconds const& _timeToLive)
{
    auto const expiration = std::chrono::steady_clock::now() - _timeToLive;
    for (auto it = m_statementsMap.begin(); it != m_statementsMap.end();)
    {
        if (it->second.second < expiration)
            it = removeStatement(it);
        else
            ++it;
    }
}

size_t EndpointTracker::addStatement(
    bi::udp::endpoint const& _sourceEndpoint, bi::udp::endpoint const& _externalEndpoint)
{
    EndpointAndTimePoint endpointAndTime{_externalEndpoint, std::chrono::steady_clock::now()};
    m_statementsMap.insert({_sourceEndpoint, endpointAndTime});
    return ++m_endpointStatementCountMap[_externalEndpoint];
}

EndpointTracker::SourceToStatementMap::iterator EndpointTracker::removeStatement(
    SourceToStatementMap::iterator _it)
{
    // first decrement statement counter
    auto itCount = m_endpointStatementCountMap.find(_it->second.first);
    assert(itCount != m_endpointStatementCountMap.end() && itCount->second > 0);
    if (--itCount->second == 0)
        m_endpointStatementCountMap.erase(itCount);

    return m_statementsMap.erase(_it);
}

}  // namespace p2p
}  // namespace dev