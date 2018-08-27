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
/** @file Block.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "Block.h"

#include <ctime>
#include <boost/filesystem.hpp>
#include <boost/timer.hpp>
#include <libdevcore/CommonIO.h>
#include <libdevcore/Assertions.h>
#include <libdevcore/TrieHash.h>
#include <libethcore/Exceptions.h>
#include <libethcore/SealEngine.h>
#include <libevm/VMFactory.h>
#include "BlockChain.h"
#include "Defaults.h"
#include "ExtVM.h"
#include "Executive.h"
#include "TransactionQueue.h"
#include "GenesisInfo.h"
using namespace std;
using namespace dev;
using namespace dev::eth;
namespace fs = boost::filesystem;

#define ETH_TIMED_ENACTMENTS 0

static const unsigned c_maxSyncTransactions = 1024;

namespace
{

class DummyLastBlockHashes: public eth::LastBlockHashesFace
{
public:
    h256s precedingHashes(h256 const& /* _mostRecentHash */) const override { return {}; }
    void clear() override {}
};

}


Block::Block(BlockChain const& _bc, OverlayDB const& _db, BaseState _bs, Address const& _author):
    m_state(Invalid256, _db, _bs),
    m_precommit(Invalid256),
    m_author(_author)
{
    noteChain(_bc);
    m_previousBlock.clear();
    m_currentBlock.clear();
//	assert(m_state.root() == m_previousBlock.stateRoot());
}

Block::Block(BlockChain const& _bc, OverlayDB const& _db, h256 const& _root, Address const& _author):
    m_state(Invalid256, _db, BaseState::PreExisting),
    m_precommit(Invalid256),
    m_author(_author)
{
    noteChain(_bc);
    m_state.setRoot(_root);
    m_previousBlock.clear();
    m_currentBlock.clear();
//	assert(m_state.root() == m_previousBlock.stateRoot());
}

Block::Block(Block const& _s):
    m_state(_s.m_state),
    m_transactions(_s.m_transactions),
    m_receipts(_s.m_receipts),
    m_transactionSet(_s.m_transactionSet),
    m_precommit(_s.m_state),
    m_previousBlock(_s.m_previousBlock),
    m_currentBlock(_s.m_currentBlock),
    m_currentBytes(_s.m_currentBytes),
    m_author(_s.m_author),
    m_sealEngine(_s.m_sealEngine)
{
    m_committedToSeal = false;
}

Block& Block::operator=(Block const& _s)
{
    if (&_s == this)
        return *this;

    m_state = _s.m_state;
    m_transactions = _s.m_transactions;
    m_receipts = _s.m_receipts;
    m_transactionSet = _s.m_transactionSet;
    m_previousBlock = _s.m_previousBlock;
    m_currentBlock = _s.m_currentBlock;
    m_currentBytes = _s.m_currentBytes;
    m_author = _s.m_author;
    m_sealEngine = _s.m_sealEngine;

    m_precommit = m_state;
    m_committedToSeal = false;
    return *this;
}

void Block::resetCurrent(int64_t _timestamp)
{
    m_transactions.clear();
    m_receipts.clear();
    m_transactionSet.clear();
    m_currentBlock = BlockHeader();
    m_currentBlock.setAuthor(m_author);
    m_currentBlock.setTimestamp(max(m_previousBlock.timestamp() + 1, _timestamp));
    m_currentBytes.clear();
    sealEngine()->populateFromParent(m_currentBlock, m_previousBlock);

    // TODO: check.

    m_state.setRoot(m_previousBlock.stateRoot());
    m_precommit = m_state;
    m_committedToSeal = false;

    performIrregularModifications();
    updateBlockhashContract();
}

SealEngineFace* Block::sealEngine() const
{
    if (!m_sealEngine)
        BOOST_THROW_EXCEPTION(ChainOperationWithUnknownBlockChain());
    return m_sealEngine;
}

