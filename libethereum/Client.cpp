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
/** @file Client.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "Client.h"

#include <chrono>
#include <thread>
#include <future>
#include <boost/filesystem.hpp>
#include <libethential/Log.h>
#include "Defaults.h"
#include "PeerServer.h"
using namespace std;
using namespace eth;

void MessageFilter::fillStream(RLPStream& _s) const
{
	_s.appendList(8) << m_from << m_to << m_stateAltered << m_altered << m_earliest << m_latest << m_max << m_skip;
}

h256 MessageFilter::sha3() const
{
	RLPStream s;
	fillStream(s);
	return eth::sha3(s.out());
}

VersionChecker::VersionChecker(string const& _dbPath):
	m_path(_dbPath.size() ? _dbPath : Defaults::dbPath())
{
	m_ok = RLP(contents(m_path + "/protocol")).toInt<unsigned>(RLP::LaisezFaire) == c_protocolVersion && RLP(contents(m_path + "/database")).toInt<unsigned>(RLP::LaisezFaire) == c_databaseVersion;
}

void VersionChecker::setOk()
{
	if (!m_ok)
	{
		try
		{
			boost::filesystem::create_directory(m_path);
		}
		catch (...) {}
		writeFile(m_path + "/protocol", rlp(c_protocolVersion));
		writeFile(m_path + "/database", rlp(c_databaseVersion));
	}
}

Client::Client(std::string const& _clientVersion, Address _us, std::string const& _dbPath, bool _forceClean):
	m_clientVersion(_clientVersion),
	m_vc(_dbPath),
	m_bc(_dbPath, !m_vc.ok() || _forceClean),
	m_stateDB(State::openDB(_dbPath, !m_vc.ok() || _forceClean)),
	m_preMine(_us, m_stateDB),
	m_postMine(_us, m_stateDB)
{
	if (_dbPath.size())
		Defaults::setDBPath(_dbPath);
	m_vc.setOk();

	m_miner.reset(new Miner(m_bc));
	
	// todo: shouldn't start threads from constructors
	const char* c_threadName = "eth";
	m_run.reset(new thread([=]()
	{
		setThreadName(c_threadName);
		m_stop.store(false, std::memory_order_release);
		while (!m_stop.load(std::memory_order_acquire))
		{
		   ensureWorking();
		   this_thread::sleep_for(chrono::milliseconds(250));
		}
	}));
}

Client::~Client()
{
	// Stop Client thread.
	m_stop.store(true, std::memory_order_release);
	m_run->join();
	
	// Stop additional threads in a well mannered fashion.
	if (m_net.get())
		m_net->stop();
	m_miner->stop();
	m_bc.stop();
	
	// Synchronise state according to the head of the block chain.
	// TODO: currently it contains keys for *all* blocks. Make it remove old ones.
	WriteGuard l(x_stateDB);
	m_preMine.sync(m_bc);
}

/// ensure blockchain, network, and miner are working
void Client::ensureWorking()
{
	// flushTransactions implicitly ensures blockchain is running
	if (!m_bc.running())
		flushTransactions();
	
	// run network
	if (m_net.get())
		m_net->run(m_tq, m_bq);
	
	// run mining
	if (m_doMine && (m_forceMining || m_pendingCount) && !m_miner->running())
		m_miner->run(m_postMine, [&](MineProgress _progress, bool _complete, State& _mined){
			cworkin << "WORK";
			m_mineProgress = _progress;
			if (_complete)
			{
				cwork << "COMPLETE MINE";
				
				lock_guard<std::mutex> l(x_run);
				
				// TODO: is stop() still necessary if moved into BlockChain::run() callback?
				m_bc.stop(); // prevent blockchain commiting state to disk
				_mined.completeMine(); // commit to db
				
				h256Set changeds;
				cwork << "CHAIN <== postSTATE";
				h256s hs = m_bc.attemptImport(_mined.blockData(), m_stateDB);
				if (hs.size())
				{
					for (auto h: hs)
						appendFromNewBlock(h, changeds);
					changeds.insert(ChainChangedFilter);
					//_changeds.insert(PendingChangedFilter);	// if we mined the new block, then we've probably reset the pending transactions.
				}
				
				cwork << "noteChanged" << changeds.size() << "items";
				noteChanged(changeds);
				cworkout << "WORK";
				
				m_postMine = State(std::move(_mined));
			}
		});
	else if (!m_doMine && m_miner->running())
		m_miner->stop();
}

void Client::flushTransactions()
{
	// blockchain runtime will only sync when blockqueue is affected;
	// call this method when transaction queue is modified
	lock_guard<std::mutex> l(x_run);
	m_bc.run(m_bq, m_stateDB, [&](h256s newBlocks, OverlayDB &stateDB){
		sync(newBlocks, stateDB);
	});
}

void Client::sync(h256s _newBlocks, OverlayDB& _stateDB)
{
	h256Set changeds;
	
	// Synchronise state to block chain.
	// This should remove any transactions on our queue that are included within our state.
	// It also guarantees that the state reflects the longest (valid!) chain on the block chain.
	//   This might mean reverting to an earlier state and replaying some blocks, or, (worst-case:
	//   if there are no checkpoints before our fork) reverting to the genesis block and replaying
	//   all blocks.
	// Resynchronise state with block chain & trans
	
	cwork << "BQ ==> CHAIN ==> STATE";
	// TODO: remove transactions from m_tq nicely rather than relying on out of date nonce later on.
	if (_newBlocks.size())
	{
		for (auto i: _newBlocks)
			appendFromNewBlock(i, changeds);
		changeds.insert(ChainChangedFilter);
	}
	
	WriteGuard l(x_stateDB);
	if (_newBlocks.size())
		m_stateDB = _stateDB;
	
	bool restartMining;
	cwork << "preSTATE <== CHAIN";
	if (m_preMine.sync(m_bc) || m_postMine.address() != m_preMine.address())
	{
		if (m_doMine)
			cnote << "New block on chain: Updating state.";
		restartMining = true;	// need to re-commit to mine.
		m_postMine = m_preMine;

		changeds.insert(PendingChangedFilter);
	}
	
	// returns h256s as blooms, once for each transaction.
	cwork << "postSTATE <== TQ";
	h256s newPendingBlooms = m_postMine.sync(m_tq);
	if (newPendingBlooms.size())
	{
		for (auto i: newPendingBlooms)
			appendFromNewPending(i, changeds);
		changeds.insert(PendingChangedFilter);
		
		if (m_doMine)
			cnote << "Additional transaction ready: Restarting mining operation.";
		restartMining = true;
	}
	m_pendingCount = m_postMine.pending().size();
	
	if (m_doMine && restartMining)
		m_miner->restart(m_postMine);
	
	l.unlock();
	noteChanged(changeds);
}

void Client::clearPending()
{
	WriteGuard l(x_stateDB);
	if (!m_postMine.pending().size())
		return;
	h256Set changeds;
	for (unsigned i = 0; i < m_postMine.pending().size(); ++i)
		appendFromNewPending(m_postMine.bloom(i), changeds);
	changeds.insert(PendingChangedFilter);
	m_postMine = m_preMine;
	noteChanged(changeds);
}

unsigned Client::installWatch(h256 _h)
{
	auto ret = m_watches.size() ? m_watches.rbegin()->first + 1 : 0;
	m_watches[ret] = ClientWatch(_h);
	cwatch << "+++" << ret << _h;
	return ret;
}

unsigned Client::installWatch(MessageFilter const& _f)
{
	lock_guard<mutex> l(m_filterLock);

	h256 h = _f.sha3();

	if (!m_filters.count(h))
		m_filters.insert(make_pair(h, _f));

	return installWatch(h);
}

void Client::uninstallWatch(unsigned _i)
{
	cwatch << "XXX" << _i;

	lock_guard<mutex> l(m_filterLock);

	auto it = m_watches.find(_i);
	if (it == m_watches.end())
		return;
	auto id = it->second.id;
	m_watches.erase(it);

	auto fit = m_filters.find(id);
	if (fit != m_filters.end())
		if (!--fit->second.refCount)
			m_filters.erase(fit);
}

void Client::appendFromNewPending(h256 _bloom, h256Set& o_changed) const
{
	lock_guard<mutex> l(m_filterLock);
	for (pair<h256, InstalledFilter> const& i: m_filters)
		if ((unsigned)i.second.filter.latest() > m_bc.number() && i.second.filter.matches(_bloom))
			o_changed.insert(i.first);
}

void Client::appendFromNewBlock(h256 _block, h256Set& o_changed) const
{
	auto d = m_bc.details(_block);

	lock_guard<mutex> l(m_filterLock);
	for (pair<h256, InstalledFilter> const& i: m_filters)
		if ((unsigned)i.second.filter.latest() >= d.number && (unsigned)i.second.filter.earliest() <= d.number && i.second.filter.matches(d.bloom))
			o_changed.insert(i.first);
}

void Client::noteChanged(h256Set const& _filters)
{
	lock_guard<mutex> l(m_filterLock);
	for (auto& i: m_watches)
		if (_filters.count(i.second.id))
		{
			cwatch << "!!!" << i.first << i.second.id;
			i.second.changes++;
		}
}

void Client::startNetwork(unsigned short _listenPort, std::string const& _seedHost, unsigned short _port, NodeMode _mode, unsigned _peers, string const& _publicIP, bool _upnp, u256 _networkId)
{
	std::async(std::launch::async, [=](){
		UpgradableGuard l(x_net);
		if (m_net.get())
			return;
		try
		{
			m_net.reset(new PeerServer(m_clientVersion, m_bc, _networkId, _listenPort, _mode, _publicIP, _upnp));
		}
		catch (std::exception const&)
		{
			// Probably already have the port open.
			cwarn << "Could not initialize with specified/default port. Trying system-assigned port";
			m_net.reset(new PeerServer(m_clientVersion, m_bc, _networkId, _mode, _publicIP, _upnp));
		}
		m_net->setIdealPeerCount(_peers);

		if (_seedHost.size())
			connect(_seedHost, _port);

		m_net->run(m_tq, m_bq);
	});
}

void Client::stopNetwork()
{
	if (m_net.get())
		m_net.reset();
}

std::vector<PeerInfo> Client::peers()
{
	ReadGuard l(x_net);
	return m_net ? m_net->peers() : std::vector<PeerInfo>();
}

size_t Client::peerCount() const
{
	ReadGuard l(x_net);
	return m_net ? m_net->peerCount() : 0;
}

void Client::setIdealPeerCount(size_t _n) const
{
	ReadGuard l(x_net);
	if (m_net)
		return m_net->setIdealPeerCount(_n);
}

bytes Client::savePeers()
{
	ReadGuard l(x_net);
	if (m_net)
		return m_net->savePeers();
	return bytes();
}

void Client::restorePeers(bytesConstRef _saved)
{
	ReadGuard l(x_net);
	if (m_net)
		return m_net->restorePeers(_saved);
}

void Client::connect(std::string const& _seedHost, unsigned short _port)
{
	ReadGuard l(x_net);
	if (!m_net.get())
		return;
	m_net->connect(_seedHost, _port);
}

void Client::startMining()
{
	m_doMine = true;
}

void Client::stopMining()
{
	m_doMine = false;
}

void Client::transact(Secret _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice)
{
	Transaction t;
//	cdebug << "Nonce at " << toAddress(_secret) << " pre:" << m_preMine.transactionsFrom(toAddress(_secret)) << " post:" << m_postMine.transactionsFrom(toAddress(_secret));
	{
		ReadGuard l(x_stateDB);
		t.nonce = m_postMine.transactionsFrom(toAddress(_secret));
	}
	t.value = _value;
	t.gasPrice = _gasPrice;
	t.gas = _gas;
	t.receiveAddress = _dest;
	t.data = _data;
	t.sign(_secret);
	cnote << "New transaction " << t;
	m_tq.attemptImport(t.rlp());
	flushTransactions();
}

bytes Client::call(Secret _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice)
{
	State temp;
	Transaction t;
//	cdebug << "Nonce at " << toAddress(_secret) << " pre:" << m_preMine.transactionsFrom(toAddress(_secret)) << " post:" << m_postMine.transactionsFrom(toAddress(_secret));
	{
		ReadGuard l(x_stateDB);
		temp = m_postMine;
		t.nonce = temp.transactionsFrom(toAddress(_secret));
	}
	t.value = _value;
	t.gasPrice = _gasPrice;
	t.gas = _gas;
	t.receiveAddress = _dest;
	t.data = _data;
	t.sign(_secret);
	bytes out;
	u256 gasUsed = temp.execute(t.data, &out, false);
	(void)gasUsed; // TODO: do something with gasused which it returns.
	return out;
}

Address Client::transact(Secret _secret, u256 _endowment, bytes const& _init, u256 _gas, u256 _gasPrice)
{
	Transaction t;
	{
		ReadGuard l(x_stateDB);
		t.nonce = m_postMine.transactionsFrom(toAddress(_secret));
	}
	t.value = _endowment;
	t.gasPrice = _gasPrice;
	t.gas = _gas;
	t.receiveAddress = Address();
	t.data = _init;
	t.sign(_secret);
	cnote << "New transaction " << t;
	m_tq.attemptImport(t.rlp());
	flushTransactions();
	return right160(sha3(rlpList(t.sender(), t.nonce)));
}

void Client::inject(bytesConstRef _rlp)
{
	m_tq.attemptImport(_rlp);
	flushTransactions();
}

unsigned Client::numberOf(int _n) const
{
	if (_n > 0)
		return _n;
	else if (_n == GenesisBlock)
		return 0;
	else
		return m_bc.details().number + max(-(int)m_bc.details().number, 1 + _n);
}

State Client::asOf(int _h) const
{
	ReadGuard l(x_stateDB);
	if (_h == 0)
		return m_postMine;
	else if (_h == -1)
		return m_preMine;
	else
		return State(m_stateDB, m_bc, m_bc.numberHash(numberOf(_h)));
}

State Client::state(unsigned _txi, h256 _block) const
{
	ReadGuard l(x_stateDB);
	return State(m_stateDB, m_bc, _block).fromPending(_txi);
}

eth::State Client::state(h256 _block) const
{
	ReadGuard l(x_stateDB);
	return State(m_stateDB, m_bc, _block);
}

eth::State Client::state(unsigned _txi) const
{
	ReadGuard l(x_stateDB);
	return m_postMine.fromPending(_txi);
}

StateDiff Client::diff(unsigned _txi, int _block) const
{
	State st = state(_block);
	return st.fromPending(_txi).diff(st.fromPending(_txi + 1));
}

StateDiff Client::diff(unsigned _txi, h256 _block) const
{
	State st = state(_block);
	return st.fromPending(_txi).diff(st.fromPending(_txi + 1));
}

std::vector<Address> Client::addresses(int _block) const
{
	vector<Address> ret;
	for (auto const& i: asOf(_block).addresses())
		ret.push_back(i.first);
	return ret;
}

u256 Client::balanceAt(Address _a, int _block) const
{
	return asOf(_block).balance(_a);
}

std::map<u256, u256> Client::storageAt(Address _a, int _block) const
{
	return asOf(_block).storage(_a);
}

u256 Client::countAt(Address _a, int _block) const
{
	return asOf(_block).transactionsFrom(_a);
}

u256 Client::stateAt(Address _a, u256 _l, int _block) const
{
	return asOf(_block).storage(_a, _l);
}

bytes Client::codeAt(Address _a, int _block) const
{
	return asOf(_block).code(_a);
}

bool MessageFilter::matches(h256 _bloom) const
{
	auto have = [=](Address const& a) { return _bloom.contains(a.bloom()); };
	if (m_from.size())
	{
		for (auto i: m_from)
			if (have(i))
				goto OK1;
		return false;
	}
	OK1:
	if (m_to.size())
	{
		for (auto i: m_to)
			if (have(i))
				goto OK2;
		return false;
	}
	OK2:
	if (m_stateAltered.size() || m_altered.size())
	{
		for (auto i: m_altered)
			if (have(i))
				goto OK3;
		for (auto i: m_stateAltered)
			if (have(i.first) && _bloom.contains(h256(i.second).bloom()))
				goto OK3;
		return false;
	}
	OK3:
	return true;
}

bool MessageFilter::matches(State const& _s, unsigned _i) const
{
	h256 b = _s.changesFromPending(_i).bloom();
	if (!matches(b))
		return false;

	Transaction t = _s.pending()[_i];
	if (!m_to.empty() && !m_to.count(t.receiveAddress))
		return false;
	if (!m_from.empty() && !m_from.count(t.sender()))
		return false;
	if (m_stateAltered.empty() && m_altered.empty())
		return true;
	StateDiff d = _s.pendingDiff(_i);
	if (!m_altered.empty())
	{
		for (auto const& s: m_altered)
			if (d.accounts.count(s))
				return true;
		return false;
	}
	if (!m_stateAltered.empty())
	{
		for (auto const& s: m_stateAltered)
			if (d.accounts.count(s.first) && d.accounts.at(s.first).storage.count(s.second))
				return true;
		return false;
	}
	return true;
}

PastMessages MessageFilter::matches(Manifest const& _m, unsigned _i) const
{
	PastMessages ret;
	matches(_m, vector<unsigned>(1, _i), _m.from, PastMessages(), ret);
	return ret;
}

bool MessageFilter::matches(Manifest const& _m, vector<unsigned> _p, Address _o, PastMessages _limbo, PastMessages& o_ret) const
{
	bool ret;

	if ((m_from.empty() || m_from.count(_m.from)) && (m_to.empty() || m_to.count(_m.to)))
		_limbo.push_back(PastMessage(_m, _p, _o));

	// Handle limbos, by checking against all addresses in alteration.
	bool alters = m_altered.empty() && m_stateAltered.empty();
	alters = alters || m_altered.count(_m.from) || m_altered.count(_m.to);

	if (!alters)
		for (auto const& i: _m.altered)
			if (m_altered.count(_m.to) || m_stateAltered.count(make_pair(_m.to, i)))
			{
				alters = true;
				break;
			}
	// If we do alter stuff,
	if (alters)
	{
		o_ret += _limbo;
		_limbo.clear();
		ret = true;
	}

	_p.push_back(0);
	for (auto const& m: _m.internal)
	{
		if (matches(m, _p, _o, _limbo, o_ret))
		{
			_limbo.clear();
			ret = true;
		}
		_p.back()++;
	}

	return ret;
}

PastMessages Client::messages(MessageFilter const& _f) const
{
	PastMessages ret;
	unsigned begin = min<unsigned>(m_bc.number(), (unsigned)_f.latest());
	unsigned end = min(begin, (unsigned)_f.earliest());
	unsigned m = _f.max();
	unsigned s = _f.skip();

	// Handle pending transactions differently as they're not on the block chain.
	if (begin == m_bc.number())
	{
		ReadGuard l(x_stateDB);
		for (unsigned i = 0; i < m_postMine.pending().size(); ++i)
		{
			// Might have a transaction that contains a matching message.
			Manifest const& ms = m_postMine.changesFromPending(i);
			PastMessages pm = _f.matches(ms, i);
			if (pm.size())
			{
				auto ts = time(0);
				for (unsigned j = 0; j < pm.size() && ret.size() != m; ++j)
					if (s)
						s--;
					else
						// Have a transaction that contains a matching message.
						ret.insert(ret.begin(), pm[j].polish(h256(), ts, m_bc.number() + 1, m_postMine.address()));
			}
		}
	}

#if ETH_DEBUG
	unsigned skipped = 0;
	unsigned falsePos = 0;
#endif
	auto h = m_bc.numberHash(begin);
	unsigned n = begin;
	for (; ret.size() != m && n != end; n--, h = m_bc.details(h).parent)
	{
		auto d = m_bc.details(h);
#if ETH_DEBUG
		int total = 0;
#endif
		if (_f.matches(d.bloom))
		{
			// Might have a block that contains a transaction that contains a matching message.
			auto bs = m_bc.blooms(h).blooms;
			Manifests ms;
			BlockInfo bi;
			for (unsigned i = 0; i < bs.size(); ++i)
				if (_f.matches(bs[i]))
				{
					// Might have a transaction that contains a matching message.
					if (ms.empty())
						ms = m_bc.traces(h).traces;
					Manifest const& changes = ms[i];
					PastMessages pm = _f.matches(changes, i);
					if (pm.size())
					{
#if ETH_DEBUG
						total += pm.size();
#endif
						if (!bi)
							bi.populate(m_bc.block(h));
						auto ts = bi.timestamp;
						auto cb = bi.coinbaseAddress;
						for (unsigned j = 0; j < pm.size() && ret.size() != m; ++j)
							if (s)
								s--;
							else
								// Have a transaction that contains a matching message.
								ret.push_back(pm[j].polish(h, ts, n, cb));
					}
				}
#if ETH_DEBUG
			if (!total)
				falsePos++;
		}
		else
			skipped++;
#else
		}
#endif
		if (n == end)
			break;
	}
#if ETH_DEBUG
//	cdebug << (begin - n) << "searched; " << skipped << "skipped; " << falsePos << "false +ves";
#endif
	return ret;
}
