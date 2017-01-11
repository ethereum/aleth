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
/** @file State.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <array>
#include <unordered_map>
#include <libdevcore/Common.h>
#include <libdevcore/RLP.h>
#include <libdevcore/TrieDB.h>
#include <libdevcore/OverlayDB.h>
#include <libethcore/Exceptions.h>
#include <libethcore/BlockHeader.h>
#include <libethereum/CodeSizeCache.h>
#include <libethereum/GenericMiner.h>
#include <libevm/ExtVMFace.h>
#include "Account.h"
#include "Transaction.h"
#include "TransactionReceipt.h"
#include "GasPricer.h"

namespace dev
{

namespace test { class ImportTest; class StateLoader; }

namespace eth
{

// Import-specific errinfos
using errinfo_uncleIndex = boost::error_info<struct tag_uncleIndex, unsigned>;
using errinfo_currentNumber = boost::error_info<struct tag_currentNumber, u256>;
using errinfo_uncleNumber = boost::error_info<struct tag_uncleNumber, u256>;
using errinfo_unclesExcluded = boost::error_info<struct tag_unclesExcluded, h256Hash>;
using errinfo_block = boost::error_info<struct tag_block, bytes>;
using errinfo_now = boost::error_info<struct tag_now, unsigned>;

using errinfo_transactionIndex = boost::error_info<struct tag_transactionIndex, unsigned>;

using errinfo_vmtrace = boost::error_info<struct tag_vmtrace, std::string>;
using errinfo_receipts = boost::error_info<struct tag_receipts, std::vector<bytes>>;
using errinfo_transaction = boost::error_info<struct tag_transaction, bytes>;
using errinfo_phase = boost::error_info<struct tag_phase, unsigned>;
using errinfo_required_LogBloom = boost::error_info<struct tag_required_LogBloom, LogBloom>;
using errinfo_got_LogBloom = boost::error_info<struct tag_get_LogBloom, LogBloom>;
using LogBloomRequirementError = boost::tuple<errinfo_required_LogBloom, errinfo_got_LogBloom>;

class BlockChain;
class State;
class TransactionQueue;
struct VerifiedBlockRef;

struct StateChat: public LogChannel { static const char* name(); static const int verbosity = 4; };
struct StateTrace: public LogChannel { static const char* name(); static const int verbosity = 5; };
struct StateDetail: public LogChannel { static const char* name(); static const int verbosity = 14; };
struct StateSafeExceptions: public LogChannel { static const char* name(); static const int verbosity = 21; };

enum class BaseState
{
	PreExisting,
	Empty
};

enum class Permanence
{
	Reverted,
	Committed
};

#if ETH_FATDB
template <class KeyType, class DB> using SecureTrieDB = SpecificTrieDB<FatGenericTrieDB<DB>, KeyType>;
#else
template <class KeyType, class DB> using SecureTrieDB = SpecificTrieDB<HashedGenericTrieDB<DB>, KeyType>;
#endif

DEV_SIMPLE_EXCEPTION(InvalidAccountStartNonceInState);
DEV_SIMPLE_EXCEPTION(IncorrectAccountStartNonceInState);

//#define ETH_ALLOW_EMPTY_BLOCK_AND_STATE 1

class SealEngineFace;

/**
 * @brief Model of an Ethereum state, essentially a facade for the trie.
 * Allows you to query the state of accounts, and has built-in caching for various aspects of the
 * state.
 */
class State
{
	friend class ExtVM;
	friend class dev::test::ImportTest;
	friend class dev::test::StateLoader;
	friend class BlockChain;

public:
	enum class CommitBehaviour
	{
		KeepEmptyAccounts,
		RemoveEmptyAccounts
	};

	/// Default constructor; creates with a blank database prepopulated with the genesis block.
	explicit State(u256 const& _accountStartNonce): State(_accountStartNonce, OverlayDB(), BaseState::Empty) {}

	/// Basic state object from database.
	/// Use the default when you already have a database and you just want to make a State object
	/// which uses it. If you have no preexisting database then set BaseState to something other
	/// than BaseState::PreExisting in order to prepopulate the Trie.
	explicit State(u256 const& _accountStartNonce, OverlayDB const& _db, BaseState _bs = BaseState::PreExisting);

#if ETH_ALLOW_EMPTY_BLOCK_AND_STATE
	State(): State(Invalid256, OverlayDB(), BaseState::Empty) {}
	explicit State(OverlayDB const& _db, BaseState _bs = BaseState::PreExisting): State(Invalid256, _db, _bs) {}
#endif

