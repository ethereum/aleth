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
/** @file Block.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <array>
#include <unordered_map>
#include <libdevcore/Common.h>
#include <libdevcore/RLP.h>
#include <libdevcore/TrieDB.h>
#include <libdevcrypto/OverlayDB.h>
#include <libethcore/Exceptions.h>
#include <libethcore/BlockInfo.h>
#include <libethcore/Miner.h>
#include <libevm/ExtVMFace.h>
#include "Account.h"
#include "Transaction.h"
#include "TransactionReceipt.h"
#include "AccountDiff.h"
#include "GasPricer.h"
#include "State.h"

namespace dev
{

namespace test { class ImportTest; class StateLoader; }

namespace eth
{

class BlockChain;
class State;
class TransactionQueue;
struct VerifiedBlockRef;

struct BlockChat: public LogChannel { static const char* name(); static const int verbosity = 4; };
struct BlockTrace: public LogChannel { static const char* name(); static const int verbosity = 5; };
struct BlockDetail: public LogChannel { static const char* name(); static const int verbosity = 14; };
struct BlockSafeExceptions: public LogChannel { static const char* name(); static const int verbosity = 21; };

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

struct PopulationStatistics
{
	double verify;
	double enact;
};

/**
 * @brief Active model of a block within the block chain.
 * Keeps track of all transactions, receipts and state for a particular block. Can apply all
 * needed transforms of the state for rewards and contains logic for sealing the block.
 */
class Block
{
	friend class ExtVM;
	friend class dev::test::ImportTest;
	friend class dev::test::StateLoader;
	friend class Executive;
	friend class BlockChain;

public:
	/// Default constructor; creates with a blank database prepopulated with the genesis block.
	Block(): m_state(OverlayDB(), BaseState::Empty) {}

	/// Basic state object from database.
	/// Use the default when you already have a database and you just want to make a State object
	/// which uses it. If you have no preexisting database then set BaseState to something other
	/// than BaseState::PreExisting in order to prepopulate the Trie.
	/// You can also set the coinbase address.
	explicit Block(OverlayDB const& _db, BaseState _bs = BaseState::PreExisting, Address _coinbaseAddress = Address());

	/// Copy state object.
	Block(Block const& _s);

	/// Copy state object.
	Block& operator=(Block const& _s);

	/// Construct state object from arbitrary point in blockchain.
	PopulationStatistics populateFromChain(BlockChain const& _bc, h256 const& _hash, ImportRequirements::value _ir = ImportRequirements::None);

	/// Set the coinbase address for any transactions we do and rewards we get.
	/// This causes a complete reset of current block.
	void setAddress(Address _coinbaseAddress) { m_ourAddress = _coinbaseAddress; resetCurrent(); }
	Address address() const { return m_ourAddress; }

	/// Open a DB - useful for passing into the constructor & keeping for other states that are necessary.
	OverlayDB const& db() const { return m_state.db(); }
	OverlayDB& db() { return m_state.db(); }

	/// @returns the set containing all addresses currently in use in Ethereum.
	/// @throws InterfaceNotSupported if compiled without ETH_FATDB.
	std::unordered_map<Address, u256> addresses() const { return m_state.addresses(); }

	/// Get the header information on the present block.
	BlockInfo const& info() const { return m_currentBlock; }

	/// Prepares the current state for mining.
	/// Commits all transactions into the trie, compiles uncles and transactions list, applies all
	/// rewards and populates the current block header with the appropriate hashes.
	/// The only thing left to do after this is to actually mine().
	///
	/// This may be called multiple times and without issue.
	void commitToMine(BlockChain const& _bc, bytes const& _extraData = {});

	/// Pass in a solution to the proof-of-work.
	/// @returns true iff we were previously committed to mining.
	/// TODO: verify it prior to calling this.
	/** Commit to DB and build the final block if the previous call to mine()'s result is completion.
	 * Typically looks like:
	 * @code
	 * while (notYetMined)
	 * {
	 * // lock
	 * commitToMine(_blockChain);  // will call uncommitToMine if a repeat.
	 * completeMine();
	 * // unlock
	 * @endcode
	 */
	bool sealBlock(bytes const& _header) { return sealBlock(&_header); }
	bool sealBlock(bytesConstRef _header);

