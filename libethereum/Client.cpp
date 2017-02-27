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
#include <memory>
#include <thread>
#include <boost/filesystem.hpp>
#include <libdevcore/Log.h>
#include <libp2p/Host.h>
#include "Defaults.h"
#include "Executive.h"
#include "EthereumHost.h"
#include "Block.h"
#include "TransactionQueue.h"
using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace p2p;

static_assert(BOOST_VERSION == 106300, "Wrong boost headers version");

std::ostream& dev::eth::operator<<(std::ostream& _out, ActivityReport const& _r)
{
	_out << "Since " << toString(_r.since) << " (" << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - _r.since).count();
	_out << "): " << _r.ticks << "ticks";
	return _out;
}

#if defined(_WIN32)
const char* ClientNote::name() { return EthTeal "^" EthBlue " i"; }
const char* ClientChat::name() { return EthTeal "^" EthWhite " o"; }
const char* ClientTrace::name() { return EthTeal "^" EthGray " O"; }
const char* ClientDetail::name() { return EthTeal "^" EthCoal " 0"; }
#else
const char* ClientNote::name() { return EthTeal "⧫" EthBlue " ℹ"; }
const char* ClientChat::name() { return EthTeal "⧫" EthWhite " ◌"; }
const char* ClientTrace::name() { return EthTeal "⧫" EthGray " ◎"; }
const char* ClientDetail::name() { return EthTeal "⧫" EthCoal " ●"; }
#endif

Client::Client(
	ChainParams const& _params,
	int _networkID,
	p2p::Host* _host,
	std::shared_ptr<GasPricer> _gpForAdoption,
	std::string const& _dbPath,
	WithExisting _forceAction,
	TransactionQueue::Limits const& _l
):
	ClientBase(_l),
	Worker("eth", 0),
	m_bc(_params, _dbPath, _forceAction, [](unsigned d, unsigned t){ std::cerr << "REVISING BLOCKCHAIN: Processed " << d << " of " << t << "...\r"; }),
	m_gp(_gpForAdoption ? _gpForAdoption : make_shared<TrivialGasPricer>()),
	m_preSeal(chainParams().accountStartNonce),
	m_postSeal(chainParams().accountStartNonce),
	m_working(chainParams().accountStartNonce)
{
	init(_host, _dbPath, _forceAction, _networkID);
}

Client::~Client()
{
	stopWorking();
}

void Client::init(p2p::Host* _extNet, std::string const& _dbPath, WithExisting _forceAction, u256 _networkId)
{
	DEV_TIMED_FUNCTION_ABOVE(500);

	// Cannot be opened until after blockchain is open, since BlockChain may upgrade the database.
	// TODO: consider returning the upgrade mechanism here. will delaying the opening of the blockchain database
	// until after the construction.
	m_stateDB = State::openDB(_dbPath, bc().genesisHash(), _forceAction);
	// LAZY. TODO: move genesis state construction/commiting to stateDB openning and have this just take the root from the genesis block.
	m_preSeal = bc().genesisBlock(m_stateDB);
	m_postSeal = m_preSeal;

	m_bq.setChain(bc());

	m_lastGetWork = std::chrono::system_clock::now() - chrono::seconds(30);
	m_tqReady = m_tq.onReady([=](){ this->onTransactionQueueReady(); });	// TODO: should read m_tq->onReady(thisThread, syncTransactionQueue);
	m_tqReplaced = m_tq.onReplaced([=](h256 const&){ m_needStateReset = true; });
	m_bqReady = m_bq.onReady([=](){ this->onBlockQueueReady(); });			// TODO: should read m_bq->onReady(thisThread, syncBlockQueue);
	m_bq.setOnBad([=](Exception& ex){ this->onBadBlock(ex); });
	bc().setOnBad([=](Exception& ex){ this->onBadBlock(ex); });
	bc().setOnBlockImport([=](BlockHeader const& _info){
		if (auto h = m_host.lock())
			h->onBlockImported(_info);
	});

	if (_forceAction == WithExisting::Rescue)
		bc().rescue(m_stateDB);

	m_gp->update(bc());

	auto host = _extNet->registerCapability(make_shared<EthereumHost>(bc(), m_stateDB, m_tq, m_bq, _networkId));
	m_host = host;

	_extNet->addCapability(host, EthereumHost::staticName(), EthereumHost::c_oldProtocolVersion); //TODO: remove this once v61+ protocol is common


	if (_dbPath.size())
		Defaults::setDBPath(_dbPath);
	doWork(false);
	startWorking();
}