	enum NullType { Null };
	State(NullType): State(Invalid256, OverlayDB(), BaseState::Empty) {}

	/// Copy state object.
	State(State const& _s);

	/// Copy state object.
	State& operator=(State const& _s);

	/// Open a DB - useful for passing into the constructor & keeping for other states that are necessary.
	static OverlayDB openDB(std::string const& _path, h256 const& _genesisHash, WithExisting _we = WithExisting::Trust);
	static OverlayDB openDB(h256 const& _genesisHash, WithExisting _we = WithExisting::Trust) { return openDB(std::string(), _genesisHash, _we); }
	OverlayDB const& db() const { return m_db; }
	OverlayDB& db() { return m_db; }

	/// Populate the state from the given AccountMap. Just uses dev::eth::commit().
	void populateFrom(AccountMap const& _map);

	/// @returns the set containing all addresses currently in use in Ethereum.
	/// @warning This is slowslowslow. Don't use it unless you want to lock the object for seconds or minutes at a time.
	/// @throws InterfaceNotSupported if compiled without ETH_FATDB.
	std::unordered_map<Address, u256> addresses() const;

	/// Execute a given transaction.
	/// This will change the state accordingly.
	std::pair<ExecutionResult, TransactionReceipt> execute(EnvInfo const& _envInfo, SealEngineFace const& _sealEngine, Transaction const& _t, Permanence _p = Permanence::Committed, OnOpFunc const& _onOp = OnOpFunc());

	/// Check if the address is in use.
	bool addressInUse(Address const& _address) const;

	bool isTouched(Address const& _address) const { auto a = account(_address); return !a || a->isDirty(); }
	void untouch(Address const& _address)
	{
		m_cache[_address].untouch();
		m_unchangedCacheEntries.emplace_back(_address);
	}

	/// Check if the account exists in the state and is non empty (nonce > 0 || balance > 0 || code nonempty).
	/// These two notions are equivalent after EIP158.
	bool accountNonemptyAndExisting(Address const& _address) const;

	/// Check if the address contains executable code.
	bool addressHasCode(Address const& _address) const;

	/// Get an account's balance.
	/// @returns 0 if the address has never been used.
	u256 balance(Address const& _id) const;

	/// Add some amount to balance.
	/// Will initialise the address if it has never been used.
	void addBalance(Address const& _id, u256 const& _amount);

	/** Subtract some amount from balance.
	 * @throws NotEnoughCash if balance of @a _id is less than @a _value (or has never been used).
	 * @note We use bigint here as we don't want any accidental problems with negative numbers.
	 */
	void subBalance(Address const& _id, bigint const& _value);

	/**
	 * @brief Transfers "the balance @a _value between two accounts.
	 * @param _from Account from which @a _value will be deducted.
	 * @param _to Account to which @a _value will be added.
	 * @param _value Amount to be transferred.
	 */
	void transferBalance(Address const& _from, Address const& _to, u256 const& _value) { subBalance(_from, _value); addBalance(_to, _value); }

	/// Get the root of the storage of an account.
	h256 storageRoot(Address const& _contract) const;

	/// Get the value of a storage position of an account.
	/// @returns 0 if no account exists at that address.
	u256 storage(Address const& _contract, u256 const& _memory) const;

	/// Set the value of a storage position of an account.
	void setStorage(Address const& _contract, u256 const& _location, u256 const& _value) { m_cache[_contract].setStorage(_location, _value); }

	/// Create a contract at the given address (with unset code and unchanged balance).
	/// If @a _incrementNonce is true, increment the nonce upon creation.
	void createContract(Address const& _address, bool _incrementNonce);

	/// Similar to `createContract`, but used in a normal transaction that targets _address.
	void ensureAccountExists(Address const& _address);

	/// Sets the code of the account. Must only be called during / after contract creation.
	void setCode(Address const& _address, bytes&& _code) { m_cache[_address].setCode(std::move(_code)); }

	void clearStorageChanges(Address const& _addr) { m_cache[_addr].clearStorageChanges(); }

	/// Delete an account (used for processing suicides).
	void kill(Address _a);

