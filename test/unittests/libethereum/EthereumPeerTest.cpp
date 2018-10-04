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

#include <libethereum/EthereumHost.h>
#include <libp2p/Host.h>
#include <test/tools/libtesteth/TestHelper.h>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::p2p;
using namespace dev::test;

class MockHostCapability: public p2p::HostCapabilityFace
{
public:
    string name() const override { return "mock capability name"; }
    u256 version() const override { return 0; }
    unsigned messageCount() const override { return 0; }

    void onStarting() override {}
    void onStopping() override {}

    void onConnect(NodeID const&, u256 const&) override {}
    bool interpretCapabilityPacket(NodeID const&, unsigned, RLP const&) override
    {
        return true;
    }
    void onDisconnect(NodeID const&) override {}
};

class MockSession: public SessionFace
{
public:
    void start() override { }
    void disconnect(DisconnectReason /*_reason*/) override { }

    void ping() override { }

    bool isConnected() const override { return true; }

    NodeID id() const override { return {}; }

    void sealAndSend(RLPStream& _s) override
    {
        _s.swapOut(m_bytesSent);
    }

    int rating() const override { return 0; }
    void addRating(int /*_r*/) override { }

    void addNote(string const& _k, string const& _v) override
    {
        m_notes[_k] = _v;
    }

    PeerSessionInfo info() const override { return PeerSessionInfo{ NodeID{}, "", "", 0, std::chrono::steady_clock::duration{}, {}, 0, {}, 0 }; }
    std::chrono::steady_clock::time_point connectionTime() override { return std::chrono::steady_clock::time_point{}; }

    std::shared_ptr<Peer> peer() const override { return nullptr;  }

    std::chrono::steady_clock::time_point lastReceived() const override { return std::chrono::steady_clock::time_point{}; }

    ReputationManager& repMan() override { return m_repMan; }

    void disableCapability(CapDesc const&, std::string const&) override {}

    std::map<CapDesc, std::shared_ptr<HostCapabilityFace>> const& capabilities() const override
    {
        return m_caps;
    }

    ReputationManager m_repMan;
    bytes m_bytesSent;
    map<string, string> m_notes;
    std::map<CapDesc, std::shared_ptr<HostCapabilityFace>> m_caps;
};


class MockEthereumPeerObserver: public EthereumPeerObserverFace
{
public:
    void onPeerStatus(p2p::NodeID const&, EthereumPeerStatus const&) override {}

    void onPeerTransactions(p2p::NodeID const&, RLP const&) override {}

    void onPeerBlockHeaders(p2p::NodeID const&, RLP const&) override {}

    void onPeerBlockBodies(p2p::NodeID const&, RLP const&) override {}

    void onPeerNewHashes(p2p::NodeID const&, std::vector<std::pair<h256, u256>> const&) override {}

    void onPeerNewBlock(p2p::NodeID const&, RLP const&) override {}

    void onPeerNodeData(p2p::NodeID const&, RLP const&) override {}

    void onPeerReceipts(p2p::NodeID const&, RLP const&) override {}

    void onPeerAborting() override {}
};

class EthereumPeerTestFixture: public TestOutputHelperFixture
{
public:
    EthereumPeerTestFixture()
      : session(std::make_shared<MockSession>()),
        observer(std::make_shared<MockEthereumPeerObserver>()),
        offset(UserPacket),
        peer(session, hostCap.name(), hostCap.messageCount(), offset, {"eth", 0})
    {
        peer.init(63, 2, 0, h256(0), h256(0), std::shared_ptr<EthereumHostDataFace>(), observer);
    }

    MockHostCapability hostCap;
    std::shared_ptr<MockSession> session;
    std::shared_ptr<MockEthereumPeerObserver> observer;
    uint8_t offset;
    EthereumPeer peer;
};

BOOST_FIXTURE_TEST_SUITE(EthereumPeerSuite, EthereumPeerTestFixture)

