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
/** @file WebThree.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <thread>
#include <mutex>
#include <list>
#include <atomic>
#include <boost/asio.hpp> // Make sure boost/asio.hpp is included before windows.h.
#include <boost/utility.hpp>
#include <libdevcore/Common.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Guards.h>
#include <libdevcore/Exceptions.h>
#include <libp2p/Host.h>
#include <libethereum/Client.h>
#include <libethereum/ChainParams.h>

namespace dev
{

enum WorkState
{
    Active = 0,
    Deleting,
    Deleted
};

namespace eth { class Interface; }
namespace shh { class Interface; }
namespace bzz { class Interface; class Client; }

class NetworkFace
{
public:
    virtual ~NetworkFace() = default;

    /// Get information concerning this node.
    virtual p2p::NodeInfo nodeInfo() const = 0;

    /// Get information on the current peer set.
    virtual std::vector<p2p::PeerSessionInfo> peers() = 0;

    /// Same as peers().size(), but more efficient.
    virtual size_t peerCount() const = 0;

    /// Generalised peer addition.
    virtual void addPeer(p2p::NodeSpec const& _node, p2p::PeerType _t) = 0;

    /// Add node to connect to.
    virtual void addNode(p2p::NodeID const& _node, bi::tcp::endpoint const& _hostEndpoint) = 0;
    
    /// Require connection to peer.
    virtual void requirePeer(p2p::NodeID const& _node, bi::tcp::endpoint const& _endpoint) = 0;
    
    /// Save peers
    virtual dev::bytes saveNetwork() = 0;

    /// Sets the ideal number of peers.
    virtual void setIdealPeerCount(size_t _n) = 0;

    /// Get network id
    virtual u256 networkId() const = 0;

    /// Start the network subsystem.
    virtual void startNetwork() = 0;

    /// Stop the network subsystem.
    virtual void stopNetwork() = 0;

    /// Is network working? there may not be any peers yet.
    virtual bool isNetworkStarted() const = 0;

    /// Get enode string.
    virtual std::string enode() const = 0;
};


/**
 * @brief Main API hub for interfacing with Web 3 components. This doesn't do any local multiplexing, so you can only have one
 * running on any given machine for the provided DB path.
 *
 * Keeps a libp2p Host going (administering the work thread with m_workNet).
 *
 * Encapsulates a bunch of P2P protocols (interfaces), each using the same underlying libp2p Host.
 *
 * Provides a baseline for the multiplexed multi-protocol session class, WebThree.
 */
class WebThreeDirect: public NetworkFace
{
public:
    /// Constructor for private instance. If there is already another process on the machine using @a _dbPath, then this will throw an exception.
    /// ethereum() may be safely static_cast()ed to a eth::Client*.
    WebThreeDirect(std::string const& _clientVersion, boost::filesystem::path const& _dbPath,
        boost::filesystem::path const& _snapshotPath, eth::ChainParams const& _params,
        WithExisting _we = WithExisting::Trust,
        std::set<std::string> const& _interfaces = {"eth", "shh", "bzz"},
        p2p::NetworkConfig const& _n = p2p::NetworkConfig{},
        bytesConstRef _network = bytesConstRef(), bool _testing = false);

    /// Destructor.
    ~WebThreeDirect() override;

    // The mainline interfaces:

    eth::Client* ethereum() const
    {
        if (!m_ethereum)
            BOOST_THROW_EXCEPTION(InterfaceNotSupported() << errinfo_interface("eth"));
        return m_ethereum.get();
    }

    // Misc stuff:

    static std::string composeClientVersion(std::string const& _client);
    std::string const& clientVersion() const { return m_clientVersion; }

    // Network stuff:

    /// Get information on the current peer set.
    std::vector<p2p::PeerSessionInfo> peers() override;

    /// Same as peers().size(), but more efficient.
    size_t peerCount() const override;
    
    /// Generalised peer addition.
    virtual void addPeer(p2p::NodeSpec const& _node, p2p::PeerType _t) override;

    /// Add node to connect to.
    virtual void addNode(p2p::NodeID const& _node, bi::tcp::endpoint const& _hostEndpoint) override;

    /// Add node to connect to.
    void addNode(p2p::NodeID const& _node, std::string const& _hostString) { addNode(_node, p2p::Network::resolveHost(_hostString)); }
    
    /// Add node to connect to.
    void addNode(bi::tcp::endpoint const& _endpoint) { addNode(p2p::NodeID(), _endpoint); }

    /// Add node to connect to.
    void addNode(std::string const& _hostString) { addNode(p2p::NodeID(), _hostString); }
    
    /// Require connection to peer.
    void requirePeer(p2p::NodeID const& _node, bi::tcp::endpoint const& _endpoint) override;

    /// Require connection to peer.
    void requirePeer(p2p::NodeID const& _node, std::string const& _hostString) { requirePeer(_node, p2p::Network::resolveHost(_hostString)); }

    /// Save peers
    dev::bytes saveNetwork() override;

    /// Sets the ideal number of peers.
    void setIdealPeerCount(size_t _n) override;

    /// Experimental. Sets ceiling for incoming connections to multiple of ideal peer count.
    void setPeerStretch(size_t _n);
    
    p2p::NodeInfo nodeInfo() const override { return m_net.nodeInfo(); }

    u256 networkId() const override { return m_ethereum.get()->networkId(); }

    std::string enode() const override { return m_net.enode(); }

    /// Start the network subsystem.
    void startNetwork() override { m_net.start(); }

    /// Stop the network subsystem.
    void stopNetwork() override { m_net.stop(); }

    /// Is network working? there may not be any peers yet.
    bool isNetworkStarted() const override { return m_net.isStarted(); }

private:
    std::string m_clientVersion;                    ///< Our end-application client's name/version.

    std::unique_ptr<eth::Client> m_ethereum;        ///< Client for Ethereum ("eth") protocol.

    p2p::Host m_net;                                ///< Should run in background and send us events when blocks found and allow us to send blocks as required.
};


}
