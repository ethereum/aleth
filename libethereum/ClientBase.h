// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#pragma once

#include <chrono>
#include "Interface.h"
#include "LogFilter.h"
#include "TransactionQueue.h"
#include "Block.h"
#include "CommonNet.h"

namespace dev
{

namespace eth
{

struct InstalledFilter
{
    InstalledFilter(LogFilter const& _f): filter(_f) {}

    LogFilter filter;
    unsigned refCount = 1;
    LocalisedLogEntries changes;
};

static const h256 PendingChangedFilter = u256(0);
static const h256 ChainChangedFilter = u256(1);

static const LogEntry SpecialLogEntry = LogEntry(Address(), h256s(), bytes());
static const LocalisedLogEntry InitialChange(SpecialLogEntry);

struct ClientWatch
{
    ClientWatch(): lastPoll(std::chrono::system_clock::now()) {}
    explicit ClientWatch(h256 _id, Reaping _r): id(_id), lastPoll(_r == Reaping::Automatic ? std::chrono::system_clock::now() : std::chrono::system_clock::time_point::max()) {}

    h256 id;
#if INITIAL_STATE_AS_CHANGES
    LocalisedLogEntries changes = LocalisedLogEntries{ InitialChange };
#else
    LocalisedLogEntries changes;
#endif
    mutable std::chrono::system_clock::time_point lastPoll = std::chrono::system_clock::now();
};

class ClientBase: public Interface
{
public:
    ClientBase() {}
    virtual ~ClientBase() {}

    /// Estimate gas usage for call/create.
    /// @param _maxGas An upper bound value for estimation, if not provided default value of c_maxGasEstimate will be used.
    /// @param _callback Optional callback function for progress reporting
    std::pair<u256, ExecutionResult> estimateGas(Address const& _from, u256 _value, Address _dest, bytes const& _data, int64_t _maxGas, u256 _gasPrice, BlockNumber _blockNumber, GasEstimationCallback const& _callback) override;

    using Interface::balanceAt;
    using Interface::countAt;
    using Interface::stateAt;
    using Interface::codeAt;
    using Interface::codeHashAt;
    using Interface::storageAt;

    u256 balanceAt(Address _a, BlockNumber _block) const override;
    u256 countAt(Address _a, BlockNumber _block) const override;
    u256 stateAt(Address _a, u256 _l, BlockNumber _block) const override;
    h256 stateRootAt(Address _a, BlockNumber _block) const override;
    bytes codeAt(Address _a, BlockNumber _block) const override;
    h256 codeHashAt(Address _a, BlockNumber _block) const override;
    std::map<h256, std::pair<u256, u256>> storageAt(Address _a, BlockNumber _block) const override;

    LocalisedLogEntries logs(unsigned _watchId) const override;
    LocalisedLogEntries logs(LogFilter const& _filter) const override;
    virtual void prependLogsFromBlock(LogFilter const& _filter, h256 const& _blockHash, BlockPolarity _polarity, LocalisedLogEntries& io_logs) const;

    /// Install, uninstall and query watches.
    unsigned installWatch(LogFilter const& _filter, Reaping _r = Reaping::Automatic) override;
    unsigned installWatch(h256 _filterId, Reaping _r = Reaping::Automatic) override;
    bool uninstallWatch(unsigned _watchId) override;
    LocalisedLogEntries peekWatch(unsigned _watchId) const override;
    LocalisedLogEntries checkWatch(unsigned _watchId) override;