BOOST_AUTO_TEST_CASE(EthereumPeerSuite_requestNodeData)
{
    h256 dataHash("0x949d991d685738352398dff73219ab19c62c06e6f8ce899fbae755d5127ed1ef");
    peer.requestNodeData({ dataHash });

    uint8_t code = static_cast<uint8_t>(session->m_bytesSent[0]);
    BOOST_REQUIRE_EQUAL(code, offset + 0x0d);

    bytes payloadSent(session->m_bytesSent.begin() + 1, session->m_bytesSent.end());
    RLP rlp(payloadSent);
    BOOST_REQUIRE(rlp.isList());
    BOOST_REQUIRE_EQUAL(rlp.itemCount(), 1);
    BOOST_REQUIRE_EQUAL(static_cast<h256>(rlp[0]), dataHash);
}

BOOST_AUTO_TEST_CASE(EthereumPeerSuite_requestNodeDataSeveralHashes)
{
    h256 dataHash0("0x949d991d685738352398dff73219ab19c62c06e6f8ce899fbae755d5127ed1ef");
    h256 dataHash1("0x0e4562a10381dec21b205ed72637e6b1b523bdd0e4d4d50af5cd23dd4500a217");
    h256 dataHash2("0x53ab44f45948543775a4c405085b918e5e648db1201283bb54a59701afdaedf3");
    peer.requestNodeData({ dataHash0, dataHash1, dataHash2 });

    bytes payloadSent(session->m_bytesSent.begin() + 1, session->m_bytesSent.end());
    RLP rlp(payloadSent);
    BOOST_REQUIRE_EQUAL(rlp.itemCount(), 3);
    BOOST_REQUIRE_EQUAL(static_cast<h256>(rlp[0]), dataHash0);
    BOOST_REQUIRE_EQUAL(static_cast<h256>(rlp[1]), dataHash1);
    BOOST_REQUIRE_EQUAL(static_cast<h256>(rlp[2]), dataHash2);
}

BOOST_AUTO_TEST_CASE(EthereumPeerSuite_requestNodeDataAddsAskingNote)
{
    h256 dataHash("0x949d991d685738352398dff73219ab19c62c06e6f8ce899fbae755d5127ed1ef");
    peer.requestNodeData({ h256("0x949d991d685738352398dff73219ab19c62c06e6f8ce899fbae755d5127ed1ef") });

    BOOST_REQUIRE(session->m_notes.find("ask") != session->m_notes.end());
    BOOST_REQUIRE(session->m_notes["ask"] == "NodeData");
}

BOOST_AUTO_TEST_CASE(EthereumPeerSuite_requestNodeDataWithNoHashesSetsAskNoteToNothing)
{
    peer.requestNodeData({});

    BOOST_REQUIRE(session->m_notes.find("ask") != session->m_notes.end());
    BOOST_REQUIRE(session->m_notes["ask"] == "Nothing");
}

BOOST_AUTO_TEST_CASE(EthereumPeerSuite_requestReceipts)
{
    h256 blockHash0("0x949d991d685738352398dff73219ab19c62c06e6f8ce899fbae755d5127ed1ef");
    h256 blockHash1("0x0e4562a10381dec21b205ed72637e6b1b523bdd0e4d4d50af5cd23dd4500a217");
    h256 blockHash2("0x53ab44f45948543775a4c405085b918e5e648db1201283bb54a59701afdaedf3");
    peer.requestReceipts({ blockHash0, blockHash1, blockHash2 });

    bytes payloadSent(session->m_bytesSent.begin() + 1, session->m_bytesSent.end());
    RLP rlp(payloadSent);
    BOOST_REQUIRE_EQUAL(rlp.itemCount(), 3);
    BOOST_REQUIRE_EQUAL(static_cast<h256>(rlp[0]), blockHash0);
    BOOST_REQUIRE_EQUAL(static_cast<h256>(rlp[1]), blockHash1);
    BOOST_REQUIRE_EQUAL(static_cast<h256>(rlp[2]), blockHash2);
}

BOOST_AUTO_TEST_SUITE_END()
