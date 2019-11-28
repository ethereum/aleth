// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2013-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "State.h"

#include "Block.h"
#include "BlockChain.h"
#include "ExtVM.h"
#include "TransactionQueue.h"
#include "DatabasePaths.h"
#include <libdevcore/Assertions.h>
#include <libdevcore/DBFactory.h>
#include <libdevcore/TrieHash.h>
#include <libevm/VMFactory.h>
#include <boost/filesystem.hpp>

using namespace std;
using namespace dev;
using namespace dev::eth;
namespace fs = boost::filesystem;

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
    m_unrevertablyTouched(_s.m_unrevertablyTouched),
    m_accountStartNonce(_s.m_accountStartNonce)
{}

OverlayDB State::openDB(fs::path const& _basePath, h256 const& _genesisHash, WithExisting _we)
{
    DatabasePaths const dbPaths{_basePath, _genesisHash};
    if (db::isDiskDatabase())
    {
        if (_we == WithExisting::Kill)
        {
            clog(VerbosityInfo, "statedb") << "Deleting state database: " << dbPaths.statePath();
            fs::remove_all(dbPaths.statePath());
        }

        clog(VerbosityDebug, "statedb")
            << "Verifying path exists (and creating if not present): " << dbPaths.chainPath();
        fs::create_directories(dbPaths.chainPath());
        clog(VerbosityDebug, "statedb")
            << "Ensuring permissions are set for path: " << dbPaths.chainPath();
        DEV_IGNORE_EXCEPTIONS(fs::permissions(dbPaths.chainPath(), fs::owner_all));
    }

    try
    {
        clog(VerbosityTrace, "statedb") << "Opening state database";
        std::unique_ptr<db::DatabaseFace> db = db::DBFactory::create(dbPaths.statePath());
        return OverlayDB(std::move(db));
    }
    catch (boost::exception const& ex)
    {
        if (db::isDiskDatabase())
        {
            clog(VerbosityError, "statedb")
                << "Error opening state database: " << dbPaths.statePath();
            db::DatabaseStatus const dbStatus =
                *boost::get_error_info<db::errinfo_dbStatusCode>(ex);
            if (fs::space(dbPaths.statePath()).available < 1024)
            {
                clog(VerbosityError, "statedb")
                    << "Not enough available space found on hard drive. Please free some up and "
                       "then re-run. Bailing.";
                BOOST_THROW_EXCEPTION(NotEnoughAvailableSpace());
            }
            else if (dbStatus == db::DatabaseStatus::Corruption)
            {
                clog(VerbosityError, "statedb")
                    << "Database corruption detected. Please see the exception for corruption "
                       "details. Exception: "
                    << boost::diagnostic_information(ex);
                BOOST_THROW_EXCEPTION(DatabaseCorruption());
            }
            else if (dbStatus == db::DatabaseStatus::IOError)
            {
                clog(VerbosityError, "statedb") << "Database already open. You appear to have "
                                                   "another instance of Aleth running.";
                BOOST_THROW_EXCEPTION(DatabaseAlreadyOpen());
            }
        }
        clog(VerbosityError, "statedb")
            << "Unknown error encountered when opening state database. Exception details: "
            << boost::diagnostic_information(ex);
        throw;
    }
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

    for (auto const& _address : m_unrevertablyTouched)
    {
        Account* a = account(_address);
        if (a && a->isEmpty())
            a->kill();
    }
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
    m_unrevertablyTouched = _s.m_unrevertablyTouched;
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
    auto const nonce = state[0].toInt<u256>();
    auto const balance = state[1].toInt<u256>();
    auto const storageRoot = state[2].toHash<h256>();
    auto const codeHash = state[3].toHash<h256>();
    // version is 0 if absent from RLP
    auto const version = state[4] ? state[4].toInt<u256>() : 0;

    auto i = m_cache.emplace(piecewise_construct, forward_as_tuple(_addr),
        forward_as_tuple(nonce, balance, storageRoot, codeHash, version, Account::Unchanged));
    m_unchangedCacheEntries.push_back(_addr);
    return &i.first->second;
}

