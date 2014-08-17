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
/** @file PeerServer.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <mutex>
#include <map>
#include <vector>
#include <set>
#include <memory>
#include <utility>
#include <thread>
#include <libethcore/CommonEth.h>
#include "PeerNetwork.h"
#include "Guards.h"
namespace ba = boost::asio;
namespace bi = boost::asio::ip;

namespace eth
{

class TransactionQueue;
class BlockQueue;

/**
 * @brief The PeerServer class
 * @warning None of this is thread-safe. You have been warned.
 */
class PeerServer
{
	friend class PeerSession;

public:
	/// Start server, listening for connections on the given port.
	PeerServer(std::string const& _clientVersion, BlockChain const& _ch, u256 _networkId, unsigned short _port, NodeMode _m = NodeMode::Full, std::string const& _publicAddress = std::string(), bool _upnp = true);
	/// Start server, listening for connections on a system-assigned port.
	PeerServer(std::string const& _clientVersion, BlockChain const& _ch, u256 _networkId, NodeMode _m = NodeMode::Full, std::string const& _publicAddress = std::string(), bool _upnp = true);
	/// Start server, but don't listen.
	PeerServer(std::string const& _clientVersion, BlockChain const& _ch, u256 _networkId, NodeMode _m = NodeMode::Full);

	/// Will block on network process events.
	~PeerServer();

	/// Start peer server thread
	void run(TransactionQueue& _tq, BlockQueue& _bq);
	
	/// @returns if server is running
	bool running() { std::lock_guard<std::mutex> l(x_run); return m_stop ? false : !!m_run; };
	
	/// Disconnect all peers and stop thread
	void stop();
	
	/// Closes all peers.
	void disconnectPeers();

	static unsigned protocolVersion();
	u256 networkId() { return m_networkId; }

	/// Connect to a peer explicitly.
	void connect(std::string const& _addr, unsigned short _port = 30303) noexcept;
	void connect(bi::tcp::endpoint const& _ep);

	/// Sync with the BlockChain. It might contain one of our mined blocks, we might have new candidates from the network.
	bool sync(TransactionQueue&, BlockQueue& _bc);

	/// Conduct I/O, polling, syncing, whatever.
	void process() { if (isInitialised()) m_ioService.poll(); }

	bool havePeer(Public _id) const { Guard l(x_peers); return m_peers.count(_id) != 0; }

	/// Set ideal number of peers.
	void setIdealPeerCount(unsigned _n) { m_idealPeerCount = _n; }

	void setMode(NodeMode _m) { m_mode = _m; }

	/// Get peer information.
    std::vector<PeerInfo> peers(bool _updatePing = false) const;

	/// Get number of peers connected; equivalent to, but faster than, peers().size().
	size_t peerCount() const { Guard l(x_peers); return m_peers.size(); }

	/// Ping the peers, to update the latency information.
	void pingAll();

	/// Get the port we're listening on currently.
	unsigned short listenPort() const { return m_public.port(); }

	bytes savePeers() const;
	void restorePeers(bytesConstRef _b);

private:
	void peerEvent(PeerEvent _e, std::shared_ptr<PeerSession> const& _s);
	void registerPeer(std::shared_ptr<PeerSession> _s);

	/// Session wants to pass us a block that we might not have.
	/// @returns true if we didn't have it.
	bool noteBlock(h256 _hash, bytesConstRef _data);

	void seal(bytes& _b);
	void populateAddresses();
	void determinePublic(std::string const& _publicAddress, bool _upnp);
	void ensureAccepting();

	void growPeers();
	void prunePeers();
	void maintainTransactions(TransactionQueue& _tq, h256 _currentBlock);
	void maintainBlocks(BlockQueue& _bq, h256 _currentBlock);

	///	Check to see if the network peer-state initialisation has happened.
	bool isInitialised() const { return m_latestBlockSent; }
	/// Initialises the network peer-state, doing the stuff that needs to be once-only. @returns true if it really was first.
	bool ensureInitialised(TransactionQueue& _tq);

	std::map<Public, bi::tcp::endpoint> potentialPeers();

	std::unique_ptr<std::thread> m_run;		///< Thread which periodically clalls process() and sync().
	mutable std::mutex x_run;				///< Ensure a single thread and state (start or stop).
	std::atomic<bool> m_stop;				///< Setting to false stops running thread in threadsafe manner.
	
	std::string m_clientVersion;
	NodeMode m_mode = NodeMode::Full;

	unsigned short m_listenPort;

	BlockChain const* m_chain = nullptr;
	ba::io_service m_ioService;
	bi::tcp::acceptor m_acceptor;
	bi::tcp::socket m_socket;

	UPnP* m_upnp = nullptr;
	bi::tcp::endpoint m_public;
	KeyPair m_key;

	u256 m_networkId;

	mutable std::mutex x_peers;
	std::map<Public, std::weak_ptr<PeerSession>> m_peers;
	std::deque<std::pair<PeerEvent, std::weak_ptr<PeerSession>>> m_peerEvents;

	mutable std::recursive_mutex m_incomingLock;
	std::vector<bytes> m_incomingTransactions;
	std::vector<bytes> m_incomingBlocks;
	std::map<Public, std::pair<bi::tcp::endpoint, unsigned>> m_incomingPeers;
	std::vector<Public> m_freePeers;
	
	h256 m_latestBlockSent;
	std::set<h256> m_transactionsSent;

	std::chrono::steady_clock::time_point m_lastPeersRequest;
	unsigned m_idealPeerCount = 5;

	std::vector<bi::address_v4> m_addresses;
	std::vector<bi::address_v4> m_peerAddresses;

	bool m_accepting = false;
};

}
