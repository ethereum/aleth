// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#pragma once

#include "BlockDetails.h"
#include "GasPricer.h"
#include "LogFilter.h"
#include "Transaction.h"
#include <libdevcore/Common.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Guards.h>
#include <libdevcrypto/Common.h>
#include <libethcore/EVMSchedule.h>
#include <libethcore/SealEngine.h>

namespace dev
{
namespace eth
{

struct SyncStatus;

using TransactionHashes = h256s;
using UncleHashes = h256s;

enum class Reaping
{
	Automatic,
	Manual
};

enum class FudgeFactor
{
	Strict,
	Lenient
};

struct GasEstimationProgress
{
	u256 lowerBound;
	u256 upperBound;
};

using GasEstimationCallback = std::function<void(GasEstimationProgress const&)>;

/**
 * @brief Main API hub for interfacing with Ethereum.
 */
class Interface
{
public:
	/// Constructor.
	Interface() {}

	/// Destructor.
	virtual ~Interface() {}

	// [TRANSACTION API]

	/// Submits a new transaction.
	/// @returns the transaction's hash.
	virtual h256 submitTransaction(TransactionSkeleton const& _t, Secret const& _secret) = 0;

	/// Submits the given message-call transaction.
	void submitTransaction(Secret const& _secret, u256 const& _value, Address const& _dest, bytes const& _data = bytes(), u256 const& _gas = 1000000, u256 const& _gasPrice = DefaultGasPrice, u256 const& _nonce = Invalid256);

    /// Imports the given transaction into the transaction queue
	virtual h256 importTransaction(Transaction const& _t) = 0;

	/// Blocks until all pending transactions have been processed.
	virtual void flushTransactions() = 0;

	/// Makes the given call. Nothing is recorded into the state.
	virtual ExecutionResult call(Address const& _from, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice, BlockNumber _blockNumber, FudgeFactor _ff = FudgeFactor::Strict) = 0;
	ExecutionResult call(Address const& _from, u256 _value, Address _dest, bytes const& _data = bytes(), u256 _gas = 1000000, u256 _gasPrice = DefaultGasPrice, FudgeFactor _ff = FudgeFactor::Strict) { return call(_from, _value, _dest, _data, _gas, _gasPrice, m_default, _ff); }
	ExecutionResult call(Secret const& _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice, BlockNumber _blockNumber, FudgeFactor _ff = FudgeFactor::Strict) { return call(toAddress(_secret), _value, _dest, _data, _gas, _gasPrice, _blockNumber, _ff); }
	ExecutionResult call(Secret const& _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice, FudgeFactor _ff = FudgeFactor::Strict) { return call(toAddress(_secret), _value, _dest, _data, _gas, _gasPrice, _ff); }

	/// Injects the RLP-encoded block given by the _rlp into the block queue directly.
	virtual ImportResult injectBlock(bytes const& _block) = 0;

	/// Estimate gas usage for call/create.
	/// @param _maxGas An upper bound value for estimation, if not provided default value of c_maxGasEstimate will be used.
	/// @param _callback Optional callback function for progress reporting
	virtual std::pair<u256, ExecutionResult> estimateGas(Address const& _from, u256 _value, Address _dest, bytes const& _data, int64_t _maxGas, u256 _gasPrice, BlockNumber _blockNumber, GasEstimationCallback const& _callback = GasEstimationCallback()) = 0;

	// [STATE-QUERY API]

	int getDefault() const { return m_default; }
	void setDefault(BlockNumber _block) { m_default = _block; }

	u256 balanceAt(Address _a) const { return balanceAt(_a, m_default); }
	u256 countAt(Address _a) const { return countAt(_a, m_default); }
	u256 stateAt(Address _a, u256 _l) const { return stateAt(_a, _l, m_default); }
	bytes codeAt(Address _a) const { return codeAt(_a, m_default); }
	h256 codeHashAt(Address _a) const { return codeHashAt(_a, m_default); }
	std::map<h256, std::pair<u256, u256>> storageAt(Address _a) const { return storageAt(_a, m_default); }