ImportResult Client::queueBlock(bytes const& _block, bool _isSafe)
{
	if (m_bq.status().verified + m_bq.status().verifying + m_bq.status().unverified > 10000)
		this_thread::sleep_for(std::chrono::milliseconds(500));
	return m_bq.import(&_block, _isSafe);
}

tuple<ImportRoute, bool, unsigned> Client::syncQueue(unsigned _max)
{
	stopWorking();
	return bc().sync(m_bq, m_stateDB, _max);
}

void Client::onBadBlock(Exception& _ex) const
{
	// BAD BLOCK!!!
	bytes const* block = boost::get_error_info<errinfo_block>(_ex);
	if (!block)
	{
		cwarn << "ODD: onBadBlock called but exception (" << _ex.what() << ") has no block in it.";
		cwarn << boost::diagnostic_information(_ex);
		return;
	}

	badBlock(*block, _ex.what());
}

void Client::callQueuedFunctions()
{
	while (true)
	{
		function<void()> f;
		DEV_WRITE_GUARDED(x_functionQueue)
			if (!m_functionQueue.empty())
			{
				f = m_functionQueue.front();
				m_functionQueue.pop();
			}
		if (f)
			f();
		else
			break;
	}
}

u256 Client::networkId() const
{
	if (auto h = m_host.lock())
		return h->networkId();
	return 0;
}

void Client::setNetworkId(u256 const& _n)
{
	if (auto h = m_host.lock())
		h->setNetworkId(_n);
}

bool Client::isSyncing() const
{
	if (auto h = m_host.lock())
		return h->isSyncing();
	return false;
}

bool Client::isMajorSyncing() const
{
	if (auto h = m_host.lock())
	{
		SyncState state = h->status().state;
		return (state != SyncState::Idle && state != SyncState::NewBlocks) || h->bq().items().first > 10;
	}
	return false;
}

void Client::startedWorking()
{
	// Synchronise the state according to the head of the block chain.
	// TODO: currently it contains keys for *all* blocks. Make it remove old ones.
	clog(ClientTrace) << "startedWorking()";

	DEV_WRITE_GUARDED(x_preSeal)
		m_preSeal.sync(bc());
	DEV_READ_GUARDED(x_preSeal)
	{
		DEV_WRITE_GUARDED(x_working)
			m_working = m_preSeal;
		DEV_WRITE_GUARDED(x_postSeal)
			m_postSeal = m_preSeal;
	}
}

void Client::doneWorking()
{
	// Synchronise the state according to the head of the block chain.
	// TODO: currently it contains keys for *all* blocks. Make it remove old ones.
	DEV_WRITE_GUARDED(x_preSeal)
		m_preSeal.sync(bc());
	DEV_READ_GUARDED(x_preSeal)
	{
		DEV_WRITE_GUARDED(x_working)
			m_working = m_preSeal;
		DEV_WRITE_GUARDED(x_postSeal)
			m_postSeal = m_preSeal;
	}
}

void Client::reopenChain(WithExisting _we)
{
	reopenChain(bc().chainParams(), _we);
}

void Client::reopenChain(ChainParams const& _p, WithExisting _we)
{
	bool wasSealing = wouldSeal();
	if (wasSealing)
		stopSealing();
	stopWorking();

	m_tq.clear();
	m_bq.clear();
	sealEngine()->cancelGeneration();

	{
		WriteGuard l(x_postSeal);
		WriteGuard l2(x_preSeal);
		WriteGuard l3(x_working);

		auto author = m_preSeal.author();	// backup and restore author.
		m_preSeal = Block(chainParams().accountStartNonce);
		m_postSeal = Block(chainParams().accountStartNonce);
		m_working = Block(chainParams().accountStartNonce);

		m_stateDB = OverlayDB();
		bc().reopen(_p, _we);
		m_stateDB = State::openDB(Defaults::dbPath(), bc().genesisHash(), _we);

		m_preSeal = bc().genesisBlock(m_stateDB);
		m_preSeal.setAuthor(author);
		m_postSeal = m_preSeal;
		m_working = Block(chainParams().accountStartNonce);
	}

	if (auto h = m_host.lock())
		h->reset();

	startedWorking();
	doWork();

	startWorking();
	if (wasSealing)
		startSealing();
}

void Client::executeInMainThread(function<void ()> const& _function)
{
	DEV_WRITE_GUARDED(x_functionQueue)
		m_functionQueue.push(_function);
	m_signalled.notify_all();
}