void Block::noteChain(BlockChain const& _bc)
{
    if (!m_sealEngine)
    {
        m_state.noteAccountStartNonce(_bc.chainParams().accountStartNonce);
        m_precommit.noteAccountStartNonce(_bc.chainParams().accountStartNonce);
        m_sealEngine = _bc.sealEngine();
    }
}

PopulationStatistics Block::populateFromChain(BlockChain const& _bc, h256 const& _h, ImportRequirements::value _ir)
{
    noteChain(_bc);

    PopulationStatistics ret { 0.0, 0.0 };

    if (!_bc.isKnown(_h))
    {
        // Might be worth throwing here.
        cwarn << "Invalid block given for state population: " << _h;
        BOOST_THROW_EXCEPTION(BlockNotFound() << errinfo_target(_h));
    }

    auto b = _bc.block(_h);
    BlockHeader bi(b);		// No need to check - it's already in the DB.
    if (bi.number())
    {
        // Non-genesis:

        // 1. Start at parent's end state (state root).
        BlockHeader bip(_bc.block(bi.parentHash()));
        sync(_bc, bi.parentHash(), bip);

        // 2. Enact the block's transactions onto this state.
        m_author = bi.author();
        Timer t;
        auto vb = _bc.verifyBlock(&b, function<void(Exception&)>(), _ir | ImportRequirements::TransactionBasic);
        ret.verify = t.elapsed();
        t.restart();
        enact(vb, _bc);
        ret.enact = t.elapsed();
    }
    else
    {
        // Genesis required:
        // We know there are no transactions, so just populate directly.
        m_state = State(m_state.accountStartNonce(), m_state.db(), BaseState::Empty);	// TODO: try with PreExisting.
        sync(_bc, _h, bi);
    }

    return ret;
}

bool Block::sync(BlockChain const& _bc)
{
    return sync(_bc, _bc.currentHash());
}

bool Block::sync(BlockChain const& _bc, h256 const& _block, BlockHeader const& _bi)
{
    noteChain(_bc);

    bool ret = false;
    // BLOCK
    BlockHeader bi = _bi ? _bi : _bc.info(_block);
#if ETH_PARANOIA
    if (!bi)
        while (1)
        {
            try
            {
                auto b = _bc.block(_block);
                bi.populate(b);
                break;
            }
            catch (Exception const& _e)
            {
                // TODO: Slightly nicer handling? :-)
                cerr << "ERROR: Corrupt block-chain! Delete your block-chain DB and restart." << endl;
                cerr << diagnostic_information(_e) << endl;
            }
            catch (std::exception const& _e)
            {
                // TODO: Slightly nicer handling? :-)
                cerr << "ERROR: Corrupt block-chain! Delete your block-chain DB and restart." << endl;
                cerr << _e.what() << endl;
            }
        }
#endif
    if (bi == m_currentBlock)
    {
        // We mined the last block.
        // Our state is good - we just need to move on to next.
        m_previousBlock = m_currentBlock;
        resetCurrent();
        ret = true;
    }
    else if (bi == m_previousBlock)
    {
        // No change since last sync.
        // Carry on as we were.
    }
    else
    {
        // New blocks available, or we've switched to a different branch. All change.
        // Find most recent state dump and replay what's left.
        // (Most recent state dump might end up being genesis.)

        if (m_state.db().lookup(bi.stateRoot()).empty())	// TODO: API in State for this?
        {
            cwarn << "Unable to sync to" << bi.hash() << "; state root" << bi.stateRoot() << "not found in database.";
            cwarn << "Database corrupt: contains block without stateRoot:" << bi;
            cwarn << "Try rescuing the database by running: eth --rescue";
            BOOST_THROW_EXCEPTION(InvalidStateRoot() << errinfo_target(bi.stateRoot()));
        }
        m_previousBlock = bi;
        resetCurrent();
        ret = true;
    }
#if ALLOW_REBUILD
    else
    {
        // New blocks available, or we've switched to a different branch. All change.
        // Find most recent state dump and replay what's left.
        // (Most recent state dump might end up being genesis.)

        std::vector<h256> chain;
        while (bi.number() != 0 && m_db.lookup(bi.stateRoot()).empty())	// while we don't have the state root of the latest block...
        {
            chain.push_back(bi.hash());				// push back for later replay.
            bi.populate(_bc.block(bi.parentHash()));	// move to parent.
        }

        m_previousBlock = bi;
        resetCurrent();

        // Iterate through in reverse, playing back each of the blocks.
        try
        {
            for (auto it = chain.rbegin(); it != chain.rend(); ++it)
            {
                auto b = _bc.block(*it);
                enact(&b, _bc, _ir);
                cleanup(true);
            }
        }
        catch (...)
        {
            // TODO: Slightly nicer handling? :-)
            cerr << "ERROR: Corrupt block-chain! Delete your block-chain DB and restart." << endl;
            cerr << boost::current_exception_diagnostic_information() << endl;
            exit(1);
        }

        resetCurrent();
        ret = true;
    }
#endif
    return ret;
}

