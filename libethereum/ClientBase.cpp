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
/** @file ClientBase.cpp
 * @author Gav Wood <i@gavwood.com>
 * @author Marek Kotewicz <marek@ethdev.com>
 * @date 2015
 */

#include "ClientBase.h"
#include <algorithm>
#include "BlockChain.h"
#include "Executive.h"
#include "State.h"

using namespace std;
using namespace dev;
using namespace dev::eth;

const char* WatchChannel::name() { return EthBlue "ℹ" EthWhite "  "; }
const char* WorkInChannel::name() { return EthOrange "⚒" EthGreen "▬▶"; }
const char* WorkOutChannel::name() { return EthOrange "⚒" EthNavy "◀▬"; }
const char* WorkChannel::name() { return EthOrange "⚒" EthWhite "  "; }

static const int64_t c_maxGasEstimate = 50000000;

pair<h256, Address> ClientBase::submitTransaction(TransactionSkeleton const& _t, Secret const& _secret)
{
	prepareForTransaction();
	
	TransactionSkeleton ts(_t);
	ts.from = toAddress(_secret);
	if (_t.nonce == Invalid256)
		ts.nonce = max<u256>(postSeal().transactionsFrom(ts.from), m_tq.maxNonce(ts.from));
	if (ts.gasPrice == Invalid256)
		ts.gasPrice = gasBidPrice();
	if (ts.gas == Invalid256)
		ts.gas = min<u256>(gasLimitRemaining() / 5, balanceAt(ts.from) / ts.gasPrice);

	Transaction t(ts, _secret);
	m_tq.import(t.rlp());
	
	return make_pair(t.sha3(), toAddress(ts.from, ts.nonce));
}

// TODO: remove try/catch, allow exceptions
ExecutionResult ClientBase::call(Address const& _from, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice, BlockNumber _blockNumber, FudgeFactor _ff)
{
	ExecutionResult ret;
	try
	{
		Block temp = block(_blockNumber);
		u256 nonce = max<u256>(temp.transactionsFrom(_from), m_tq.maxNonce(_from));
		u256 gas = _gas == Invalid256 ? gasLimitRemaining() : _gas;
		u256 gasPrice = _gasPrice == Invalid256 ? gasBidPrice() : _gasPrice;
		Transaction t(_value, gasPrice, gas, _dest, _data, nonce);
		t.forceSender(_from);
		if (_ff == FudgeFactor::Lenient)
			temp.mutableState().addBalance(_from, (u256)(t.gas() * t.gasPrice() + t.value()));
		ret = temp.execute(bc().lastHashes(), t, Permanence::Reverted);
	}
	catch (...)
	{
		// TODO: Some sort of notification of failure.
	}
	return ret;
}

ExecutionResult ClientBase::create(Address const& _from, u256 _value, bytes const& _data, u256 _gas, u256 _gasPrice, BlockNumber _blockNumber, FudgeFactor _ff)
{
	ExecutionResult ret;
	try
	{
		Block temp = block(_blockNumber);
		u256 n = temp.transactionsFrom(_from);
		//	cdebug << "Nonce at " << toAddress(_secret) << " pre:" << m_preSeal.transactionsFrom(toAddress(_secret)) << " post:" << m_postSeal.transactionsFrom(toAddress(_secret));
		Transaction t(_value, _gasPrice, _gas, _data, n);
		t.forceSender(_from);
		if (_ff == FudgeFactor::Lenient)
			temp.mutableState().addBalance(_from, (u256)(t.gas() * t.gasPrice() + t.value()));
		ret = temp.execute(bc().lastHashes(), t, Permanence::Reverted);
	}
	catch (...)
	{
		// TODO: Some sort of notification of failure.
	}
	return ret;
}