	/// Get the complete current block, including valid nonce.
	/// Only valid after mine() returns true.
	bytes const& blockData() const { return m_currentBytes; }

	/// Sync our transactions, killing those from the queue that we have and assimilating those that we don't.
	/// @returns a list of receipts one for each transaction placed from the queue into the state and bool, true iff there are more transactions to be processed.
	std::pair<TransactionReceipts, bool> sync(BlockChain const& _bc, TransactionQueue& _tq, GasPricer const& _gp, unsigned _msTimeout = 100);

	/// Execute a given transaction.
	/// This will append @a _t to the transaction list and change the state accordingly.
	ExecutionResult execute(LastHashes const& _lh, Transaction const& _t, Permanence _p = Permanence::Committed, OnOpFunc const& _onOp = OnOpFunc());

	/// Get the remaining gas limit in this block.
	u256 gasLimitRemaining() const { return m_currentBlock.gasLimit() - gasUsed(); }

	/// Check if the address is in use.
	bool addressInUse(Address const& _address) const { return m_state.addressInUse(_address); }

	/// Check if the address contains executable code.
	bool addressHasCode(Address const& _address) const { return m_state.addressHasCode(_address); }

	/// Get an account's balance.
	/// @returns 0 if the address has never been used.
	u256 balance(Address const& _id) const { return m_state.balance(_id); }

	/// Add some amount to balance.
	/// Will initialise the address if it has never been used.
	void addBalance(Address const& _id, u256 const& _amount) { m_state.addBalance(_id, _amount); }

	/** Subtract some amount from balance.
	 * @throws NotEnoughCash if balance of @a _id is less than @a _value (or has never been used).
	 * @note We use bigint here as we don't want any accidental problems with negative numbers.
	 */
	void subBalance(Address const& _id, bigint const& _value) { m_state.subBalance(_id, _value); }

	/**
	 * @brief Transfers "the balance @a _value between two accounts.
	 * @param _from Account from which @a _value will be deducted.
	 * @param _to Account to which @a _value will be added.
	 * @param _value Amount to be transferred.
	 */
	void transferBalance(Address const& _from, Address const& _to, u256 const& _value) { subBalance(_from, _value); addBalance(_to, _value); }

	/// Get the root of the storage of an account.
	h256 storageRoot(Address const& _contract) const { return m_state.storageRoot(_contract); }

	/// Get the value of a storage position of an account.
	/// @returns 0 if no account exists at that address.
	u256 storage(Address const& _contract, u256 const& _memory) const { return m_state.storage(_contract, _memory); }

	/// Set the value of a storage position of an account.
	void setStorage(Address const& _contract, u256 const& _location, u256 const& _value) { m_state.setStorage(_contract, _location, _value); }

	/// Create a new contract.
	Address newContract(u256 const& _balance, bytes const& _code) { return m_state.newContract(_balance, _code); }

	/// Get the storage of an account.
	/// @note This is expensive. Don't use it unless you need to.
	/// @returns std::unordered_map<u256, u256> if no account exists at that address.
	std::unordered_map<u256, u256> storage(Address const& _contract) const { return m_state.storage(_contract); }

	/// Get the code of an account.
	/// @returns bytes() if no account exists at that address.
	bytes const& code(Address const& _contract) const { return m_state.code(_contract); }

	/// Get the code hash of an account.
	/// @returns EmptySHA3 if no account exists at that address or if there is no code associated with the address.
	h256 codeHash(Address const& _contract) const { return m_state.codeHash(_contract); }

	/// Note that the given address is sending a transaction and thus increment the associated ticker.
	void noteSending(Address const& _id) { m_state.noteSending(_id); }

	/// Get the number of transactions a particular address has sent (used for the transaction nonce).
	/// @returns 0 if the address has never been used.
	u256 transactionsFrom(Address const& _address) const { return m_state.transactionsFrom(_address); }

	/// The hash of the root of our state tree.
	h256 rootHash() const { return m_state.root(); }

	/// Get the list of pending transactions.
	Transactions const& pending() const { return m_transactions; }

