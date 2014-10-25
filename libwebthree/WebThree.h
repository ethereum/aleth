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
/** @file Client.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <thread>
#include <mutex>
#include <list>
#include <atomic>
#include <boost/utility.hpp>
#include <libdevcore/Common.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Guards.h>
#include <libdevcore/Exceptions.h>
#include <libp2p/Host.h>

#include <libwhisper/WhisperHost.h>
#include <libethereum/Client.h>

namespace dev
{

class InterfaceNotSupported: virtual public Exception { public: InterfaceNotSupported(std::string _f): m_f(_f) {} virtual const char* what() const noexcept {static std::string s_what; s_what ="Interface " + m_f + " not supported."; return s_what.c_str(); } private: std::string m_f; };

enum WorkState
{
	Active = 0,
	Deleting,
	Deleted
};

namespace eth { class Interface; }
namespace shh { class Interface; }
namespace bzz { class Interface; }

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
class WebThreeDirect
{
public:
	/// Constructor for private instance. If there is already another process on the machine using @a _dbPath, then this will throw an exception.
	/// ethereum() may be safely static_cast()ed to a eth::Client*.
	WebThreeDirect(std::string const& _clientVersion, std::string const& _dbPath, bool _forceClean = false, std::set<std::string> const& _interfaces = {"eth", "shh"}, p2p::NetworkPreferences const& _n = p2p::NetworkPreferences());

	/// Destructor.
	~WebThreeDirect();

	// The mainline interfaces:

	eth::Client* ethereum() const { if (!m_ethereum) BOOST_THROW_EXCEPTION(InterfaceNotSupported("eth")); return m_ethereum.get(); }
	std::shared_ptr<shh::WhisperHost> whisper() const { auto w = m_whisper.lock(); if (!w) BOOST_THROW_EXCEPTION(InterfaceNotSupported("shh")); return w; }
	bzz::Interface* swarm() const { BOOST_THROW_EXCEPTION(InterfaceNotSupported("bzz")); }

	// Misc stuff:

	void setClientVersion(std::string const& _name) { m_clientVersion = _name; }

	// Network stuff:

	/// Get information on the current peer set.
	std::vector<p2p::PeerInfo> peers();

	/// Same as peers().size(), but more efficient.
	size_t peerCount() const;

	/// Connect to a particular peer.
	void connect(std::string const& _seedHost, unsigned short _port = 30303);

	/// Is the network subsystem up?
	bool haveNetwork() { return peerCount() != 0; }

	/// Save peers
	dev::bytes saveNodes();

	/// Restore peers
	void restoreNodes(bytesConstRef _saved);

	/// Sets the ideal number of peers.
	void setIdealPeerCount(size_t _n);

	bool haveNetwork() const { return m_net.isStarted(); }

	void setNetworkPreferences(p2p::NetworkPreferences const& _n);

	p2p::NodeId id() const { return m_net.id(); }

	/// Gets the nodes.
	p2p::Nodes nodes() const { return m_net.nodes(); }

	/// Start the network subsystem.
	void startNetwork() { m_net.start(); }

	/// Stop the network subsystem.
	void stopNetwork() { m_net.stop(); }

private:
	std::string m_clientVersion;					///< Our end-application client's name/version.

	p2p::Host m_net;								///< Should run in background and send us events when blocks found and allow us to send blocks as required.

	std::unique_ptr<eth::Client> m_ethereum;		///< Main interface for Ethereum ("eth") protocol.
	std::weak_ptr<shh::WhisperHost> m_whisper;		///< Main interface for Whisper ("shh") protocol.
};



// TODO, probably move into libdevrpc:

class RPCSlave {};
class RPCMaster {};

// TODO, probably move into eth:

class EthereumSlave: public eth::Interface
{
public:
	EthereumSlave(RPCSlave*) {}

	// TODO: implement all of the virtuals with the RLPClient link.
};

class EthereumMaster
{
public:
	EthereumMaster(RPCMaster*) {}

	// TODO: implement the master-end of whatever the RLPClient link will send over.
};

// TODO, probably move into shh:

class WhisperSlave: public shh::Interface
{
public:
	WhisperSlave(RPCSlave*) {}

	// TODO: implement all of the virtuals with the RLPClient link.
};

class WhisperMaster
{
public:
	WhisperMaster(RPCMaster*) {}

	// TODO: implement the master-end of whatever the RLPClient link will send over.
};

/**
 * @brief Main API hub for interfacing with Web 3 components.
 *
 * This does transparent local multiplexing, so you can have as many running on the
 * same machine all working from a single DB path.
 */
class WebThree
{
public:
	/// Constructor for public instance. This will be shared across the local machine.
	WebThree();

	/// Destructor.
	~WebThree();

	// The mainline interfaces.

	eth::Interface* ethereum() const { if (!m_ethereum) BOOST_THROW_EXCEPTION(InterfaceNotSupported("eth")); return m_ethereum; }
	shh::Interface* whisper() const { if (!m_whisper) BOOST_THROW_EXCEPTION(InterfaceNotSupported("shh")); return m_whisper; }
	bzz::Interface* swarm() const { BOOST_THROW_EXCEPTION(InterfaceNotSupported("bzz")); }

	// Peer network stuff - forward through RPCSlave, probably with P2PNetworkSlave/Master classes like Whisper & Ethereum.

	/// Get information on the current peer set.
	std::vector<p2p::PeerInfo> peers();

	/// Same as peers().size(), but more efficient.
	size_t peerCount() const;

	/// Connect to a particular peer.
	void connect(std::string const& _seedHost, unsigned short _port = 30303);

	/// Is the network subsystem up?
	bool haveNetwork();

	/// Save peers
	dev::bytes savePeers();

	/// Restore peers
	void restorePeers(bytesConstRef _saved);

private:
	EthereumSlave* m_ethereum = nullptr;
	WhisperSlave* m_whisper = nullptr;

	// TODO:
	RPCSlave m_rpcSlave;
};

}