std::pair<u256, ExecutionResult> ClientBase::estimateGas(Address const& _from, u256 _value, Address _dest, bytes const& _data, int64_t _maxGas, u256 _gasPrice, BlockNumber _blockNumber, GasEstimationCallback const& _callback)
{
	try
	{
		int64_t upperBound = _maxGas;
		if (upperBound == Invalid256 || upperBound > c_maxGasEstimate)
			upperBound = c_maxGasEstimate;
		int64_t lowerBound = Transaction::baseGasRequired(!_dest, &_data, EVMSchedule());
		Block bk = block(_blockNumber);
		u256 gasPrice = _gasPrice == Invalid256 ? gasBidPrice() : _gasPrice;
		ExecutionResult er;
		ExecutionResult lastGood;
		bool good = false;
		while (upperBound != lowerBound)
		{
			int64_t mid = (lowerBound + upperBound) / 2;
			u256 n = bk.transactionsFrom(_from);
			Transaction t;
			if (_dest)
				t = Transaction(_value, gasPrice, mid, _dest, _data, n);
			else
				t = Transaction(_value, gasPrice, mid, _data, n);
			t.forceSender(_from);
			EnvInfo env(bk.info(), bc().lastHashes(), 0);
			env.setGasLimit(mid);
			State tempState(bk.state());
			tempState.addBalance(_from, (u256)(t.gas() * t.gasPrice() + t.value()));
			er = tempState.execute(env, *bc().sealEngine(), t, Permanence::Reverted).first;
			if (er.excepted == TransactionException::OutOfGas ||
				er.excepted == TransactionException::OutOfGasBase ||
				er.excepted == TransactionException::OutOfGasIntrinsic ||
				er.codeDeposit == CodeDeposit::Failed ||
				er.excepted == TransactionException::BadJumpDestination)
					lowerBound = lowerBound == mid ? upperBound : mid;
			else
			{
				lastGood = er;
				upperBound = upperBound == mid ? lowerBound : mid;
				good = true;
			}

			if (_callback)
				_callback(GasEstimationProgress { lowerBound, upperBound });
		}
		if (_callback)
			_callback(GasEstimationProgress { lowerBound, upperBound });
		return make_pair(upperBound, good ? lastGood : er);
	}
	catch (...)
	{
		// TODO: Some sort of notification of failure.
		return make_pair(u256(), ExecutionResult());
	}
}

ImportResult ClientBase::injectBlock(bytes const& _block)
{
	return bc().attemptImport(_block, preSeal().db()).first;
}

u256 ClientBase::balanceAt(Address _a, BlockNumber _block) const
{
	return block(_block).balance(_a);
}

u256 ClientBase::countAt(Address _a, BlockNumber _block) const
{
	return block(_block).transactionsFrom(_a);
}

u256 ClientBase::stateAt(Address _a, u256 _l, BlockNumber _block) const
{
	return block(_block).storage(_a, _l);
}

h256 ClientBase::stateRootAt(Address _a, BlockNumber _block) const
{
	return block(_block).storageRoot(_a);
}

bytes ClientBase::codeAt(Address _a, BlockNumber _block) const
{
	return block(_block).code(_a);
}

h256 ClientBase::codeHashAt(Address _a, BlockNumber _block) const
{
	return block(_block).codeHash(_a);
}

map<h256, pair<u256, u256>> ClientBase::storageAt(Address _a, BlockNumber _block) const
{
	return block(_block).storage(_a);
}

// TODO: remove try/catch, allow exceptions
LocalisedLogEntries ClientBase::logs(unsigned _watchId) const
{
	LogFilter f;
	try
	{
		Guard l(x_filtersWatches);
		f = m_filters.at(m_watches.at(_watchId).id).filter;
	}
	catch (...)
	{
		return LocalisedLogEntries();
	}
	return logs(f);
}

