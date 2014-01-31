/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	Foobar is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file Client.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "Common.h"
#include "Client.h"
using namespace std;
using namespace eth;

Client::Client(std::string const& _clientVersion, Address _us, std::string const& _dbPath):
	m_clientVersion(_clientVersion),
	m_bc(_dbPath),
	m_stateDB(State::openDB(_dbPath)),
	m_s(_us, m_stateDB)
{
	Defaults::setDBPath(_dbPath);

	// Synchronise the state according to the block chain - i.e. replay all transactions in block chain, in order.
	// In practise this won't need to be done since the State DB will contain the keys for the tries for most recent (and many old) blocks.
	// TODO: currently it contains keys for *all* blocks. Make it remove old ones.
	m_s.sync(m_bc);
	m_s.sync(m_tq);
	m_changed = true;

	m_work = new thread([&](){ while (m_workState != Deleting) work(); m_workState = Deleted; });
}

Client::~Client()
{
	if (m_workState == Active)
		m_workState = Deleting;
	while (m_workState != Deleted)
		usleep(10000);
}

void Client::startNetwork(short _listenPort, std::string const& _seedHost, short _port, unsigned _verbosity, NodeMode _mode, unsigned _peers, string const& _publicIP)
{
	if (m_net)
		return;
	m_net = new PeerServer(m_clientVersion, m_bc, 0, _listenPort, _mode, _publicIP);
	m_net->setIdealPeerCount(_peers);
	m_net->setVerbosity(_verbosity);
	if (_seedHost.size())
		m_net->connect(_seedHost, _port);
}

void Client::connect(std::string const& _seedHost, short _port)
{
	if (!m_net)
		return;
	m_net->connect(_seedHost, _port);
}

void Client::stopNetwork()
{
	delete m_net;
	m_net = nullptr;
}

void Client::startMining()
{
	m_doMine = true;
}

void Client::stopMining()
{
	m_doMine = false;
}

void Client::transact(Secret _secret, Address _dest, u256 _amount, u256 _fee, u256s _data)
{
	m_lock.lock();
	Transaction t;
	t.nonce = m_s.transactionsFrom(toAddress(_secret));
	t.receiveAddress = _dest;
	t.value = _amount;
	t.fee = _fee;
	t.data = _data;
	t.sign(_secret);
	m_tq.attemptImport(t.rlp());
	m_lock.unlock();
	m_changed = true;
}

void Client::work()
{
	m_lock.lock();
	// Process network events.
	// Synchronise block chain with network.
	// Will broadcast any of our (new) transactions and blocks, and collect & add any of their (new) transactions and blocks.
	if (m_net)
		if (m_net->process(m_bc, m_tq, m_stateDB))
			m_changed = true;

	// Synchronise state to block chain.
	// This should remove any transactions on our queue that are included within our state.
	// It also guarantees that the state reflects the longest (valid!) chain on the block chain.
	//   This might mean reverting to an earlier state and replaying some blocks, or, (worst-case:
	//   if there are no checkpoints before our fork) reverting to the genesis block and replaying
	//   all blocks.
	 // Resynchronise state with block chain & trans
	if (m_s.sync(m_bc))
		m_changed = true;
	if (m_s.sync(m_tq))
		m_changed = true;

	m_lock.unlock();
	if (m_doMine)
	{
		// Mine for a while.
		m_s.commitToMine(m_bc);
		MineInfo mineInfo = m_s.mine(100);
		m_mineProgress.best = max(m_mineProgress.best, mineInfo.best);
		m_mineProgress.current = mineInfo.best;
		m_mineProgress.requirement = mineInfo.requirement;

		if (mineInfo.completed)
		{
			// Import block.
			m_lock.lock();
			m_bc.attemptImport(m_s.blockData(), m_stateDB);
			m_mineProgress.best = 0;
			m_lock.unlock();
			m_changed = true;
		}
	}
	else
		usleep(100000);
}

void Client::lock()
{
	m_lock.lock();
}

void Client::unlock()
{
	m_lock.unlock();
}
