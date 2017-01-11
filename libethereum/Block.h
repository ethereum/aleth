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
#include <libdevcore/OverlayDB.h>
#include <libethcore/Exceptions.h>
#include <libethcore/BlockHeader.h>
#include <libethcore/ChainOperationParams.h>
#include <libevm/ExtVMFace.h>
#include "Account.h"
#include "Transaction.h"
#include "TransactionReceipt.h"
#include "GasPricer.h"
#include "State.h"

namespace dev
{

namespace test { class ImportTest; class StateLoader; }

namespace eth
{

class SealEngineFace;
class BlockChain;
class State;
class TransactionQueue;
struct VerifiedBlockRef;

struct BlockChat: public LogChannel { static const char* name(); static const int verbosity = 4; };
struct BlockTrace: public LogChannel { static const char* name(); static const int verbosity = 5; };
struct BlockDetail: public LogChannel { static const char* name(); static const int verbosity = 14; };
struct BlockSafeExceptions: public LogChannel { static const char* name(); static const int verbosity = 21; };

struct PopulationStatistics
{
	double verify;
	double enact;
};

DEV_SIMPLE_EXCEPTION(ChainOperationWithUnknownBlockChain);
DEV_SIMPLE_EXCEPTION(InvalidOperationOnSealedBlock);

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
	// TODO: pass in ChainOperationParams rather than u256

	/// Default constructor; creates with a blank database prepopulated with the genesis block.
	Block(u256 const& _accountStartNonce): m_state(_accountStartNonce, OverlayDB(), BaseState::Empty), m_precommit(_accountStartNonce) {}

	/// Basic state object from database.
	/// Use the default when you already have a database and you just want to make a Block object
	/// which uses it. If you have no preexisting database then set BaseState to something other
	/// than BaseState::PreExisting in order to prepopulate the Trie.
	/// You can also set the author address.
	Block(BlockChain const& _bc, OverlayDB const& _db, BaseState _bs = BaseState::PreExisting, Address const& _author = Address());

	/// Basic state object from database.
	/// Use the default when you already have a database and you just want to make a Block object
	/// which uses it.
	/// Will throw InvalidRoot if the root passed is not in the database.
	/// You can also set the author address.
	Block(BlockChain const& _bc, OverlayDB const& _db, h256 const& _root, Address const& _author = Address());

#if ETH_ALLOW_EMPTY_BLOCK_AND_STATE
	Block(): Block(Block::Null) {}
	explicit Block(OverlayDB const& _db, BaseState _bs = BaseState::PreExisting, Address const& _author = Address()): Block(Invalid256, _db, _bs, _author) {}
	Block(OverlayDB const& _db, h256 const& _root, Address const& _author = Address()): Block(Invalid256, _db, _root, _author) {}
#endif

	enum NullType { Null };
	Block(NullType): m_state(0, OverlayDB(), BaseState::Empty), m_precommit(0) {}

	/// Construct from a given blockchain. Empty, but associated with @a _bc 's chain params.
	explicit Block(BlockChain const& _bc): Block(Null) { noteChain(_bc); }

	/// Copy state object.
	Block(Block const& _s);

	/// Copy state object.
	Block& operator=(Block const& _s);

	/// Get the author address for any transactions we do and rewards we get.
	Address author() const { return m_author; }

	/// Set the author address for any transactions we do and rewards we get.
	/// This causes a complete reset of current block.
	void setAuthor(Address const& _id) { m_author = _id; resetCurrent(); }

	/// Note the fact that this block is being used with a particular chain.
	/// Call this before using any non-const methods.
	void noteChain(BlockChain const& _bc);

	// Account-getters. All operate on the final state.

	/// Get an account's balance.
	/// @returns 0 if the address has never been used.
	u256 balance(Address const& _address) const { return m_state.balance(_address); }

	/// Get the number of transactions a particular address has sent (used for the transaction nonce).
	/// @returns 0 if the address has never been used.
	u256 transactionsFrom(Address const& _address) const { return m_state.getNonce(_address); }

	/// Check if the address is in use.
	bool addressInUse(Address const& _address) const { return m_state.addressInUse(_address); }

	/// Check if the address contains executable code.
	bool addressHasCode(Address const& _address) const { return m_state.addressHasCode(_address); }

	/// Get the root of the storage of an account.
	h256 storageRoot(Address const& _contract) const { return m_state.storageRoot(_contract); }

	/// Get the value of a storage position of an account.
	/// @returns 0 if no account exists at that address.
	u256 storage(Address const& _contract, u256 const& _memory) const { return m_state.storage(_contract, _memory); }

	/// Get the storage of an account.
	/// @note This is expensive. Don't use it unless you need to.
	/// @returns map of hashed keys to key-value pairs or empty map if no account exists at that address.
	std::map<h256, std::pair<u256, u256>> storage(Address const& _contract) const { return m_state.storage(_contract); }

	/// Get the code of an account.
	/// @returns bytes() if no account exists at that address.
	bytes const& code(Address const& _contract) const { return m_state.code(_contract); }

	/// Get the code hash of an account.
	/// @returns EmptySHA3 if no account exists at that address or if there is no code associated with the address.
	h256 codeHash(Address const& _contract) const { return m_state.codeHash(_contract); }

	// General information from state

	/// Get the backing state object.
	State const& state() const { return m_state; }

	/// Open a DB - useful for passing into the constructor & keeping for other states that are necessary.
	OverlayDB const& db() const { return m_state.db(); }

	/// The hash of the root of our state tree.
	h256 rootHash() const { return m_state.rootHash(); }