pair<TransactionReceipts, bool> Block::sync(BlockChain const& _bc, TransactionQueue& _tq, GasPricer const& _gp, unsigned msTimeout)
{
    if (isSealed())
        BOOST_THROW_EXCEPTION(InvalidOperationOnSealedBlock());

    noteChain(_bc);

    // TRANSACTIONS
    pair<TransactionReceipts, bool> ret;

    Transactions transactions = _tq.topTransactions(c_maxSyncTransactions, m_transactionSet);
    ret.second = (transactions.size() == c_maxSyncTransactions);  // say there's more to the caller
                                                                  // if we hit the limit

    assert(_bc.currentHash() == m_currentBlock.parentHash());
    auto deadline =  chrono::steady_clock::now() + chrono::milliseconds(msTimeout);

    for (int goodTxs = max(0, (int)transactions.size() - 1); goodTxs < (int)transactions.size();)
    {
        goodTxs = 0;
        for (auto const& t : transactions)
            if (!m_transactionSet.count(t.sha3()))
            {
                try
                {
                    if (t.gasPrice() >= _gp.ask(*this))
                    {
//						Timer t;
                        execute(_bc.lastBlockHashes(), t);
                        ret.first.push_back(m_receipts.back());
                        ++goodTxs;
//						cnote << "TX took:" << t.elapsed() * 1000;
                    }
                    else if (t.gasPrice() < _gp.ask(*this) * 9 / 10)
                    {
                        LOG(m_logger)
                            << t.sha3() << " Dropping El Cheapo transaction (<90% of ask price)";
                        _tq.drop(t.sha3());
                    }
                }
                catch (InvalidNonce const& in)
                {
                    bigint const& req = *boost::get_error_info<errinfo_required>(in);
                    bigint const& got = *boost::get_error_info<errinfo_got>(in);

                    if (req > got)
                    {
                        // too old
                        LOG(m_logger) << t.sha3() << " Dropping old transaction (nonce too low)";
                        _tq.drop(t.sha3());
                    }
                    else if (got > req + _tq.waiting(t.sender()))
                    {
                        // too new
                        LOG(m_logger)
                            << t.sha3() << " Dropping new transaction (too many nonces ahead)";
                        _tq.drop(t.sha3());
                    }
                    else
                        _tq.setFuture(t.sha3());
                }
                catch (BlockGasLimitReached const& e)
                {
                    bigint const& got = *boost::get_error_info<errinfo_got>(e);
                    if (got > m_currentBlock.gasLimit())
                    {
                        LOG(m_logger)
                            << t.sha3()
                            << " Dropping over-gassy transaction (gas > block's gas limit)";
                        LOG(m_logger)
                            << "got: " << got << " required: " << m_currentBlock.gasLimit();
                        _tq.drop(t.sha3());
                    }
                    else
                    {
                        LOG(m_logger) << t.sha3()
                                      << " Temporarily no gas left in current block (txs gas > "
                                         "block's gas limit)";
                        //_tq.drop(t.sha3());
                        // Temporarily no gas left in current block.
                        // OPTIMISE: could note this and then we don't evaluate until a block that does have the gas left.
                        // for now, just leave alone.
                    }
                }
                catch (Exception const& _e)
                {
                    // Something else went wrong - drop it.
                    LOG(m_logger) << t.sha3() << " Dropping invalid transaction: "
                                  << diagnostic_information(_e);
                    _tq.drop(t.sha3());
                }
                catch (std::exception const&)
                {
                    // Something else went wrong - drop it.
                    _tq.drop(t.sha3());
                    cwarn << t.sha3() << "Transaction caused low-level exception :(";
                }
            }
        if (chrono::steady_clock::now() > deadline)
        {
            ret.second = true;	// say there's more to the caller if we ended up crossing the deadline.
            break;
        }
    }
    return ret;
}