LocalisedLogEntries ClientBase::logs(LogFilter const& _f) const
{
	LocalisedLogEntries ret;
	unsigned begin = min(bc().number() + 1, (unsigned)numberFromHash(_f.latest()));
	unsigned end = min(bc().number(), min(begin, (unsigned)numberFromHash(_f.earliest())));
	
	// Handle pending transactions differently as they're not on the block chain.
	if (begin > bc().number())
	{
		Block temp = postSeal();
		for (unsigned i = 0; i < temp.pending().size(); ++i)
		{
			// Might have a transaction that contains a matching log.
			TransactionReceipt const& tr = temp.receipt(i);
			LogEntries le = _f.matches(tr);
			for (unsigned j = 0; j < le.size(); ++j)
				ret.insert(ret.begin(), LocalisedLogEntry(le[j]));
		}
		begin = bc().number();
	}

	// Handle reverted blocks
	// There are not so many, so let's iterate over them
	h256s blocks;
	h256 ancestor;
	unsigned ancestorIndex;
	tie(blocks, ancestor, ancestorIndex) = bc().treeRoute(_f.earliest(), _f.latest(), false);

	for (size_t i = 0; i < ancestorIndex; i++)
		prependLogsFromBlock(_f, blocks[i], BlockPolarity::Dead, ret);

	// cause end is our earliest block, let's compare it with our ancestor
	// if ancestor is smaller let's move our end to it
	// example:
	//
	// 3b -> 2b -> 1b
	//                -> g
	// 3a -> 2a -> 1a
	//
	// if earliest is at 2a and latest is a 3b, coverting them to numbers
	// will give us pair (2, 3)
	// and we want to get all logs from 1 (ancestor + 1) to 3
	// so we have to move 2a to g + 1
	end = min(end, (unsigned)numberFromHash(ancestor) + 1);

	// Handle blocks from main chain
	set<unsigned> matchingBlocks;
	if (!_f.isRangeFilter())
		for (auto const& i: _f.bloomPossibilities())
			for (auto u: bc().withBlockBloom(i, end, begin))
				matchingBlocks.insert(u);
	else
		// if it is a range filter, we want to get all logs from all blocks in given range
		for (unsigned i = end; i <= begin; i++)
			matchingBlocks.insert(i);

	for (auto n: matchingBlocks)
		prependLogsFromBlock(_f, bc().numberHash(n), BlockPolarity::Live, ret);

	reverse(ret.begin(), ret.end());
	return ret;
}

void ClientBase::prependLogsFromBlock(LogFilter const& _f, h256 const& _blockHash, BlockPolarity _polarity, LocalisedLogEntries& io_logs) const
{
	auto receipts = bc().receipts(_blockHash).receipts;
	for (size_t i = 0; i < receipts.size(); i++)
	{
		TransactionReceipt receipt = receipts[i];
		auto th = transaction(_blockHash, i).sha3();
		LogEntries le = _f.matches(receipt);
		for (unsigned j = 0; j < le.size(); ++j)
			io_logs.insert(io_logs.begin(), LocalisedLogEntry(le[j], _blockHash, (BlockNumber)bc().number(_blockHash), th, i, 0, _polarity));
	}
}

unsigned ClientBase::installWatch(LogFilter const& _f, Reaping _r)
{
	h256 h = _f.sha3();
	{
		Guard l(x_filtersWatches);
		if (!m_filters.count(h))
		{
			cwatch << "FFF" << _f << h;
			m_filters.insert(make_pair(h, _f));
		}
	}
	return installWatch(h, _r);
}

unsigned ClientBase::installWatch(h256 _h, Reaping _r)
{
	unsigned ret;
	{
		Guard l(x_filtersWatches);
		ret = m_watches.size() ? m_watches.rbegin()->first + 1 : 0;
		m_watches[ret] = ClientWatch(_h, _r);
		cwatch << "+++" << ret << _h;
	}
#if INITIAL_STATE_AS_CHANGES
	auto ch = logs(ret);
	if (ch.empty())
		ch.push_back(InitialChange);
	{
		Guard l(x_filtersWatches);
		swap(m_watches[ret].changes, ch);
	}
#endif
	return ret;
}

