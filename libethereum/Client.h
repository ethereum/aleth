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
/** @file Client.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <thread>
#include <condition_variable>
#include <mutex>
#include <list>
#include <queue>
#include <atomic>
#include <string>
#include <array>
#include <libdevcore/Common.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Guards.h>
#include <libdevcore/Worker.h>
#include <libethcore/SealEngine.h>
#include <libethcore/ABI.h>
#include <libp2p/Common.h>
#include "BlockChain.h"
#include "Block.h"
#include "CommonNet.h"
#include "ClientBase.h"

namespace dev
{
namespace eth
{

class Client;
class DownloadMan;

enum ClientWorkState
{
	Active = 0,
	Deleting,
	Deleted
};

struct ClientNote: public LogChannel { static const char* name(); static const int verbosity = 2; };
struct ClientChat: public LogChannel { static const char* name(); static const int verbosity = 4; };
struct ClientTrace: public LogChannel { static const char* name(); static const int verbosity = 7; };
struct ClientDetail: public LogChannel { static const char* name(); static const int verbosity = 14; };

struct ActivityReport
{
	unsigned ticks = 0;
	std::chrono::system_clock::time_point since = std::chrono::system_clock::now();
};

std::ostream& operator<<(std::ostream& _out, ActivityReport const& _r);

/**
 * @brief Main API hub for interfacing with Ethereum.
 */
class Client: public ClientBase, protected Worker
{
public:
	Client(
		ChainParams const& _params,
		int _networkID,
		p2p::Host* _host,
		std::shared_ptr<GasPricer> _gpForAdoption,
		std::string const& _dbPath = std::string(),
		WithExisting _forceAction = WithExisting::Trust,
		TransactionQueue::Limits const& _l = TransactionQueue::Limits{1024, 1024}
	);
	/// Destructor.
	virtual ~Client();

	/// Get information on this chain.
	ChainParams const& chainParams() const { return bc().chainParams(); }

	/// Resets the gas pricer to some other object.
	void setGasPricer(std::shared_ptr<GasPricer> _gp) { m_gp = _gp; }
	std::shared_ptr<GasPricer> gasPricer() const { return m_gp; }

	/// Blocks until all pending transactions have been processed.
	virtual void flushTransactions() override;

	/// Queues a block for import.
	ImportResult queueBlock(bytes const& _block, bool _isSafe = false);

	using Interface::call; // to remove warning about hiding virtual function
	/// Makes the given call. Nothing is recorded into the state. This cheats by creating a null address and endowing it with a lot of ETH.
	ExecutionResult call(Address _dest, bytes const& _data = bytes(), u256 _gas = 125000, u256 _value = 0, u256 _gasPrice = 1 * ether, Address const& _from = Address());

	/// Get the remaining gas limit in this block.
	virtual u256 gasLimitRemaining() const override { return m_postSeal.gasLimitRemaining(); }
	/// Get the gas bid price
	virtual u256 gasBidPrice() const override { return m_gp->bid(); }

	// [PRIVATE API - only relevant for base clients, not available in general]
	/// Get the block.
	dev::eth::Block block(h256 const& _blockHash, PopulationStatistics* o_stats) const;
	/// Get the state of the given block part way through execution, immediately before transaction
	/// index @a _txi.
	dev::eth::State state(unsigned _txi, h256 const& _block) const;
	/// Get the state of the currently pending block part way through execution, immediately before
	/// transaction index @a _txi.
	dev::eth::State state(unsigned _txi) const;

	/// Get the object representing the current state of Ethereum.
	dev::eth::Block postState() const { ReadGuard l(x_postSeal); return m_postSeal; }
	/// Get the object representing the current canonical blockchain.
	BlockChain const& blockChain() const { return bc(); }
	/// Get some information on the block queue.
	BlockQueueStatus blockQueueStatus() const { return m_bq.status(); }
	/// Get some information on the block syncing.
	SyncStatus syncStatus() const override;
	/// Get the block queue.
	BlockQueue const& blockQueue() const { return m_bq; }
	/// Get the block queue.
	OverlayDB const& stateDB() const { return m_stateDB; }
	/// Get some information on the transaction queue.
	TransactionQueue::Status transactionQueueStatus() const { return m_tq.status(); }
	TransactionQueue::Limits transactionQueueLimits() const { return m_tq.limits(); }

	/// Freeze worker thread and sync some of the block queue.
	std::tuple<ImportRoute, bool, unsigned> syncQueue(unsigned _max = 1);