	virtual u256 balanceAt(Address _a, BlockNumber _block) const = 0;
	virtual u256 countAt(Address _a, BlockNumber _block) const = 0;
	virtual u256 stateAt(Address _a, u256 _l, BlockNumber _block) const = 0;
	virtual h256 stateRootAt(Address _a, BlockNumber _block) const = 0;
	virtual bytes codeAt(Address _a, BlockNumber _block) const = 0;
	virtual h256 codeHashAt(Address _a, BlockNumber _block) const = 0;
	virtual std::map<h256, std::pair<u256, u256>> storageAt(Address _a, BlockNumber _block) const = 0;

	// [LOGS API]
	
	virtual LocalisedLogEntries logs(unsigned _watchId) const = 0;
	virtual LocalisedLogEntries logs(LogFilter const& _filter) const = 0;

	/// Install, uninstall and query watches.
	virtual unsigned installWatch(LogFilter const& _filter, Reaping _r = Reaping::Automatic) = 0;
	virtual unsigned installWatch(h256 _filterId, Reaping _r = Reaping::Automatic) = 0;
	virtual bool uninstallWatch(unsigned _watchId) = 0;
	LocalisedLogEntries peekWatchSafe(unsigned _watchId) const { try { return peekWatch(_watchId); } catch (...) { return LocalisedLogEntries(); } }
	LocalisedLogEntries checkWatchSafe(unsigned _watchId) { try { return checkWatch(_watchId); } catch (...) { return LocalisedLogEntries(); } }
	virtual LocalisedLogEntries peekWatch(unsigned _watchId) const = 0;
	virtual LocalisedLogEntries checkWatch(unsigned _watchId) = 0;

	// [BLOCK QUERY API]

	virtual bool isKnownTransaction(h256 const& _transactionHash) const = 0;
	virtual bool isKnownTransaction(h256 const& _blockHash, unsigned _i) const = 0;
	virtual Transaction transaction(h256 _transactionHash) const = 0;
	virtual LocalisedTransaction localisedTransaction(h256 const& _transactionHash) const = 0;
	virtual TransactionReceipt transactionReceipt(h256 const& _transactionHash) const = 0;
	virtual LocalisedTransactionReceipt localisedTransactionReceipt(h256 const& _transactionHash) const = 0;
	virtual std::pair<h256, unsigned> transactionLocation(h256 const& _transactionHash) const = 0;
	virtual h256 hashFromNumber(BlockNumber _number) const = 0;
	virtual BlockNumber numberFromHash(h256 _blockHash) const = 0;
	virtual int compareBlockHashes(h256 _h1, h256 _h2) const = 0;

	virtual bool isKnown(BlockNumber _block) const = 0;
	virtual bool isKnown(h256 const& _hash) const = 0;
	virtual BlockHeader blockInfo(h256 _hash) const = 0;
	virtual BlockDetails blockDetails(h256 _hash) const = 0;
	virtual Transaction transaction(h256 _blockHash, unsigned _i) const = 0;
	virtual LocalisedTransaction localisedTransaction(h256 const& _blockHash, unsigned _i) const = 0;
	virtual BlockHeader uncle(h256 _blockHash, unsigned _i) const = 0;
	virtual UncleHashes uncleHashes(h256 _blockHash) const = 0;
	virtual unsigned transactionCount(h256 _blockHash) const = 0;
	virtual unsigned transactionCount(BlockNumber _block) const = 0;
	virtual unsigned uncleCount(h256 _blockHash) const = 0;
	virtual Transactions transactions(h256 _blockHash) const = 0;
	virtual Transactions transactions(BlockNumber _block) const = 0;
	virtual TransactionHashes transactionHashes(h256 _blockHash) const = 0;

	virtual BlockHeader pendingInfo() const { return BlockHeader(); }
	virtual BlockDetails pendingDetails() const { return BlockDetails(); }
	/// @returns the EVMSchedule in the context of the pending block.
	virtual EVMSchedule evmSchedule() const { return EVMSchedule(); }