u256 Block::enactOn(VerifiedBlockRef const& _block, BlockChain const& _bc)
{
    noteChain(_bc);

#if ETH_TIMED_ENACTMENTS
    Timer t;
    double populateVerify;
    double populateGrand;
    double syncReset;
    double enactment;
#endif

    // Check family:
    BlockHeader biParent = _bc.info(_block.info.parentHash());
    _block.info.verify(CheckNothingNew/*CheckParent*/, biParent);

#if ETH_TIMED_ENACTMENTS
    populateVerify = t.elapsed();
    t.restart();
#endif

    BlockHeader biGrandParent;
    if (biParent.number())
        biGrandParent = _bc.info(biParent.parentHash());

#if ETH_TIMED_ENACTMENTS
    populateGrand = t.elapsed();
    t.restart();
#endif

    sync(_bc, _block.info.parentHash(), BlockHeader());
    resetCurrent();

#if ETH_TIMED_ENACTMENTS
    syncReset = t.elapsed();
    t.restart();
#endif

    m_previousBlock = biParent;
    auto ret = enact(_block, _bc);

#if ETH_TIMED_ENACTMENTS
    enactment = t.elapsed();
    if (populateVerify + populateGrand + syncReset + enactment > 0.5)
        LOG(m_logger) << "popVer/popGrand/syncReset/enactment = " << populateVerify << " / "
                      << populateGrand << " / " << syncReset << " / " << enactment;
#endif
    return ret;
}

