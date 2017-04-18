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
/** @file State.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "State.h"

#include <ctime>
#include <boost/filesystem.hpp>
#include <boost/timer.hpp>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Assertions.h>
#include <libdevcore/TrieHash.h>
#include <libevmcore/Instruction.h>
#include <libethcore/Exceptions.h>
#include <libevm/VMFactory.h>
#include "BlockChain.h"
#include "CodeSizeCache.h"
#include "Defaults.h"
#include "ExtVM.h"
#include "Executive.h"
#include "BlockChain.h"
#include "TransactionQueue.h"

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace dev::eth::detail;
namespace fs = boost::filesystem;

const char* StateSafeExceptions::name() { return EthViolet "⚙" EthBlue " ℹ"; }
const char* StateDetail::name() { return EthViolet "⚙" EthWhite " ◌"; }
const char* StateTrace::name() { return EthViolet "⚙" EthGray " ◎"; }
const char* StateChat::name() { return EthViolet "⚙" EthWhite " ◌"; }

State::State(u256 const& _accountStartNonce, OverlayDB const& _db, BaseState _bs):
	m_db(_db),
	m_state(&m_db),
	m_accountStartNonce(_accountStartNonce)
{
	if (_bs != BaseState::PreExisting)
		// Initialise to the state entailed by the genesis block; this guarantees the trie is built correctly.
		m_state.init();
}

State::State(State const& _s):
	m_db(_s.m_db),
	m_state(&m_db, _s.m_state.root(), Verification::Skip),
	m_cache(_s.m_cache),
	m_unchangedCacheEntries(_s.m_unchangedCacheEntries),
	m_nonExistingAccountsCache(_s.m_nonExistingAccountsCache),
	m_touched(_s.m_touched),
	m_accountStartNonce(_s.m_accountStartNonce)
{}

OverlayDB State::openDB(std::string const& _basePath, h256 const& _genesisHash, WithExisting _we)
{
	std::string path = _basePath.empty() ? Defaults::get()->m_dbPath : _basePath;

	if (_we == WithExisting::Kill)
	{
		cnote << "Killing state database (WithExisting::Kill).";
		boost::filesystem::remove_all(path + "/state");
	}

	path += "/" + toHex(_genesisHash.ref().cropped(0, 4)) + "/" + toString(c_databaseVersion);
	boost::filesystem::create_directories(path);
	DEV_IGNORE_EXCEPTIONS(fs::permissions(path, fs::owner_all));

	ldb::Options o;
	o.max_open_files = 256;
	o.create_if_missing = true;
	ldb::DB* db = nullptr;
	ldb::Status status = ldb::DB::Open(o, path + "/state", &db);
	if (!status.ok() || !db)
	{
		if (boost::filesystem::space(path + "/state").available < 1024)
		{
			cwarn << "Not enough available space found on hard drive. Please free some up and then re-run. Bailing.";
			BOOST_THROW_EXCEPTION(NotEnoughAvailableSpace());
		}
		else
		{
			cwarn << status.ToString();
			cwarn <<
				"Database " <<
				(path + "/state") <<
				"already open. You appear to have another instance of ethereum running. Bailing.";
			BOOST_THROW_EXCEPTION(DatabaseAlreadyOpen());
		}
	}

	ctrace << "Opened state DB.";
	return OverlayDB(db);
}

void State::populateFrom(AccountMap const& _map)
{
	eth::commit(_map, m_state);
	commit(State::CommitBehaviour::KeepEmptyAccounts);
}

u256 const& State::requireAccountStartNonce() const
{
	if (m_accountStartNonce == Invalid256)
		BOOST_THROW_EXCEPTION(InvalidAccountStartNonceInState());
	return m_accountStartNonce;
}

void State::noteAccountStartNonce(u256 const& _actual)
{
	if (m_accountStartNonce == Invalid256)
		m_accountStartNonce = _actual;
	else if (m_accountStartNonce != _actual)
		BOOST_THROW_EXCEPTION(IncorrectAccountStartNonceInState());
}

void State::removeEmptyAccounts()
{
	for (auto& i: m_cache)
		if (i.second.isDirty() && i.second.isEmpty())
			i.second.kill();
}

State& State::operator=(State const& _s)
{
	if (&_s == this)
		return *this;

	m_db = _s.m_db;
	m_state.open(&m_db, _s.m_state.root(), Verification::Skip);
	m_cache = _s.m_cache;
	m_unchangedCacheEntries = _s.m_unchangedCacheEntries;
	m_nonExistingAccountsCache = _s.m_nonExistingAccountsCache;
	m_touched = _s.m_touched;
	m_accountStartNonce = _s.m_accountStartNonce;
	return *this;
}

Account const* State::account(Address const& _a) const
{
	return const_cast<State*>(this)->account(_a);
}

Account* State::account(Address const& _addr)
{
	auto it = m_cache.find(_addr);
	if (it != m_cache.end())
		return &it->second;

	if (m_nonExistingAccountsCache.count(_addr))
		return nullptr;

	// Populate basic info.
	string stateBack = m_state.at(_addr);
	if (stateBack.empty())
	{
		m_nonExistingAccountsCache.insert(_addr);
		return nullptr;
	}

	clearCacheIfTooLarge();

	RLP state(stateBack);
	auto i = m_cache.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(_addr),
		std::forward_as_tuple(state[0].toInt<u256>(), state[1].toInt<u256>(), state[2].toHash<h256>(), state[3].toHash<h256>(), Account::Unchanged)
	);
	m_unchangedCacheEntries.push_back(_addr);
	return &i.first->second;
}

void State::clearCacheIfTooLarge() const
{
	// TODO: Find a good magic number
	while (m_unchangedCacheEntries.size() > 1000)
	{
		// Remove a random element
		size_t const randomIndex = boost::random::uniform_int_distribution<size_t>(0, m_unchangedCacheEntries.size() - 1)(dev::s_fixedHashEngine);

		Address const addr = m_unchangedCacheEntries[randomIndex];
		swap(m_unchangedCacheEntries[randomIndex], m_unchangedCacheEntries.back());
		m_unchangedCacheEntries.pop_back();

		auto cacheEntry = m_cache.find(addr);
		if (cacheEntry != m_cache.end() && !cacheEntry->second.isDirty())
			m_cache.erase(cacheEntry);
	}
}

void State::commit(CommitBehaviour _commitBehaviour)
{
	if (_commitBehaviour == CommitBehaviour::RemoveEmptyAccounts)
		removeEmptyAccounts();
	m_touched += dev::eth::commit(m_cache, m_state);
	m_changeLog.clear();
	m_cache.clear();
	m_unchangedCacheEntries.clear();
}

unordered_map<Address, u256> State::addresses() const
{
#if ETH_FATDB
	unordered_map<Address, u256> ret;
	for (auto& i: m_cache)
		if (i.second.isAlive())
			ret[i.first] = i.second.balance();
	for (auto const& i: m_state)
		if (m_cache.find(i.first) == m_cache.end())
			ret[i.first] = RLP(i.second)[1].toInt<u256>();
	return ret;
#else
	BOOST_THROW_EXCEPTION(InterfaceNotSupported("State::addresses()"));
#endif
}

void State::setRoot(h256 const& _r)
{
	m_cache.clear();
	m_unchangedCacheEntries.clear();
	m_nonExistingAccountsCache.clear();
//	m_touched.clear();
	m_state.setRoot(_r);
}

bool State::addressInUse(Address const& _id) const
{
	return !!account(_id);
}

bool State::accountNonemptyAndExisting(Address const& _address) const
{
	if (Account const* a = account(_address))
		return !a->isEmpty();
	else
		return false;
}

bool State::addressHasCode(Address const& _id) const
{
	if (auto a = account(_id))
		return a->codeHash() != EmptySHA3;
	else
		return false;
}

u256 State::balance(Address const& _id) const
{
	if (auto a = account(_id))
		return a->balance();
	else
		return 0;
}

void State::incNonce(Address const& _addr)
{
	if (Account* a = account(_addr))
	{
		a->incNonce();
		m_changeLog.emplace_back(Change::Nonce, _addr);
	}
	else
		// This is possible if a transaction has gas price 0.
		createAccount(_addr, Account(requireAccountStartNonce() + 1, 0));
}

void State::addBalance(Address const& _id, u256 const& _amount)
{
	if (Account* a = account(_id))
	{
		// Log empty account being touched. Empty touched accounts are cleared
		// after the transaction, so this event must be also reverted.
		// We only log the first touch (not dirty yet), and only for empty
		// accounts, as other accounts does not matter.
		// TODO: to save space we can combine this event with Balance by having
		//       Balance and Balance+Touch events.
		if (!a->isDirty() && a->isEmpty())
			m_changeLog.emplace_back(Change::Touch, _id);

		// Increase the account balance. This also is done for value 0 to mark
		// the account as dirty. Dirty account are not removed from the cache
		// and are cleared if empty at the end of the transaction.
		a->addBalance(_amount);
	}
	else
		createAccount(_id, {requireAccountStartNonce(), _amount});

	if (_amount)
		m_changeLog.emplace_back(Change::Balance, _id, _amount);
}

void State::subBalance(Address const& _addr, u256 const& _value)
{
	if (_value == 0)
		return;

	Account* a = account(_addr);
	if (!a || a->balance() < _value)
		// TODO: I expect this never happens.
		BOOST_THROW_EXCEPTION(NotEnoughCash());

	// Fall back to addBalance().
	addBalance(_addr, 0 - _value);
}

void State::createContract(Address const& _address)
{
	createAccount(_address, {requireAccountStartNonce(), 0});
}

void State::createAccount(Address const& _address, Account const&& _account)
{
	assert(!addressInUse(_address) && "Account already exists");
	m_cache[_address] = std::move(_account);
	m_nonExistingAccountsCache.erase(_address);
	m_changeLog.emplace_back(Change::Create, _address);
}

void State::kill(Address _addr)
{
	if (auto a = account(_addr))
		a->kill();
	// If the account is not in the db, nothing to kill.
}

u256 State::getNonce(Address const& _addr) const
{
	if (auto a = account(_addr))
		return a->nonce();
	else
		return m_accountStartNonce;
}

u256 State::storage(Address const& _id, u256 const& _key) const
{
	if (Account const* a = account(_id))
	{
		auto mit = a->storageOverlay().find(_key);
		if (mit != a->storageOverlay().end())
			return mit->second;

		// Not in the storage cache - go to the DB.
		SecureTrieDB<h256, OverlayDB> memdb(const_cast<OverlayDB*>(&m_db), a->baseRoot());			// promise we won't change the overlay! :)
		string payload = memdb.at(_key);
		u256 ret = payload.size() ? RLP(payload).toInt<u256>() : 0;
		a->setStorageCache(_key, ret);
		return ret;
	}
	else
		return 0;
}

void State::setStorage(Address const& _contract, u256 const& _key, u256 const& _value)
{
	m_changeLog.emplace_back(_contract, _key, storage(_contract, _key));
	m_cache[_contract].setStorage(_key, _value);
}

map<h256, pair<u256, u256>> State::storage(Address const& _id) const
{
	map<h256, pair<u256, u256>> ret;

	if (Account const* a = account(_id))
	{
		// Pull out all values from trie storage.
		if (h256 root = a->baseRoot())
		{
			SecureTrieDB<h256, OverlayDB> memdb(const_cast<OverlayDB*>(&m_db), root);		// promise we won't alter the overlay! :)

			for (auto it = memdb.hashedBegin(); it != memdb.hashedEnd(); ++it)
			{
				h256 const hashedKey((*it).first);
				u256 const key = h256(it.key());
				u256 const value = RLP((*it).second).toInt<u256>();
				ret[hashedKey] = make_pair(key, value);
			}
		}

		// Then merge cached storage over the top.
		for (auto const& i : a->storageOverlay())
		{
			h256 const key = i.first;
			h256 const hashedKey = sha3(key);
			if (i.second)
				ret[hashedKey] = i;
			else
				ret.erase(hashedKey);
		}
	}
	return ret;
}

h256 State::storageRoot(Address const& _id) const
{
	string s = m_state.at(_id);
	if (s.size())
	{
		RLP r(s);
		return r[2].toHash<h256>();
	}
	return EmptyTrie;
}

bytes const& State::code(Address const& _addr) const
{
	Account const* a = account(_addr);
	if (!a || a->codeHash() == EmptySHA3)
		return NullBytes;

	if (a->code().empty())
	{
		// Load the code from the backend.
		Account* mutableAccount = const_cast<Account*>(a);
		mutableAccount->noteCode(m_db.lookup(a->codeHash()));
		CodeSizeCache::instance().store(a->codeHash(), a->code().size());
	}

	return a->code();
}

void State::setNewCode(Address const& _address, bytes&& _code)
{
	m_cache[_address].setNewCode(std::move(_code));
	m_changeLog.emplace_back(Change::NewCode, _address);
}

h256 State::codeHash(Address const& _a) const
{
	if (Account const* a = account(_a))
		return a->codeHash();
	else
		return EmptySHA3;
}

size_t State::codeSize(Address const& _a) const
{
	if (Account const* a = account(_a))
	{
		if (a->hasNewCode())
			return a->code().size();
		auto& codeSizeCache = CodeSizeCache::instance();
		h256 codeHash = a->codeHash();
		if (codeSizeCache.contains(codeHash))
			return codeSizeCache.get(codeHash);
		else
		{
			size_t size = code(_a).size();
			codeSizeCache.store(codeHash, size);
			return size;
		}
	}
	else
		return 0;
}

size_t State::savepoint() const
{
	return m_changeLog.size();
}

void State::rollback(size_t _savepoint)
{
	while (_savepoint != m_changeLog.size())
	{
		auto& change = m_changeLog.back();
		auto& account = m_cache[change.address];

		// Public State API cannot be used here because it will add another
		// change log entry.
		switch (change.kind)
		{
		case Change::Storage:
			account.setStorage(change.key, change.value);
			break;
		case Change::Balance:
			account.addBalance(0 - change.value);
			break;
		case Change::Nonce:
			account.setNonce(account.nonce() - 1);
			break;
		case Change::Create:
			m_cache.erase(change.address);
			break;
		case Change::NewCode:
			account.resetCode();
			break;
		case Change::Touch:
			account.untouch();
			m_unchangedCacheEntries.emplace_back(change.address);
			break;
		}
		m_changeLog.pop_back();
	}
}

std::pair<ExecutionResult, TransactionReceipt> State::execute(EnvInfo const& _envInfo, SealEngineFace const& _sealEngine, Transaction const& _t, Permanence _p, OnOpFunc const& _onOp)
{
	auto onOp = _onOp;
#if ETH_VMTRACE
	if (isChannelVisible<VMTraceChannel>())
		onOp = Executive::simpleTrace(); // override tracer
#endif

	// Create and initialize the executive. This will throw fairly cheaply and quickly if the
	// transaction is bad in any way.
	Executive e(*this, _envInfo, _sealEngine);
	ExecutionResult res;
	e.setResultRecipient(res);
	e.initialize(_t);

	// OK - transaction looks valid - execute.
	u256 startGasUsed = _envInfo.gasUsed();
	if (!e.execute())
		e.go(onOp);
	e.finalize();

	if (_p == Permanence::Reverted)
		m_cache.clear();
	else
	{
		bool removeEmptyAccounts = _envInfo.number() >= _sealEngine.chainParams().u256Param("EIP158ForkBlock");
		commit(removeEmptyAccounts ? State::CommitBehaviour::RemoveEmptyAccounts : State::CommitBehaviour::KeepEmptyAccounts);
	}

	return make_pair(res, TransactionReceipt(rootHash(), startGasUsed + e.gasUsed(), e.logs()));
}

std::ostream& dev::eth::operator<<(std::ostream& _out, State const& _s)
{
	_out << "--- " << _s.rootHash() << std::endl;
	std::set<Address> d;
	std::set<Address> dtr;
	auto trie = SecureTrieDB<Address, OverlayDB>(const_cast<OverlayDB*>(&_s.m_db), _s.rootHash());
	for (auto i: trie)
		d.insert(i.first), dtr.insert(i.first);
	for (auto i: _s.m_cache)
		d.insert(i.first);

	for (auto i: d)
	{
		auto it = _s.m_cache.find(i);
		Account* cache = it != _s.m_cache.end() ? &it->second : nullptr;
		string rlpString = dtr.count(i) ? trie.at(i) : "";
		RLP r(rlpString);
		assert(cache || r);

		if (cache && !cache->isAlive())
			_out << "XXX  " << i << std::endl;
		else
		{
			string lead = (cache ? r ? " *   " : " +   " : "     ");
			if (cache && r && cache->nonce() == r[0].toInt<u256>() && cache->balance() == r[1].toInt<u256>())
				lead = " .   ";

			stringstream contout;

			if ((cache && cache->codeHash() == EmptySHA3) || (!cache && r && (h256)r[3] != EmptySHA3))
			{
				std::map<u256, u256> mem;
				std::set<u256> back;
				std::set<u256> delta;
				std::set<u256> cached;
				if (r)
				{
					SecureTrieDB<h256, OverlayDB> memdb(const_cast<OverlayDB*>(&_s.m_db), r[2].toHash<h256>());		// promise we won't alter the overlay! :)
					for (auto const& j: memdb)
						mem[j.first] = RLP(j.second).toInt<u256>(), back.insert(j.first);
				}
				if (cache)
					for (auto const& j: cache->storageOverlay())
					{
						if ((!mem.count(j.first) && j.second) || (mem.count(j.first) && mem.at(j.first) != j.second))
							mem[j.first] = j.second, delta.insert(j.first);
						else if (j.second)
							cached.insert(j.first);
					}
				if (!delta.empty())
					lead = (lead == " .   ") ? "*.*  " : "***  ";

				contout << " @:";
				if (!delta.empty())
					contout << "???";
				else
					contout << r[2].toHash<h256>();
				if (cache && cache->hasNewCode())
					contout << " $" << toHex(cache->code());
				else
					contout << " $" << (cache ? cache->codeHash() : r[3].toHash<h256>());

				for (auto const& j: mem)
					if (j.second)
						contout << std::endl << (delta.count(j.first) ? back.count(j.first) ? " *     " : " +     " : cached.count(j.first) ? " .     " : "       ") << std::hex << nouppercase << std::setw(64) << j.first << ": " << std::setw(0) << j.second ;
					else
						contout << std::endl << "XXX    " << std::hex << nouppercase << std::setw(64) << j.first << "";
			}
			else
				contout << " [SIMPLE]";
			_out << lead << i << ": " << std::dec << (cache ? cache->nonce() : r[0].toInt<u256>()) << " #:" << (cache ? cache->balance() : r[1].toInt<u256>()) << contout.str() << std::endl;
		}
	}
	return _out;
}