	BlockHeader blockInfo(BlockNumber _block) const;
	BlockDetails blockDetails(BlockNumber _block) const;
	Transaction transaction(BlockNumber _block, unsigned _i) const { auto p = transactions(_block); return _i < p.size() ? p[_i] : Transaction(); }
	TransactionHashes transactionHashes(BlockNumber _block) const { if (_block == PendingBlock) return pendingHashes(); return transactionHashes(hashFromNumber(_block)); }
	BlockHeader uncle(BlockNumber _block, unsigned _i) const { return uncle(hashFromNumber(_block), _i); }
	UncleHashes uncleHashes(BlockNumber _block) const { return uncleHashes(hashFromNumber(_block)); }
	unsigned uncleCount(BlockNumber _block) const { return uncleCount(hashFromNumber(_block)); }

	// [EXTRA API]:

	/// @returns The height of the chain.
	virtual unsigned number() const = 0;

	/// Get a map containing each of the pending transactions (transactions from accounts managed by this node which have not yet made it into a mined block)
	/// @TODO: Remove in favour of transactions().
	virtual Transactions pending() const = 0;
	virtual h256s pendingHashes() const = 0;

	/// Get a list of all active addresses.
	/// NOTE: This only works when compiled with ETH_FATDB; otherwise will throw InterfaceNotSupported.
	virtual Addresses addresses() const { return addresses(m_default); }
	virtual Addresses addresses(BlockNumber _block) const = 0;

	/// Get the remaining gas limit in this block.
	virtual u256 gasLimitRemaining() const = 0;
	// Get the gas bidding price
	virtual u256 gasBidPrice() const = 0;

	/// Get some information on the block queue.
	virtual SyncStatus syncStatus() const = 0;

	/// Populate the uninitialized fields in the supplied transaction with default values
	virtual TransactionSkeleton populateTransactionWithDefaults(TransactionSkeleton const& _t) const = 0;

	// [SEALING API]:

	/// Set the block author address.
	virtual void setAuthor(Address const& _us) = 0;
	/// Get the block author address.
	virtual Address author() const = 0;

	/// Start sealing.
	/// NOT thread-safe - call it & stopSealing only from a single thread
	virtual void startSealing() = 0;
	/// Stop sealing.
	/// NOT thread-safe
	virtual void stopSealing() = 0;
	/// Would we like to be sealing now?
	virtual bool wouldSeal() const = 0;

	/// Are we updating the chain (syncing or importing a new block)?
	virtual bool isSyncing() const { return false; }
	/// Are we syncing the chain?
	virtual bool isMajorSyncing() const { return false; }

	/// Gets the network id.
	virtual u256 networkId() const { return 0; }
	/// Sets the network id.
	virtual void setNetworkId(u256 const&) {}

	/// Gets the chain id
	virtual int chainId() const { return 0; }

	/// Get the seal engine.
	virtual SealEngineFace* sealEngine() const { return nullptr; }

    ///< Get POW depending on sealengine it's using
    virtual std::tuple<h256, h256, h256> getWork() = 0;

protected:
	int m_default = PendingBlock;
};

class Watch;

}
}

namespace std { void swap(dev::eth::Watch& _a, dev::eth::Watch& _b); }

namespace dev
{
namespace eth
{

class Watch: public boost::noncopyable
{
	friend void std::swap(Watch& _a, Watch& _b);

public:
	Watch() {}
	Watch(Interface& _c, h256 _f): m_c(&_c), m_id(_c.installWatch(_f)) {}
	Watch(Interface& _c, LogFilter const& _tf): m_c(&_c), m_id(_c.installWatch(_tf)) {}
	~Watch() { if (m_c) m_c->uninstallWatch(m_id); }

	LocalisedLogEntries check() { return m_c ? m_c->checkWatch(m_id) : LocalisedLogEntries(); }
	LocalisedLogEntries peek() { return m_c ? m_c->peekWatch(m_id) : LocalisedLogEntries(); }
	LocalisedLogEntries logs() const { return m_c->logs(m_id); }

private:
	Interface* m_c = nullptr;
	unsigned m_id = 0;
};

}
}

namespace std
{

inline void swap(dev::eth::Watch& _a, dev::eth::Watch& _b)
{
	swap(_a.m_c, _b.m_c);
	swap(_a.m_id, _b.m_id);
}

}