void Client::clearPending()
{
	DEV_WRITE_GUARDED(x_postSeal)
	{
		if (!m_postSeal.pending().size())
			return;
		m_tq.clear();
		DEV_READ_GUARDED(x_preSeal)
			m_postSeal = m_preSeal;
	}

	startSealing();
	h256Hash changeds;
	noteChanged(changeds);
}

template <class S, class T>
static S& filtersStreamOut(S& _out, T const& _fs)
{
	_out << "{";
	unsigned i = 0;
	for (h256 const& f: _fs)
	{
		_out << (i++ ? ", " : "");
		if (f == PendingChangedFilter)
			_out << LogTag::Special << "pending";
		else if (f == ChainChangedFilter)
			_out << LogTag::Special << "chain";
		else
			_out << f;
	}
	_out << "}";
	return _out;
}

void Client::appendFromNewPending(TransactionReceipt const& _receipt, h256Hash& io_changed, h256 _sha3)
{
	Guard l(x_filtersWatches);
	io_changed.insert(PendingChangedFilter);
	m_specialFilters.at(PendingChangedFilter).push_back(_sha3);
	for (pair<h256 const, InstalledFilter>& i: m_filters)
	{
		// acceptable number.
		auto m = i.second.filter.matches(_receipt);
		if (m.size())
		{
			// filter catches them
			for (LogEntry const& l: m)
				i.second.changes.push_back(LocalisedLogEntry(l));
			io_changed.insert(i.first);
		}
	}
}

void Client::appendFromBlock(h256 const& _block, BlockPolarity _polarity, h256Hash& io_changed)
{
	// TODO: more precise check on whether the txs match.
	auto receipts = bc().receipts(_block).receipts;

	Guard l(x_filtersWatches);
	io_changed.insert(ChainChangedFilter);
	m_specialFilters.at(ChainChangedFilter).push_back(_block);
	for (pair<h256 const, InstalledFilter>& i: m_filters)
	{
		// acceptable number & looks like block may contain a matching log entry.
		for (size_t j = 0; j < receipts.size(); j++)
		{
			auto tr = receipts[j];
			auto m = i.second.filter.matches(tr);
			if (m.size())
			{
				auto transactionHash = transaction(_block, j).sha3();
				// filter catches them
				for (LogEntry const& l: m)
					i.second.changes.push_back(LocalisedLogEntry(l, _block, (BlockNumber)bc().number(_block), transactionHash, j, 0, _polarity));
				io_changed.insert(i.first);
			}
		}
	}
}

ExecutionResult Client::call(Address _dest, bytes const& _data, u256 _gas, u256 _value, u256 _gasPrice, Address const& _from)
{
	ExecutionResult ret;
	try
	{
		Block temp(chainParams().accountStartNonce);
		clog(ClientDetail) << "Nonce at " << _dest << " pre:" << m_preSeal.transactionsFrom(_dest) << " post:" << m_postSeal.transactionsFrom(_dest);
		DEV_READ_GUARDED(x_postSeal)
			temp = m_postSeal;
		temp.mutableState().addBalance(_from, _value + _gasPrice * _gas);
		Executive e(temp);
		e.setResultRecipient(ret);
		if (!e.call(_dest, _from, _value, _gasPrice, &_data, _gas))
			e.go();
		e.finalize();
	}
	catch (...)
	{
		cwarn << "Client::call failed: " << boost::current_exception_diagnostic_information();
	}
	return ret;
}

unsigned static const c_syncMin = 1;
unsigned static const c_syncMax = 1000;
double static const c_targetDuration = 1;

void Client::syncBlockQueue()
{
//	cdebug << "syncBlockQueue()";

	ImportRoute ir;
	unsigned count;
	Timer t;
	tie(ir, m_syncBlockQueue, count) = bc().sync(m_bq, m_stateDB, m_syncAmount);
	double elapsed = t.elapsed();

	if (count)
	{
		clog(ClientNote) << count << "blocks imported in" << unsigned(elapsed * 1000) << "ms (" << (count / elapsed) << "blocks/s) in #" << bc().number();
	}

	if (elapsed > c_targetDuration * 1.1 && count > c_syncMin)
		m_syncAmount = max(c_syncMin, count * 9 / 10);
	else if (count == m_syncAmount && elapsed < c_targetDuration * 0.9 && m_syncAmount < c_syncMax)
		m_syncAmount = min(c_syncMax, m_syncAmount * 11 / 10 + 1);
	if (ir.liveBlocks.empty())
		return;
	onChainChanged(ir);
}

