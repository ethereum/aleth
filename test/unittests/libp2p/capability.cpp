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
/** @file capability.cpp
* @author Vladislav Gluhovsky <vlad@ethdev.com>
* @date May 2015
*/

#include <libp2p/CapabilityHost.h>
#include <libp2p/Common.h>
#include <libp2p/Host.h>
#include <libp2p/HostCapability.h>
#include <libp2p/Session.h>
#include <test/tools/libtesteth/Options.h>
#include <test/tools/libtesteth/TestOutputHelper.h>
#include <boost/test/unit_test.hpp>
#include <chrono>
#include <thread>

using namespace std;
using namespace dev;
using namespace dev::test;
using namespace dev::p2p;

struct P2PFixture: public TestOutputHelperFixture
{
    P2PFixture() { dev::p2p::NodeIPEndpoint::test_allowLocal = true; }
    ~P2PFixture() { dev::p2p::NodeIPEndpoint::test_allowLocal = false; }
};

class TestHostCapability : public HostCapabilityFace, public Worker
{
public:
    explicit TestHostCapability(Host const& _host) : Worker("test"), m_host(_host) {}

    std::string name() const override { return "test"; }
    u256 version() const override { return 2; }
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
        // cnote << "Capability::interpret(): custom message received";
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

        return std::pair<int, int>(cnt, checksum);
    }

    Host const& m_host;
    std::unordered_map<NodeID, int> m_cntReceivedMessages;
    std::unordered_map<NodeID, int> m_testSums;
};

BOOST_FIXTURE_TEST_SUITE(p2pCapability, P2PFixture)

BOOST_AUTO_TEST_CASE(capability)
{
    cnote << "Testing Capability...";

    int const step = 10;
    const char* const localhost = "127.0.0.1";
    NetworkConfig prefs1(localhost, 0, false);
    NetworkConfig prefs2(localhost, 0, false);
    Host host1("Test", prefs1);
    Host host2("Test", prefs2);
    auto thc1 = make_shared<TestHostCapability>(host1);
    host1.registerCapability(thc1);
    auto thc2 = make_shared<TestHostCapability>(host2);
    host2.registerCapability(thc2);
    host1.start();	
    host2.start();
    auto port1 = host1.listenPort();
    auto port2 = host2.listenPort();
    BOOST_REQUIRE(port1);
    BOOST_REQUIRE(port2);	
    BOOST_REQUIRE_NE(port1, port2);

    for (unsigned i = 0; i < 3000; i += step)
    {
        this_thread::sleep_for(chrono::milliseconds(step));

        if (host1.isStarted() && host2.isStarted())
            break;
    }

    BOOST_REQUIRE(host1.isStarted() && host2.isStarted());
    host1.requirePeer(host2.id(), NodeIPEndpoint(bi::address::from_string(localhost), port2, port2));

    // Wait for up to 12 seconds, to give the hosts time to connect to each other.
    for (unsigned i = 0; i < 12000; i += step)
    {
        this_thread::sleep_for(chrono::milliseconds(step));

        if ((host1.peerCount() > 0) && (host2.peerCount() > 0))
            break;
    }

    BOOST_REQUIRE(host1.peerCount() > 0 && host2.peerCount() > 0);

    int const target = 64;
    int checksum = 0;
    for (int i = 0; i < target; checksum += i++)
        thc2->sendTestMessage(host1.id(), i);

    this_thread::sleep_for(chrono::seconds(target / 64 + 1));
    std::pair<int, int> testData = thc1->retrieveTestData(host2.id());
    BOOST_REQUIRE_EQUAL(target, testData.first);
    BOOST_REQUIRE_EQUAL(checksum, testData.second);
}

BOOST_AUTO_TEST_SUITE_END()


