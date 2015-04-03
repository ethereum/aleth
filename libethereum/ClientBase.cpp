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

#include <libdevcore/StructuredLogger.h>
#include "ClientBase.h"
#include "BlockChain.h"
#include "Executive.h"

using namespace std;
using namespace dev;
using namespace dev::eth;

State ClientBase::asOf(BlockNumber _h) const
{
	if (_h == PendingBlock)
		return postMine();
	else if (_h == LatestBlock)
		return preMine();
	return asOf(bc().numberHash(_h));
}

void ClientBase::submitTransaction(Secret _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice)
{
	prepareForTransaction();
	
	u256 n = postMine().transactionsFrom(toAddress(_secret));
	Transaction t(_value, _gasPrice, _gas, _dest, _data, n, _secret);
	m_tq.attemptImport(t.rlp());
	
	StructuredLogger::transactionReceived(t.sha3().abridged(), t.sender().abridged());
	cnote << "New transaction " << t;
}

Address ClientBase::submitTransaction(Secret _secret, u256 _endowment, bytes const& _init, u256 _gas, u256 _gasPrice)
{
	prepareForTransaction();
	
	u256 n = postMine().transactionsFrom(toAddress(_secret));
	Transaction t(_endowment, _gasPrice, _gas, _init, n, _secret);
	m_tq.attemptImport(t.rlp());

	StructuredLogger::transactionReceived(t.sha3().abridged(), t.sender().abridged());
	cnote << "New transaction " << t;
	
	return right160(sha3(rlpList(t.sender(), t.nonce())));
}

// TODO: remove try/catch, allow exceptions
ExecutionResult ClientBase::call(Secret _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice, BlockNumber _blockNumber)
{
	ExecutionResult ret;
	try
	{
		State temp = asOf(_blockNumber);
		u256 n = temp.transactionsFrom(toAddress(_secret));
		Transaction t(_value, _gasPrice, _gas, _dest, _data, n, _secret);
		ret = temp.execute(bc().lastHashes(), t, Permanence::Reverted);
	}
	catch (...)
	{
		// TODO: Some sort of notification of failure.
	}
	return ret;
}

ExecutionResult ClientBase::create(Secret _secret, u256 _value, bytes const& _data, u256 _gas, u256 _gasPrice, BlockNumber _blockNumber)
{
	ExecutionResult ret;
	try
	{
		State temp = asOf(_blockNumber);
		u256 n = temp.transactionsFrom(toAddress(_secret));
		//	cdebug << "Nonce at " << toAddress(_secret) << " pre:" << m_preMine.transactionsFrom(toAddress(_secret)) << " post:" << m_postMine.transactionsFrom(toAddress(_secret));
		
		Transaction t(_value, _gasPrice, _gas, _data, n, _secret);
		ret = temp.execute(bc().lastHashes(), t, Permanence::Reverted);
	}
	catch (...)
	{
		// TODO: Some sort of notification of failure.
	}
	return ret;
}

u256 ClientBase::balanceAt(Address _a, BlockNumber _block) const
{
	return asOf(_block).balance(_a);
}

u256 ClientBase::countAt(Address _a, BlockNumber _block) const
{
	return asOf(_block).transactionsFrom(_a);
}

u256 ClientBase::stateAt(Address _a, u256 _l, BlockNumber _block) const
{
	return asOf(_block).storage(_a, _l);
}

bytes ClientBase::codeAt(Address _a, BlockNumber _block) const
{
	return asOf(_block).code(_a);
}