void State::clearCacheIfTooLarge() const
{
    // TODO: Find a good magic number
    while (m_unchangedCacheEntries.size() > 1000)
    {
        // Remove a random element
        // FIXME: Do not use random device as the engine. The random device should be only used to seed other engine.
        size_t const randomIndex = std::uniform_int_distribution<size_t>(0, m_unchangedCacheEntries.size() - 1)(dev::s_fixedHashEngine);

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
    BOOST_THROW_EXCEPTION(InterfaceNotSupported() << errinfo_interface("State::addresses()"));
#endif
}

std::pair<State::AddressMap, h256> State::addresses(
    h256 const& _beginHash, size_t _maxResults) const
{
    AddressMap addresses;
    h256 nextKey;

#if ETH_FATDB
    for (auto it = m_state.hashedLowerBound(_beginHash); it != m_state.hashedEnd(); ++it)
    {
        auto const address = Address(it.key());
        auto const itCachedAddress = m_cache.find(address);

        // skip if deleted in cache
        if (itCachedAddress != m_cache.end() && itCachedAddress->second.isDirty() &&
            !itCachedAddress->second.isAlive())
            continue;

        // break when _maxResults fetched
        if (addresses.size() == _maxResults)
        {
            nextKey = h256((*it).first);
            break;
        }

        h256 const hashedAddress((*it).first);
        addresses[hashedAddress] = address;
    }
#endif

    // get addresses from cache with hash >= _beginHash (both new and old touched, we can't
    // distinguish them) and order by hash
    AddressMap cacheAddresses;
    for (auto const& addressAndAccount : m_cache)
    {
        auto const& address = addressAndAccount.first;
        auto const addressHash = sha3(address);
        auto const& account = addressAndAccount.second;
        if (account.isDirty() && account.isAlive() && addressHash >= _beginHash)
            cacheAddresses.emplace(addressHash, address);
    }

    // merge addresses from DB and addresses from cache
    addresses.insert(cacheAddresses.begin(), cacheAddresses.end());

    // if some new accounts were created in cache we need to return fewer results
    if (addresses.size() > _maxResults)
    {
        auto itEnd = std::next(addresses.begin(), _maxResults);
        nextKey = itEnd->first;
        addresses.erase(itEnd, addresses.end());
    }

    return {addresses, nextKey};
}


void State::setRoot(h256 const& _r)
{
    m_cache.clear();
    m_unchangedCacheEntries.clear();
    m_nonExistingAccountsCache.clear();
//  m_touched.clear();
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
        auto oldNonce = a->nonce();
        a->incNonce();
        m_changeLog.emplace_back(_addr, oldNonce);
    }
    else
        // This is possible if a transaction has gas price 0.
        createAccount(_addr, Account(requireAccountStartNonce() + 1, 0));
}

