// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#pragma once

#include <libdevcore/Common.h>
#include <libdevcore/SHA3.h>
#include <libdevcore/TrieCommon.h>
#include <libethcore/Common.h>

#include <boost/filesystem/path.hpp>

namespace dev
{
class OverlayDB;

namespace eth
{

/**
 * Models the state of a single Ethereum account.
 * Used to cache a portion of the full Ethereum state. State keeps a mapping of Address's to Accounts.
 *
 * Aside from storing the nonce and balance, the account may also be "dead" (where isAlive() returns false).
 * This allows State to explicitly store the notion of a deleted account in it's cache. kill() can be used
 * for this.
 *
 * For the account's storage, the class operates a cache. baseRoot() specifies the base state of the storage
 * given as the Trie root to be looked up in the state database. Alterations beyond this base are specified
 * in the overlay, stored in this class and retrieved with storageOverlay(). setStorage allows the overlay
 * to be altered.
 *
 * The constructor allows you to create an one of a number of "types" of accounts. The default constructor
 * makes a dead account (this is ignored by State when writing out the Trie). Another three allow a basic
 * or contract account to be specified along with an initial balance. The fina two allow either a basic or
 * a contract account to be created with arbitrary values.
 */
class Account
{
public:
    /// Changedness of account to create.
    enum Changedness
    {
        /// Account starts as though it has been changed.
        Changed,
        /// Account starts as though it has not been changed.
        Unchanged
    };

    /// Construct a dead Account.
    Account() {}

    /// Construct an alive Account, with given endowment, for either a normal (non-contract) account
    /// or for a contract account in the conception phase, where the code is not yet known.
    Account(u256 _nonce, u256 _balance, Changedness _c = Changed): m_isAlive(true), m_isUnchanged(_c == Unchanged), m_nonce(_nonce), m_balance(_balance) {}

    /// Explicit constructor for wierd cases of construction or a contract account.
    Account(u256 const& _nonce, u256 const& _balance, h256 const& _contractRoot,
        h256 const& _codeHash, u256 const& _version, Changedness _c)
      : m_isAlive(true),
        m_isUnchanged(_c == Unchanged),
        m_nonce(_nonce),
        m_balance(_balance),
        m_storageRoot(_contractRoot),
        m_codeHash(_codeHash),
        m_version(_version)
    {
        assert(_contractRoot);
    }


    /// Kill this account. Useful for the SELFDESTRUCT instruction.
    /// Following this call, isAlive() returns false.
    void kill()
    {
        m_isAlive = false;
        m_storageOverlay.clear();
        m_storageOriginal.clear();
        m_codeHash = EmptySHA3;
        m_storageRoot = EmptyTrie;
        m_balance = 0;
        m_nonce = 0;
        m_version = 0;
        changed();
    }

    /// @returns true iff this object represents an account in the state.
    /// Returns false if this object represents an account that should no longer exist in the trie
    /// (an account that never existed or was selfdestructed).
    bool isAlive() const { return m_isAlive; }

    /// @returns true if the account is unchanged from creation.
    bool isDirty() const { return !m_isUnchanged; }

    void untouch() { m_isUnchanged = true; }

    /// @returns true if the nonce, balance and code is zero / empty. Code is considered empty
    /// during creation phase.
    bool isEmpty() const { return nonce() == 0 && balance() == 0 && codeHash() == EmptySHA3; }

    /// @returns the balance of this account.
    u256 const& balance() const { return m_balance; }

    /// Increments the balance of this account by the given amount.
    void addBalance(u256 _value) { m_balance += _value; changed(); }

    /// @returns the nonce of the account.
    u256 nonce() const { return m_nonce; }

    /// Increment the nonce of the account by one.
    void incNonce() { ++m_nonce; changed(); }

    /// Set nonce to a new value. This is used when reverting changes made to
    /// the account.
    void setNonce(u256 const& _nonce) { m_nonce = _nonce; changed(); }

    /// @returns the root of the trie (whose nodes are stored in the state db externally to this class)
    /// which encodes the base-state of the account's storage (upon which the storage is overlaid).
    h256 baseRoot() const { assert(m_storageRoot); return m_storageRoot; }

    /// @returns account's storage value corresponding to the @_key
    /// taking into account overlayed modifications
    u256 storageValue(u256 const& _key, OverlayDB const& _db) const
    {
        auto mit = m_storageOverlay.find(_key);
        if (mit != m_storageOverlay.end())
            return mit->second;

        return originalStorageValue(_key, _db);
    }

    /// @returns account's original storage value corresponding to the @_key
    /// not taking into account overlayed modifications
    u256 originalStorageValue(u256 const& _key, OverlayDB const& _db) const;

