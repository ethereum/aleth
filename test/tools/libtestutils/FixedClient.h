// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include <libethereum/ClientBase.h>
#include <libethereum/BlockChain.h>
#include <libethcore/Common.h>

namespace dev
{
namespace test
{

/**
 * @brief mvp implementation of ClientBase
 * Doesn't support mining interface
 */
class FixedClient: public dev::eth::ClientBase
{
public:
    FixedClient(eth::BlockChain const& _bc, eth::Block const& _block) :  m_bc(_bc), m_block(_block) {}
    
    // stub
    void flushTransactions() override {}
    eth::Transactions pending() const override { eth::Transactions res; return res; }
    eth::BlockChain& bc() override
    {
        BOOST_THROW_EXCEPTION(InterfaceNotSupported() << errinfo_interface("FixedClient::bc()"));
    }
    eth::BlockChain const& bc() const override { return m_bc; }
    using ClientBase::block;
    eth::Block block(h256 const& _h) const override;
    eth::Block preSeal() const override { ReadGuard l(x_stateDB); return m_block; }
    eth::Block postSeal() const override { ReadGuard l(x_stateDB); return m_block; }
    void setAuthor(Address const& _us) override { WriteGuard l(x_stateDB); m_block.setAuthor(_us); }
    void prepareForTransaction() override {}
    h256 submitTransaction(eth::TransactionSkeleton const&, Secret const&) override { return {}; };
    h256 importTransaction(eth::Transaction const&) override { return {}; }
    eth::ExecutionResult call(Address const&, u256, Address, bytes const&, u256, u256, eth::BlockNumber, eth::FudgeFactor) override { return {}; };
    eth::TransactionSkeleton populateTransactionWithDefaults(eth::TransactionSkeleton const&) const override { return {}; };
    std::tuple<h256, h256, h256> getWork() override { return std::tuple<h256, h256, h256>{}; }

private:
    eth::BlockChain const& m_bc;
    eth::Block m_block;
    mutable SharedMutex x_stateDB;			///< Lock on the state DB, effectively a lock on m_postSeal.
};

}
}