u256 Block::enact(VerifiedBlockRef const& _block, BlockChain const& _bc)
{
    noteChain(_bc);

    DEV_TIMED_FUNCTION_ABOVE(500);

    // m_currentBlock is assumed to be prepopulated and reset.
    assert(m_previousBlock.hash() == _block.info.parentHash());
    assert(m_currentBlock.parentHash() == _block.info.parentHash());

    if (m_currentBlock.parentHash() != m_previousBlock.hash())
        // Internal client error.
        BOOST_THROW_EXCEPTION(InvalidParentHash());

    // Populate m_currentBlock with the correct values.
    m_currentBlock.noteDirty();
    m_currentBlock = _block.info;

//	cnote << "playback begins:" << m_currentBlock.hash() << "(without: " << m_currentBlock.hash(WithoutSeal) << ")";
//	cnote << m_state;

    RLP rlp(_block.block);

    vector<bytes> receipts;

    // All ok with the block generally. Play back the transactions now...
    unsigned i = 0;
    DEV_TIMED_ABOVE("txExec", 500)
        for (Transaction const& tr: _block.transactions)
        {
            try
            {
//				cnote << "Enacting transaction: " << tr.nonce() << tr.from() << state().transactionsFrom(tr.from()) << tr.value();
                execute(_bc.lastBlockHashes(), tr);
//				cnote << "Now: " << tr.from() << state().transactionsFrom(tr.from());
//				cnote << m_state;
            }
            catch (Exception& ex)
            {
                ex << errinfo_transactionIndex(i);
                throw;
            }

            RLPStream receiptRLP;
            m_receipts.back().streamRLP(receiptRLP);
            receipts.push_back(receiptRLP.out());
            ++i;
        }

    h256 receiptsRoot;
    DEV_TIMED_ABOVE(".receiptsRoot()", 500)
        receiptsRoot = orderedTrieRoot(receipts);

    if (receiptsRoot != m_currentBlock.receiptsRoot())
    {
        InvalidReceiptsStateRoot ex;
        ex << Hash256RequirementError(m_currentBlock.receiptsRoot(), receiptsRoot);
        ex << errinfo_receipts(receipts);
//		ex << errinfo_vmtrace(vmTrace(_block.block, _bc, ImportRequirements::None));
        BOOST_THROW_EXCEPTION(ex);
    }

    if (m_currentBlock.logBloom() != logBloom())
    {
        InvalidLogBloom ex;
        ex << LogBloomRequirementError(m_currentBlock.logBloom(), logBloom());
        ex << errinfo_receipts(receipts);
        BOOST_THROW_EXCEPTION(ex);
    }

    // Initialise total difficulty calculation.
    u256 tdIncrease = m_currentBlock.difficulty();

    // Check uncles & apply their rewards to state.
    if (rlp[2].itemCount() > 2)
    {
        TooManyUncles ex;
        ex << errinfo_max(2);
        ex << errinfo_got(rlp[2].itemCount());
        BOOST_THROW_EXCEPTION(ex);
    }

    vector<BlockHeader> rewarded;
    h256Hash excluded;
    DEV_TIMED_ABOVE("allKin", 500)
        excluded = _bc.allKinFrom(m_currentBlock.parentHash(), 6);
    excluded.insert(m_currentBlock.hash());

    unsigned ii = 0;
    DEV_TIMED_ABOVE("uncleCheck", 500)
        for (auto const& i: rlp[2])
        {
            try
            {
                auto h = sha3(i.data());
                if (excluded.count(h))
                {
                    UncleInChain ex;
                    ex << errinfo_comment("Uncle in block already mentioned");
                    ex << errinfo_unclesExcluded(excluded);
                    ex << errinfo_hash256(sha3(i.data()));
                    BOOST_THROW_EXCEPTION(ex);
                }
                excluded.insert(h);

                // CheckNothing since it's a VerifiedBlock.
                BlockHeader uncle(i.data(), HeaderData, h);

                BlockHeader uncleParent;
                if (!_bc.isKnown(uncle.parentHash()))
                    BOOST_THROW_EXCEPTION(UnknownParent() << errinfo_hash256(uncle.parentHash()));
                uncleParent = BlockHeader(_bc.block(uncle.parentHash()));

                // m_currentBlock.number() - uncle.number()		m_cB.n - uP.n()
                // 1											2
                // 2
                // 3
                // 4
                // 5
                // 6											7
                //												(8 Invalid)
                bigint depth = (bigint)m_currentBlock.number() - (bigint)uncle.number();
                if (depth > 6)
                {
                    UncleTooOld ex;
                    ex << errinfo_uncleNumber(uncle.number());
                    ex << errinfo_currentNumber(m_currentBlock.number());
                    BOOST_THROW_EXCEPTION(ex);
                }
                else if (depth < 1)
                {
                    UncleIsBrother ex;
                    ex << errinfo_uncleNumber(uncle.number());
                    ex << errinfo_currentNumber(m_currentBlock.number());
                    BOOST_THROW_EXCEPTION(ex);
                }
                // cB
                // cB.p^1	    1 depth, valid uncle
                // cB.p^2	---/  2
                // cB.p^3	-----/  3
                // cB.p^4	-------/  4
                // cB.p^5	---------/  5
                // cB.p^6	-----------/  6
                // cB.p^7	-------------/
                // cB.p^8
                auto expectedUncleParent = _bc.details(m_currentBlock.parentHash()).parent;
                for (unsigned i = 1; i < depth; expectedUncleParent = _bc.details(expectedUncleParent).parent, ++i) {}
                if (expectedUncleParent != uncleParent.hash())
                {
                    UncleParentNotInChain ex;
                    ex << errinfo_uncleNumber(uncle.number());
                    ex << errinfo_currentNumber(m_currentBlock.number());
                    BOOST_THROW_EXCEPTION(ex);
                }
                uncle.verify(CheckNothingNew/*CheckParent*/, uncleParent);

                rewarded.push_back(uncle);
                ++ii;
            }
            catch (Exception& ex)
            {
                ex << errinfo_uncleIndex(ii);
                throw;
            }
        }

    assert(_bc.sealEngine());
    DEV_TIMED_ABOVE("applyRewards", 500)
        applyRewards(rewarded, _bc.sealEngine()->blockReward(m_currentBlock.number()));

    // Commit all cached state changes to the state trie.
    bool removeEmptyAccounts = m_currentBlock.number() >= _bc.chainParams().EIP158ForkBlock; // TODO: use EVMSchedule
    DEV_TIMED_ABOVE("commit", 500)
        m_state.commit(removeEmptyAccounts ? State::CommitBehaviour::RemoveEmptyAccounts : State::CommitBehaviour::KeepEmptyAccounts);

    // Hash the state trie and check against the state_root hash in m_currentBlock.
    if (m_currentBlock.stateRoot() != m_previousBlock.stateRoot() && m_currentBlock.stateRoot() != rootHash())
    {
        auto r = rootHash();
        m_state.db().rollback();		// TODO: API in State for this?
        BOOST_THROW_EXCEPTION(InvalidStateRoot() << Hash256RequirementError(m_currentBlock.stateRoot(), r));
    }

    if (m_currentBlock.gasUsed() != gasUsed())
    {
        // Rollback the trie.
        m_state.db().rollback();		// TODO: API in State for this?
        BOOST_THROW_EXCEPTION(InvalidGasUsed() << RequirementError(bigint(m_currentBlock.gasUsed()), bigint(gasUsed())));
    }

    return tdIncrease;
}

