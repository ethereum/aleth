// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include <libp2p/EndpointTracker.h>
#include <gtest/gtest.h>
#include <thread>

using namespace dev;
using namespace dev::p2p;

namespace
{
    bi::udp::endpoint createEndpoint(std::string const& _address, uint16_t _port)
    {
        return bi::udp::endpoint{bi::make_address(_address), _port};
    }
}


TEST(endpointTracker, findBestEndpoint)
{
    EndpointTracker tracker;

    auto const endpoint1 = createEndpoint("204.25.170.185", 30303);
    auto const endpoint2 = createEndpoint("53.124.81.255", 30305);

    auto const sourceEndpoint1 = createEndpoint("13.74.189.147", 30303);
    auto const sourceEndpoint2 = createEndpoint("13.74.189.148", 30303);
    auto const sourceEndpoint3 = createEndpoint("13.74.189.149", 30303);

    EXPECT_EQ(tracker.addEndpointStatement(sourceEndpoint1, endpoint1), 1);
    EXPECT_EQ(tracker.addEndpointStatement(sourceEndpoint2, endpoint1), 2);
    EXPECT_EQ(tracker.addEndpointStatement(sourceEndpoint3, endpoint2), 1);

    EXPECT_EQ(tracker.bestEndpoint(), endpoint1);
}

TEST(endpointTracker, sourceSendsDifferentEndpoints)
{
    EndpointTracker tracker;

    auto const endpoint1 = createEndpoint("204.25.170.185", 30303);
    auto const endpoint2 = createEndpoint("53.124.81.255", 30305);
    auto const endpoint3 = createEndpoint("111.175.56.85", 30306);
    auto const endpoint4 = createEndpoint("215.185.124.47", 30303);

    auto const sourceEndpoint1 = createEndpoint("13.74.189.147", 30303);
    auto const sourceEndpoint2 = createEndpoint("13.74.189.148", 30303);
    auto const sourceEndpoint3 = createEndpoint("13.74.189.149", 30303);
    auto const sourceEndpoint4 = createEndpoint("13.74.189.150", 30303);

    EXPECT_EQ(tracker.addEndpointStatement(sourceEndpoint1, endpoint1), 1);
    EXPECT_EQ(tracker.addEndpointStatement(sourceEndpoint1, endpoint2), 1);
    EXPECT_EQ(tracker.addEndpointStatement(sourceEndpoint1, endpoint3), 1);

    EXPECT_EQ(tracker.addEndpointStatement(sourceEndpoint2, endpoint1), 1);

    EXPECT_EQ(tracker.addEndpointStatement(sourceEndpoint3, endpoint4), 1);
    EXPECT_EQ(tracker.addEndpointStatement(sourceEndpoint4, endpoint4), 2);

    EXPECT_EQ(tracker.bestEndpoint(), endpoint4);
}

TEST(endpointTracker, garbageCollection)
{
    EndpointTracker tracker;

    auto const endpoint1 = createEndpoint("204.25.170.185", 30303);
    auto const endpoint2 = createEndpoint("53.124.81.255", 30305);

    auto const sourceEndpoint1 = createEndpoint("13.74.189.147", 30303);
    auto const sourceEndpoint2 = createEndpoint("13.74.189.148", 30303);
    auto const sourceEndpoint3 = createEndpoint("13.74.189.149", 30303);
    auto const sourceEndpoint4 = createEndpoint("13.74.189.150", 30303);
    auto const sourceEndpoint5 = createEndpoint("13.74.189.151", 30303);

    EXPECT_EQ(tracker.addEndpointStatement(sourceEndpoint1, endpoint1), 1);
    EXPECT_EQ(tracker.addEndpointStatement(sourceEndpoint2, endpoint1), 2);
    EXPECT_EQ(tracker.addEndpointStatement(sourceEndpoint3, endpoint1), 3);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    tracker.garbageCollectStatements(std::chrono::seconds(1));

    EXPECT_EQ(tracker.addEndpointStatement(sourceEndpoint4, endpoint2), 1);
    EXPECT_EQ(tracker.addEndpointStatement(sourceEndpoint5, endpoint2), 2);

    EXPECT_EQ(tracker.bestEndpoint(), endpoint2);
}
