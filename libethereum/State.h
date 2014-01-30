/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Foobar is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file State.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <array>
#include <map>
#include <unordered_map>
#include "Common.h"
#include "RLP.h"
#include "TransactionQueue.h"
#include "Exceptions.h"
#include "BlockInfo.h"
#include "AddressState.h"
#include "Transaction.h"
#include "TrieDB.h"
#include "Dagger.h"

namespace eth
{

class BlockChain;

/**
 * @brief Model of the current state of the ledger.
 * Maintains current ledger (m_current) as a fast hash-map. This is hashed only when required (i.e. to create or verify a block).
 * Should maintain ledger as of last N blocks, also, in case we end up on the wrong branch.
 */
class State
{
public:
	/// Construct state object.
	State(Address _coinbaseAddress, Overlay const& _db);

	/// Set the coinbase address for any transactions we do.
	/// This causes a complete reset of current block.
	void setAddress(Address _coinbaseAddress) { m_ourAddress = _coinbaseAddress; resetCurrent(); }
	Address address() const { return m_ourAddress; }

	/// Open a DB - useful for passing into the constructor & keeping for other states that are necessary.
	static Overlay openDB(std::string _path, bool _killExisting = false);
	static Overlay openDB(bool _killExisting = false) { return openDB(std::string(), _killExisting); }

	/// @returns the set containing all addresses currently in use in Ethereum.
	std::map<Address, u256> addresses() const;

	/// Cancels transactions and rolls back the state to the end of the previous block.
	/// @warning This will only work for on any transactions after you called the last commitToMine().
	/// It's one or the other.
	void rollback() { m_cache.clear(); }

	/// Prepares the current state for mining.
	/// Commits all transactions into the trie, compiles uncles and transactions list, applies all
	/// rewards and populates the current block header with the appropriate hashes.
	/// The only thing left to do after this is to actually mine().
	///
	/// This may be called multiple times and without issue, however, until the current state is cleared,
	/// calls after the first are ignored.
	void commitToMine(BlockChain const& _bc);

	/// Attempt to find valid nonce for block that this state represents.
	/// @param _msTimeout Timeout before return in milliseconds.
	/// @returns a non-empty byte array containing the block if it got lucky. In this case, call blockData()
	/// to get the block if you need it later.
	MineInfo mine(uint _msTimeout = 1000);

	/// Get the complete current block, including valid nonce.
	/// Only valid after mine() returns true.
	bytes const& blockData() const { return m_currentBytes; }

	/// Sync our state with the block chain.
	/// This basically involves wiping ourselves if we've been superceded and rebuilding from the transaction queue.
	bool sync(BlockChain const& _bc);

	/// Sync with the block chain, but rather than synching to the latest block, instead sync to the given block.
	bool sync(BlockChain const& _bc, h256 _blockHash);

	/// Sync our transactions, killing those from the queue that we have and assimilating those that we don't.
	bool sync(TransactionQueue& _tq);

	/// Execute a given transaction.
	void execute(bytes const& _rlp) { return execute(&_rlp); }
	void execute(bytesConstRef _rlp);

	/// Check if the address is a valid normal (non-contract) account address.
	bool isNormalAddress(Address _address) const;

	/// Check if the address is a valid contract's address.
	bool isContractAddress(Address _address) const;

	/// Get an account's balance.
	/// @returns 0 if the address has never been used.
	u256 balance(Address _id) const;

	/// Add some amount to balance.
	/// Will initialise the address if it has never been used.
	void addBalance(Address _id, u256 _amount);

	/** Subtract some amount from balance.
	 * @throws NotEnoughCash if balance of @a _id is less than @a _value (or has never been used).
	 * @note We use bigint here as we don't want any accidental problems with negative numbers.
	 */
	void subBalance(Address _id, bigint _value);

	/// Get the value of a memory position of a contract.
	/// @returns 0 if no contract exists at that address.
	u256 contractMemory(Address _contract, u256 _memory) const;

	/// Note that the given address is sending a transaction and thus increment the associated ticker.
	void noteSending(Address _id);

	/// Get the number of transactions a particular address has sent (used for the transaction nonce).
	/// @returns 0 if the address has never been used.
	u256 transactionsFrom(Address _address) const;

	/// The hash of the root of our state tree.
	h256 rootHash() const { return m_state.root(); }