ExecutionResult Block::execute(LastBlockHashesFace const& _lh, Transaction const& _t, Permanence _p, OnOpFunc const& _onOp)
{
    if (isSealed())
        BOOST_THROW_EXCEPTION(InvalidOperationOnSealedBlock());

    // Uncommitting is a non-trivial operation - only do it once we've verified as much of the
    // transaction as possible.
    uncommitToSeal();

    std::pair<ExecutionResult, TransactionReceipt> resultReceipt = m_state.execute(EnvInfo(info(), _lh, gasUsed()), *m_sealEngine, _t, _p, _onOp);

    if (_p == Permanence::Committed)
    {
        // Add to the user-originated transactions that we've executed.
        m_transactions.push_back(_t);
        m_receipts.push_back(resultReceipt.second);
        m_transactionSet.insert(_t.sha3());
    }

    return resultReceipt.first;
}

void Block::applyRewards(vector<BlockHeader> const& _uncleBlockHeaders, u256 const& _blockReward)
{
    u256 r = _blockReward;
    for (auto const& i: _uncleBlockHeaders)
    {
        m_state.addBalance(i.author(), _blockReward * (8 + i.number() - m_currentBlock.number()) / 8);
        r += _blockReward / 32;
    }
    m_state.addBalance(m_currentBlock.author(), r);
}