void Client::syncTransactionQueue()
{
	Timer timer;

	h256Hash changeds;
	TransactionReceipts newPendingReceipts;

	DEV_WRITE_GUARDED(x_working)
	{
		if (m_working.isSealed())
		{
			ctrace << "Skipping txq sync for a sealed block.";
			return;
		}

		tie(newPendingReceipts, m_syncTransactionQueue) = m_working.sync(bc(), m_tq, *m_gp);
	}

	if (newPendingReceipts.empty())
	{
		auto s = m_tq.status();
		ctrace << "No transactions to process. " << m_working.pending().size() << " pending, " << s.current << " queued, " << s.future << " future, " << s.unverified << " unverified";
		return;
	}

	DEV_READ_GUARDED(x_working)
		DEV_WRITE_GUARDED(x_postSeal)
			m_postSeal = m_working;

	DEV_READ_GUARDED(x_postSeal)
		for (size_t i = 0; i < newPendingReceipts.size(); i++)
			appendFromNewPending(newPendingReceipts[i], changeds, m_postSeal.pending()[i].sha3());

	// Tell farm about new transaction (i.e. restart mining).
	onPostStateChanged();

	// Tell watches about the new transactions.
	noteChanged(changeds);

	// Tell network about the new transactions.
	if (auto h = m_host.lock())
		h->noteNewTransactions();

	ctrace << "Processed " << newPendingReceipts.size() << " transactions in" << (timer.elapsed() * 1000) << "(" << (bool)m_syncTransactionQueue << ")";
}

void Client::onDeadBlocks(h256s const& _blocks, h256Hash& io_changed)
{
	// insert transactions that we are declaring the dead part of the chain
	for (auto const& h: _blocks)
	{
		clog(ClientTrace) << "Dead block:" << h;
		for (auto const& t: bc().transactions(h))
		{
			clog(ClientTrace) << "Resubmitting dead-block transaction " << Transaction(t, CheckTransaction::None);
			ctrace << "Resubmitting dead-block transaction " << Transaction(t, CheckTransaction::None);
			m_tq.import(t, IfDropped::Retry);
		}
	}

	for (auto const& h: _blocks)
		appendFromBlock(h, BlockPolarity::Dead, io_changed);
}

void Client::onNewBlocks(h256s const& _blocks, h256Hash& io_changed)
{
	// remove transactions from m_tq nicely rather than relying on out of date nonce later on.
	for (auto const& h: _blocks)
		clog(ClientTrace) << "Live block:" << h;

	if (auto h = m_host.lock())
		h->noteNewBlocks();

	for (auto const& h: _blocks)
		appendFromBlock(h, BlockPolarity::Live, io_changed);
}

void Client::resyncStateFromChain()
{
	// RESTART MINING

//	ctrace << "resyncStateFromChain()";

	if (!isMajorSyncing())
	{
		bool preChanged = false;
		Block newPreMine(chainParams().accountStartNonce);
		DEV_READ_GUARDED(x_preSeal)
			newPreMine = m_preSeal;

		// TODO: use m_postSeal to avoid re-evaluating our own blocks.
		preChanged = newPreMine.sync(bc());

		if (preChanged || m_postSeal.author() != m_preSeal.author())
		{
			DEV_WRITE_GUARDED(x_preSeal)
				m_preSeal = newPreMine;
			DEV_WRITE_GUARDED(x_working)
				m_working = newPreMine;
			DEV_READ_GUARDED(x_postSeal)
				if (!m_postSeal.isSealed() || m_postSeal.info().hash() != newPreMine.info().parentHash())
					for (auto const& t: m_postSeal.pending())
					{
						clog(ClientTrace) << "Resubmitting post-seal transaction " << t;
//						ctrace << "Resubmitting post-seal transaction " << t;
						auto ir = m_tq.import(t, IfDropped::Retry);
						if (ir != ImportResult::Success)
							onTransactionQueueReady();
					}
			DEV_READ_GUARDED(x_working) DEV_WRITE_GUARDED(x_postSeal)
				m_postSeal = m_working;

			onPostStateChanged();
		}

		// Quick hack for now - the TQ at this point already has the prior pending transactions in it;
		// we should resync with it manually until we are stricter about what constitutes "knowing".
		onTransactionQueueReady();
	}
}