	/// Finalise the block, applying the earned rewards.
	void applyRewards(Addresses const& _uncleAddresses);

	/// Execute all transactions within a given block.
	/// @returns the additional total difficulty.
	/// If the _grandParent is passed, it will check the validity of each of the uncles.
	/// This might throw.
	u256 playback(bytesConstRef _block, BlockInfo const& _bi, BlockInfo const& _parent, BlockInfo const& _grandParent, bool _fullCommit);

private:
	/// Fee-adder on destruction RAII class.
	struct MinerFeeAdder
	{
		~MinerFeeAdder() { state->addBalance(state->m_currentBlock.coinbaseAddress, fee); }
		State* state;
		u256 fee;
	};

	/// Retrieve all information about a given address into the cache.
	/// If _requireMemory is true, grab the full memory should it be a contract item.
	/// If _forceCreate is true, then insert a default item into the cache, in the case it doesn't
	/// exist in the DB.
	void ensureCached(Address _a, bool _requireMemory, bool _forceCreate) const;

	/// Commit all changes waiting in the address cache.
	void commit();

	/// Execute the given block on our previous block. This will set up m_currentBlock first, then call the other playback().
	/// Any failure will be critical.
	u256 playback(bytesConstRef _block, bool _fullCommit);

	/// Execute the given block, assuming it corresponds to m_currentBlock. If _grandParent is passed, it will be used to check the uncles.
	/// Throws on failure.
	u256 playback(bytesConstRef _block, BlockInfo const& _grandParent, bool _fullCommit);

	/// Execute a decoded transaction object, given a sender.
	/// This will append @a _t to the transaction list and change the state accordingly.
	void executeBare(Transaction const& _t, Address _sender);

	/// Execute a contract transaction.
	void execute(Address _myAddress, Address _txSender, u256 _txValue, u256 _txFee, u256s const& _txData, u256* o_totalFee);

	/// Sets m_currentBlock to a clean state, (i.e. no change from m_previousBlock).
	void resetCurrent();

	Overlay m_db;								///< Our overlay for the state tree.
	TrieDB<Address, Overlay> m_state;			///< Our state tree, as an Overlay DB.
	std::map<h256, Transaction> m_transactions;	///< The current list of transactions that we've included in the state.

	mutable std::map<Address, AddressState> m_cache;	///< Our address cache. This stores the states of each address that has (or at least might have) been changed.

	BlockInfo m_previousBlock;					///< The previous block's information.
	BlockInfo m_currentBlock;					///< The current block's information.
	bytes m_currentBytes;						///< The current block.
	uint m_currentNumber;

	bytes m_currentTxs;
	bytes m_currentUncles;

	Address m_ourAddress;						///< Our address (i.e. the address to which fees go).

	Dagger m_dagger;
	
	/// The fee structure. Values yet to be agreed on...
	static const u256 c_stepFee;
	static const u256 c_dataFee;
	static const u256 c_memoryFee;
	static const u256 c_extroFee;
	static const u256 c_cryptoFee;
	static const u256 c_newContractFee;
	static const u256 c_txFee;
	static const u256 c_blockReward;

	static std::string c_defaultPath;

	friend std::ostream& operator<<(std::ostream& _out, State const& _s);
};

inline std::ostream& operator<<(std::ostream& _out, State const& _s)
{
	_out << "--- " << _s.rootHash() << std::endl;
	std::set<Address> d;
	for (auto const& i: TrieDB<Address, Overlay>(const_cast<Overlay*>(&_s.m_db), _s.m_currentBlock.stateRoot))
	{
		auto it = _s.m_cache.find(i.first);
		if (it == _s.m_cache.end())
		{
			RLP r(i.second);
			_out << "[    " << (r.itemCount() == 3 ? "CONTRACT] " : "   NORMAL] ") << i.first << ": " << std::dec << r[1].toInt<u256>() << "@" << r[0].toInt<u256>() << std::endl;
		}
		else
			d.insert(i.first);
	}
	for (auto i: _s.m_cache)
		if (i.second.type() == AddressType::Dead)
			_out << "[XXX " << i.first << std::endl;
		else
			_out << (d.count(i.first) ? "[ !  " : "[ *  ") << (i.second.type() == AddressType::Contract ? "CONTRACT] " : "   NORMAL] ") << i.first << ": " << std::dec << i.second.nonce() << "@" << i.second.balance() << std::endl;
	return _out;
}

}


