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
/** @file Ethereum.h
 * @author Gav Wood <i@gavwood.com>
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2014
 */

#pragma once

#include <thread>
#include <mutex>
#include <list>
#include <atomic>
#include <deque>
#include <boost/utility.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <libdevcore/Common.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Guards.h>
#include <libevm/FeeStructure.h>
#include <libp2p/Common.h>
#include <libethcore/Dagger.h>
#include <libethereum/PastMessage.h>
#include <libethereum/MessageFilter.h>
#include <libethereum/CommonNet.h>
#include "Common.h"

namespace dev
{
namespace eth
{

class Client;
class EthereumSession;

/**
 * @brief Main API hub for interfacing with Ethereum.
 * This class is automatically able to share a single machine-wide Client instance with other
 * instances, cross-process.
 *
 * Other than that, it provides much the same subset of functionality as Client.
 *
 * Client requests are synchronous and expect a response from the server before returning.
 *
 * Ethx Wire Protocol:
 * The RPC Server is a serial request/response protocol. Request and response packets have
 * the same structure with the exception that the response packet type is generic (success or failure).
 *
 * Packets consists of a 4-byte length header followed by RLP.
 * The RLP is a 2-item list; the 1st item is the packet type and 2nd is serialized RLP of the request or response.
 *
 * When an RPC Client connects the server expects a RequestVersion packet to be sent by the client.
 *
 */
class Ethereum: public std::enable_shared_from_this<Ethereum>
{
	friend class Miner;
	friend class EthereumSession;

public:
	/// Constructor. After this, everything should be set up to go.
	Ethereum();

	/// Destructor.
	~Ethereum();
	
	unsigned socketId() const { return m_socket.native_handle(); }

	/// Check to see if the client connection is open.
	bool clientConnectionOpen() const;
	
	/// Submits the given message-call transaction.
	void transact(Secret _secret, u256 _value, Address _dest, bytes const& _data = bytes(), u256 _gas = 10000, u256 _gasPrice = 10 * szabo);

	/// Submits a new contract-creation transaction.
	/// @returns the new contract's address (assuming it all goes through).
	Address transact(Secret _secret, u256 _endowment, bytes const& _init, u256 _gas = 10000, u256 _gasPrice = 10 * szabo);

	/// Injects the RLP-encoded transaction given by the _rlp into the transaction queue directly.
	void inject(bytesConstRef _rlp);

	/// Blocks until all pending transactions have been processed.
	void flushTransactions();

	/// Makes the given call. Nothing is recorded into the state.
	bytes call(Secret _secret, u256 _value, Address _dest, bytes const& _data = bytes(), u256 _gas = 10000, u256 _gasPrice = 10 * szabo);

	// Informational stuff

	// [NEW API]

	int getDefault() const { return m_default; }
	void setDefault(int _block) { m_default = _block; }

	u256 balanceAt(Address _a) const { return balanceAt(_a, m_default); }
	u256 countAt(Address _a) const { return countAt(_a, m_default); }
	u256 stateAt(Address _a, u256 _l) const { return stateAt(_a, _l, m_default); }
	bytes codeAt(Address _a) const { return codeAt(_a, m_default); }
	std::map<u256, u256> storageAt(Address _a) const { return storageAt(_a, m_default); }

	u256 balanceAt(Address _a, int _block) const;
	u256 countAt(Address _a, int _block) const;
	u256 stateAt(Address _a, u256 _l, int _block) const;
	bytes codeAt(Address _a, int _block) const;
	std::map<u256, u256> storageAt(Address _a, int _block) const;

	PastMessages messages(MessageFilter const& _filter) const;

	// [EXTRA API]:
#if 0
	/// Get a map containing each of the pending transactions.
	/// @TODO: Remove in favour of transactions().
	Transactions pending() const { return m_postMine.pending(); }

	/// Differences between transactions.
	StateDiff diff(unsigned _txi) const { return diff(_txi, m_default); }
	StateDiff diff(unsigned _txi, h256 _block) const;
	StateDiff diff(unsigned _txi, int _block) const;

	/// Get a list of all active addresses.
	std::vector<Address> addresses() const { return addresses(m_default); }
	std::vector<Address> addresses(int _block) const;

	/// Get the fee associated for a transaction with the given data.
	static u256 txGas(uint _dataCount, u256 _gas = 0) { return c_txDataGas * _dataCount + c_txGas + _gas; }

	/// Get the remaining gas limit in this block.
	u256 gasLimitRemaining() const { return m_postMine.gasLimitRemaining(); }
#endif
	// Network stuff:
	
	
	/// Get information on the current peer set.
	std::vector<p2p::PeerInfo> peers();
	/// Same as peers().size(), but more efficient.
	size_t peerCount() const;

	/// Connect to a particular peer.
	void connect(std::string const& _seedHost, unsigned short _port = 30303);

private:
	/// Ensure that through either an rpc connection to server or, if directly from client if acting server
	void ensureReady();
	/// Start the API client. (connects to local RPC server)
	void connectToRPCServer();
	/// Start the API server.
	void startRPCServer();
	/// Responds to requests and continuously dispatches socket acceptor until shutdown.
	void runServer();
	/// @returns if instance is server
	bool isServer() const { std::lock_guard<std::mutex> l(x_client); return !!m_client.get(); }
	
	/// @returns if RLP message size is valid and matches length from 4-byte header
	bool checkPacket(bytesConstRef _msg) const;
	
	/// Send request from client to server and @return response. I/O is blocking and serialized by x_socket mutex.
	/// If server goes away midrequest, server-start and/or client connection is restarted;
	/// RPCServerWentAway will be thrown and calling-method must retry request.
	/// RPCInvalidResponse is thrown if checkPacket fails or RLP message !isList()
	/// RPCRemoteException is thrown if RLP contains remote exception (this will change)
	bytes sendRequest(RPCRequestPacketType _type, RLP const& _rlp) const;

	// RCP Shared:
	bi::tcp::endpoint m_endpoint;								///< Endpoint of 127.0.0.1:30310
	
	// RPC Server:
	boost::asio::io_service m_ioService;						///< Service for I/O. Only used by server at this time; client uses sync/blocking calls.
	std::unique_ptr<Client> m_client;							///< Ethereum Client, created when isServer(). (RPC Server)
	mutable std::mutex x_client;								///< m_client mutex. (RPC Server)
	std::thread m_serverThread;								///< Thread for IO polling, created when isServer(). (RPC Server)
	boost::asio::ip::tcp::acceptor m_acceptor;					///< Socket acceptor for incoming client connection.
	std::vector<std::weak_ptr<EthereumSession>> m_sessions;	///< Client sessions. (RPC Server)
	std::mutex x_sessions;									///< m_sessions mutex. (RPC Server)
	std::atomic<bool> m_shutdown;								///< Signals Ethereum to shutdown; prevents switching roles when sockets are closed.

	// RPC Client:
	mutable boost::asio::ip::tcp::socket m_socket;				///< Socket for local or remote connection.
	mutable std::mutex x_socket;								///< Currently used to serialize requests.
	mutable std::atomic<int> m_pendingRequests;				///< Tracks pending client I/O. Destructor will not close socket until 0.

	int m_default = -1;
};

}
}