    /// @returns the storage overlay as a simple hash map.
    std::unordered_map<u256, u256> const& storageOverlay() const { return m_storageOverlay; }

    /// Set a key/value pair in the account's storage. This actually goes into the overlay, for committing
    /// to the trie later.
    void setStorage(u256 _p, u256 _v) { m_storageOverlay[_p] = _v; changed(); }

    /// Empty the storage.  Used when a contract is overwritten.
    void clearStorage()
    {
        m_storageOverlay.clear();
        m_storageOriginal.clear();
        m_storageRoot = EmptyTrie;
        changed();
    }

    /// Set the storage root.  Used when clearStorage() is reverted.
    void setStorageRoot(h256 const& _root)
    {
        m_storageOverlay.clear();
        m_storageOriginal.clear();
        m_storageRoot = _root;
        changed();
    }

    /// @returns the hash of the account's code.
    h256 codeHash() const { return m_codeHash; }

    bool hasNewCode() const { return m_hasNewCode; }

    /// Sets the code of the account. Used by "create" messages.
    void setCode(bytes&& _code, u256 const& _version);

    /// Reset the code set by previous setCode
    void resetCode();

    /// Specify to the object what the actual code is for the account. @a _code must have a SHA3
    /// equal to codeHash().
    void noteCode(bytesConstRef _code) { assert(sha3(_code) == m_codeHash); m_codeCache = _code.toBytes(); }

    /// @returns the account's code.
    bytes const& code() const { return m_codeCache; }

    u256 version() const { return m_version; }

private:
    /// Note that we've altered the account.
    void changed() { m_isUnchanged = false; }

    /// Is this account existant? If not, it represents a deleted account.
    bool m_isAlive = false;

    /// True if we've not made any alteration to the account having been given it's properties directly.
    bool m_isUnchanged = false;

    /// True if new code was deployed to the account
    bool m_hasNewCode = false;

    /// Account's nonce.
    u256 m_nonce;

    /// Account's balance.
    u256 m_balance = 0;

    /// The base storage root. Used with the state DB to give a base to the storage. m_storageOverlay is
    /// overlaid on this and takes precedence for all values set.
    h256 m_storageRoot = EmptyTrie;

    /** If c_contractConceptionCodeHash then we're in the limbo where we're running the initialisation code.
     * We expect a setCode() at some point later.
     * If EmptySHA3, then m_code, which should be empty, is valid.
     * If anything else, then m_code is valid iff it's not empty, otherwise, State::ensureCached() needs to
     * be called with the correct args.
     */
    h256 m_codeHash = EmptySHA3;

    /// Account's version
    u256 m_version = 0;

    /// The map with is overlaid onto whatever storage is implied by the m_storageRoot in the trie.
    mutable std::unordered_map<u256, u256> m_storageOverlay;

    /// The cache of unmodifed storage items
    mutable std::unordered_map<u256, u256> m_storageOriginal;

    /// The associated code for this account. The SHA3 of this should be equal to m_codeHash unless
    /// m_codeHash equals c_contractConceptionCodeHash.
    bytes m_codeCache;

    /// Value for m_codeHash when this account is having its code determined.
    static const h256 c_contractConceptionCodeHash;
};

class AccountMask
{
public:
    AccountMask(bool _all = false):
        m_hasBalance(_all),
        m_hasNonce(_all),
        m_hasCode(_all),
        m_hasStorage(_all)
    {}

    AccountMask(
        bool _hasBalance,
        bool _hasNonce,
        bool _hasCode,
        bool _hasStorage,
        bool _shouldNotExist = false
    ):
        m_hasBalance(_hasBalance),
        m_hasNonce(_hasNonce),
        m_hasCode(_hasCode),
        m_hasStorage(_hasStorage),
        m_shouldNotExist(_shouldNotExist)
    {}

    bool allSet() const { return m_hasBalance && m_hasNonce && m_hasCode && m_hasStorage; }
    bool hasBalance() const { return m_hasBalance; }
    bool hasNonce() const { return m_hasNonce; }
    bool hasCode() const { return m_hasCode; }
    bool hasStorage() const { return m_hasStorage; }
    bool shouldExist() const { return !m_shouldNotExist; }

private:
    bool m_hasBalance;
    bool m_hasNonce;
    bool m_hasCode;
    bool m_hasStorage;
    bool m_shouldNotExist = false;
};

using AccountMap = std::unordered_map<Address, Account>;
using AccountMaskMap = std::unordered_map<Address, AccountMask>;

AccountMap jsonToAccountMap(std::string const& _json, u256 const& _defaultNonce = 0,
    AccountMaskMap* o_mask = nullptr, const boost::filesystem::path& _configPath = {});
}
}
