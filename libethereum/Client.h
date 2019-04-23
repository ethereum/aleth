// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include "Block.h"
#include "BlockChain.h"
#include "BlockChainImporter.h"
#include "ClientBase.h"
#include "CommonNet.h"
#include "StateImporter.h"
#include "WarpCapability.h"
#include <libdevcore/Common.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Guards.h>
#include <libdevcore/Worker.h>
#include <libethcore/SealEngine.h>
#include <libp2p/Common.h>
#include <array>
#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include <boost/filesystem/path.hpp>


namespace dev
{
namespace eth
{

class Client;
class DownloadMan;
class EthereumCapability;

enum ClientWorkState
{
    Active = 0,
    Deleting,
    Deleted
};

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
    Client(ChainParams const& _params, int _networkID, p2p::Host& _host,
        std::shared_ptr<GasPricer> _gpForAdoption,
        boost::filesystem::path const& _dbPath = boost::filesystem::path(),
        boost::filesystem::path const& _snapshotPath = boost::filesystem::path(),
        WithExisting _forceAction = WithExisting::Trust,
        TransactionQueue::Limits const& _l = TransactionQueue::Limits{1024, 1024});
    /// Destructor.
    virtual ~Client();

    /// Get information on this chain.
    ChainParams const& chainParams() const { return bc().chainParams(); }

    /// Resets the gas pricer to some other object.
    void setGasPricer(std::shared_ptr<GasPricer> _gp) { m_gp = _gp; }
    std::shared_ptr<GasPricer> gasPricer() const { return m_gp; }

    /// Submits the given transaction.
    /// @returns the new transaction's hash.
    h256 submitTransaction(TransactionSkeleton const& _t, Secret const& _secret) override;
    
    /// Imports the given transaction into the transaction queue
    h256 importTransaction(Transaction const& _t) override;

    /// Makes the given call. Nothing is recorded into the state.
    ExecutionResult call(Address const& _secret, u256 _value, Address _dest, bytes const& _data, u256 _gas, u256 _gasPrice, BlockNumber _blockNumber, FudgeFactor _ff = FudgeFactor::Strict) override;

    /// Blocks until all pending transactions have been processed.
    void flushTransactions() override;

    /// Retrieve pending transactions
    Transactions pending() const override;

    /// Queues a block for import.
    ImportResult queueBlock(bytes const& _block, bool _isSafe = false);

    /// Get the remaining gas limit in this block.
    u256 gasLimitRemaining() const override { return m_postSeal.gasLimitRemaining(); }
    /// Get the gas bid price
    u256 gasBidPrice() const override { return m_gp->bid(); }

    // [PRIVATE API - only relevant for base clients, not available in general]
    /// Get the block.
    dev::eth::Block block(h256 const& _blockHash, PopulationStatistics* o_stats) const;

    /// Get the object representing the current state of Ethereum.
    dev::eth::Block postState() const { ReadGuard l(x_postSeal); return m_postSeal; }
    /// Get the object representing the current canonical blockchain.
    BlockChain const& blockChain() const { return bc(); }
    /// Get some information on the block queue.
    BlockQueueStatus blockQueueStatus() const { return m_bq.status(); }
    /// Get some information on the block syncing.
    SyncStatus syncStatus() const override;
    /// Populate the uninitialized fields in the supplied transaction with default values
    TransactionSkeleton populateTransactionWithDefaults(TransactionSkeleton const& _t) const override;
    /// Get the block queue.
    BlockQueue const& blockQueue() const { return m_bq; }
    /// Get the state database.
    OverlayDB const& stateDB() const { return m_stateDB; }
    /// Get some information on the transaction queue.
    TransactionQueue::Status transactionQueueStatus() const { return m_tq.status(); }
    TransactionQueue::Limits transactionQueueLimits() const { return m_tq.limits(); }

    /// Freeze worker thread and sync some of the block queue.
    std::tuple<ImportRoute, bool, unsigned> syncQueue(unsigned _max = 1);

    // Sealing stuff:
    // Note: "mining"/"miner" is deprecated. Use "sealing"/"sealer".

    Address author() const override { ReadGuard l(x_preSeal); return m_preSeal.author(); }
    void setAuthor(Address const& _us) override
    {
        DEV_WRITE_GUARDED(x_preSeal)
            m_preSeal.setAuthor(_us);
        restartMining();
    }

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

    std::unique_ptr<StateImporterFace> createStateImporter() { return dev::eth::createStateImporter(m_stateDB); }
    std::unique_ptr<BlockChainImporterFace> createBlockChainImporter() { return dev::eth::createBlockChainImporter(m_bc); }

    /// Queues a function to be executed in the main thread (that owns the blockchain, etc).
    void executeInMainThread(std::function<void()> const& _function);

    Block block(h256 const& _block) const override;
    using ClientBase::block;

    /// should be called after the constructor of the most derived class finishes.
    void startWorking() { Worker::startWorking(); };