void Client::resetState()
{
	Block newPreMine(chainParams().accountStartNonce);
	DEV_READ_GUARDED(x_preSeal)
		newPreMine = m_preSeal;

	DEV_WRITE_GUARDED(x_working)
		m_working = newPreMine;
	DEV_READ_GUARDED(x_working) DEV_WRITE_GUARDED(x_postSeal)
		m_postSeal = m_working;

	onPostStateChanged();
	onTransactionQueueReady();
}

void Client::onChainChanged(ImportRoute const& _ir)
{
//	ctrace << "onChainChanged()";
	h256Hash changeds;
	onDeadBlocks(_ir.deadBlocks, changeds);
	for (auto const& t: _ir.goodTranactions)
	{
		clog(ClientTrace) << "Safely dropping transaction " << t.sha3();
		m_tq.dropGood(t);
	}
	onNewBlocks(_ir.liveBlocks, changeds);
	resyncStateFromChain();
	noteChanged(changeds);
}

bool Client::remoteActive() const
{
	return chrono::system_clock::now() - m_lastGetWork < chrono::seconds(30);
}

void Client::onPostStateChanged()
{
	clog(ClientTrace) << "Post state changed.";
	m_signalled.notify_all();
	m_remoteWorking = false;
}

void Client::startSealing()
{
	if (m_wouldSeal == true)
		return;
	clog(ClientNote) << "Mining Beneficiary: " << author();
	if (author())
	{
		m_wouldSeal = true;
		m_signalled.notify_all();
	}
	else
		clog(ClientNote) << "You need to set an author in order to seal!";
}

void Client::rejigSealing()
{
	if ((wouldSeal() || remoteActive()) && !isMajorSyncing())
	{
		if (sealEngine()->shouldSeal(this))
		{
			m_wouldButShouldnot = false;

			clog(ClientTrace) << "Rejigging seal engine...";
			DEV_WRITE_GUARDED(x_working)
			{
				if (m_working.isSealed())
				{
					clog(ClientNote) << "Tried to seal sealed block...";
					return;
				}
				m_working.commitToSeal(bc(), m_extraData);
			}
			DEV_READ_GUARDED(x_working)
			{
				DEV_WRITE_GUARDED(x_postSeal)
					m_postSeal = m_working;
				m_sealingInfo = m_working.info();
			}

			if (wouldSeal())
			{
				sealEngine()->onSealGenerated([=](bytes const& header){
					if (!this->submitSealed(header))
						clog(ClientNote) << "Submitting block failed...";
				});
				ctrace << "Generating seal on" << m_sealingInfo.hash(WithoutSeal) << "#" << m_sealingInfo.number();
				sealEngine()->generateSeal(m_sealingInfo);
			}
		}
		else
			m_wouldButShouldnot = true;
	}
	if (!m_wouldSeal)
		sealEngine()->cancelGeneration();
}

void Client::noteChanged(h256Hash const& _filters)
{
	Guard l(x_filtersWatches);
	if (_filters.size())
		filtersStreamOut(cwatch << "noteChanged:", _filters);
	// accrue all changes left in each filter into the watches.
	for (auto& w: m_watches)
		if (_filters.count(w.second.id))
		{
			if (m_filters.count(w.second.id))
			{
				cwatch << "!!!" << w.first << w.second.id.abridged();
				w.second.changes += m_filters.at(w.second.id).changes;
			}
			else if (m_specialFilters.count(w.second.id))
				for (h256 const& hash: m_specialFilters.at(w.second.id))
				{
					cwatch << "!!!" << w.first << LogTag::Special << (w.second.id == PendingChangedFilter ? "pending" : w.second.id == ChainChangedFilter ? "chain" : "???");
					w.second.changes.push_back(LocalisedLogEntry(SpecialLogEntry, hash));
				}
		}
	// clear the filters now.
	for (auto& i: m_filters)
		i.second.changes.clear();
	for (auto& i: m_specialFilters)
		i.second.clear();
}

void Client::doWork(bool _doWait)
{
	bool t = true;
	if (m_syncBlockQueue.compare_exchange_strong(t, false))
		syncBlockQueue();

	if (m_needStateReset)
	{
		resetState();
		m_needStateReset = false;
	}

	t = true;
	bool isSealed = false;
	DEV_READ_GUARDED(x_working)
		isSealed = m_working.isSealed();
	if (!isSealed && !isSyncing() && !m_remoteWorking && m_syncTransactionQueue.compare_exchange_strong(t, false))
		syncTransactionQueue();

	tick();

	rejigSealing();

	callQueuedFunctions();

	DEV_READ_GUARDED(x_working)
		isSealed = m_working.isSealed();
	// If the block is sealed, we have to wait for it to tickle through the block queue
	// (which only signals as wanting to be synced if it is ready).
	if (!m_syncBlockQueue && !m_syncTransactionQueue && (_doWait || isSealed))
	{
		std::unique_lock<std::mutex> l(x_signalled);
		m_signalled.wait_for(l, chrono::seconds(1));
	}
}