void Block::performIrregularModifications()
{
    u256 const& daoHardfork = m_sealEngine->chainParams().daoHardforkBlock;
    if (daoHardfork != 0 && info().number() == daoHardfork)
    {
        Address recipient("0xbf4ed7b27f1d666546e30d74d50d173d20bca754");
        Addresses allDAOs = childDaos();
        for (Address const& dao: allDAOs)
            m_state.transferBalance(dao, recipient, m_state.balance(dao));
        m_state.commit(State::CommitBehaviour::KeepEmptyAccounts);
    }
}

void Block::updateBlockhashContract()
{
    u256 const& blockNumber = info().number();

    u256 const& forkBlock = m_sealEngine->chainParams().experimentalForkBlock;
    if (blockNumber == forkBlock)
    {
        m_state.createContract(c_blockhashContractAddress);
        m_state.setCode(c_blockhashContractAddress, bytes(c_blockhashContractCode));
        m_state.commit(State::CommitBehaviour::KeepEmptyAccounts);
    }

    if (blockNumber >= forkBlock)
    {
        DummyLastBlockHashes lastBlockHashes; // assuming blockhash contract won't need BLOCKHASH itself
        Executive e(*this, lastBlockHashes);
        h256 const parentHash = m_previousBlock.hash();
        if (!e.call(c_blockhashContractAddress, SystemAddress, 0, 0, parentHash.ref(), 1000000))
            e.go();
        e.finalize();

        m_state.commit(State::CommitBehaviour::RemoveEmptyAccounts);
    }
}

void Block::commitToSeal(BlockChain const& _bc, bytes const& _extraData)
{
    if (isSealed())
        BOOST_THROW_EXCEPTION(InvalidOperationOnSealedBlock());

    noteChain(_bc);

    if (m_committedToSeal)
        uncommitToSeal();
    else
        m_precommit = m_state;

    vector<BlockHeader> uncleBlockHeaders;

    RLPStream unclesData;
    unsigned unclesCount = 0;
    if (m_previousBlock.number() != 0)
    {
        // Find great-uncles (or second-cousins or whatever they are) - children of great-grandparents, great-great-grandparents... that were not already uncles in previous generations.
        LOG(m_loggerDetailed) << "Checking " << m_previousBlock.hash()
                              << ", parent = " << m_previousBlock.parentHash();
        h256Hash excluded = _bc.allKinFrom(m_currentBlock.parentHash(), 6);
        auto p = m_previousBlock.parentHash();
        for (unsigned gen = 0; gen < 6 && p != _bc.genesisHash() && unclesCount < 2; ++gen, p = _bc.details(p).parent)
        {
            auto us = _bc.details(p).children;
            assert(us.size() >= 1);	// must be at least 1 child of our grandparent - it's our own parent!
            for (auto const& u: us)
                if (!excluded.count(u))	// ignore any uncles/mainline blocks that we know about.
                {
                    uncleBlockHeaders.push_back(_bc.info(u));
                    unclesData.appendRaw(_bc.headerData(u));
                    ++unclesCount;
                    if (unclesCount == 2)
                        break;
                    excluded.insert(u);
                }
        }
    }

    BytesMap transactionsMap;
    BytesMap receiptsMap;

    RLPStream txs;
    txs.appendList(m_transactions.size());

    for (unsigned i = 0; i < m_transactions.size(); ++i)
    {
        RLPStream k;
        k << i;

        RLPStream receiptrlp;
        receipt(i).streamRLP(receiptrlp);
        receiptsMap.insert(std::make_pair(k.out(), receiptrlp.out()));

        RLPStream txrlp;
        m_transactions[i].streamRLP(txrlp);
        transactionsMap.insert(std::make_pair(k.out(), txrlp.out()));

        txs.appendRaw(txrlp.out());
    }

    txs.swapOut(m_currentTxs);

    RLPStream(unclesCount).appendRaw(unclesData.out(), unclesCount).swapOut(m_currentUncles);

    // Apply rewards last of all.
    assert(_bc.sealEngine());
    applyRewards(uncleBlockHeaders, _bc.sealEngine()->blockReward(m_currentBlock.number()));

    // Commit any and all changes to the trie that are in the cache, then update the state root accordingly.
    bool removeEmptyAccounts = m_currentBlock.number() >= _bc.chainParams().EIP158ForkBlock; // TODO: use EVMSchedule
    DEV_TIMED_ABOVE("commit", 500)
        m_state.commit(removeEmptyAccounts ? State::CommitBehaviour::RemoveEmptyAccounts : State::CommitBehaviour::KeepEmptyAccounts);

        LOG(m_loggerDetailed) << "Post-reward stateRoot: " << m_state.rootHash();
        LOG(m_loggerDetailed) << m_state;

        m_currentBlock.setLogBloom(logBloom());
        m_currentBlock.setGasUsed(gasUsed());
        m_currentBlock.setRoots(hash256(transactionsMap), hash256(receiptsMap),
            sha3(m_currentUncles), m_state.rootHash());

        m_currentBlock.setParentHash(m_previousBlock.hash());
        m_currentBlock.setExtraData(_extraData);
        if (m_currentBlock.extraData().size() > 32)
        {
            auto ed = m_currentBlock.extraData();
            ed.resize(32);
            m_currentBlock.setExtraData(ed);
    }

    m_committedToSeal = true;
}