	/// @returns the set containing all addresses currently in use in Ethereum.
	/// @throws InterfaceNotSupported if compiled without ETH_FATDB.
	std::unordered_map<Address, u256> addresses() const { return m_state.addresses(); }

	// For altering accounts behind-the-scenes

	/// Get a mutable State object which is backing this block.
	/// @warning Only use this is you know what you're doing. If you use it while constructing a
	/// normal sealable block, don't expect things to work right.
	State& mutableState() { return m_state; }

	// Information concerning ongoing transactions

	/// Get the remaining gas limit in this block.
	u256 gasLimitRemaining() const { return m_currentBlock.gasLimit() - gasUsed(); }

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

	// State-change operations

	/// Construct state object from arbitrary point in blockchain.
	PopulationStatistics populateFromChain(BlockChain const& _bc, h256 const& _hash, ImportRequirements::value _ir = ImportRequirements::None);

	/// Execute a given transaction.
	/// This will append @a _t to the transaction list and change the state accordingly.
	ExecutionResult execute(LastHashes const& _lh, Transaction const& _t, Permanence _p = Permanence::Committed, OnOpFunc const& _onOp = OnOpFunc());

	/// Sync our transactions, killing those from the queue that we have and assimilating those that we don't.
	/// @returns a list of receipts one for each transaction placed from the queue into the state and bool, true iff there are more transactions to be processed.
	std::pair<TransactionReceipts, bool> sync(BlockChain const& _bc, TransactionQueue& _tq, GasPricer const& _gp, unsigned _msTimeout = 100);

	/// Sync our state with the block chain.
	/// This basically involves wiping ourselves if we've been superceded and rebuilding from the transaction queue.
	bool sync(BlockChain const& _bc);

	/// Sync with the block chain, but rather than synching to the latest block, instead sync to the given block.
	bool sync(BlockChain const& _bc, h256 const& _blockHash, BlockHeader const& _bi = BlockHeader());

	/// Execute all transactions within a given block.
	/// @returns the additional total difficulty.
	u256 enactOn(VerifiedBlockRef const& _block, BlockChain const& _bc);

	/// Returns back to a pristine state after having done a playback.
	/// @arg _fullCommit if true flush everything out to disk. If false, this effectively only validates
	/// the block since all state changes are ultimately reversed.
	void cleanup(bool _fullCommit);

	/// Sets m_currentBlock to a clean state, (i.e. no change from m_previousBlock) and
	/// optionally modifies the timestamp.
	void resetCurrent(u256 const& _timestamp = u256(utcTime()));

	// Sealing

	/// Prepares the current state for mining.
	/// Commits all transactions into the trie, compiles uncles and transactions list, applies all
	/// rewards and populates the current block header with the appropriate hashes.
	/// The only thing left to do after this is to actually mine().
	///
	/// This may be called multiple times and without issue.
	void commitToSeal(BlockChain const& _bc, bytes const& _extraData = {});

	/// Pass in a properly sealed header matching this block.
	/// @returns true iff we were previously committed to sealing, the header is valid and it
	/// corresponds to this block.
	/// TODO: verify it prior to calling this.
	/** Commit to DB and build the final block if the previous call to mine()'s result is completion.
	 * Typically looks like:
	 * @code
	 * while (!isSealed)
	 * {
	 * // lock
	 * commitToSeal(_blockChain);  // will call uncommitToSeal if a repeat.
	 * sealBlock(sealedHeader);
	 * // unlock
	 * @endcode
	 */
	bool sealBlock(bytes const& _header) { return sealBlock(&_header); }
	bool sealBlock(bytesConstRef _header);

	/// @returns true if sealed - in this case you can no longer append transactions.
	bool isSealed() const { return !m_currentBytes.empty(); }

	/// Get the complete current block, including valid nonce.
	/// Only valid when isSealed() is true.
	bytes const& blockData() const { return m_currentBytes; }

	/// Get the header information on the present block.
	BlockHeader const& info() const { return m_currentBlock; }

private:
	SealEngineFace* sealEngine() const;

	/// Undo the changes to the state for committing to mine.
	void uncommitToSeal();

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
	void applyRewards(std::vector<BlockHeader> const& _uncleBlockHeaders, u256 const& _blockReward);

	/// @returns gas used by transactions thus far executed.
	u256 gasUsed() const { return m_receipts.size() ? m_receipts.back().gasUsed() : 0; }

	/// Performs irregular modifications right after initialization, e.g. to implement a hard fork.
	void performIrregularModifications();

	/// Provide a standard VM trace for debugging purposes.
	std::string vmTrace(bytesConstRef _block, BlockChain const& _bc, ImportRequirements::value _ir);

	State m_state;								///< Our state tree, as an OverlayDB DB.
	Transactions m_transactions;				///< The current list of transactions that we've included in the state.
	TransactionReceipts m_receipts;				///< The corresponding list of transaction receipts.
	h256Hash m_transactionSet;					///< The set of transaction hashes that we've included in the state.
	State m_precommit;							///< State at the point immediately prior to rewards.

	BlockHeader m_previousBlock;				///< The previous block's information.
	BlockHeader m_currentBlock;					///< The current block's information.
	bytes m_currentBytes;						///< The current block's bytes.
	bool m_committedToSeal = false;				///< Have we committed to mine on the present m_currentBlock?

	bytes m_currentTxs;							///< The RLP-encoded block of transactions.
	bytes m_currentUncles;						///< The RLP-encoded block of uncles.

	Address m_author;							///< Our address (i.e. the address to which fees go).

	SealEngineFace* m_sealEngine = nullptr;		///< The chain's seal engine.

	friend std::ostream& operator<<(std::ostream& _out, Block const& _s);
};

std::ostream& operator<<(std::ostream& _out, Block const& _s);


}

}
