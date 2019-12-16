// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#include <libdevcore/CommonJS.h>
#include <libethashseal/Ethash.h>
#include <libethereum/ClientTest.h>
#include <libethereum/EthereumCapability.h>
#include <boost/filesystem/path.hpp>
#include <future>

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace p2p;
namespace fs = boost::filesystem;

ClientTest& dev::eth::asClientTest(Interface& _c)
{
    return dynamic_cast<ClientTest&>(_c);
}

ClientTest* dev::eth::asClientTest(Interface* _c)
{
    return &dynamic_cast<ClientTest&>(*_c);
}

ClientTest::ClientTest(ChainParams const& _params, int _networkID, p2p::Host& _host,
    std::shared_ptr<GasPricer> _gpForAdoption, fs::path const& _dbPath, WithExisting _forceAction,
    TransactionQueue::Limits const& _limits)
  : Client(
        _params, _networkID, _host, _gpForAdoption, _dbPath, std::string(), _forceAction, _limits)
{
    m_bq.setOnBad([this](Exception& _ex) { onBadBlock(_ex); });
    bc().setOnBad([this](Exception& _ex) { onBadBlock(_ex); });
}

ClientTest::~ClientTest()
{
    m_signalled.notify_all(); // to wake up the thread from Client::doWork()
    terminate();
}

void ClientTest::onBadBlock(Exception& _ex)
{
    {
        Guard guard(m_badBlockMutex);
        // To preserve original exception type we need to use current_exception().
        // This assumes we're inside catch.
        m_lastImportError = boost::current_exception();
        bytes const* block = boost::get_error_info<errinfo_block>(_ex);
        m_lastBadBlock = block ? *block : bytes{};
    }

    Client::onBadBlock(_ex);
}

void ClientTest::setChainParams(string const& _genesis)
{
    try
    {
        ChainParams const params{_genesis};
        if (params.sealEngineName != NoProof::name() && params.sealEngineName != Ethash::name() &&
            params.sealEngineName != NoReward::name())
            BOOST_THROW_EXCEPTION(
                ChainParamsInvalid() << errinfo_comment("Seal engine is not supported!"));

        reopenChain(params, WithExisting::Kill);
    }
    catch (std::exception const& ex)
    {
        BOOST_THROW_EXCEPTION(ChainParamsInvalid() << errinfo_comment(ex.what()));
    }
}

void ClientTest::modifyTimestamp(int64_t _timestamp)
{
    Block block(chainParams().accountStartNonce);
    DEV_READ_GUARDED(x_preSeal)
        block = m_preSeal;

    Transactions transactions;
    DEV_READ_GUARDED(x_postSeal)
        transactions = m_postSeal.pending();
    block.resetCurrent(_timestamp);

    DEV_WRITE_GUARDED(x_preSeal)
        m_preSeal = block;

    auto& lastHashes = bc().lastBlockHashes();
    assert(bc().currentHash() == block.info().parentHash());
    for (auto const& t: transactions)
        block.execute(lastHashes, t);

    DEV_WRITE_GUARDED(x_working)
        m_working = block;
    DEV_READ_GUARDED(x_postSeal)
        m_postSeal = block;

    onPostStateChanged();
}

bool ClientTest::mineBlocks(unsigned _count) noexcept
{
    if (wouldSeal())
        return false;
    try
    {
        unsigned sealedBlocks = 0;
        auto sealHandler = setOnBlockSealed([this, _count, &sealedBlocks](bytes const&) {
            if (++sealedBlocks == _count)
                stopSealing();
        });

        std::promise<void> allBlocksImported;
        unsigned importedBlocks = 0;
        auto chainChangedHandler = setOnChainChanged(
            [_count, &importedBlocks, &allBlocksImported](h256s const&, h256s const& _newBlocks) {
                importedBlocks += _newBlocks.size();
                if (importedBlocks == _count)
                    allBlocksImported.set_value();
            });

        startSealing();
        future_status ret = allBlocksImported.get_future().wait_for(
            std::chrono::seconds(m_singleBlockMaxMiningTimeInSeconds * _count));
        return (ret == future_status::ready);
    }
    catch (std::exception const&)
    {
        LOG(m_logger) << boost::current_exception_diagnostic_information();
        return false;
    }
}

bool ClientTest::completeSync()
{
    auto h = m_host.lock();
    if (!h)
        return false;

    h->completeSync();
    return true;
}

h256 ClientTest::importRawBlock(const string& _blockRLP)
{
    bytes blockBytes = jsToBytes(_blockRLP, OnFailed::Throw);
    h256 blockHash = BlockHeader::headerHashFromBlock(blockBytes);

    ImportResult result = queueBlock(blockBytes, true);
    if (result != ImportResult::Success)
    {
        auto ex = ImportBlockFailed{} << errinfo_importResult(result);
        addNestedBadBlockException(blockBytes, ex);
        BOOST_THROW_EXCEPTION(ex);
    }

    if (auto h = m_host.lock())
        h->noteNewBlocks();

    bool moreToImport = true;
    while (moreToImport)
    {
        tie(ignore, moreToImport, ignore) = syncQueue(100000);
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    // check that it was really imported and not rejected as invalid
    if (!bc().isKnown(blockHash))
    {
        auto ex = ImportBlockFailed{};
        addNestedBadBlockException(blockBytes, ex);
        BOOST_THROW_EXCEPTION(ex);
    }

    return blockHash;
}

void ClientTest::addNestedBadBlockException(bytes const& _blockBytes, Exception& io_ex)
{
    Guard guard(m_badBlockMutex);
    if (_blockBytes == m_lastBadBlock)
        io_ex << errinfo_nestedException(m_lastImportError);
}