	// Sealing stuff:
	// Note: "mining"/"miner" is deprecated. Use "sealing"/"sealer".

	virtual Address author() const override { ReadGuard l(x_preSeal); return m_preSeal.author(); }
	virtual void setAuthor(Address const& _us) override { WriteGuard l(x_preSeal); m_preSeal.setAuthor(_us); }

	/// Type of sealers available for this seal engine.
	strings sealers() const { return sealEngine()->sealers(); }
	/// Current sealer in use.
	std::string sealer() const { return sealEngine()->sealer(); }
	/// Change sealer.
	void setSealer(std::string const& _id) { sealEngine()->setSealer(_id); if (wouldSeal()) startSealing(); }
	/// Review option for the sealer.
	bytes sealOption(std::string const& _name) const { return sealEngine()->option(_name); }
	/// Set option for the sealer.
	bool setSealOption(std::string const& _name, bytes const& _value) { auto ret = sealEngine()->setOption(_name, _value); if (wouldSeal()) startSealing(); return ret; }

	/// Start sealing.
	void startSealing() override;
	/// Stop sealing.
	void stopSealing() override { m_wouldSeal = false; }
	/// Are we sealing now?
	bool wouldSeal() const override { return m_wouldSeal; }

	/// Are we updating the chain (syncing or importing a new block)?
	bool isSyncing() const override;
	/// Are we syncing the chain?
	bool isMajorSyncing() const override;

	/// Gets the network id.
	u256 networkId() const override;
	/// Sets the network id.
	void setNetworkId(u256 const& _n) override;

	/// Get the seal engine.
	SealEngineFace* sealEngine() const override { return bc().sealEngine(); }

	// Debug stuff:

	DownloadMan const* downloadMan() const;
	/// Clears pending transactions. Just for debug use.
	void clearPending();
	/// Kills the blockchain. Just for debug use.
	void killChain() { reopenChain(WithExisting::Kill); }
	/// Reloads the blockchain. Just for debug use.
	void reopenChain(ChainParams const& _p, WithExisting _we = WithExisting::Trust);
	void reopenChain(WithExisting _we);
	/// Retries all blocks with unknown parents.
	void retryUnknown() { m_bq.retryAllUnknown(); }
	/// Get a report of activity.
	ActivityReport activityReport() { ActivityReport ret; std::swap(m_report, ret); return ret; }
	/// Set the extra data that goes into sealed blocks.
	void setExtraData(bytes const& _extraData) { m_extraData = _extraData; }
	/// Rewind to a prior head.
	void rewind(unsigned _n);
	/// Rescue the chain.
	void rescue() { bc().rescue(m_stateDB); }

	/// Queues a function to be executed in the main thread (that owns the blockchain, etc).
	void executeInMainThread(std::function<void()> const& _function);

	virtual Block block(h256 const& _block) const override;
	using ClientBase::block;

protected:
	/// Perform critical setup functions.
	/// Must be called in the constructor of the finally derived class.
	void init(p2p::Host* _extNet, std::string const& _dbPath, WithExisting _forceAction, u256 _networkId);

	/// InterfaceStub methods
	BlockChain& bc() override { return m_bc; }
	BlockChain const& bc() const override { return m_bc; }

	/// Returns the state object for the full block (i.e. the terminal state) for index _h.
	/// Works properly with LatestBlock and PendingBlock.
	virtual Block preSeal() const override { ReadGuard l(x_preSeal); return m_preSeal; }
	virtual Block postSeal() const override { ReadGuard l(x_postSeal); return m_postSeal; }
	virtual void prepareForTransaction() override;

	/// Collate the changed filters for the bloom filter of the given pending transaction.
	/// Insert any filters that are activated into @a o_changed.
	void appendFromNewPending(TransactionReceipt const& _receipt, h256Hash& io_changed, h256 _sha3);

	/// Collate the changed filters for the hash of the given block.
	/// Insert any filters that are activated into @a o_changed.
	void appendFromBlock(h256 const& _blockHash, BlockPolarity _polarity, h256Hash& io_changed);

	/// Record that the set of filters @a _filters have changed.
	/// This doesn't actually make any callbacks, but incrememnts some counters in m_watches.
	void noteChanged(h256Hash const& _filters);

	/// Submit
	virtual bool submitSealed(bytes const& _s);

protected:
	/// Called when Worker is starting.
	void startedWorking() override;