    /// Change the function that is called when a new block is sealed
    Handler<bytes const&> setOnBlockSealed(std::function<void(bytes const&)> _handler)
    {
        return m_onBlockSealed.add(_handler);
    }
    /// Change the function that is called when blockchain was changed
    Handler<h256s const&, h256s const&> setOnChainChanged(
        std::function<void(h256s const&, h256s const&)> _handler)
    {
        return m_onChainChanged.add(_handler);
    }

    ///< Get POW depending on sealengine it's using
    std::tuple<h256, h256, h256> getWork() override;

protected:
    /// Perform critical setup functions.
    /// Must be called in the constructor of the finally derived class.
    void init(p2p::Host& _extNet, boost::filesystem::path const& _dbPath,
        boost::filesystem::path const& _snapshotPath, WithExisting _forceAction, u256 _networkId);

    /// InterfaceStub methods
    BlockChain& bc() override { return m_bc; }
    BlockChain const& bc() const override { return m_bc; }

    /// Returns the state object for the full block (i.e. the terminal state) for index _h.
    /// Works properly with LatestBlock and PendingBlock.
    Block preSeal() const override { ReadGuard l(x_preSeal); return m_preSeal; }
    Block postSeal() const override { ReadGuard l(x_postSeal); return m_postSeal; }
    void prepareForTransaction() override;

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
    /// Update m_preSeal, m_working, m_postSeal blocks from the latest state of the chain
    void restartMining();

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

    /// @returns true only if it's worth bothering to prep the mining block.
    bool shouldServeWork() const
    {
        return blockQueue().items().first == 0 && (sealEngine()->isMining() || remoteActive());
    }

    ///< Is there an active and valid remote worker?
    bool remoteActive() const;

    BlockChain m_bc;                        ///< Maintains block database and owns the seal engine.
    BlockQueue m_bq;                        ///< Maintains a list of incoming blocks not yet on the blockchain (to be imported).
    TransactionQueue m_tq;                  ///< Maintains a list of incoming transactions not yet in a block on the blockchain.

    std::shared_ptr<GasPricer> m_gp;        ///< The gas pricer.

    OverlayDB m_stateDB;                    ///< Acts as the central point for the state database, so multiple States can share it.
    mutable SharedMutex x_preSeal;          ///< Lock on m_preSeal.
    Block m_preSeal;                        ///< The present state of the client.
    mutable SharedMutex x_postSeal;         ///< Lock on m_postSeal.
    Block m_postSeal;                       ///< The state of the client which we're sealing (i.e. it'll have all the rewards added).
    mutable SharedMutex x_working;          ///< Lock on m_working.
    Block m_working;                        ///< The state of the client which we're sealing (i.e. it'll have all the rewards added), while we're actually working on it.
    BlockHeader m_sealingInfo;              ///< The header we're attempting to seal on (derived from m_postSeal).
    std::atomic<bool> m_remoteWorking = { false };          ///< Has the remote worker recently been reset?
    std::atomic<bool> m_needStateReset = { false };         ///< Need reset working state to premin on next sync
    std::chrono::system_clock::time_point m_lastGetWork;    ///< Is there an active and valid remote worker?

    std::weak_ptr<EthereumCapability> m_host;
    std::weak_ptr<WarpCapability> m_warpHost;

    std::condition_variable m_signalled;
    Mutex x_signalled;

    Handler<> m_tqReady;
    Handler<h256 const&> m_tqReplaced;
    Handler<> m_bqReady;

    std::atomic<bool> m_wouldSeal = { false };               ///< True if we /should/ be sealing.
    bool m_wouldButShouldnot = false;       ///< True if the last time we called rejigSealing wouldSeal() was true but sealer's shouldSeal() was false.

    mutable std::chrono::system_clock::time_point m_lastGarbageCollection;
                                            ///< When did we last both doing GC on the watches?
    mutable std::chrono::system_clock::time_point m_lastTick = std::chrono::system_clock::now();
                                            ///< When did we last tick()?

    unsigned m_syncAmount = 50;             ///< Number of blocks to sync in each go.

    ActivityReport m_report;

    SharedMutex x_functionQueue;
    std::queue<std::function<void()>> m_functionQueue;  ///< Functions waiting to be executed in the main thread.

    std::atomic<bool> m_syncTransactionQueue = {false};
    std::atomic<bool> m_syncBlockQueue = {false};

    Timer m_timeSinceLastTxQueueSync;
    SharedMutex x_timeSinceLastTxQueueSync;

    Timer m_timeSinceLastBqSync;
    SharedMutex x_timeSinceLastBqSync;

    Timer m_timeSinceLastBqSyncLog;
    SharedMutex x_timeSinceLastBqSyncLog;

    bytes m_extraData;

    Signal<bytes const&> m_onBlockSealed;        ///< Called if we have sealed a new block

    /// Called when blockchain was changed
    Signal<h256s const&, h256s const&> m_onChainChanged;

    Logger m_logger{createLogger(VerbosityInfo, "client")};
    Logger m_loggerDetail{createLogger(VerbosityDebug, "client")};
};

}
}
