// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include <libp2p/Capability.h>
#include <libp2p/CapabilityHost.h>
#include <libp2p/Common.h>
#include <libp2p/Host.h>
#include <libp2p/Session.h>
#include <gtest/gtest.h>
#include <chrono>
#include <thread>

using namespace std;
using namespace dev;
using namespace dev::p2p;

class TestCapability : public CapabilityFace, public Worker
{
public:
    explicit TestCapability(Host const& _host) : Worker("test"), m_host(_host) {}

    std::string name() const override { return "test"; }
    unsigned version() const override { return 2; }
    unsigned messageCount() const override { return UserPacket + 1; }

    void onStarting() override {}
    void onStopping() override {}

    void onConnect(NodeID const& _nodeID, u256 const&) override
    {
        m_cntReceivedMessages[_nodeID] = 0;
        m_testSums[_nodeID] = 0;
    }
    bool interpretCapabilityPacket(NodeID const& _nodeID, unsigned _id, RLP const& _r) override
    {
        ++m_cntReceivedMessages[_nodeID];
        m_testSums[_nodeID] += _r[0].toInt();
        BOOST_ASSERT(_id == UserPacket);
        return (_id == UserPacket);
    }
    void onDisconnect(NodeID const& _nodeID) override
    {
        m_cntReceivedMessages.erase(_nodeID);
        m_testSums.erase(_nodeID);
    }

    void sendTestMessage(NodeID const& _id, int _x)
    {
        RLPStream s;
        m_host.capabilityHost()->sealAndSend(
            _id, m_host.capabilityHost()->prep(_id, name(), s, UserPacket, 1) << _x);
    }

    std::pair<int, int> retrieveTestData(NodeID const& _id)
    {
        int cnt = 0;
        int checksum = 0;
        for (auto i : m_cntReceivedMessages)
            if (_id == i.first)
            {
                cnt += i.second;
                checksum += m_testSums[_id];
            }

        return {cnt, checksum};
    }

    Host const& m_host;
    std::unordered_map<NodeID, int> m_cntReceivedMessages;
    std::unordered_map<NodeID, int> m_testSums;
};

TEST(p2p, capability)
{
    int const step = 10;
    const char* const localhost = "127.0.0.1";
    NetworkConfig prefs1(localhost, 0, false /* upnp */, true /* allow local discovery */);
    NetworkConfig prefs2(localhost, 0, false /* upnp */, true /* allow local discovery */);
    Host host1("Test", prefs1);
    Host host2("Test", prefs2);
    auto thc1 = make_shared<TestCapability>(host1);
    host1.registerCapability(thc1);
    auto thc2 = make_shared<TestCapability>(host2);
    host2.registerCapability(thc2);
    host1.start();
    host2.start();
    auto port1 = host1.listenPort();
    auto port2 = host2.listenPort();
    EXPECT_NE(port1, 0);
    EXPECT_NE(port2, 0);
    EXPECT_NE(port1, port2);

    for (unsigned i = 0; i < 3000; i += step)
    {
        this_thread::sleep_for(chrono::milliseconds(step));

        if (host1.isStarted() && host2.isStarted())
            break;
    }

    EXPECT_TRUE(host1.isStarted());
    EXPECT_TRUE(host2.isStarted());
    host1.requirePeer(
        host2.id(), NodeIPEndpoint(bi::address::from_string(localhost), port2, port2));

    // Wait for up to 12 seconds, to give the hosts time to connect to each other.
    for (unsigned i = 0; i < 12000; i += step)
    {
        this_thread::sleep_for(chrono::milliseconds(step));

        if ((host1.peerCount() > 0) && (host2.peerCount() > 0))
            break;
    }

    EXPECT_GT(host1.peerCount(), 0);
    EXPECT_GT(host2.peerCount(), 0);

    int const target = 64;
    int checksum = 0;
    for (int i = 0; i < target; checksum += i++)
        thc2->sendTestMessage(host1.id(), i);

    this_thread::sleep_for(chrono::seconds(target / 64 + 1));
    std::pair<int, int> testData = thc1->retrieveTestData(host2.id());
    EXPECT_EQ(target, testData.first);
    EXPECT_EQ(checksum, testData.second);
}