	bool isAlive(Address const& _addr) const { auto a = account(_addr); return a && a->isAlive(); }

	/// Get the storage of an account.
	/// @note This is expensive. Don't use it unless you need to.
	/// @returns map of hashed keys to key-value pairs or empty map if no account exists at that address.
	std::map<h256, std::pair<u256, u256>> storage(Address const& _contract) const;

	/// Get the code of an account.
	/// @returns bytes() if no account exists at that address.
	bytes const& code(Address const& _contract) const;

	/// Get the code hash of an account.
	/// @returns EmptySHA3 if no account exists at that address or if there is no code associated with the address.
	h256 codeHash(Address const& _contract) const;

	/// Get the byte-size of the code of an account.
	/// @returns code(_contract).size(), but utilizes CodeSizeHash.
	size_t codeSize(Address const& _contract) const;

	/// Increament the account nonce.
	void incNonce(Address const& _id);

	/// Set the account nonce to the given value. Is used to revert account
	/// changes.
	void setNonce(Address const& _addr, u256 const& _nonce);

	/// Get the account nonce -- the number of transactions it has sent.
	/// @returns 0 if the address has never been used.
	u256 getNonce(Address const& _addr) const;

	/// The hash of the root of our state tree.
	h256 rootHash() const { return m_state.root(); }

	/// Commit all changes waiting in the address cache to the DB.
	/// @param _commitBehaviour whether or not to remove empty accounts during commit.
	void commit(CommitBehaviour _commitBehaviour);

	/// Resets any uncommitted changes to the cache.
	void setRoot(h256 const& _root);

	/// Get the account start nonce. May be required.
	u256 const& accountStartNonce() const { return m_accountStartNonce; }
	u256 const& requireAccountStartNonce() const;
	void noteAccountStartNonce(u256 const& _actual);

private:
	/// Turns all "touched" empty accounts into non-alive accounts.
	void removeEmptyAccounts();

	/// @returns the account at the given address or a null pointer if it does not exist.
	/// The pointer is valid until the next access to the state or account.
	Account const* account(Address const& _a, bool _requireCode = false) const;

	/// @returns the account at the given address or a null pointer if it does not exist.
	/// The pointer is valid until the next access to the state or account.
	Account* account(Address const& _a, bool _requireCode = false);

	/// Purges non-modified entries in m_cache if it grows too large.
	void clearCacheIfTooLarge() const;

	void createAccount(Address const& _address, Account const&& _account);

	/// Debugging only. Good for checking the Trie is in shape.
	bool isTrieGood(bool _enforceRefs, bool _requireNoLeftOvers) const;

	/// Debugging only. Good for checking the Trie is in shape.
	void paranoia(std::string const& _when, bool _enforceRefs = false) const;

	OverlayDB m_db;								///< Our overlay for the state tree.
	SecureTrieDB<Address, OverlayDB> m_state;	///< Our state tree, as an OverlayDB DB.
	mutable std::unordered_map<Address, Account> m_cache;	///< Our address cache. This stores the states of each address that has (or at least might have) been changed.
	mutable std::vector<Address> m_unchangedCacheEntries;	///< Tracks entries in m_cache that can potentially be purged if it grows too large.
	mutable std::set<Address> m_nonExistingAccountsCache;	///< Tracks addresses that are known to not exist.
	AddressHash m_touched;						///< Tracks all addresses touched so far.

	u256 m_accountStartNonce;

	friend std::ostream& operator<<(std::ostream& _out, State const& _s);
};

std::ostream& operator<<(std::ostream& _out, State const& _s);

template <class DB>
AddressHash commit(AccountMap const& _cache, SecureTrieDB<Address, DB>& _state)
{
	AddressHash ret;
	for (auto const& i: _cache)
		if (i.second.isDirty())
		{
			if (!i.second.isAlive())
				_state.remove(i.first);
			else
			{
				RLPStream s(4);
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

				if (i.second.isFreshCode())
				{
					h256 ch = sha3(i.second.code());
					// Store the size of the code
					CodeSizeCache::instance().store(ch, i.second.code().size());
					_state.db()->insert(ch, &i.second.code());
					s << ch;
				}
				else
					s << i.second.codeHash();

				_state.insert(i.first, &s.out());
			}
			ret.insert(i.first);
		}
	return ret;
}

}
}