void State::setNonce(Address const& _addr, u256 const& _newNonce)
{
    if (Account* a = account(_addr))
    {
        auto oldNonce = a->nonce();
        a->setNonce(_newNonce);
        m_changeLog.emplace_back(_addr, oldNonce);
    }
    else
        // This is possible when a contract is being created.
        createAccount(_addr, Account(_newNonce, 0));
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

void State::setBalance(Address const& _addr, u256 const& _value)
{
    Account* a = account(_addr);
    u256 original = a ? a->balance() : 0;

    // Fall back to addBalance().
    addBalance(_addr, _value - original);
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
        return a->storageValue(_key, m_db);
    else
        return 0;
}

void State::setStorage(Address const& _contract, u256 const& _key, u256 const& _value)
{
    m_changeLog.emplace_back(_contract, _key, storage(_contract, _key));
    m_cache[_contract].setStorage(_key, _value);
}

u256 State::originalStorageValue(Address const& _contract, u256 const& _key) const
{
    if (Account const* a = account(_contract))
        return a->originalStorageValue(_key, m_db);
    else
        return 0;
}

void State::clearStorage(Address const& _contract)
{
    h256 const& oldHash{m_cache[_contract].baseRoot()};
    if (oldHash == EmptyTrie)
        return;
    m_changeLog.emplace_back(Change::StorageRoot, _contract, oldHash);
    m_cache[_contract].clearStorage();
}

map<h256, pair<u256, u256>> State::storage(Address const& _id) const
{
#if ETH_FATDB
    map<h256, pair<u256, u256>> ret;

    if (Account const* a = account(_id))
    {
        // Pull out all values from trie storage.
        if (h256 root = a->baseRoot())
        {
            SecureTrieDB<h256, OverlayDB> memdb(const_cast<OverlayDB*>(&m_db), root);       // promise we won't alter the overlay! :)

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
#else
    (void) _id;
    BOOST_THROW_EXCEPTION(InterfaceNotSupported() << errinfo_interface("State::storage(Address const& _id)"));
#endif
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

void State::setCode(Address const& _address, bytes&& _code, u256 const& _version)
{
    // rollback assumes that overwriting of the code never happens
    // (not allowed in contract creation logic in Executive)
    assert(!addressHasCode(_address));
    m_changeLog.emplace_back(Change::Code, _address);
    m_cache[_address].setCode(move(_code), _version);
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

u256 State::version(Address const& _a) const
{
    Account const* a = account(_a);
    return a ? a->version() : 0;
}

void State::unrevertableTouch(Address const& _address)
{
    m_unrevertablyTouched.insert(_address);
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
        case Change::StorageRoot:
            account.setStorageRoot(change.value);
            break;
        case Change::Balance:
            account.addBalance(0 - change.value);
            break;
        case Change::Nonce:
            account.setNonce(change.value);
            break;
        case Change::Create:
            m_cache.erase(change.address);
            break;
        case Change::Code:
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
    // Create and initialize the executive. This will throw fairly cheaply and quickly if the
    // transaction is bad in any way.
    Executive e(*this, _envInfo, _sealEngine);
    ExecutionResult res;
    e.setResultRecipient(res);

    auto onOp = _onOp;
    if (isVmTraceEnabled() && !onOp)
        onOp = e.simpleTrace();

    u256 const startGasUsed = _envInfo.gasUsed();
    bool const statusCode = executeTransaction(e, _t, onOp);

    bool removeEmptyAccounts = false;
    switch (_p)
    {
        case Permanence::Reverted:
            m_cache.clear();
            break;
        case Permanence::Committed:
            removeEmptyAccounts = _envInfo.number() >= _sealEngine.chainParams().EIP158ForkBlock;
            commit(removeEmptyAccounts ? State::CommitBehaviour::RemoveEmptyAccounts : State::CommitBehaviour::KeepEmptyAccounts);
            break;
        case Permanence::Uncommitted:
            break;
    }

    TransactionReceipt const receipt = _envInfo.number() >= _sealEngine.chainParams().byzantiumForkBlock ?
        TransactionReceipt(statusCode, startGasUsed + e.gasUsed(), e.logs()) :
        TransactionReceipt(rootHash(), startGasUsed + e.gasUsed(), e.logs());
    return make_pair(res, receipt);
}

void State::executeBlockTransactions(Block const& _block, unsigned _txCount,
    LastBlockHashesFace const& _lastHashes, SealEngineFace const& _sealEngine)
{
    u256 gasUsed = 0;
    for (unsigned i = 0; i < _txCount; ++i)
    {
        EnvInfo envInfo(_block.info(), _lastHashes, gasUsed, _sealEngine.chainParams().chainID);

        Executive e(*this, envInfo, _sealEngine);
        executeTransaction(e, _block.pending()[i], OnOpFunc());

        gasUsed += e.gasUsed();
    }
}

/// @returns true when normally halted; false when exceptionally halted; throws when internal VM
/// exception occurred.
bool State::executeTransaction(Executive& _e, Transaction const& _t, OnOpFunc const& _onOp)
{
    size_t const savept = savepoint();
    try
    {
        _e.initialize(_t);

        if (!_e.execute())
            _e.go(_onOp);
        return _e.finalize();
    }
    catch (Exception const&)
    {
        rollback(savept);
        throw;
    }
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
                    SecureTrieDB<h256, OverlayDB> memdb(const_cast<OverlayDB*>(&_s.m_db), r[2].toHash<h256>());     // promise we won't alter the overlay! :)
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

State& dev::eth::createIntermediateState(State& o_s, Block const& _block, unsigned _txIndex, BlockChain const& _bc)
{
    o_s = _block.state();
    u256 const rootHash = _block.stateRootBeforeTx(_txIndex);
    if (rootHash)
        o_s.setRoot(rootHash);
    else
    {
        o_s.setRoot(_block.stateRootBeforeTx(0));
        o_s.executeBlockTransactions(_block, _txIndex, _bc.lastBlockHashes(), *_bc.sealEngine());
    }
    return o_s;
}

template <class DB>
AddressHash dev::eth::commit(AccountMap const& _cache, SecureTrieDB<Address, DB>& _state)
{
    AddressHash ret;
    for (auto const& i: _cache)
        if (i.second.isDirty())
        {
            if (!i.second.isAlive())
                _state.remove(i.first);
            else
            {
                auto const version = i.second.version();

                // version = 0: [nonce, balance, storageRoot, codeHash]
                // version > 0: [nonce, balance, storageRoot, codeHash, version]
                RLPStream s(version != 0 ? 5 : 4);
                s << i.second.nonce() << i.second.balance();

                if (i.second.storageOverlay().empty())
                {
                    assert(i.second.baseRoot());
                    s.append(i.second.baseRoot());
                }
                else
                {
                    SecureTrieDB<h256, DB> storageDB(_state.db(), i.second.baseRoot());
                    for (auto const& j: i.second.storageOverlay())
                        if (j.second)
                            storageDB.insert(j.first, rlp(j.second));
                        else
                            storageDB.remove(j.first);
                    assert(storageDB.root());
                    s.append(storageDB.root());
                }

                if (i.second.hasNewCode())
                {
                    h256 ch = i.second.codeHash();
                    // Store the size of the code
                    CodeSizeCache::instance().store(ch, i.second.code().size());
                    _state.db()->insert(ch, &i.second.code());
                    s << ch;
                }
                else
                    s << i.second.codeHash();

                if (version != 0)
                    s << i.second.version();

                _state.insert(i.first, &s.out());
            }
            ret.insert(i.first);
        }
    return ret;
}


template AddressHash dev::eth::commit<OverlayDB>(AccountMap const& _cache, SecureTrieDB<Address, OverlayDB>& _state);
template AddressHash dev::eth::commit<StateCacheDB>(AccountMap const& _cache, SecureTrieDB<Address, StateCacheDB>& _state);