	/// Do some work. Handles blockchain maintenance and sealing.
	void doWork(bool _doWait);
	void doWork() override { doWork(true); }

	/// Called when Worker is exiting.
	void doneWorking() override;

	/// Called when wouldSeal(), pendingTransactions() have changed.
	void rejigSealing();

	/// Called on chain changes
	void onDeadBlocks(h256s const& _blocks, h256Hash& io_changed);

	/// Called on chain changes
	virtual void onNewBlocks(h256s const& _blocks, h256Hash& io_changed);

	/// Called after processing blocks by onChainChanged(_ir)
	void resyncStateFromChain();

	/// Clear working state of transactions
	void resetState();

	/// Magically called when the chain has changed. An import route is provided.
	/// Called by either submitWork() or in our main thread through syncBlockQueue().
	void onChainChanged(ImportRoute const& _ir);

	/// Signal handler for when the block queue needs processing.
	void syncBlockQueue();

	/// Signal handler for when the block queue needs processing.
	void syncTransactionQueue();

	/// Magically called when m_tq needs syncing. Be nice and don't block.
	void onTransactionQueueReady() { m_syncTransactionQueue = true; m_signalled.notify_all(); }

	/// Magically called when m_bq needs syncing. Be nice and don't block.
	void onBlockQueueReady() { m_syncBlockQueue = true; m_signalled.notify_all(); }

	/// Called when the post state has changed (i.e. when more transactions are in it or we're sealing on a new block).
	/// This updates m_sealingInfo.
	void onPostStateChanged();

	/// Does garbage collection on watches.
	void checkWatchGarbage();

	/// Ticks various system-level objects.
	void tick();

	/// Called when we have attempted to import a bad block.
	/// @warning May be called from any thread.
	void onBadBlock(Exception& _ex) const;

	/// Executes the pending functions in m_functionQueue
	void callQueuedFunctions();

	BlockChain m_bc;						///< Maintains block database and owns the seal engine.
	BlockQueue m_bq;						///< Maintains a list of incoming blocks not yet on the blockchain (to be imported).
	std::shared_ptr<GasPricer> m_gp;		///< The gas pricer.

	OverlayDB m_stateDB;					///< Acts as the central point for the state database, so multiple States can share it.
	mutable SharedMutex x_preSeal;			///< Lock on m_preSeal.
	Block m_preSeal;						///< The present state of the client.
	mutable SharedMutex x_postSeal;			///< Lock on m_postSeal.
	Block m_postSeal;						///< The state of the client which we're sealing (i.e. it'll have all the rewards added).
	mutable SharedMutex x_working;			///< Lock on m_working.
	Block m_working;						///< The state of the client which we're sealing (i.e. it'll have all the rewards added), while we're actually working on it.
	BlockHeader m_sealingInfo;				///< The header we're attempting to seal on (derived from m_postSeal).
	bool remoteActive() const;				///< Is there an active and valid remote worker?
	bool m_remoteWorking = false;			///< Has the remote worker recently been reset?
	std::atomic<bool> m_needStateReset = { false };			///< Need reset working state to premin on next sync
	std::chrono::system_clock::time_point m_lastGetWork;	///< Is there an active and valid remote worker?

	std::weak_ptr<EthereumHost> m_host;		///< Our Ethereum Host. Don't do anything if we can't lock.

	Handler<> m_tqReady;
	Handler<h256 const&> m_tqReplaced;
	Handler<> m_bqReady;

	bool m_wouldSeal = false;				///< True if we /should/ be sealing.
	bool m_wouldButShouldnot = false;		///< True if the last time we called rejigSealing wouldSeal() was true but sealer's shouldSeal() was false.

	mutable std::chrono::system_clock::time_point m_lastGarbageCollection;
											///< When did we last both doing GC on the watches?
	mutable std::chrono::system_clock::time_point m_lastTick = std::chrono::system_clock::now();
											///< When did we last tick()?

	unsigned m_syncAmount = 50;				///< Number of blocks to sync in each go.

	ActivityReport m_report;

	SharedMutex x_functionQueue;
	std::queue<std::function<void()>> m_functionQueue;	///< Functions waiting to be executed in the main thread.

	std::condition_variable m_signalled;
	Mutex x_signalled;
	std::atomic<bool> m_syncTransactionQueue = {false};
	std::atomic<bool> m_syncBlockQueue = {false};

	bytes m_extraData;
};

}
}