bool ClientBase::uninstallWatch(unsigned _i)
{
	cwatch << "XXX" << _i;
	
	Guard l(x_filtersWatches);
	
	auto it = m_watches.find(_i);
	if (it == m_watches.end())
		return false;
	auto id = it->second.id;
	m_watches.erase(it);
	
	auto fit = m_filters.find(id);
	if (fit != m_filters.end())
		if (!--fit->second.refCount)
		{
			cwatch << "*X*" << fit->first << ":" << fit->second.filter;
			m_filters.erase(fit);
		}
	return true;
}

LocalisedLogEntries ClientBase::peekWatch(unsigned _watchId) const
{
	Guard l(x_filtersWatches);
	
//	cwatch << "peekWatch" << _watchId;
	auto& w = m_watches.at(_watchId);
//	cwatch << "lastPoll updated to " << chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count();
	if (w.lastPoll != chrono::system_clock::time_point::max())
		w.lastPoll = chrono::system_clock::now();
	return w.changes;
}

LocalisedLogEntries ClientBase::checkWatch(unsigned _watchId)
{
	Guard l(x_filtersWatches);
	LocalisedLogEntries ret;
	
//	cwatch << "checkWatch" << _watchId;
	auto& w = m_watches.at(_watchId);
//	cwatch << "lastPoll updated to " << chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count();
	std::swap(ret, w.changes);
	if (w.lastPoll != chrono::system_clock::time_point::max())
		w.lastPoll = chrono::system_clock::now();

	return ret;
}

BlockHeader ClientBase::blockInfo(h256 _hash) const
{
	if (_hash == PendingBlockHash)
		return preSeal().info();
	return BlockHeader(bc().block(_hash));
}

BlockDetails ClientBase::blockDetails(h256 _hash) const
{
	return bc().details(_hash);
}

Transaction ClientBase::transaction(h256 _transactionHash) const
{
	return Transaction(bc().transaction(_transactionHash), CheckTransaction::Cheap);
}

LocalisedTransaction ClientBase::localisedTransaction(h256 const& _transactionHash) const
{
	std::pair<h256, unsigned> tl = bc().transactionLocation(_transactionHash);
	return localisedTransaction(tl.first, tl.second);
}

Transaction ClientBase::transaction(h256 _blockHash, unsigned _i) const
{
	auto bl = bc().block(_blockHash);
	RLP b(bl);
	if (_i < b[1].itemCount())
		return Transaction(b[1][_i].data(), CheckTransaction::Cheap);
	else
		return Transaction();
}

LocalisedTransaction ClientBase::localisedTransaction(h256 const& _blockHash, unsigned _i) const
{
	Transaction t = Transaction(bc().transaction(_blockHash, _i), CheckTransaction::Cheap);
	return LocalisedTransaction(t, _blockHash, _i, numberFromHash(_blockHash));
}

TransactionReceipt ClientBase::transactionReceipt(h256 const& _transactionHash) const
{
	return bc().transactionReceipt(_transactionHash);
}

LocalisedTransactionReceipt ClientBase::localisedTransactionReceipt(h256 const& _transactionHash) const
{
	std::pair<h256, unsigned> tl = bc().transactionLocation(_transactionHash);
	Transaction t = Transaction(bc().transaction(tl.first, tl.second), CheckTransaction::Cheap);
	TransactionReceipt tr = bc().transactionReceipt(tl.first, tl.second);
	return LocalisedTransactionReceipt(
		tr,
		t.sha3(),
		tl.first,
		numberFromHash(tl.first),
		tl.second,
		toAddress(t.from(), t.nonce()));
}

pair<h256, unsigned> ClientBase::transactionLocation(h256 const& _transactionHash) const
{
	return bc().transactionLocation(_transactionHash);
}

Transactions ClientBase::transactions(h256 _blockHash) const
{
	auto bl = bc().block(_blockHash);
	RLP b(bl);
	Transactions res;
	for (unsigned i = 0; i < b[1].itemCount(); i++)
		res.emplace_back(b[1][i].data(), CheckTransaction::Cheap);
	return res;
}