void Client::tick()
{
	if (chrono::system_clock::now() - m_lastTick > chrono::seconds(1))
	{
		m_report.ticks++;
		checkWatchGarbage();
		m_bq.tick();
		m_lastTick = chrono::system_clock::now();
		if (m_report.ticks == 15)
			clog(ClientTrace) << activityReport();
	}
}

void Client::checkWatchGarbage()
{
	if (chrono::system_clock::now() - m_lastGarbageCollection > chrono::seconds(5))
	{
		// watches garbage collection
		vector<unsigned> toUninstall;
		DEV_GUARDED(x_filtersWatches)
			for (auto key: keysOf(m_watches))
				if (m_watches[key].lastPoll != chrono::system_clock::time_point::max() && chrono::system_clock::now() - m_watches[key].lastPoll > chrono::seconds(20))
				{
					toUninstall.push_back(key);
					clog(ClientTrace) << "GC: Uninstall" << key << "(" << chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_watches[key].lastPoll).count() << "s old)";
				}
		for (auto i: toUninstall)
			uninstallWatch(i);

		// blockchain GC
		bc().garbageCollect();

		m_lastGarbageCollection = chrono::system_clock::now();
	}
}

void Client::prepareForTransaction()
{
	startWorking();
}

Block Client::block(h256 const& _block) const
{
	try
	{
		Block ret(bc(), m_stateDB);
		ret.populateFromChain(bc(), _block);
		return ret;
	}
	catch (Exception& ex)
	{
		ex << errinfo_block(bc().block(_block));
		onBadBlock(ex);
		return Block(bc());
	}
}

Block Client::block(h256 const& _blockHash, PopulationStatistics* o_stats) const
{
	try
	{
		Block ret(bc(), m_stateDB);
		PopulationStatistics s = ret.populateFromChain(bc(), _blockHash);
		if (o_stats)
			swap(s, *o_stats);
		return ret;
	}
	catch (Exception& ex)
	{
		ex << errinfo_block(bc().block(_blockHash));
		onBadBlock(ex);
		return Block(bc());
	}
}

State Client::state(unsigned _txi, h256 const& _blockHash) const
{
	try
	{
		return block(_blockHash).fromPending(_txi);
	}
	catch (Exception& ex)
	{
		ex << errinfo_block(bc().block(_blockHash));
		onBadBlock(ex);
		return State(chainParams().accountStartNonce);
	}
}

eth::State Client::state(unsigned _txi) const
{
	DEV_READ_GUARDED(x_postSeal)
		return m_postSeal.fromPending(_txi);
	assert(false);
	return State(chainParams().accountStartNonce);
}

void Client::flushTransactions()
{
	doWork();
}

SyncStatus Client::syncStatus() const
{
	auto h = m_host.lock();
	if (!h)
		return SyncStatus();
	SyncStatus status = h->status();
	status.majorSyncing = isMajorSyncing();
	return status;
}

bool Client::submitSealed(bytes const& _header)
{
	bytes newBlock;
	{
		UpgradableGuard l(x_working);
		{
			UpgradeGuard l2(l);
			if (!m_working.sealBlock(_header))
				return false;
		}
		DEV_WRITE_GUARDED(x_postSeal)
			m_postSeal = m_working;
		newBlock = m_working.blockData();
	}

	// OPTIMISE: very inefficient to not utilise the existing OverlayDB in m_postSeal that contains all trie changes.
	return m_bq.import(&newBlock, true) == ImportResult::Success;
}

void Client::rewind(unsigned _n)
{
	executeInMainThread([=]() {
		bc().rewind(_n);
		onChainChanged(ImportRoute());
	});

	for (unsigned i = 0; i < 10; ++i)
	{
		u256 n;
		DEV_READ_GUARDED(x_working)
			n = m_working.info().number();
		if (n == _n + 1)
			break;
		this_thread::sleep_for(std::chrono::milliseconds(50));
	}
	auto h = m_host.lock();
	if (h)
		h->reset();
}