    h256 hashFromNumber(BlockNumber _number) const override;
    BlockNumber numberFromHash(h256 _blockHash) const override;
    int compareBlockHashes(h256 _h1, h256 _h2) const override;
    BlockHeader blockInfo(h256 _hash) const override;
    BlockDetails blockDetails(h256 _hash) const override;
    Transaction transaction(h256 _transactionHash) const override;
    LocalisedTransaction localisedTransaction(h256 const& _transactionHash) const override;
    Transaction transaction(h256 _blockHash, unsigned _i) const override;
    LocalisedTransaction localisedTransaction(h256 const& _blockHash, unsigned _i) const override;
    TransactionReceipt transactionReceipt(h256 const& _transactionHash) const override;
    LocalisedTransactionReceipt localisedTransactionReceipt(h256 const& _transactionHash) const override;
    std::pair<h256, unsigned> transactionLocation(h256 const& _transactionHash) const override;
    Transactions transactions(h256 _blockHash) const override;
    Transactions transactions(BlockNumber _block) const override { if (_block == PendingBlock) return postSeal().pending(); return transactions(hashFromNumber(_block)); }
    TransactionHashes transactionHashes(h256 _blockHash) const override;
    BlockHeader uncle(h256 _blockHash, unsigned _i) const override;
    UncleHashes uncleHashes(h256 _blockHash) const override;
    unsigned transactionCount(h256 _blockHash) const override;
    unsigned transactionCount(BlockNumber _block) const override { if (_block == PendingBlock) { auto p = postSeal().pending(); return p.size(); } return transactionCount(hashFromNumber(_block)); }
    unsigned uncleCount(h256 _blockHash) const override;
    unsigned number() const override;
    h256s pendingHashes() const override;
    BlockHeader pendingInfo() const override;
    BlockDetails pendingDetails() const override;

    EVMSchedule evmSchedule() const override { return sealEngine()->evmSchedule(pendingInfo().number()); }

    ImportResult injectBlock(bytes const& _block) override;

    using Interface::addresses;
    Addresses addresses(BlockNumber _block) const override;
    u256 gasLimitRemaining() const override;
    u256 gasBidPrice() const override { return DefaultGasPrice; }

    /// Get the block author
    Address author() const override;

    bool isKnown(h256 const& _hash) const override;
    bool isKnown(BlockNumber _block) const override;
    bool isKnownTransaction(h256 const& _transactionHash) const override;
    bool isKnownTransaction(h256 const& _blockHash, unsigned _i) const override;

    void startSealing() override
    {
        BOOST_THROW_EXCEPTION(
            InterfaceNotSupported() << errinfo_interface("ClientBase::startSealing"));
    }
    void stopSealing() override
    {
        BOOST_THROW_EXCEPTION(
            InterfaceNotSupported() << errinfo_interface("ClientBase::stopSealing"));
    }
    bool wouldSeal() const override
    {
        BOOST_THROW_EXCEPTION(
            InterfaceNotSupported() << errinfo_interface("ClientBase::wouldSeal"));
    }

    SyncStatus syncStatus() const override
    {
        BOOST_THROW_EXCEPTION(
            InterfaceNotSupported() << errinfo_interface("ClientBase::syncStatus"));
    }

    Block blockByNumber(BlockNumber _h) const;

    int chainId() const override;

protected:
    /// The interface that must be implemented in any class deriving this.
    /// {
    virtual BlockChain& bc() = 0;
    virtual BlockChain const& bc() const = 0;
    virtual Block block(h256 const& _h) const = 0;
    virtual Block preSeal() const = 0;
    virtual Block postSeal() const = 0;
    virtual void prepareForTransaction() = 0;
    /// }

    // filters
    mutable Mutex x_filtersWatches;							///< Our lock.
    std::unordered_map<h256, InstalledFilter> m_filters;	///< The dictionary of filters that are active.
    std::unordered_map<h256, h256s> m_specialFilters = std::unordered_map<h256, std::vector<h256>>{{PendingChangedFilter, {}}, {ChainChangedFilter, {}}};
                                                            ///< The dictionary of special filters and their additional data
    std::map<unsigned, ClientWatch> m_watches;				///< Each and every watch - these reference a filter.

    Logger m_loggerWatch{createLogger(VerbosityDebug, "watch")};
};

}}