map<u256, u256> ClientBase::storageAt(Address _a, BlockNumber _block) const
{
	return asOf(_block).storage(_a);
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
	unsigned begin = min<unsigned>(bc().number() + 1, (unsigned)_f.latest());
	unsigned end = min(bc().number(), min(begin, (unsigned)_f.earliest()));
	
	// Handle pending transactions differently as they're not on the block chain.
	if (begin > bc().number())
	{
		State temp = postMine();
		for (unsigned i = 0; i < temp.pending().size(); ++i)
		{
			// Might have a transaction that contains a matching log.
			TransactionReceipt const& tr = temp.receipt(i);
			auto th = temp.pending()[i].sha3();
			LogEntries le = _f.matches(tr);
			if (le.size())
				for (unsigned j = 0; j < le.size(); ++j)
					ret.insert(ret.begin(), LocalisedLogEntry(le[j], begin, th));
		}
		begin = bc().number();
	}
	
	set<unsigned> matchingBlocks;
	for (auto const& i: _f.bloomPossibilities())
		for (auto u: bc().withBlockBloom(i, end, begin))
			matchingBlocks.insert(u);

	unsigned falsePos = 0;
	for (auto n: matchingBlocks)
	{
		int total = 0;
		auto h = bc().numberHash(n);
		auto receipts = bc().receipts(h).receipts;
		for (size_t i = 0; i < receipts.size(); i++)
		{
			TransactionReceipt receipt = receipts[i];
			if (_f.matches(receipt.bloom()))
			{
				auto info = bc().info(h);
				auto th = transaction(info.hash, i).sha3();
				LogEntries le = _f.matches(receipt);
				if (le.size())
				{
					total += le.size();
					for (unsigned j = 0; j < le.size(); ++j)
						ret.insert(ret.begin(), LocalisedLogEntry(le[j], n, th));
				}
			}
			
			if (!total)
				falsePos++;
		}
	}

	cdebug << matchingBlocks.size() << "searched from" << (end - begin) << "skipped; " << falsePos << "false +ves";
	return ret;
}

unsigned ClientBase::installWatch(LogFilter const& _f, Reaping _r)
{
	h256 h = _f.sha3();
	{
		Guard l(x_filtersWatches);
		if (!m_filters.count(h))
		{
			cwatch << "FFF" << _f << h.abridged();
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
		cwatch << "+++" << ret << _h.abridged();
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
	w.lastPoll = chrono::system_clock::now();
	
	return ret;
}

h256 ClientBase::hashFromNumber(unsigned _number) const
{
	return bc().numberHash(_number);
}

BlockInfo ClientBase::blockInfo(h256 _hash) const
{
	return BlockInfo(bc().block(_hash));
}

BlockDetails ClientBase::blockDetails(h256 _hash) const
{
	return bc().details(_hash);
}

Transaction ClientBase::transaction(h256 _transactionHash) const
{
	return Transaction(bc().transaction(_transactionHash), CheckSignature::Range);
}

Transaction ClientBase::transaction(h256 _blockHash, unsigned _i) const
{
	auto bl = bc().block(_blockHash);
	RLP b(bl);
	if (_i < b[1].itemCount())
		return Transaction(b[1][_i].data(), CheckSignature::Range);
	else
		return Transaction();
}

Transactions ClientBase::transactions(h256 _blockHash) const
{
	auto bl = bc().block(_blockHash);
	RLP b(bl);
	Transactions res;
	for (unsigned i = 0; i < b[1].itemCount(); i++)
		res.emplace_back(b[1][i].data(), CheckSignature::Range);
	return res;
}

TransactionHashes ClientBase::transactionHashes(h256 _blockHash) const
{
	return bc().transactionHashes(_blockHash);
}

BlockInfo ClientBase::uncle(h256 _blockHash, unsigned _i) const
{
	auto bl = bc().block(_blockHash);
	RLP b(bl);
	if (_i < b[2].itemCount())
		return BlockInfo::fromHeader(b[2][_i].data());
	else
		return BlockInfo();
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
	return postMine().pending();
}


StateDiff ClientBase::diff(unsigned _txi, h256 _block) const
{
	State st = asOf(_block);
	return st.fromPending(_txi).diff(st.fromPending(_txi + 1));
}

StateDiff ClientBase::diff(unsigned _txi, BlockNumber _block) const
{
	State st = asOf(_block);
	return st.fromPending(_txi).diff(st.fromPending(_txi + 1));
}

Addresses ClientBase::addresses(BlockNumber _block) const
{
	Addresses ret;
	for (auto const& i: asOf(_block).addresses())
		ret.push_back(i.first);
	return ret;
}

u256 ClientBase::gasLimitRemaining() const
{
	return postMine().gasLimitRemaining();
}

Address ClientBase::address() const
{
	return preMine().address();
}