	/// Get the list of hashes of pending transactions.
	h256Hash const& pendingHashes() const { return m_transactionSet; }

	/// Get the transaction receipt for the transaction of the given index.
	TransactionReceipt const& receipt(unsigned _i) const { return m_receipts[_i]; }

	/// Get the list of pending transactions.
	LogEntries const& log(unsigned _i) const { return m_receipts[_i].log(); }

	/// Get the bloom filter of all logs that happened in the block.
	LogBloom logBloom() const;

	/// Get the bloom filter of a particular transaction that happened in the block.
	LogBloom const& logBloom(unsigned _i) const { return m_receipts[_i].bloom(); }

	/// Get the State immediately after the given number of pending transactions have been applied.
	/// If (_i == 0) returns the initial state of the block.
	/// If (_i == pending().size()) returns the final state of the block, prior to rewards.
	State fromPending(unsigned _i) const;

	/// @returns the StateDiff caused by the pending transaction of index @a _i.
	StateDiff pendingDiff(unsigned _i) const { return fromPending(_i).diff(fromPending(_i + 1), true); }

	/// Sync our state with the block chain.
	/// This basically involves wiping ourselves if we've been superceded and rebuilding from the transaction queue.
	bool sync(BlockChain const& _bc);

	/// Sync with the block chain, but rather than synching to the latest block, instead sync to the given block.
	bool sync(BlockChain const& _bc, h256 const& _blockHash, BlockInfo const& _bi = BlockInfo());

	/// Execute all transactions within a given block.
	/// @returns the additional total difficulty.
	u256 enactOn(VerifiedBlockRef const& _block, BlockChain const& _bc);

	/// Returns back to a pristine state after having done a playback.
	/// @arg _fullCommit if true flush everything out to disk. If false, this effectively only validates
	/// the block since all state changes are ultimately reversed.
	void cleanup(bool _fullCommit);

	/// Sets m_currentBlock to a clean state, (i.e. no change from m_previousBlock).
	void resetCurrent();

private:
	/// Undo the changes to the state for committing to mine.
	void uncommitToMine();

	/// Retrieve all information about a given address into the cache.
	/// If _requireMemory is true, grab the full memory should it be a contract item.
	/// If _forceCreate is true, then insert a default item into the cache, in the case it doesn't
	/// exist in the DB.
	void ensureCached(Address const& _a, bool _requireCode, bool _forceCreate) const;

	/// Retrieve all information about a given address into a cache.
	void ensureCached(std::unordered_map<Address, Account>& _cache, Address const& _a, bool _requireCode, bool _forceCreate) const;

	/// Execute the given block, assuming it corresponds to m_currentBlock.
	/// Throws on failure.
	u256 enact(VerifiedBlockRef const& _block, BlockChain const& _bc);

	/// Finalise the block, applying the earned rewards.
	void applyRewards(std::vector<BlockInfo> const& _uncleBlockHeaders);

	/// @returns gas used by transactions thus far executed.
	u256 gasUsed() const { return m_receipts.size() ? m_receipts.back().gasUsed() : 0; }

	/// Provide a standard VM trace for debugging purposes.
	std::string vmTrace(bytesConstRef _block, BlockChain const& _bc, ImportRequirements::value _ir);

	State m_state;								///< Our state tree, as an OverlayDB DB.
	Transactions m_transactions;				///< The current list of transactions that we've included in the state.
	TransactionReceipts m_receipts;				///< The corresponding list of transaction receipts.
	h256Hash m_transactionSet;					///< The set of transaction hashes that we've included in the state.
	State m_precommit;							///< State at the point immediately prior to rewards.
	AddressHash m_touched;						///< Tracks all addresses touched by transactions so far.

	BlockInfo m_previousBlock;					///< The previous block's information.
	BlockInfo m_currentBlock;					///< The current block's information.
	bytes m_currentBytes;						///< The current block.
	bool m_committedToMine = false;				///< Have we committed to mine on the present m_currentBlock?

	bytes m_currentTxs;							///< The RLP-encoded block of transactions.
	bytes m_currentUncles;						///< The RLP-encoded block of uncles.

	Address m_ourAddress;						///< Our address (i.e. the address to which fees go).

	u256 m_blockReward;
};