void Block::uncommitToSeal()
{
    if (m_committedToSeal)
    {
        m_state = m_precommit;
        m_committedToSeal = false;
    }
}

bool Block::sealBlock(bytesConstRef _header)
{
    if (!m_committedToSeal)
        return false;

    if (BlockHeader(_header, HeaderData).hash(WithoutSeal) != m_currentBlock.hash(WithoutSeal))
        return false;

    // Compile block:
    RLPStream ret;
    ret.appendList(3);
    ret.appendRaw(_header);
    ret.appendRaw(m_currentTxs);
    ret.appendRaw(m_currentUncles);
    ret.swapOut(m_currentBytes);
    m_currentBlock = BlockHeader(_header, HeaderData);
//	cnote << "Mined " << m_currentBlock.hash() << "(parent: " << m_currentBlock.parentHash() << ")";
    // TODO: move into SealEngine

    m_state = m_precommit;

    // m_currentBytes is now non-empty; we're in a sealed state so no more transactions can be added.

    return true;
}

h256 Block::stateRootBeforeTx(unsigned _i) const
{
    _i = min<unsigned>(_i, m_transactions.size());
    try
    {
        return (_i > 0 ? receipt(_i - 1).stateRoot() : m_previousBlock.stateRoot());
    }
    catch (TransactionReceiptVersionError const&)
    {
        return {};
    }
}

LogBloom Block::logBloom() const
{
    LogBloom ret;
    for (TransactionReceipt const& i: m_receipts)
        ret |= i.bloom();
    return ret;
}

void Block::cleanup()
{
    // Commit the new trie to disk.
    LOG(m_logger) << "Committing to disk: stateRoot " << m_currentBlock.stateRoot() << " = "
                  << rootHash() << " = " << toHex(asBytes(db().lookup(rootHash())));

    try
    {
        EnforceRefs er(db(), true);
        rootHash();
    }
    catch (BadRoot const&)
    {
        cwarn << "Trie corrupt! :-(";
        throw;
    }

    m_state.db().commit();	// TODO: State API for this?

    LOG(m_logger) << "Committed: stateRoot " << m_currentBlock.stateRoot() << " = " << rootHash()
                  << " = " << toHex(asBytes(db().lookup(rootHash())));

    m_previousBlock = m_currentBlock;
    sealEngine()->populateFromParent(m_currentBlock, m_previousBlock);

    LOG(m_logger) << "finalising enactment. current -> previous, hash is "
                  << m_previousBlock.hash();

    resetCurrent();
}