TransactionHashes ClientBase::transactionHashes(h256 _blockHash) const
{
	return bc().transactionHashes(_blockHash);
}

BlockHeader ClientBase::uncle(h256 _blockHash, unsigned _i) const
{
	auto bl = bc().block(_blockHash);
	RLP b(bl);
	if (_i < b[2].itemCount())
		return BlockHeader(b[2][_i].data(), HeaderData);
	else
		return BlockHeader();
}

UncleHashes ClientBase::uncleHashes(h256 _blockHash) const
{
	return bc().uncleHashes(_blockHash);
}

unsigned ClientBase::transactionCount(h256 _blockHash) const
{
	auto bl = bc().block(_blockHash);
	RLP b(bl);
	return b[1].itemCount();
}

unsigned ClientBase::uncleCount(h256 _blockHash) const
{
	auto bl = bc().block(_blockHash);
	RLP b(bl);
	return b[2].itemCount();
}

unsigned ClientBase::number() const
{
	return bc().number();
}

Transactions ClientBase::pending() const
{
	return postSeal().pending();
}

h256s ClientBase::pendingHashes() const
{
	return h256s() + postSeal().pendingHashes();
}

BlockHeader ClientBase::pendingInfo() const
{
	return postSeal().info();
}

BlockDetails ClientBase::pendingDetails() const
{
	auto pm = postSeal().info();
	auto li = Interface::blockDetails(LatestBlock);
	return BlockDetails((unsigned)pm.number(), li.totalDifficulty + pm.difficulty(), pm.parentHash(), h256s{});
}

Addresses ClientBase::addresses(BlockNumber _block) const
{
	Addresses ret;
	for (auto const& i: block(_block).addresses())
		ret.push_back(i.first);
	return ret;
}

u256 ClientBase::gasLimitRemaining() const
{
	return postSeal().gasLimitRemaining();
}

Address ClientBase::author() const
{
	return preSeal().author();
}

h256 ClientBase::hashFromNumber(BlockNumber _number) const
{
	if (_number == PendingBlock)
		return h256();
	if (_number == LatestBlock)
		return bc().currentHash();
	return bc().numberHash(_number);
}

BlockNumber ClientBase::numberFromHash(h256 _blockHash) const
{
	if (_blockHash == PendingBlockHash)
		return bc().number() + 1;
	else if (_blockHash == LatestBlockHash)
		return bc().number();
	else if (_blockHash == EarliestBlockHash)
		return 0;
	return bc().number(_blockHash);
}

int ClientBase::compareBlockHashes(h256 _h1, h256 _h2) const
{
	BlockNumber n1 = numberFromHash(_h1);
	BlockNumber n2 = numberFromHash(_h2);

	if (n1 > n2)
		return 1;
	else if (n1 == n2)
		return 0;
	return -1;
}

bool ClientBase::isKnown(h256 const& _hash) const
{
	return _hash == PendingBlockHash ||
		_hash == LatestBlockHash ||
		_hash == EarliestBlockHash ||
		bc().isKnown(_hash);
}

bool ClientBase::isKnown(BlockNumber _block) const
{
	return _block == PendingBlock ||
		_block == LatestBlock ||
		bc().numberHash(_block) != h256();
}

bool ClientBase::isKnownTransaction(h256 const& _transactionHash) const
{
	return bc().isKnownTransaction(_transactionHash);
}

bool ClientBase::isKnownTransaction(h256 const& _blockHash, unsigned _i) const
{
	return isKnown(_blockHash) && block(_blockHash).pending().size() > _i;
}

Block ClientBase::block(BlockNumber _h) const
{
	if (_h == PendingBlock)
		return postSeal();
	else if (_h == LatestBlock)
		return preSeal();
	return block(bc().numberHash(_h));
}
